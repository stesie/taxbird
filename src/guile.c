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
#include <guile/gh.h>
#include <string.h>

#include "guile.h"
#include "form.h"

static SCM taxbird_guile_eval_file_SCM(SCM scm_fn);

/* initialize taxbird's guile backend */
void taxbird_guile_init(void)
{
  /* search current and taxbird's system directory by default */
  gh_define("tb:scm-directories", scm_list_1(scm_makfrom0str(".")));

  gh_define("tb:field:text-input", gh_int2scm(FIELD_TEXT_INPUT));
  gh_define("tb:field:text-output", gh_int2scm(FIELD_TEXT_OUTPUT));
  gh_define("tb:field:chooser", gh_int2scm(FIELD_CHOOSER));
  gh_define("tb:field:text-input-calc", gh_int2scm(FIELD_TEXT_INPUT_CALC));

  gh_new_procedure1_0("tb:eval-file", taxbird_guile_eval_file_SCM);
  gh_new_procedure5_0("tb:form-register", taxbird_form_register);

  taxbird_guile_eval_file("startup.scm");
}



/* evaluate the file with the given filename */
int
taxbird_guile_eval_file(const char *fn)
{
  size_t fn_len = strlen(fn);
  SCM dirlist = gh_lookup("tb:scm-directories");

  while(! gh_null_p(dirlist)) {
    size_t path_len;
    char *buf = gh_scm2newstr(gh_car(dirlist), &path_len);

    buf = realloc(buf, path_len + fn_len + 2);
    if(! buf) {
      perror(PACKAGE_NAME);
      return -1; /* out of memory */
    }

    buf[path_len++] = '/';
    memmove(&buf[path_len], fn, fn_len + 1);

    /* filename (with path) is in 'buf' now, try to open */
    {
      FILE *handle = fopen(buf, "r");

      if(handle) {
	fclose(handle);

	/* gh_eval_file_with_standard_handler(buf); */
	gh_eval_file_with_catch(buf, scm_handle_by_message_noexit);
	return 0;
      }
    }

    free(buf);

    /* next element */
    dirlist = gh_cdr(dirlist);
  }

  fprintf(stderr, "cannot find file '%s'.\n", fn);
  
  return -1;
}



static SCM 
taxbird_guile_eval_file_SCM(SCM scm_fn)
{
  char *fn = gh_scm2newstr(scm_fn, NULL);
  int ret = taxbird_guile_eval_file(fn);
  free(fn);
  return gh_bool2scm(! ret);
}

