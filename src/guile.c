/* Copyright(C) 2004,05 Stefan Siegl <ssiegl@gmx.de>
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

#include <stdio.h>
#include <libguile.h>
#include <string.h>
#include <glib.h>
#include <dirent.h>

#include "guile.h"
#include "form.h"

static SCM taxbird_guile_eval_file_SCM(SCM scm_fn);

/* initialize taxbird's guile backend */
void taxbird_guile_init(void)
{
  char *loadpath_home = g_strdup_printf("%s/.taxbird/guile", getenv("HOME"));
  SCM loadpath = scm_list_3(scm_makfrom0str("."),
			    scm_take0str(loadpath_home),
			    scm_makfrom0str(PACKAGE_DATA_DIR "/taxbird/guile"));

  /* search current and taxbird's system directory by default */
  scm_c_define("tb:scm-directories", loadpath);

  scm_c_define("tb:field:text-input", scm_int2num(FIELD_TEXT_INPUT));
  scm_c_define("tb:field:text-output", scm_int2num(FIELD_TEXT_OUTPUT));
  scm_c_define("tb:field:chooser", scm_int2num(FIELD_CHOOSER));
  scm_c_define("tb:field:text-input-calc", scm_int2num(FIELD_TEXT_INPUT_CALC));
  scm_c_define("tb:field:text-input-input",
	       scm_int2num(FIELD_TEXT_INPUT_INPUT));

  scm_c_define_gsubr("tb:eval-file", 1, 0, 0, taxbird_guile_eval_file_SCM);
  scm_c_define_gsubr("tb:form-register", 5, 0, 0, taxbird_form_register);

  /* Scan autoload/ directories for files, that should be loaded automatically.
   * However don't load each file from these directories in order, but 
   * call taxbird_guile_eval_file to always evaluate the first file in the
   * loadpath chain, i.e. allow the user to overwrite autoload/ files by
   * putting hisself's in his private directory.
   */
  while(scm_ilength(loadpath)) {
    struct dirent *dirent;
    char *dirname = g_strdup_printf("%s/autoload",
				    SCM_STRING_CHARS(SCM_CAR(loadpath)));
    DIR *dir = opendir(dirname);

    if(dir) {
      while((dirent = readdir(dir))) {
	char *fname;

	if(dirent->d_type != DT_REG) continue; /* don't load directories :-) */

	fname = g_strdup_printf("autoload/%s", dirent->d_name);
	taxbird_guile_eval_file(fname);
	g_free(fname);
      }

      closedir(dir);
      g_free(dirname);
    }

    loadpath = SCM_CDR(loadpath);
  }
}



/* evaluate the file with the given filename */
int
taxbird_guile_eval_file(const char *fn)
{
  SCM dirlist = scm_c_lookup_ref("tb:scm-directories");

  while(scm_ilength(dirlist)) {
    size_t path_len;
    char *buf;
    FILE *handle;
    SCM path = SCM_CAR(dirlist);
   
    g_return_val_if_fail(SCM_STRINGP(path), -1);
    path_len = SCM_STRING_LENGTH(path);

    buf = g_strdup_printf("%s/%s", SCM_STRING_CHARS(path), fn);
    if(! buf) {
      perror(PACKAGE_NAME);
      return -1; /* out of memory */
    }

    handle = fopen(buf, "r");
    if(handle) {
      fclose(handle);
      
      scm_c_primitive_load(buf); 
      free(buf);
      return 0;
    }

    free(buf);

    /* next element */
    dirlist = SCM_CDR(dirlist);
  }

  fprintf(stderr, "cannot find file '%s'.\n", fn);
  return -1;
}



static SCM 
taxbird_guile_eval_file_SCM(SCM scm_fn)
{
  g_return_val_if_fail(SCM_STRINGP(scm_fn), SCM_BOOL(0));
  return SCM_BOOL(! taxbird_guile_eval_file(SCM_STRING_CHARS(scm_fn)));
}

