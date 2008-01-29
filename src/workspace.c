/* Copyright(C) 2004,2005,2007,2008 Stefan Siegl <stesie@brokenpipe.de>
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

#include <gnome.h>
#include <assert.h>
#include <glade/glade.h>

#include "dialog.h"
#include "workspace.h"
#include "form.h"
#include "guile.h"
#include "glade.h"

/* add necessary items (taken from the dataset) to the
 * GtkTreeStore recursively */
static void taxbird_ws_fill_tree_store(GtkTreeStore *ts, GtkTreeIter *iter,
				       SCM dataset);

static void taxbird_ws_store_event(GtkWidget *w, gpointer user_data);

static void taxbird_ws_retrieve_field(GtkWidget *w, const char *field_name);

/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignore, may be NULL  */
static void taxbird_ws_update_fields(const char *exception);


static gboolean taxbird_ws_show_appbar_help(GtkWidget *widget,
					    GdkEventFocus *event,
					    gpointer user_data);

/* set this to true to disable the storage hook,
 * this is thought to be set while the fields are loaded */
static int taxbird_ws_disable_storage_hook = 0;


/* Pointer to the Taxbird application window. */
GtkWidget *taxbird_window = NULL;

/* Pointer to the form, which is currently marked active. */
struct form *taxbird_current_form = NULL;

/* Whether the document has been changed and shalt be saved. */
int taxbird_document_changed = 0;

/* The filename of the current document, if any. */
char *taxbird_document_filename = NULL;

/* The SCM dataset of the current document. */
SCM taxbird_document_data;

/* create new taxbird workspace */
GtkWidget *
taxbird_ws_new(void)
{
  taxbird_window = taxbird_glade_create(&taxbird_gladexml_app, "taxbird");
  if(! taxbird_window) return NULL;

  gtk_widget_show(taxbird_window);

  taxbird_current_form = NULL;
  taxbird_document_filename = NULL;
  taxbird_document_changed = 0;
  taxbird_document_data = SCM_BOOL(0);

  /* add new window to the list of windows */
  return taxbird_window;
}



/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void
taxbird_ws_sel_form(int formid)
{
  GtkTreeViewColumn *column;
  GtkTreeStore *tree;
  GtkTreeModel *model;
  GtkTreeView *tv_sheets =
    GTK_TREE_VIEW(taxbird_glade_lookup(taxbird_gladexml_app, "tv_sheets"));

  g_return_if_fail(formid >= 0);
  g_return_if_fail(tv_sheets);

  if((model = gtk_tree_view_get_model(tv_sheets))) {
    /* there already is an attached model, replace it. */
    tree = GTK_TREE_STORE(model);
    gtk_tree_store_clear(model);
  } 
  else {
    /* add column */
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Available Sheets"),
						      renderer, "text", 0, NULL);
    gtk_tree_view_append_column(tv_sheets, column);

    /* add names of available sheets */
    tree = gtk_tree_store_new(1, G_TYPE_STRING);
    gtk_tree_view_set_model(tv_sheets, GTK_TREE_MODEL(tree));
  }

  SCM sheet_tree = scm_call_0(forms[formid]->get_sheet_tree);
  taxbird_ws_fill_tree_store(tree, NULL, sheet_tree);

  /* store selected form's id with the application window */
  taxbird_current_form = forms[formid];

  /* create a new dataset and attach to the application window */
  SCM data = scm_call_0(taxbird_current_form->dataset_create);

  /* we need to protect taxbird_document_data, since it resides on heap */
  if(SCM_NFALSEP(scm_list_p(taxbird_document_data)))
    scm_gc_unprotect_object(taxbird_document_data);
  taxbird_document_data = scm_gc_protect_object(scm_list_copy(data));

  /* remove old viewport */
  GtkWidget *child;
  GtkBin *viewport =
    GTK_BIN(taxbird_glade_lookup(taxbird_gladexml_app, "viewport"));
  
  if((child = GTK_WIDGET(gtk_bin_get_child(viewport))))
    gtk_widget_destroy(child);

  child = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(viewport), child);
  gtk_widget_show(child);
}



/* add necessary items (taken from the dataset) to the
 * GtkTreeStore recursively */
