/* Copyright(C) 2004,2005,2006,2007,2008 Stefan Siegl <stesie@brokenpipe.de>
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

#include <stdio.h>
#include <libguile.h>
#include <string.h>
#include <glib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <geierversion.h>

#include "guile.h"
#include "form.h"
#include "dialog.h"
#include "workspace.h"

static SCM taxbird_guile_eval_file_SCM(SCM scm_fn);
static SCM taxbird_dialog_error_SCM(SCM message);
static SCM taxbird_dialog_info_SCM(SCM message);
static SCM taxbird_get_version(void);
static SCM taxbird_get_geier_version(void);
static SCM taxbird_activate_sheet(SCM file, SCM root);

/* initialize taxbird's guile backend */
void taxbird_guile_init(void)
{
  char *loadpath_home = g_strdup_printf("%s/.taxbird/guile", getenv("HOME"));
  SCM loadpath = scm_list_4(scm_makfrom0str("."),
			    scm_makfrom0str("guile"),
			    scm_take0str(loadpath_home),
			    scm_makfrom0str(PACKAGE_DATA_DIR "guile"));

  /* search current and taxbird's system directory by default */
  scm_c_define("tb:scm-directories", loadpath);

  scm_c_define_gsubr("tb:eval-file", 1, 0, 0, taxbird_guile_eval_file_SCM);
  scm_c_define_gsubr("tb:form-register", 8, 0, 0, taxbird_form_register);
  scm_c_define_gsubr("tb:dlg-error", 1, 0, 0, taxbird_dialog_error_SCM);
  scm_c_define_gsubr("tb:dlg-info", 1, 0, 0, taxbird_dialog_info_SCM);
  scm_c_define_gsubr("tb:get-version", 0, 0, 0, taxbird_get_version);
  scm_c_define_gsubr("tb:get-geier-version", 0, 0, 0, taxbird_get_geier_version);
  scm_c_define_gsubr("tb:activate-sheet", 2, 0, 0, taxbird_activate_sheet);
  scm_c_define_gsubr("tb:chooser-additem", 2, 0, 0, taxbird_ws_chooser_additem);
}



char *
taxbird_guile_dirlist_lookup(const char *fn) 
{
  SCM dirlist = scm_c_lookup_ref("tb:scm-directories");

  while(scm_ilength(dirlist)) {
    struct stat statbuf;
    SCM path = SCM_CAR(dirlist);
   
    g_return_val_if_fail(scm_is_string(path), NULL);
    char *pathname = scm_to_locale_string(path);

    char *buf = g_strdup_printf("%s/%s", pathname, fn);

    free(pathname);

    if(! stat(buf, &statbuf)
       && ! S_ISDIR(statbuf.st_mode))
      /* not a directory ... */
      return buf;

    g_free(buf);

    /* next element */
    dirlist = SCM_CDR(dirlist);
  }

  return NULL;
}



/* evaluate the file with the given filename */
int
taxbird_guile_eval_file(const char *fn)
{
  char *lookup_fn;

  /* make sure that every file is evaluated only once */
  static GSList *evaluated_files = NULL;
  if(evaluated_files) {
    GSList *ptr = evaluated_files;
    do {
      if(! strcmp(ptr->data, fn))
	return 0;
    } while((ptr = ptr->next));
  }

  /* file hasn't been evaluated yet, add to list of evaluated files. */
  evaluated_files = g_slist_prepend(evaluated_files, g_strdup(fn));

  lookup_fn = taxbird_guile_dirlist_lookup(fn);
  if(! lookup_fn) {
    g_printerr(PACKAGE_NAME ": cannot find file: %s\n", fn);
    return -1;
  }

  g_printerr(PACKAGE_NAME ": loading '%s'\n", lookup_fn);
  scm_c_primitive_load(lookup_fn);
  g_free(lookup_fn);

  return 0;
}



static SCM 
taxbird_guile_eval_file_SCM(SCM scm_fn)
{
  g_return_val_if_fail(scm_is_string(scm_fn), SCM_BOOL(0));

  char *filename = scm_to_locale_string(scm_fn);
  int result = taxbird_guile_eval_file(filename);
  free(filename);

  return SCM_BOOL(! result);
}



/* this is our global error handler, around just everything, even the
 * Gtk+ main loop, enabling us to warn the user, that our life is over
 * within a few seconds ...
 * ... and outputting a stack trace.
 */
SCM
taxbird_guile_global_err_handler(void *data, SCM tag, SCM args)
{
  g_critical("global error handler called, damn.\n");

  scm_backtrace();
  scm_handle_by_message_noexit(data, tag, args);

  return SCM_BOOL(1); /* try to keep the main loop going */
}



/* guile wrapper around taxbird_dialog_error */
static SCM
taxbird_dialog_error_SCM(SCM message)
{
  if(! scm_is_string(message)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:dlg-error"),
		  scm_makfrom0str("invalid first argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  char *c_message = scm_to_locale_string(message);
  taxbird_dialog_error(NULL, c_message);
  free(c_message);

  return message;
}


/* guile wrapper around taxbird_dialog_error */
static SCM
taxbird_dialog_info_SCM(SCM message)
{
  if(! scm_is_string(message)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:dlg-info"),
		  scm_makfrom0str("invalid first argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  char *c_message = scm_to_locale_string(message);
  taxbird_dialog_info(NULL, c_message);
  free(c_message);

  return message;
}


/* return this taxbird's version number */
static SCM 
taxbird_get_version(void)
{
  return scm_makfrom0str(PACKAGE_VERSION);
}


/* Return libgeier's version number */
static SCM 
taxbird_get_geier_version(void)
{
  return scm_makfrom0str(LIBGEIER_DOTTED_VERSION);
}


static SCM 
taxbird_activate_sheet(SCM f, SCM r)
{
  if(! scm_is_string(f)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:activate-sheet"),
		  scm_makfrom0str("invalid first argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  if(! scm_is_string(r)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:activate-sheet"),
		  scm_makfrom0str("invalid second argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  char *filename = scm_to_locale_string(f);
  char *sheetname = scm_to_locale_string(r);

  taxbird_ws_activate_sheet(filename, sheetname);

  free(sheetname);
  free(filename);

  return SCM_BOOL(1);
}
