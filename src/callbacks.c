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
#  include <config.h>
#endif

#include <libgtkhtml/gtkhtml.h>
#include <gnome.h>

#include "callbacks.h"
#include "interface.h"
#include "dialog.h"
#include "support.h"
#include "workspace.h"
#include "form.h"
#include "guile.h"
#include "export.h"


/* callback function for 
 *  File -> New menuitem
 *  New-Button from button bar 
 */
void
on_file_new_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) user_data;

  GtkWidget *dialog = create_dlgChooseTemplate();
  GtkTreeView *tv = GTK_TREE_VIEW(lookup_widget(dialog, "lstTemplates"));
  g_return_if_fail(tv);

  /* add column */
  {
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    column = gtk_tree_view_column_new_with_attributes(_("Name of Template"),
						      renderer, "text", 0,
						      NULL);
    gtk_tree_view_append_column(tv, column);
  }

  /* add names of available templates to the listing widget */
  {
    unsigned int i;
    GtkTreeIter iter;
    GtkListStore *list = gtk_list_store_new(1, G_TYPE_STRING);

    for(i = 0; i < forms_num; i ++) {
      gtk_list_store_append (list, &iter);
      gtk_list_store_set (list, &iter, 0, forms[i]->name, -1);
    }

    gtk_tree_view_set_model(tv, GTK_TREE_MODEL(list));
    gtk_widget_show(GTK_WIDGET(tv));
  }

  /* pass a reference to this taxbird window as user data of dialog */
  gtk_object_set_user_data(GTK_OBJECT(dialog),
			   lookup_widget(GTK_WIDGET(menuitem), "taxbird"));
  gtk_widget_show(dialog);
}


void
on_file_open_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) user_data;

  GtkWidget *dialog = create_dlgChooseFile();
  /* pass a reference to this taxbird window as user data of dialog */
  gtk_object_set_user_data(GTK_OBJECT(dialog),
			   lookup_widget(GTK_WIDGET(menuitem), "taxbird"));
  gtk_widget_show(dialog);
}


void
on_file_save_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  GtkWidget *appwin = lookup_widget(GTK_WIDGET(menuitem), "taxbird");
  const char *fname = g_object_get_data(G_OBJECT(appwin), "filename");

  if(fname)
    taxbird_ws_save(appwin, fname);
  else
    /* forward to File -> Save As ... */
    on_file_saveas_activate(menuitem, user_data);
}


void
on_file_saveas_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) user_data;

  GtkWidget *dialog = create_dlgChooseFile();
  gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
			      GTK_FILE_CHOOSER_ACTION_SAVE);

  /* pass a reference to this taxbird window as user data of dialog */
  gtk_object_set_user_data(GTK_OBJECT(dialog),
			   lookup_widget(GTK_WIDGET(menuitem), "taxbird"));
  gtk_widget_show(dialog);
}


/* callback for File -> Quit menu item */
void
on_file_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  g_slist_foreach(taxbird_windows, (GFunc)on_file_close_activate, NULL);
  /* gtk_main_quit(); */
}


void
on_edit_preferences_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void) menuitem;
  (void) user_data;
}


void
on_help_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void) menuitem;
  (void) user_data;

  GtkWidget *about = create_aboutTaxBird();
  gtk_widget_show (about);
}


/* callback function for OK button in template chooser window */
void
on_choose_template_OK_clicked(GtkButton *button, gpointer user_data)
{
  (void) user_data;

  GtkWidget *appwindow;
  char *sel_form_name;
  GtkWidget *dialog = lookup_widget(GTK_WIDGET(button), "dlgChooseTemplate");
  g_return_if_fail(dialog);

  {
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeView *tv = GTK_TREE_VIEW(lookup_widget(dialog, "lstTemplates"));
    g_return_if_fail(tv);

    sel = gtk_tree_view_get_selection(tv);
    if(! gtk_tree_selection_get_selected (sel, &model, &iter)) {
      g_printerr("Please select an element from the list!");
      return;
    }

    gtk_tree_model_get(model, &iter, 0, &sel_form_name, -1);
  }

  appwindow = gtk_object_get_user_data(GTK_OBJECT(dialog));

  /* check whether the passed taxbird appwin reference already has a 
   * template associated, if yes, create a new window */
  int cf = (int) g_object_get_data(G_OBJECT(appwindow), "current_form");
  if(cf != -1)
    appwindow = taxbird_ws_new();

  if(appwindow)
    taxbird_ws_sel_form(appwindow, taxbird_form_get_by_name(sel_form_name));
  else
    taxbird_dialog_error(NULL, _("Unable to create new document. Sorry."));
 
  g_free(sel_form_name);
  gtk_widget_destroy(dialog);
}


