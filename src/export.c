/* Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>
 * taxbird - free program to interface with German IRO's Elster/Coala
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include <libgtkhtml/gtkhtml.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <geier.h>

#include "interface.h"
#include "dialog.h"
#include "support.h"
#include "export.h"
#include "guile.h"
#include "form.h"

/* ask current template's guile backend to generate the data we'd
 * like to write out */
static SCM taxbird_export_call_backend(GtkWidget *appwin);

/* fork a subprocess, RETURN: -1 on error, 0 == child, pid == parent */
static pid_t taxbird_export_launch_subproc(int *to, int *from);

typedef pid_t (* taxbird_export_subproc) (int *to, int *from);

/* ask the user whether the exported data is okay */
static void taxbird_export_ask_user(GtkWidget *appwin, HtmlDocument *doc,
				    SCM data);

/* launch the given command CMD (whether CMD may even contain arguments)
 * RETURN: pid of child on success, -1 on failure */
static pid_t taxbird_export_launch_command(const char *cmd, int *to, int *fr);



void
taxbird_export(GtkWidget *appwin)
{
  SCM data = taxbird_export_call_backend(appwin);

  if(SCM_FALSEP(scm_list_p(data)))
    return; /* exporter function didn't return a list, thus abort here.
	     * error messages should have been emitted by the called func */

  /* export data into a c-string *********************************************/
  taxbird_guile_eval_file("xml-writer.scm");
  data = scm_call_1(scm_c_lookup_ref("xml-writer:write"), data);

  g_return_if_fail(SCM_STRINGP(data));
  const char *data_text = SCM_STRING_CHARS(data);
  const int data_text_len = strlen(data_text);

  /* initialize libgeier */
  geier_context *ctx = geier_context_new();
  if(! ctx) {
    taxbird_dialog_error(appwin, _("Unable to initialize libgeier context"));
    return;
  }

  /* try to validate resulting xml document against schema file */
  if(geier_validate_text(ctx, geier_format_unencrypted,
			 data_text, data_text_len)) {
    taxbird_dialog_error(appwin, _("Unable to validate the "
				   "exported document. Sorry, but this "
				   "should not happen. \n\nPlease consider "
				   "contacting the taxbird@taxbird.de "
				   "mailing list."));
    geier_context_free(ctx);
    return;
  }
	
  /* xsltify gathered data */
  unsigned char *data_xslt;
  size_t data_xslt_len;
  if(geier_xsltify_text(ctx, data_text, data_text_len,
			&data_xslt, &data_xslt_len)) {
    taxbird_dialog_error(appwin, _("Unable to xsltify the "
				   "exported document. Sorry, but this "
				   "should not happen. \n\nPlease consider "
				   "posting to the taxbird@taxbird.de "
				   "mailing list."));
    geier_context_free(ctx);
    return;
  }

  geier_context_free(ctx);

  /* now convert xslt-result to HtmlDocument */
  HtmlDocument *doc = html_document_new();
  if(! doc) {
    taxbird_dialog_error(appwin, _("Unable to allocate HtmlDocument."));
    return;
  }

  if(! html_document_open_stream(doc, "text/html")) {
    html_document_clear(doc);
    taxbird_dialog_error(appwin, _("Unable to call open_stream"
				   "on allocated HtmlDocument."));
    return;
  }

  html_document_write_stream(doc, data_xslt, data_xslt_len);
  html_document_close_stream(doc);

  free(data_xslt);

  /* ask the user, whether it's okay to send the data ************************/
  taxbird_export_ask_user(appwin, doc, data);
  return;
}



/* export bottom half,
 * return 0 on success (i.e. destroy druid widget), 1 on error, a suitable
 * error message is emitted already */