static void
taxbird_ws_fill_tree_store(GtkTreeStore *tree, GtkTreeIter *parent, SCM data)
{
  GtkTreeIter iter;

  /* if(SCM_SYMBOLP(data))
   *   data = scm_call_0(data);
   */

  g_return_if_fail(SCM_NFALSEP(scm_list_p(data)));
  g_return_if_fail(scm_ilength(data));

  while(scm_ilength(data)) {
    if(! scm_is_string(SCM_CAR(data))) {
      g_print("get-sheet-tree returned non-string object: ");
      gh_write(SCM_CAR(data));
      g_print("\n");
      break;
    }

    gtk_tree_store_append(tree, &iter, parent);
    char *leafname = scm_to_locale_string(SCM_CAR(data));
    gtk_tree_store_set(tree, &iter, 0, leafname, -1);
    free(leafname);

    data = SCM_CDR(data);

    if(scm_ilength(data) && SCM_NFALSEP(scm_list_p(SCM_CAR(data)))) {
      /* create sheet's nodes recursively */
      taxbird_ws_fill_tree_store(tree, &iter, SCM_CAR(data));
      data = SCM_CDR(data);
    }
  }
}



/* display sheet with provided id in the workspace of app_window */
void
taxbird_ws_sel_sheet(const char *sheetname)
{
    SCM sheet = scm_call_1(taxbird_current_form->get_sheet,
			 scm_makfrom0str(sheetname));

  if(SCM_FALSEP(scm_list_p(sheet))) 
    return; /* user didn't select a real sheet but only a node representing
	     * a parent in the tree  */  

  g_return_if_fail(scm_ilength(sheet) == 2);

  char *fn = scm_to_locale_string(SCM_CAR(sheet));
  char *root = scm_to_locale_string(SCM_CADR(sheet));

  taxbird_ws_activate_sheet(fn, root);

  free(root);
  free(fn);
}

void 
taxbird_ws_activate_sheet(const char *fn, const char *root)
{
  /* seek the guile search path for the xml file to use ... */
  char *lookup_fn = taxbird_guile_dirlist_lookup(fn);
  if(! lookup_fn) {
    g_printerr(PACKAGE_NAME ": cannot find file: %s\n", fn);
    return;
  }

  g_printerr(PACKAGE_NAME ": loading '%s' from '%s'\n", root, lookup_fn);

  /* remove old widget tree ... */
  GtkWidget *table;
  GtkBin *viewport =
    GTK_BIN(taxbird_glade_lookup(taxbird_gladexml_app, "viewport"));
  
  if((table = GTK_WIDGET(gtk_bin_get_child(viewport))))
    gtk_widget_destroy(table);

  /* create new widget tree  ... */
  taxbird_gladexml_sheet = glade_xml_new(lookup_fn, root, NULL);
  if(! taxbird_gladexml_sheet) {
    g_printerr(PACKAGE_NAME ": cannot find root-element '%s' in '%s'\n",
	       root, lookup_fn);
    g_free(lookup_fn);
    return;
  }

  g_free(lookup_fn);
  table = glade_xml_get_widget(taxbird_gladexml_sheet, root);

  /* make sure not to re-store field's values when loading them all */
  taxbird_ws_disable_storage_hook = 1;

  GList *widgets = glade_xml_get_widget_prefix(taxbird_gladexml_sheet, "");
  GList *ptr = widgets;

  if(ptr) {
    do {
      GtkWidget *w = GTK_WIDGET(ptr->data);

      if(! (GTK_IS_LABEL(w) || GTK_IS_TABLE(w) || GTK_IS_SEPARATOR(w)
	    || GTK_IS_IMAGE(w) || GTK_IS_SCROLLED_WINDOW(w)
	    || GTK_IS_BOX(w) || GTK_IS_ALIGNMENT(w))) {
	g_signal_connect(w, "focus-in-event",
			 G_CALLBACK(taxbird_ws_show_appbar_help), NULL);
	if(GTK_IS_BUTTON(w))
	  g_signal_connect(w, "clicked",
			   G_CALLBACK(taxbird_ws_store_event), NULL);
	else if(GTK_IS_TOGGLE_BUTTON(w))
	  g_signal_connect(w, "toggled",
			   G_CALLBACK(taxbird_ws_store_event), NULL);
	else if(GTK_IS_TREE_VIEW(w))
	  g_signal_connect(w, "cursor_changed",
			   G_CALLBACK(taxbird_ws_store_event), NULL);
	else
	  g_signal_connect(w, "changed",
			   G_CALLBACK(taxbird_ws_store_event), NULL);

	taxbird_ws_retrieve_field(w, glade_get_widget_name(w));
      }
    } while((ptr = ptr->next));
  
    g_list_free(widgets);
  }
  
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_container_add(GTK_CONTAINER(viewport), table);

  /* re-enable storage hook */
  taxbird_ws_disable_storage_hook = 0;
}



/* callback function called when entry fields are changed,
 * need to validate and store the entered data 
 */
