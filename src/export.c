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

#include "interface.h"
#include "dialog.h"
#include "support.h"
#include "export.h"
#include "guile.h"
#include "form.h"

/* ask current template's guile backend to generate the data we'd
 * like to write out */
static SCM taxbird_export_call_backend(GtkWidget *appwin);

/* launch the xsltifying process */
static pid_t taxbird_export_launch_xsltifyer(int *to, int *from);

/* launch the dispatching process  */ 
static pid_t taxbird_export_launch_dispatcher(int *to, int *from);

/* fork a subprocess, RETURN: -1 on error, 0 == child, pid == parent */
static pid_t taxbird_export_launch_subproc(int *to, int *from);

typedef pid_t (* taxbird_export_subproc) (int *to, int *from);

/* launch subprocess and write export data to it */
static pid_t taxbird_export_fork_and_write(taxbird_export_subproc proc,
					   SCM data, int *read_fd);
					   
/* ask the user whether the exported data is okay */
static int taxbird_export_ask_user(GtkWidget *appwin, HtmlDocument *doc);

/* read html source from file descriptor FD and return a freshly allocated
 * HtmlDocument, NULL on error.  */
static HtmlDocument *taxbird_export_make_htmldoc(int fd);

void
taxbird_export(GtkWidget *appwin)
{
  SCM data = taxbird_export_call_backend(appwin);

  if(SCM_FALSEP(scm_list_p(data)))
    return; /* exporter function didn't return a list, thus abort here.
	     * error messages should have been emitted by the called func */


  /* launch xsltifyer and write the gathered data to it **********************/
  int fd_from_xsltifyer;
  pid_t pid = taxbird_export_fork_and_write(taxbird_export_launch_xsltifyer,
					    data, &fd_from_xsltifyer);
  HtmlDocument *doc = taxbird_export_make_htmldoc(fd_from_xsltifyer);
  close(fd_from_xsltifyer);

  int exit_status;
  pid_t waitpid_result = waitpid(pid, &exit_status, 0);
  g_return_if_fail(waitpid_result == pid);

  if((WIFEXITED(exit_status) && WEXITSTATUS(exit_status))
     || (WIFSIGNALED(exit_status))) {
    if(WIFSIGNALED(exit_status))
      g_printerr("got deadly signal: %d\n", WTERMSIG(exit_status));

    /* xsltifying process exited abnormally */
    taxbird_dialog_error(appwin, _("Unable to validate and xsltify the "
				   "exported document. Sorry, but this "
				   "should not happen. \n\nPlease consider "
				   "posting to the taxbird@taxbird.de "
				   "mailing list."));
    if(doc) html_document_clear(doc);
    return;
  }

  g_return_if_fail(doc); /* if we cannot read from the xsltifier,
			  * it most probably exitted with non-zero exit
			  * status anyways. */



  /* ask the user, whether it's okay to send the data ************************/
  int resp = taxbird_export_ask_user(appwin, doc);
  html_document_clear(doc);

  if(resp != GTK_RESPONSE_YES) return; /* get outta here */



  /* send the data, finally **************************************************/
  int fd_from_dispatcher;
  pid = taxbird_export_fork_and_write(taxbird_export_launch_dispatcher,
				      data, &fd_from_dispatcher);
  /* ignore all the data we get from the dispatcher (which shouldn't be
   * any byte, by the way */
  char buf[16]; while(read(fd_from_dispatcher, buf, sizeof(buf)));
  close(fd_from_dispatcher);

  if((waitpid(pid, &exit_status, 0) != pid)
     || (! WIFEXITED(exit_status))
     || (WEXITSTATUS(exit_status))) {
    /* xsltifying process exited abnormally */
    taxbird_dialog_error(appwin, _("Unable to send the exported document."));
    return;
  }

  /* hey, we did it. :) */
}


/* launch subprocess and write export data to it */
static pid_t
taxbird_export_fork_and_write(taxbird_export_subproc proc, SCM data, int *fd)
{
  int fd_to_xsltifyer, fd_from_xsltifyer;
  pid_t pid_xsltifyer = proc(&fd_to_xsltifyer, &fd_from_xsltifyer);

  if(pid_xsltifyer == -1) {
    taxbird_dialog_error(NULL, _("Cannot fork sub-process, sorry."));
    return -1;
  }

  /* map fd_to_xsltifyer filedescriptor to guile port */
  SCM handle = scm_fdes_to_port(fd_to_xsltifyer, "w",
				scm_makfrom0str("fd-to-xsltifyer"));

  /* export previously generated data to xsltifyer process */
  taxbird_guile_eval_file("xml-writer.scm");
  scm_call_2(scm_c_lookup_ref("xml-writer:write"), data, handle);
  scm_close(handle);
  /* FIXME: do we have to close(fd_to_xsltifyer) ?? */

  *fd = fd_from_xsltifyer;
  return pid_xsltifyer;
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



/* launch the xsltifying process 
 * RETURN: pid of child on success, -1 on failure */
static pid_t 
taxbird_export_launch_xsltifyer(int *to, int *from)
{
  pid_t child = taxbird_export_launch_subproc(to, from);
  if(child != 0) return child; /* main process || error -> get out */

  /** launch geier */
  execlp(GEIER_PATH, "geier", "--dry-run", "--xsltify", "--validate", NULL);

  /** if execlp fails, die. */
  exit(255);
}



/* launch the dispatching process 
 * RETURN: pid of child on success, -1 on failure */
static pid_t 
taxbird_export_launch_dispatcher(int *to, int *from)
{
  pid_t child = taxbird_export_launch_subproc(to, from);
  if(child != 0) return child; /* main process || error -> get out */

  /** launch geier */
  execlp(GEIER_PATH, "geier", NULL);

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

/* ask the user whether the exported data is okay */
static int
taxbird_export_ask_user(GtkWidget *appwin, HtmlDocument *doc)
{
  GtkWidget *confirm_dlg = create_dlgExportConfirmation();

  GtkWidget *htmlview = lookup_widget(confirm_dlg, "htmlview");
  html_view_set_document(HTML_VIEW(htmlview), doc);

  int resp = gtk_dialog_run(GTK_DIALOG(confirm_dlg));
  gtk_widget_destroy(confirm_dlg);

  return resp;
}



/* read html source from file descriptor FD and return a freshly allocated
 * HtmlDocument, NULL on error.  */
static HtmlDocument *
taxbird_export_make_htmldoc(int fd)
{
  g_return_val_if_fail(fd >= 0, NULL);

  HtmlDocument *doc = html_document_new();
  if(! doc) return NULL;

  if(! html_document_open_stream(doc, "text/html")) {
    html_document_clear(doc);
    return NULL;
  }

  unsigned char buf[4096]; /* FIXME, maybe define to page size */
  size_t bytes_read;
  while((bytes_read = read(fd, buf, sizeof(buf))))
    html_document_write_stream(doc, buf, bytes_read);
  
  html_document_close_stream(doc);
  return doc;
}
