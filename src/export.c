/* Copyright(C) 2005,2006,2007,2008,2011 Stefan Siegl <stesie@brokenpipe.de>
 * taxbird - free program to interface with German IRO's Elster/Coala
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include <gtk/gtk.h>
#include <gtkhtml/gtkhtml.h>
#include <glib/gi18n.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <geier.h>
#include <geierversion.h>

#include "dialog.h"
#include "export.h"
#include "guile.h"
#include "form.h"
#include "workspace.h"
#include "builder.h"

/* ask current template's guile backend to generate the data we'd
 * like to write out (in case testcase is set, with data being marked
 * as testcase data) */
static SCM taxbird_export_call_backend(int testcase);

/* fork a subprocess, RETURN: -1 on error, 0 == child, pid == parent */
static pid_t taxbird_export_launch_subproc(int *to, int *from);

typedef pid_t (* taxbird_export_subproc) (int *to, int *from);

/* ask the user whether the exported data is okay */
static void taxbird_export_ask_user(GtkWidget *html,
				    SCM data, SCM proto_fn, SCM softpse_fn);

/* launch the given command CMD (whether CMD may even contain arguments)
 * RETURN: pid of child on success, -1 on failure */
static pid_t taxbird_export_launch_command(const char *cmd, int *to, int *fr);

extern int geier_iso_to_utf8(const unsigned char *input, const size_t inlen,
			     unsigned char **output, size_t *outlen);

static int
taxbird_xsltify_text(geier_context *context,
		     const unsigned char *input, size_t inlen,
		     unsigned char **output, size_t *outlen)
{
  int retval;
  xmlDoc *indoc;
  xmlDoc *outdoc;

  if((retval = geier_text_to_xml(context, input, inlen, &indoc)))
    goto out0;

  if((retval = geier_xsltify(context, indoc, &outdoc)))
    goto out1;
  
  if((retval = geier_xml_to_encoded_text(context, outdoc, "ISO-8859-1",
					 output, outlen)))
    goto out2;

 out2:
  xmlFreeDoc(outdoc);
 out1:
  xmlFreeDoc(indoc);
 out0:
  return retval;
}


void
taxbird_export(int testcase)
{
  SCM data = taxbird_export_call_backend(testcase);

  if(SCM_FALSEP(scm_list_p(data)))
    return; /* exporter function didn't return a list, thus abort here.
	     * error messages should have been emitted by the called func */

  /* export data into a c-string *********************************************/
  taxbird_guile_eval_file("xml-writer.scm");
  data = scm_call_1(scm_c_lookup_ref("xml-writer:export"),
		    data);

  g_return_if_fail(scm_is_string(data));
  char *data_text = scm_to_locale_string(data);
  const int data_text_len = strlen(data_text);

  /* initialize libgeier */
  geier_context *ctx = geier_context_new();
  if(! ctx) {
    taxbird_dialog_error(NULL, _("Unable to initialize libgeier context"));
    return;
  }

  /* try to validate resulting xml document against schema file */
  if(geier_validate_text(ctx, geier_format_unencrypted,
			 (unsigned char *) data_text, data_text_len)) {
    taxbird_dialog_error(NULL, _("Unable to validate the "
				 "exported document. Sorry, but this "
				 "should not happen. \n\nPlease consider "
				 "contacting the taxbird@taxbird.de "
				 "mailing list."));

    free(data_text);
    geier_context_free(ctx);
    return;
  }
	
  /* xsltify gathered data */
  unsigned char *data_xslt;
  size_t data_xslt_len;
  int xsltify_result = taxbird_xsltify_text(ctx, (unsigned char *) data_text, 
					    data_text_len, &data_xslt, 
					    &data_xslt_len);
  free(data_text);
  geier_context_free(ctx);

  if(xsltify_result) {
    taxbird_dialog_error(NULL, _("Unable to xsltify the "
				 "exported document. Sorry, but this "
				 "should not happen. \n\nPlease consider "
				 "posting to the taxbird@taxbird.de "
				 "mailing list."));
    return;
  }

  /* convert output to UTF-8 needed by GtkHTML */
  unsigned char *data_xslt_utf8;
  size_t data_xslt_utf8_len;

  geier_iso_to_utf8(data_xslt, data_xslt_len, &data_xslt_utf8, 
		    &data_xslt_utf8_len);
  free(data_xslt);

  /* now convert xslt-result to GtkHTML */
  GtkWidget *doc = gtk_html_new_from_string
	((const char *) data_xslt_utf8, data_xslt_utf8_len);

  if(! doc) {
    taxbird_dialog_error(NULL, _("Unable to allocate GtkHTML widget."));
    return;
  }

  free(data_xslt_utf8);

  /* automatically fill protocol-store widget
     (which isn't filled by GtkBuilder) */
  taxbird_guile_eval_file("taxbird.scm");
  SCM fn = scm_call_1(scm_c_lookup_ref("tb:get-proto-filename"),
		      taxbird_document_data);
  SCM softpse = scm_call_0(scm_c_lookup_ref("tb:get-softpse-filename"));

  /* ask the user, whether it's okay to send the data ************************/
  taxbird_export_ask_user(doc, data, fn, softpse);
  return;
}