int
taxbird_export_bottom_half(GtkWidget *confirm_dlg)
{
  SCM data = (SCM)g_object_get_data(G_OBJECT(confirm_dlg), "data");
  g_return_val_if_fail(SCM_STRINGP(data), 1);

  const char *data_text = SCM_STRING_CHARS(data);
  const int data_text_len = strlen(data_text);

  /* dump the generated Coala-XML to a file, if requested ********************/
  GtkWidget *store = lookup_widget(confirm_dlg, "store");
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(store))) {
    GtkEntry *e = GTK_ENTRY(lookup_widget(confirm_dlg, "store_entry"));
    const char *filename = gtk_entry_get_text(e);

    if(! filename) {
      taxbird_dialog_error(confirm_dlg, _("You told to store Coala-XML to "
					  "a file, however didn't specify "
					  "a filename."));
      return 1;
    }

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		  S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
    if(fd < 0) {
      taxbird_dialog_error(confirm_dlg, _("Unable to create Coala-XML file."));
      return 1;
    }

    write(fd, data_text, data_text_len);
    close(fd);
  }



  /*** check whether we are requested to send the data to the IRO ************/
  GtkWidget *send = lookup_widget(confirm_dlg, "send");
  if(! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(send)))
    return 0; /* close the druid, we shalt not send the data */


  geier_context *ctx = geier_context_new();
  if(! ctx) {
    taxbird_dialog_error(confirm_dlg,
			 _("Unable to initialize libgeier context."));
    return 1;
  }

  /*** create filedescriptor for protocol output *****************************/
  pid_t pid_print = 0;
  int fd_to_lpr = -1, fd_from_lpr = -1;
  GtkWidget *print = lookup_widget(confirm_dlg, "protocol_print");
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(print))) {
    /* fork print process ***************/
    GtkWidget *w = lookup_widget(confirm_dlg, "protocol_print_command");
    const char *cmd = gtk_entry_get_text(GTK_ENTRY(w));

    pid_print = taxbird_export_launch_command(cmd, &fd_to_lpr, &fd_from_lpr);
    if(pid_print < 1) {
      taxbird_dialog_error(confirm_dlg, _("Unable to fork print process, "
					  "please verify the specified "
					  "command"));
      geier_context_free(ctx);
      return 1;
    }    
  }
  else {
    /* open file to write protocol to ***/
    GtkWidget *w = lookup_widget(confirm_dlg, "protocol_store_fileentry_text");
    const char *filename = gtk_entry_get_text(GTK_ENTRY(w));

    fd_to_lpr = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		     S_IRUSR | S_IRGRP | S_IROTH | 
		     S_IWUSR | S_IWGRP | S_IWOTH);

    if(fd_to_lpr < 0) {
      taxbird_dialog_error(confirm_dlg, _("Unable to create protocol file."));
      geier_context_free(ctx);
      return 1;
    }
  }

  g_return_val_if_fail(fd_to_lpr >= 0, 1);

  /* send the data, finally **************************************************/
  unsigned char *data_return;
  size_t data_return_len;
  if(geier_send_text(ctx, data_text, data_text_len,
		     &data_return, &data_return_len)) {
    taxbird_dialog_error(confirm_dlg,
			 _("Unable to send the exported document."));

    if(fd_from_lpr >= 0) close(fd_from_lpr);
    close(fd_to_lpr);
    return 1;
  }

  char *error_msg = geier_get_clearing_error_text(ctx, data_return,
						  data_return_len);
  if(error_msg) {
    char *msg = g_strdup_printf(_("Unable to send the tax declaration to the "
				  "inland revenue office, the error message, "
				  "returned by the clearing host was: \n\n%s"),
				error_msg);
    taxbird_dialog_error(confirm_dlg, msg);
    free(msg);
    free(data_return);
    if(fd_from_lpr >= 0) close(fd_from_lpr);
    close(fd_to_lpr);
    return 1;
  }

  /** now run xsltify to generate protocol for user */
  unsigned char *data_xslt;
  size_t data_xslt_len;
  if(geier_xsltify_text(ctx, data_return, data_return_len,
			&data_xslt, &data_xslt_len)) {
    taxbird_dialog_error(confirm_dlg, _("Unable to generate the transmission "
					"protocol. This is a local error, THIS "
					"IS, YOUR DATA HAS BEEN SENT TO THE "
					"INLAND REVENUE OFFICE AND IT WAS "
					"ACCEPTED."));
    free(data_return);
    if(fd_from_lpr >= 0) close(fd_from_lpr);
    close(fd_to_lpr);
    return 1;
  }

  write(fd_to_lpr, data_xslt, data_xslt_len);
  close(fd_to_lpr);

  /*** wait for lpr process to exit ******************************************/
  if(fd_from_lpr != -1) {
    int exit_status;
    unsigned char buf[256];
    while(read(fd_from_lpr, buf, sizeof(buf)));

    close(fd_from_lpr);

    if((waitpid(pid_print, &exit_status, 0) != pid_print)
       || (! WIFEXITED(exit_status))
       || (WEXITSTATUS(exit_status))) {
      taxbird_dialog_error(confirm_dlg,
			   _("Printer process exited abnormally, "
			     "your data has been sent however."));
      return 1;
    }
  }

  taxbird_dialog_info(NULL, _("Your data was sent to the inland "
			      "revenue office and has been accepted"));
  return 0; /* hey, we did it. :) */
}