void
on_tv_sheets_cursor_changed(GtkTreeView *tv, gpointer user_data)
{
  (void) user_data;

  char *sel_sheet_name;
  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;

  g_return_if_fail(tv);

  sel = gtk_tree_view_get_selection(tv);
  if(! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  gtk_tree_model_get(model, &iter, 0, &sel_sheet_name, -1);
  taxbird_ws_sel_sheet(lookup_widget(GTK_WIDGET(tv), "taxbird"),
		      sel_sheet_name);

  g_free(sel_sheet_name);
}



/* OK button in file open/save dialog clicked ... */
void
on_choose_file_OK_clicked(GtkButton *button, gpointer user_data)
{
  (void) user_data;

  GtkWidget *appwindow;
  gchar *fname;
  GtkWidget *dialog = lookup_widget(GTK_WIDGET(button), "dlgChooseFile");
  g_return_if_fail(dialog);

  appwindow = gtk_object_get_user_data(GTK_OBJECT(dialog));
  fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

  if(fname) {
    if(gtk_file_chooser_get_action(GTK_FILE_CHOOSER(dialog)) == 
       GTK_FILE_CHOOSER_ACTION_SAVE)
      taxbird_ws_save(appwindow, fname);
    else
      taxbird_ws_open(appwindow, fname);

    g_free(fname);
  }

  gtk_widget_destroy(dialog);
}



/* send data, i.e. export, button clicked */
void
on_file_send_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) user_data;

  GtkWidget *aw = lookup_widget(GTK_WIDGET(menuitem), "taxbird");
  int current_form = (int) g_object_get_data(G_OBJECT(aw), "current_form");

  if(current_form == -1) {
    taxbird_dialog_error(aw, _("Current document contains no data. "
			       "What do you want to export from it?"));
    return;
  }

  taxbird_export(aw);
}



gboolean
on_file_close_activate(GtkWidget *widget, GdkEvent *event, gpointer user)
{
  (void) event;
  (void) user;

  GtkWidget *aw = lookup_widget(GTK_WIDGET(widget), "taxbird");
  int changed = (int)g_object_get_data(G_OBJECT(aw), "changed");

  if(changed) {
    /* ask the user whether to save the current file */
    const char *fn = g_object_get_data(G_OBJECT(aw), "filename");
    if(! fn) fn = _("(yet unnamed)");

    char *msg = g_strdup_printf(_("Your recent changes to the document %s "
				  "haven't been stored to disk so far. Do "
				  "you want them to be stored now?"), fn);

    int resp = taxbird_dialog_yes_no_cancel(aw, msg);
    g_free(msg);

    switch(resp) {
    case GTK_RESPONSE_YES:
      on_file_save_activate((GtkMenuItem *) widget, NULL);

      /* re-read changed flag, abort if changes haven't been saved */
      changed = (int)g_object_get_data(G_OBJECT(aw), "changed");
      if(changed) return TRUE; /* abort */
      break;

    case GTK_RESPONSE_NO:
      break;

    default:
      return TRUE; /* don't destroy this window */
    }
  }

  gtk_widget_destroy(aw);
  taxbird_windows = g_slist_remove(taxbird_windows, aw);

  /* stop the main loop, in case this was the last window */
  if(! taxbird_windows) gtk_main_quit();

  return FALSE;
}



gboolean
on_export_druid_cancel                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  (void) event;
  (void) user_data;

  /* we don't have to disallocate any data - the associated data 
   * structure will be unprotected and then garbage collected */

  widget = lookup_widget(widget, "dlgExportConfirmation");
  gtk_widget_destroy(widget);

  return FALSE; /* close dialog, for delete-event */
}


void
on_export_druid_finish(GnomeDruidPage *page, GtkWidget *widget,
		       gpointer user_data)
{
  (void) page;
  (void) user_data;

  GtkWidget *confirm_dlg = lookup_widget(widget, "dlgExportConfirmation");

  if(taxbird_export_bottom_half(confirm_dlg))
    return; /* error occured */

  gtk_widget_destroy(confirm_dlg);
}



/* Handle configure-event of main app-window, i.e. resize the helptext
 * part of the window to a suitable size. On resize events caused by the
 * user, make sure the helptext windows sticks to its previous size, i.e.
 * always shrink/enlarge the upper (sheet's widgets) part */
gboolean
on_taxbird_configure(GtkWidget *widget, GdkEventConfigure *event,
		     gpointer user_data)
{
  (void) user_data;
  static int old_height = 0;

  widget = lookup_widget(widget, "vpane");
  g_return_val_if_fail(widget, FALSE);

  if(old_height)
    gtk_paned_set_position(GTK_PANED(widget),
			   event->height - old_height + 
			   gtk_paned_get_position(GTK_PANED(widget)));
  else
    gtk_paned_set_position(GTK_PANED(widget), event->height - 165);
    
  old_height = event->height;
  return FALSE;
}


GtkWidget*
htmlview_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2)
{
  (void) widget_name;
  (void) string1;
  (void) string2;
  (void) int1;
  (void) int2;

  return html_view_new();
}