/* export bottom half,
 * return 0 on success (i.e. destroy druid widget), 1 on error, a suitable
 * error message is emitted already */
int
taxbird_export_bottom_half(GtkWidget *confirm_dlg)
{
  SCM data = (SCM)g_object_get_data(G_OBJECT(confirm_dlg), "data");
  g_return_val_if_fail(scm_is_string(data), 1);

  geier_context *ctx = geier_context_new();
  if(! ctx) {
    taxbird_dialog_error(confirm_dlg,
			 _("Unable to initialize libgeier context."));
    return 1;
  }

  char *data_text = scm_to_locale_string(data);
  int data_text_len = strlen(data_text);

  /* digitally sign the Coala-XML if requested *******************************/
  GtkWidget *dsig = taxbird_builder_lookup(taxbird_builder_export, "dsig");
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dsig))) {
    GtkWidget *e;

    e = taxbird_builder_lookup(taxbird_builder_export, "dsig_fileentry");
    const char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(e));

    /* tell the guile backend about the new softpse filename ***/
    SCM softpse = scm_makfrom0str(filename);
    scm_call_1(scm_c_lookup_ref("tb:set-softpse-filename"), softpse);

    e = taxbird_builder_lookup(taxbird_builder_export, "dsig_pincode");
    const char *pincode = gtk_entry_get_text(GTK_ENTRY(e));
    
    unsigned char *output;
    size_t outlen;
#ifdef HAVE_GEIER_DSIG_SIGN_SOFTPSE
    if(geier_dsig_sign_softpse_text(ctx, (unsigned char *) data_text,
				    data_text_len, &output, &outlen,
				    filename, pincode))
#else
    if(geier_dsig_sign_text(ctx, (unsigned char *) data_text, data_text_len,
			    &output, &outlen, filename, pincode))
