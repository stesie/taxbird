/* Copyright(C) 2004,2005,2007,2008,2010 Stefan Siegl <stesie@brokenpipe.de>
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
#  include <config.h>
#endif

#include <gtkhtml/gtkhtml.h>

#include "callbacks.h"
#include "dialog.h"
#include "workspace.h"
#include "form.h"
#include "guile.h"
#include "export.h"
#include "builder.h"

/* callback function for 
 *  File -> New menuitem
 *  New-Button from button bar 
 */
void
on_file_new_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  GtkBuilder *b = NULL;
  GtkWidget *dialog = taxbird_builder_create(&b, "dlgChooseTemplate",
					     PACKAGE_DATA_DIR "choose-template.ui");
  GtkTreeView *tv = GTK_TREE_VIEW(taxbird_builder_lookup(b, "lstTemplates"));
  g_return_if_fail(tv);

  g_object_unref(b);

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

  gtk_widget_show(dialog);
}


void
on_file_new_from_last_year_activate(GtkMenuItem *mi, gpointer u)
{
  (void) mi;
  (void) u;

  GtkBuilder *b = NULL;
  GtkWidget *dialog = taxbird_builder_create(&b, "dlgChooseFile",
					     PACKAGE_DATA_DIR "choose-file.ui");
  g_signal_connect(G_OBJECT(taxbird_builder_lookup(b, "cmdOK")),
		   "clicked", G_CALLBACK(on_choose_file_copy_last_year), NULL);
  g_object_unref(b);
  gtk_widget_show(dialog);
}


void
on_file_open_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  GtkBuilder *b = NULL;
  GtkWidget *dialog = taxbird_builder_create(&b, "dlgChooseFile",
					     PACKAGE_DATA_DIR "choose-file.ui");
  g_signal_connect(G_OBJECT(taxbird_builder_lookup(b, "cmdOK")),
		   "clicked", G_CALLBACK(on_choose_file_open), NULL);
  g_object_unref(b);
  gtk_widget_show(dialog);
}

void
on_file_save_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  if(taxbird_document_filename)
    taxbird_ws_save(taxbird_document_filename);

  else
    /* forward to File -> Save As ... */
    on_file_saveas_activate(NULL, NULL);
}


void
on_file_saveas_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  GtkBuilder *b = NULL;
  GtkWidget *dialog = taxbird_builder_create(&b, "dlgChooseFile",
					     PACKAGE_DATA_DIR "choose-file.ui");
  gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
			      GTK_FILE_CHOOSER_ACTION_SAVE);
  g_signal_connect(G_OBJECT(taxbird_builder_lookup(b, "cmdOK")),
		   "clicked", G_CALLBACK(on_choose_file_save), NULL);
  g_object_unref(b);
  gtk_widget_show(dialog);
}


static gboolean
taxbird_document_ask_save_file(void)
{
  if(taxbird_document_changed) {
    /* ask the user whether to save the current file */
    const char *fn = (taxbird_document_filename
		      ? taxbird_document_filename 
		      : _("(yet unnamed)"));

    char *msg = g_strdup_printf(_("Your recent changes to the document %s "
				  "haven't been stored to disk so far. Do "
				  "you want them to be stored now?"), fn);

    int resp = taxbird_dialog_yes_no_cancel(NULL, msg);
    g_free(msg);

    switch(resp) {
    case GTK_RESPONSE_YES:
      on_file_save_activate(NULL, NULL);

      /* re-read changed flag, abort if changes haven't been saved */
      if(taxbird_document_changed) return TRUE; /* abort */
      break;

    case GTK_RESPONSE_NO:
      break;

    default:
      return TRUE; /* don't destroy this window */
    }
  }

  return FALSE;
}


/* callback for File -> Quit menu item */
void
on_file_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  if(taxbird_document_ask_save_file())
    return;

  gtk_widget_destroy(taxbird_window);
  gtk_main_quit();
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

  GtkWidget *about = taxbird_builder_create(NULL, "aboutTaxBird",
					    PACKAGE_DATA_DIR "about.ui");
  gtk_widget_show (about);
}