static void
taxbird_ws_store_event(GtkWidget *w, gpointer user_data)
{
  (void) user_data;

  if(taxbird_ws_disable_storage_hook)
    return;

  /* figure out, what value to store */
  SCM sv;
  SCM wn = scm_makfrom0str(glade_get_widget_name(w));
  
  if(GTK_IS_ENTRY(w))
    sv = scm_makfrom0str(gtk_entry_get_text(GTK_ENTRY(w)));

  else if(GTK_IS_COMBO_BOX(w)) {
    int item = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
    g_return_if_fail(item >= 0);
    
    sv = scm_int2num(item);
  }

  else if(GTK_IS_RADIO_BUTTON(w)) {
    if(! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
      return; /* don't generate store signal for non-toggled items */

    GSList *grp = gtk_radio_button_get_group(GTK_RADIO_BUTTON(w));
    sv = wn;
    wn = scm_makfrom0str(glade_get_widget_name(GTK_WIDGET(grp->data)));
  }
  
  else if(GTK_IS_TREE_VIEW(w)) {
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));

    if(! gtk_tree_selection_get_selected (sel, &model, &iter))
      sv = SCM_BOOL(0);
    else {
      char *val;
      gtk_tree_model_get(model, &iter, 0, &val, -1);
      sv = scm_take0str(val);
    }
  }

  else if(GTK_IS_TOGGLE_BUTTON(w)) {
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
    sv = scm_makfrom0str(state ? "1" : "0");

  }

  else if(GTK_IS_BUTTON(w)) 
    sv = SCM_BOOL(1);

  else {
    g_assert_not_reached();
    return;
  }

  /* we need to lookup the help-widget this early, as the widget `w'
   * might disappear during the scheme call (in case the script decides to
   * display another sheet, leading in `w' to be destroyed) 
   */
  GtkWidget *helpw = taxbird_glade_lookup(taxbird_gladexml_app, "helptext");
  g_return_if_fail(helpw);
  GtkTooltipsData *tooltip = gtk_tooltips_data_get(w);
  const char *plain_helptext = tooltip ? tooltip->tip_text : "";

  /* call storage function ... */
  SCM result = scm_call_3(taxbird_current_form->dataset_write,
			  taxbird_document_data, wn, sv);
  
  if(SCM_FALSEP(result)) {
    /* the field's content is invalid, prepend warning to the bottom bar */
    char *helptext = 
      g_strdup_printf("<span foreground=\"red\" weight=\"bold\">%s</span>"
		      "\n\n%s",
		      _("The current field's content is not valid. "
			"Sorry."), plain_helptext);
    g_return_if_fail(helptext);

    gtk_label_set_markup(GTK_LABEL(helpw), helptext);

    g_free(helptext);
    return;
  }

  /* set changed flag */
  taxbird_document_changed = 1;

  /* now update all but the current field */
  const char *widget_name = glade_get_widget_name(w);
  if(widget_name) taxbird_ws_update_fields(widget_name);

  /* write out the current field's unmodified helptext,
   * this is to replace error-message enhanced versions, etc. */
  gtk_label_set_markup(GTK_LABEL(helpw), plain_helptext);
}



/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignored, may be NULL
 */
static void
taxbird_ws_update_fields(const char *exception)
{
  g_return_if_fail(exception);

  assert(taxbird_ws_disable_storage_hook == 0);
  assert(taxbird_gladexml_sheet);
  taxbird_ws_disable_storage_hook = 1;

  GList *widgets = glade_xml_get_widget_prefix(taxbird_gladexml_sheet, "");
  GList *ptr = widgets;

  if(ptr) {
    do {
      GtkWidget *w = GTK_WIDGET(ptr->data);

      if(strcmp(glade_get_widget_name(w), exception))
	taxbird_ws_retrieve_field(w, glade_get_widget_name(w));
    } while((ptr = ptr->next));
    
    g_list_free(widgets);
  }

  taxbird_ws_disable_storage_hook = 0;
}



static void
taxbird_ws_retrieve_field(GtkWidget *w, const char *field_name)
{
  if(GTK_IS_RADIO_BUTTON(w)) {
    /* radio buttons need to be handled different, so they come first :)   */
    GSList *grp = gtk_radio_button_get_group(GTK_RADIO_BUTTON(w));
    SCM wn = scm_makfrom0str(glade_get_widget_name(GTK_WIDGET(grp->data)));
    SCM v = scm_call_2(taxbird_current_form->dataset_read,
		       taxbird_document_data, wn);
    
    int val = 0;
    if(scm_is_string(v)) {
      char *selection = scm_to_locale_string(v);
      if(strcmp(selection, field_name) == 0) val = 1;
      free(selection);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), val);
    return;
  }


  /* okay, for non-radio-buttons we read the value first ... */
  SCM v = scm_call_2(taxbird_current_form->dataset_read,
		     taxbird_document_data, scm_makfrom0str(field_name));
  
  if(GTK_IS_ENTRY(w)) {
    if(scm_is_string(v)) {
      char *val = scm_to_locale_string(v);
      gtk_entry_set_text(GTK_ENTRY(w), val);
      free(val);
    }
    else
      gtk_entry_set_text(GTK_ENTRY(w), "");
  } 

  else if(GTK_IS_COMBO_BOX(w)) {
    if(scm_is_string(v)) {
      char *val = scm_to_locale_string(v);
      gtk_combo_box_set_active(GTK_COMBO_BOX(w), atoi(val));
      free(val);
    }

    /* don't make a default choice, it's pretty much unlikely we will be
     * right anyway */
  }

  else if(GTK_IS_TOGGLE_BUTTON(w)) {
    if(scm_is_string(v)) {
      char *val = scm_to_locale_string(v);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), atoi(val));
      free(val);
    }

    /* we don't make a default choice here, probably off is more sane, 
     * but who knows ... */
  }

}