#endif
    {
      taxbird_dialog_error(confirm_dlg, 
			   _("Unable to digitally sign Elster document."));
      geier_context_free(ctx);
      return 1;
    }

    free(data_text);
    data_text = (char *) output;
    data_text_len = outlen;
  }       

  /* dump the generated Coala-XML to a file, if requested ********************/
  GtkWidget *store = taxbird_builder_lookup(taxbird_builder_export, "store");
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(store))) {
    GtkEntry *e = GTK_ENTRY(taxbird_builder_lookup(taxbird_builder_export,
						   "store_fileentry_text"));
    const char *filename = gtk_entry_get_text(e);

    if(! filename) {
      taxbird_dialog_error(confirm_dlg, _("You told to store Coala-XML to "
					  "a file, however didn't specify "
					  "a filename."));
      geier_context_free(ctx);
      free(data_text);
      return 1;
    }

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
		  S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH);
    if(fd < 0) {
      taxbird_dialog_error(confirm_dlg, _("Unable to create Coala-XML file."));
      geier_context_free(ctx);
      free(data_text);
      return 1;
    }

    if(write(fd, data_text, data_text_len) != data_text_len) {
      taxbird_dialog_error(confirm_dlg, _("Unable to write to Coala-XML file."));
      geier_context_free(ctx);
      free(data_text);
      return 1;
    }
    close(fd);
  }



  /*** check whether we are requested to send the data to the IRO ************/
  GtkWidget *send = taxbird_builder_lookup(taxbird_builder_export, "send");
  if(! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(send))) {
    geier_context_free(ctx);
    free(data_text);
    return 0; /* close the druid, we shalt not send the data */
  }

  /*** create filedescriptor for protocol output *****************************/
  pid_t pid_print = 0;
  int fd_to_lpr = -1, fd_from_lpr = -1;
  GtkWidget *print = taxbird_builder_lookup(taxbird_builder_export, 
					    "protocol_print");
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(print))) {
    /* fork print process ***************/
    GtkWidget *w = taxbird_builder_lookup(taxbird_builder_export, 
					  "protocol_print_command");
    const char *cmd = gtk_entry_get_text(GTK_ENTRY(w));

    pid_print = taxbird_export_launch_command(cmd, &fd_to_lpr, &fd_from_lpr);
    if(pid_print < 1) {
      taxbird_dialog_error(confirm_dlg, _("Unable to fork print process, "
					  "please verify the specified "
					  "command"));
      geier_context_free(ctx);
      free(data_text);
      return 1;
    }    
  }

  int fd_to_protofile = -1;
  GtkWidget *protosave = taxbird_builder_lookup(taxbird_builder_export, 
						"protocol_store");
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(protosave))) {
    /* open file to write protocol to ***/
    GtkWidget *w = taxbird_builder_lookup(taxbird_builder_export, 
					  "protocol_store_fileentry_text");
    const char *filename = gtk_entry_get_text(GTK_ENTRY(w));

    fd_to_protofile = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
			   S_IRUSR | S_IRGRP | S_IROTH | 
			   S_IWUSR | S_IWGRP | S_IWOTH);

    if(fd_to_protofile < 0) {
      taxbird_dialog_error(confirm_dlg, _("Unable to create protocol file."));
      geier_context_free(ctx);
      free(data_text);
      return 1;
    }
  }

  /* send the data, finally **************************************************/
  unsigned char *data_return;
  size_t data_return_len;
  if(geier_send_text(ctx, (unsigned char *) data_text, data_text_len,
		     &data_return, &data_return_len)) {
    taxbird_dialog_error(confirm_dlg,
			 _("Unable to send the exported document."));

    if(fd_from_lpr >= 0) close(fd_from_lpr);
    if(fd_to_lpr >= 0) close(fd_to_lpr);
    if(fd_to_protofile >= 0) close(fd_to_protofile);

    geier_context_free(ctx);
    free(data_text);
    return 1;
  }

  char *error_msg = geier_get_clearing_error_text(ctx, data_return,
						  data_return_len);
  if(error_msg) {
    char *msg = g_strdup_printf(_("Unable to send the tax declaration to the "
				  "inland revenue office, the error message, "
				  "returned by the clearing host was: \n\n%s"),
				error_msg);
    free(error_msg);

    taxbird_dialog_error(confirm_dlg, msg);
    free(msg);

    free(data_text);
    free(data_return);

    if(fd_from_lpr >= 0) close(fd_from_lpr);
    if(fd_to_lpr >= 0) close(fd_to_lpr);
    if(fd_to_protofile >= 0) close(fd_to_protofile);
    return 1;
  }

  free(data_text);

  /** now run xsltify to generate protocol for user */
  unsigned char *data_xslt;
  size_t data_xslt_len;
  if(taxbird_xsltify_text(ctx, data_return, data_return_len,
			  &data_xslt, &data_xslt_len)) {
    taxbird_dialog_error(confirm_dlg, _("Unable to generate the transmission "
					"protocol. This is a local error, THIS "
					"IS, YOUR DATA HAS BEEN SENT TO THE "
					"INLAND REVENUE OFFICE AND IT WAS "
					"ACCEPTED."));
    free(data_return);
    if(fd_from_lpr >= 0) close(fd_from_lpr);
    if(fd_to_protofile >= 0) close(fd_to_protofile);
    if(fd_to_lpr >= 0) close(fd_to_lpr);
    return 1;
  }

  if(fd_to_lpr >= 0) {
    if(write(fd_to_lpr, data_xslt, data_xslt_len) != data_xslt_len) {
      taxbird_dialog_error(confirm_dlg,
			   _("Failed to write to print-helper pipe."));
    }
    close(fd_to_lpr);
  }

  if(fd_to_protofile >= 0) {
    if(write(fd_to_protofile, data_xslt, data_xslt_len) != data_xslt_len) {
      taxbird_dialog_error(confirm_dlg,
			   _("Failed to write to protocol file."));
    }
    close(fd_to_protofile);
  }

  /*** wait for lpr process to exit ******************************************/
  if(fd_from_lpr >= 0) {
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

  taxbird_dialog_info(NULL, _("Your data has been sent to the inland "
			      "revenue office and was accepted"));
  return 0; /* hey, we did it. :) */
}


