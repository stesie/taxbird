/* Copyright(C) 2004,2005,2007 Stefan Siegl <ssiegl@gmx.de>
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

#ifndef TAXBIRD_GUILE_H
#define TAXBIRD_GUILE_H

#include <libguile.h>

#define scm_c_lookup_ref(a) SCM_VARIABLE_REF(scm_c_lookup(a))

/* initialize taxbird's guile backend */
void taxbird_guile_init(void);

/* evaluate given file */
int taxbird_guile_eval_file(const char *fn);

/* lookup a file 'fn' in the paths we search .scm files in */
char *taxbird_guile_dirlist_lookup(const char *fn);

/* our global error handler ... */
SCM taxbird_guile_global_err_handler(void *data, SCM tag, SCM args);



/*
 * libguile pre-1.8 compatibility stuff
 */
#ifndef HAVE_SCM_IS_STRING
#define scm_is_string(str) SCM_STRINGP(str)
#endif /* not HAVE_SCM_IS_STRING */

#ifndef HAVE_SCM_TO_LOCALE_STRING
#include <string.h>
inline static char *
scm_to_locale_string(SCM str)
{
  if(! scm_is_string(str))
    return NULL;
  char *p = strdup(SCM_STRING_CHARS(str));
  if(! p)
    perror(PACKAGE_NAME);
  return p;
}
#endif /* not HAVE_SCM_TO_LOCALE_STRING */



#endif /* TAXBIRD_GUILE_H */