static gboolean
taxbird_ws_show_appbar_help(GtkWidget *widget, GdkEventFocus *event,
			    gpointer user_data)
{
  (void) event;
  (void) user_data;

  GtkTooltipsData *tooltip = gtk_tooltips_data_get(widget);
  GtkWidget *helpw = taxbird_glade_lookup(taxbird_gladexml_app, "helptext");
  gtk_label_set_markup(GTK_LABEL(helpw), tooltip ? tooltip->tip_text : "");
  return FALSE; /* call other handlers as well */
}



/* load taxbird workspace from file */
void
taxbird_ws_open(const char *fname)
{
  SCM handle = scm_open_file(scm_makfrom0str(fname),
			     scm_makfrom0str("r"));

  SCM content = scm_eval_x(scm_read(handle), scm_current_module());

  if(! SCM_NFALSEP(scm_list_p(content)) || scm_ilength(content) != 2) {
    g_warning("unable to load file %s", fname);
    return;
  }

  char *formname = scm_to_locale_string(SCM_CAR(content));
  taxbird_ws_sel_form(taxbird_form_get_by_name(formname));
  free(formname);

  /* we need to protect taxbird_document_data, since it resides on heap */
  if(SCM_NFALSEP(scm_list_p(taxbird_document_data)))
    scm_gc_unprotect_object(taxbird_document_data);
  taxbird_document_data = scm_gc_protect_object(SCM_CADR(content));

  taxbird_document_filename = g_strdup(fname);
  taxbird_document_changed = 0;
}



/* store taxbird workspace to file */
void
taxbird_ws_save(const char *fname)
{
  if(! taxbird_current_form) {
    taxbird_dialog_error(NULL, _("Current document contains no data. "
				 "There's no point in writing it out."));
    return;
  }

  SCM handle = scm_open_file(scm_makfrom0str(fname), scm_makfrom0str("w"));

  /* write header */
  scm_display(scm_makfrom0str(";; This file was produced using taxbird. \n"
			      ";; You probably don't want to touch this \n"
			      ";; file. In case you do want it anyway, \n"
			      ";; be warned: BE CAREFUL!!\n;;\n\n'"), handle);

  /* write content */
  scm_write(scm_list_2(scm_makfrom0str(taxbird_current_form->name), 
		       taxbird_document_data),
	    handle);

  scm_close(handle);

  if(taxbird_document_filename != fname) {
    g_free(taxbird_document_filename);
    taxbird_document_filename = g_strdup(fname);
  }

  taxbird_document_changed = 0;
}

SCM
taxbird_ws_chooser_additem(SCM chooser, SCM item)
{
  if(! scm_is_string(chooser)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:chooser-additem"),
		  scm_makfrom0str("invalid first argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  if(! scm_is_string(item)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:chooser-additem"),
		  scm_makfrom0str("invalid second argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  char *widg = scm_to_locale_string(chooser);
  GtkTreeView *view =
    GTK_TREE_VIEW(taxbird_glade_lookup(taxbird_gladexml_sheet, widg));
  free(widg);

  g_return_val_if_fail(view, SCM_BOOL(0));

  GtkTreeStore *tree = GTK_TREE_STORE(gtk_tree_view_get_model(view));
  if(! tree) {
    tree = gtk_tree_store_new(1, G_TYPE_STRING);

    /* add column */
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = 
      gtk_tree_view_column_new_with_attributes("", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(view, column);

  }

  GtkTreeIter iter;
  gtk_tree_store_append(tree, &iter, NULL);
  char *itemname = scm_to_locale_string(item);
  gtk_tree_store_set(tree, &iter, 0, itemname, -1);
  free(itemname);
  gtk_tree_view_set_model(view, GTK_TREE_MODEL(tree));

  return item;
}

#if 0
/*
 * please leave this alone, it's just to force the catalogue to contain
 * the string `Yes!' which is used in some forms
 */
(void) _("Yes!");
#endif