/* ask current template's guile backend to generate the data we'd
 * like to write out */
static SCM
taxbird_export_call_backend(int test) 
{
  g_return_val_if_fail(taxbird_current_form, SCM_BOOL(0));

  /* recalculate the whole sheet */
  scm_call_1(taxbird_current_form->dataset_recalc, taxbird_document_data);

  /* call exporter function of current template */
  return scm_call_2(taxbird_current_form->dataset_export,
		    taxbird_document_data, SCM_BOOL(test));
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
taxbird_export_ask_user(GtkWidget *doc, SCM data,
			SCM fn, SCM softpse_fn)
{
  g_return_if_fail(doc);

  taxbird_builder_export = NULL;
  GtkWidget *confirm_dlg = taxbird_builder_create(&taxbird_builder_export, 
						  "dlgExportConfirmation",
						  PACKAGE_DATA_DIR "export.ui");
  GtkWidget *scroll = taxbird_builder_lookup(taxbird_builder_export, 
					     "scroll_htmlview");
  g_return_if_fail(confirm_dlg);
  g_return_if_fail(scroll);

  gtk_container_add(GTK_CONTAINER(scroll), doc);

  g_object_set_data_full(G_OBJECT(confirm_dlg), "data", 
			 (void*) scm_gc_protect_object(data),
			 (GDestroyNotify) scm_gc_unprotect_object);

  if(scm_is_string(fn)) {
    GtkWidget *w = taxbird_builder_lookup(taxbird_builder_export, 
					  "protocol_store_fileentry_text");
    g_return_if_fail(w);
    char *filename = scm_to_locale_string(fn);
    gtk_entry_set_text(GTK_ENTRY(w), filename);
    free(filename);
  }

  if(scm_is_string(softpse_fn)) {
    GtkWidget *dsig = taxbird_builder_lookup(taxbird_builder_export, "dsig");
    g_return_if_fail(dsig);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dsig), 1);

    GtkWidget *w = taxbird_builder_lookup(taxbird_builder_export, 
					  "dsig_fileentry");
    g_return_if_fail(w);
    char *filename = scm_to_locale_string(softpse_fn);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), filename);
    free(filename);
  }    

  gtk_widget_show_all(confirm_dlg);
}

