/* Copyright(C) 2004 Stefan Siegl <ssiegl@gmx.de>
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

#ifndef TAXBIRD_WORKSPACE_H
#define TAXBIRD_WORKSPACE_H

#include <guile/gh.h>

struct taxbird_ws {
  SCM ws_data;
  int current_form;
  char *fname;
  int changed;
};

/* create new taxbird workspace */
void taxbird_ws_new(void);

/* load taxbird workspace from file */
void taxbird_ws_open(GtkWidget *app_window, const char *fname);

/* store taxbird workspace to file */
void taxbird_ws_save(GtkWidget *app_window, const char *fname);

/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void taxbird_ws_sel_form(GtkWidget *app_window, int formid);

/* display sheet with provided id in the workspace of app_window */
void taxbird_ws_sel_sheet(GtkWidget *app_window, const char *sheet);

#define taxbird_ws_get(appwin) \
  ((struct taxbird_ws *)(gtk_object_get_user_data(GTK_OBJECT(appwin))))

#endif /* TAXBIRD_WORKSPACE_H */
