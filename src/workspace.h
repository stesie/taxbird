/* Copyright(C) 2004,2005,2008 Stefan Siegl <stesie@brokenpipe.de>
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

#ifndef TAXBIRD_WORKSPACE_H
#define TAXBIRD_WORKSPACE_H

#include <guile/gh.h>
#include <gtk/gtk.h>

/* create new taxbird workspace */
GtkWidget *taxbird_ws_new(void);

/* load taxbird workspace from file */
void taxbird_ws_open(const char *fname, gboolean last_year);

/* store taxbird workspace to file */
void taxbird_ws_save(const char *fname);

/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void taxbird_ws_sel_form(int formid);

/* display sheet with provided id in the workspace of app_window */
void taxbird_ws_sel_sheet(const char *sheet);

/* activate a sheet, loaded from the provided file and root widget */
void taxbird_ws_activate_sheet(const char *file, const char *sh);

/* add another item to a chooser */
SCM taxbird_ws_chooser_additem(SCM chooser, SCM item);

/* Pointer to the Taxbird application window. */
extern GtkWidget *taxbird_window;

/* Pointer to the form, which is currently marked active. */
extern struct form *taxbird_current_form;

/* Whether the document has been changed and shalt be saved. */
extern int taxbird_document_changed;

/* The filename of the current document, if any. */
extern char *taxbird_document_filename;

/* The SCM dataset of the current document. */
extern SCM taxbird_document_data;


#endif /* TAXBIRD_WORKSPACE_H */
