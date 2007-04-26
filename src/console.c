/* Copyright(C) 2005,2007 Stefan Siegl <stesie@brokenpipe.de>
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
#  include <config.h>
#endif

#include <string.h>

#include "console.h"
#include "guile.h"
#include "form.h"

SCM taxbird_console_select_template(SCM template_id);

void
taxbird_console_init(void)
{
  scm_c_define_gsubr("console:select-template", 1, 0, 0, 
		     taxbird_console_select_template);
  taxbird_guile_eval_file("console.scm");
}

SCM
taxbird_console_select_template(SCM template_id) 
{
  unsigned int form = -1;

  if(scm_is_string(template_id)) {
    char *template_name = scm_to_locale_string(template_id);

    unsigned int i;
    for(i = 0; i < forms_num; i ++)
      if(! strcmp(forms[i]->name, template_name)) {
	form = i;
	break;
      }

    free(template_name);
  }

  else if(SCM_NUMBERP(template_id))
    form = scm_num2int(template_id, 0, "taxbird_console_select_template");

  if(form >= forms_num)
    return SCM_BOOL(0);

  scm_c_define("console:template", scm_makfrom0str(forms[form]->name));
  scm_c_define("console:get-sheet-tree", forms[form]->get_sheet_tree);
  scm_c_define("console:get-sheet", forms[form]->get_sheet);
  scm_c_define("console:dataset-read", forms[form]->dataset_read);
  scm_c_define("console:dataset-write", forms[form]->dataset_write);
  scm_c_define("console:dataset-export", forms[form]->dataset_export);
  scm_c_define("console:dataset-create", forms[form]->dataset_create);

  /* create and reference dataset */
  scm_c_define("console:dataset", 
	       scm_list_copy(scm_call_0(forms[form]->dataset_create)));

  return scm_makfrom0str(forms[form]->name);
}
