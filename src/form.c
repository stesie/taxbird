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

#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <libguile.h>

#include "form.h"

/* instantiate variables declared (extern) in form.h */
struct form **forms = NULL;
unsigned int forms_num = 0;



/* create a new form (doesn't check whether there already is a form with
 * provided name)
 * RETURN: number of registered form, -1 on error
 */
SCM
taxbird_form_register(SCM name, SCM dataset, SCM dataset_read,
		      SCM dataset_write, SCM dataset_export, SCM dataset_create)
{
  struct form **new_f = realloc(forms, sizeof(struct form *) * (forms_num + 1));
  
  if(! new_f) {
    perror(PACKAGE_NAME);
    return SCM_BOOL(0);
  }
  else  
    forms = new_f;

  if(! (forms[forms_num] = malloc(sizeof(struct form)))) {
    perror(PACKAGE_NAME);
    return SCM_BOOL(0);
  }

  g_return_val_if_fail(SCM_STRINGP(name), SCM_BOOL(0));
  forms[forms_num]->name = g_strdup(SCM_STRING_CHARS(name)); 

  scm_gc_protect_object(forms[forms_num]->dataset = dataset);
  scm_gc_protect_object(forms[forms_num]->dataset_read = dataset_read);
  scm_gc_protect_object(forms[forms_num]->dataset_write = dataset_write);
  scm_gc_protect_object(forms[forms_num]->dataset_export = dataset_export);
  scm_gc_protect_object(forms[forms_num]->dataset_create = dataset_create);

  return scm_int2num(forms_num ++);
}



/* lookup specified form's id 
 * RETURN: number of registered form, -1 on error
 */
int 
taxbird_form_get_by_name(const char *name) 
{
  unsigned int i;
  for(i = 0; i < forms_num; i ++)
    if(! strcmp(forms[i]->name, name))
      return i;

  return -1;
}



