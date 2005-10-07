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

#ifndef TAXBIRD_FORM_H
#define TAXBIRD_FORM_H

enum field_type {
  FIELD_TEXT_INPUT   = 0,
  FIELD_TEXT_OUTPUT  = 1,
  FIELD_CHOOSER      = 2,
  FIELD_CHECKBOX     = 3,

  FIELD_UNCHANGEABLE = 4,
  FIELD_LABEL        = FIELD_UNCHANGEABLE | 0,
  FIELD_BUTTON       = FIELD_UNCHANGEABLE | 1,
};

struct form {
  char *name;

  SCM get_sheet_tree; /* function to retrieve the tree to be displayed */
  SCM get_sheet;      /* function to retrieve a sheet */

  SCM dataset_read;   /* function to read from our dataset */
  SCM dataset_write;  /* function to write to the dataset */
  SCM dataset_export; /* function to export XML stream */
  SCM dataset_create; /* empty data set */
};

/* list of forms, defined for use with taxbird */
extern struct form **forms;

/* number of registered forms */
extern unsigned int forms_num;

/* create a new form (tb:form-register) */
SCM taxbird_form_register(SCM name, 
			  SCM get_sheet_tree,
			  SCM get_sheet,
			  SCM dataset_read,
			  SCM dataset_write, 
			  SCM dataset_export,
			  SCM dataset_create);

/* lookup specified form's id 
 * RETURN: number of registered form, -1 on error
 */
int taxbird_form_get_by_name(const char *name);

#endif /* TAXBIRD_FORM_H */