/* ask current template's guile backend to generate the data we'd
 * like to write out */
static SCM
taxbird_export_call_backend(GtkWidget *appwin) 
{
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  g_return_val_if_fail(current_form >= 0, SCM_BOOL(0));

  /* data as supplied by the user */
  SCM data = (SCM) g_object_get_data(G_OBJECT(appwin), "scm_data");

  /* call exporter function of current template */
  return scm_call_1(forms[current_form]->dataset_export, data);
}



/* launch the given command CMD (whether CMD may even contain arguments)
 * RETURN: pid of child on success, -1 on failure */
static pid_t 
taxbird_export_launch_command(const char *cmd, int *to, int *from)
{
  char **array = NULL;
  const char *start = cmd, *p = cmd;
  int quote = 0, len = 0;
  for(;; p ++) {
    if(*p == '\\') {
      g_return_val_if_fail(p[1] != '0', -1);
      p ++;
    }
    else if(*p == '\'' || *p == '\"') {
      if(quote && *p == quote) quote = 0; /* reset */
      else if(! quote) quote = *p; /* set */
    }
    else if(*p == ' ' || *p == '\t' || *p == '\0') {
      array = g_realloc(array, sizeof(char *) * (len + 1));
      array[len ++] = g_strndup(start, p - start);
      start = p + 1;
      if(! *p) break;
    }
  }
   
  array = g_realloc(array, sizeof(char *) * (len + 1));
  array[len] = NULL; /* terminate array */

  pid_t child = taxbird_export_launch_subproc(to, from);
  if(child != 0) {
    do
      g_free(array[-- len]);
    while(len);

    g_free(array);
    return child; /* main process || error -> get out */
  }

  /** launch subprocess */
  g_printerr("executing %s ...\n", array[0]);
  execvp(array[0], array);

  g_printerr("failed.\n");
  /** if execlp fails, die. */
  exit(255);
}



/* fork a subprocess, RETURN: -1 on error, 0 == child, pid == parent */
static pid_t 
taxbird_export_launch_subproc(int *to, int *from)
{
  int to_pipe[2];
  int from_pipe[2];

  if(pipe(to_pipe)) {
    perror(PACKAGE_NAME);
    return -1;
  }

  if(pipe(from_pipe)) {
    perror(PACKAGE_NAME);

    close(to_pipe[0]); /* close the pipe we've already set up */
    close(to_pipe[1]);

    return -1; /* good bye. */
  }

  pid_t child = fork();

  if(child < 0) {
    perror(PACKAGE_NAME);
    return -1;
  }

  if(child) {
    close(to_pipe[0]);   /* we don't read from the to-pipe */
    close(from_pipe[1]); /* we don't write to the from-pipe */

    *to = to_pipe[1];
    *from = from_pipe[0];

    return child; /* main process, return the pid */
  }

  /** this is the child process ... */
  close(to_pipe[1]);
  close(from_pipe[0]);

  /** map the pipe to stdin/stdout */
  if(dup2(to_pipe[0], 0) == -1) exit(255);
  if(dup2(from_pipe[1], 1) == -1) exit(255);

  return 0;
}



/* show the export druid and return */
static void
taxbird_export_ask_user(GtkWidget *appwin, HtmlDocument *doc, SCM data)
{
  (void) appwin;  /* unused for the moment */

  GtkWidget *confirm_dlg = create_dlgExportConfirmation();

  GtkWidget *htmlview = lookup_widget(confirm_dlg, "htmlview");
  html_view_set_document(HTML_VIEW(htmlview), doc);

  g_object_set_data_full(G_OBJECT(confirm_dlg), "data", 
			 (void*) scm_gc_protect_object(data),
			 (GDestroyNotify) scm_gc_unprotect_object);

  gtk_widget_show(confirm_dlg);
}