/* callback function for OK button in template chooser window
 *
 * mind, that the widget GtkButton* need not be the button that has been
 * clicked, since this function is invoked from other callback functions.
 */
void
on_choose_template_OK_clicked(GtkTreeView *tv, gpointer user_data)
{
  (void) user_data;

  char *sel_form_name;

  g_return_if_fail(GTK_TREE_VIEW(tv));

  GtkWidget *dialog = gtk_widget_get_toplevel(GTK_WIDGET(tv));
  g_return_if_fail(dialog);

  GtkTreeSelection *sel;
  GtkTreeModel *model;
  GtkTreeIter iter;

  sel = gtk_tree_view_get_selection(tv);
  if(! gtk_tree_selection_get_selected (sel, &model, &iter)) {
    taxbird_dialog_error(dialog, _("Please select a template "
				   "from the list."));
    return;
  }
  
  gtk_tree_model_get(model, &iter, 0, &sel_form_name, -1);

  if(! taxbird_document_ask_save_file ())
    taxbird_ws_sel_form(taxbird_form_get_by_name(sel_form_name));
 
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
  taxbird_ws_sel_sheet(sel_sheet_name);

  g_free(sel_sheet_name);
}


static gchar *
on_choose_file_get_fname(GtkWidget *widget)
{
  GtkWidget *dialog = gtk_widget_get_toplevel(widget);
  g_return_val_if_fail(dialog, NULL);

  gchar *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

  if(fname && taxbird_document_ask_save_file ()) {
    g_free(fname);
    fname = NULL;
  }

  gtk_widget_destroy(dialog);
  return fname;
}

void
on_choose_file_open(GtkButton *button, gpointer user_data)
{
  (void) user_data;

  gchar *fname = on_choose_file_get_fname(GTK_WIDGET(button));
  if(fname)
    taxbird_ws_open(fname, FALSE);
}


void
on_choose_file_copy_last_year(GtkButton *button, gpointer user_data)
{
  (void) user_data;

  gchar *fname = on_choose_file_get_fname(GTK_WIDGET(button));
  if(fname)
    taxbird_ws_open(fname, TRUE);
}


void
on_choose_file_save(GtkButton *button, gpointer user_data)
{
  (void) user_data;

  gchar *fname;

  GtkWidget *dialog = taxbird_builder_lookup(taxbird_builder_app,
					     "dlgChooseFile");
  g_return_if_fail(dialog);

  fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

  if(fname) {
    if(! strchr(fname, '.')) {
      fname = g_realloc(fname, strlen(fname) + 5);
      strcat(fname, ".txb");
    }

    taxbird_ws_save(fname);
  }

  g_free(fname);
  gtk_widget_destroy(dialog);
}



/* send data, i.e. export, button clicked */
void
on_file_send_activate(GtkMenuItem *menuitem, gpointer user_data)
{
  (void) menuitem;
  (void) user_data;

  if(! taxbird_current_form) {
    taxbird_dialog_error(NULL, _("Current document contains no data. "));
    return;
  }

  taxbird_export(0);
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

  widget = taxbird_builder_lookup(taxbird_builder_app, "vpane");
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


gboolean
on_templates_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  if(event->type == GDK_2BUTTON_PRESS)
    on_choose_template_OK_clicked(GTK_TREE_VIEW(widget), NULL);

  return FALSE;
}


void
on_file_send_testcase_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  (void) user_data;

  if(! taxbird_current_form) {
    taxbird_dialog_error(NULL, _("Current document contains no data. "));
    return;
  }

  taxbird_export(1);
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

  widget = taxbird_builder_lookup(taxbird_builder_export, 
				  "dlgExportConfirmation");
  gtk_widget_destroy(widget);

  return FALSE; /* close dialog, for delete-event */
}


void
on_export_druid_finish(GnomeDruidPage *page, GtkWidget *widget,
		       gpointer user_data)
{
  (void) page;
  (void) user_data;

  GtkWidget *confirm_dlg = taxbird_builder_lookup(taxbird_builder_export, 
						  "dlgExportConfirmation");

  if(taxbird_export_bottom_half(confirm_dlg))
    return; /* error occured */

  gtk_widget_destroy(confirm_dlg);
}
