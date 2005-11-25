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

#include <gnome.h>
#include <assert.h>
#include <glade/glade.h>

#include "interface.h"
#include "dialog.h"
#include "workspace.h"
#include "form.h"
#include "guile.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name)              \
  g_object_set_data_full (G_OBJECT (component), name,		\
			  gtk_widget_ref (widget),              \
			  (GDestroyNotify) gtk_widget_unref)

/* add necessary items (taken from the dataset) to the
 * GtkTreeStore recursively */
static void taxbird_ws_fill_tree_store(GtkTreeStore *ts, GtkTreeIter *iter,
				       SCM dataset);

static void taxbird_ws_store_event(GtkWidget *w, gpointer user_data);

static void taxbird_ws_retrieve_field(GtkWidget *w, GtkWidget *appwin, 
				      const char *field_name);

/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignore, may be NULL  */
static void taxbird_ws_update_fields(GtkWidget *appwin, const char *exception);


static gboolean taxbird_ws_show_appbar_help(GtkWidget *widget,
					    GdkEventFocus *event,
					    gpointer user_data);

/* unprotect referenced SCM (callback for destroy notifications) */
static void taxbird_ws_unprotect_scm(gpointer d);

/* set this to true to disable the storage hook,
 * this is thought to be set while the fields are loaded */
static int taxbird_ws_disable_storage_hook = 0;

/* list of all application windows */
GSList *taxbird_windows = NULL;

static GtkWidget *taxbird_active_win = NULL;

/* create new taxbird workspace */
GtkWidget *
taxbird_ws_new(void)
{
  GtkWidget *taxbird = create_taxbird();
  if(!taxbird) return NULL;

  gtk_widget_show(taxbird);
  g_object_set_data(G_OBJECT(taxbird), "current_form", (void *) -1);
  g_object_set_data(G_OBJECT(taxbird), "filename", NULL);
  g_object_set_data(G_OBJECT(taxbird), "changed", (void *) 0);

  /* add new window to the list of windows */
  taxbird_windows = g_slist_prepend(taxbird_windows, taxbird);
  taxbird_active_win = taxbird;

  return taxbird;
}


/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void
taxbird_ws_sel_form(GtkWidget *appwin, int formid)
{
  if(! appwin) appwin = taxbird_active_win;

  GtkTreeViewColumn *column;
  GtkTreeStore *tree;
  GtkTreeView *tv_sheets = GTK_TREE_VIEW(lookup_widget(appwin, "tv_sheets"));

  g_return_if_fail(formid >= 0);
  g_return_if_fail(tv_sheets);

  /* add column */
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("Available Sheets"),
						    renderer, "text", 0, NULL);
  gtk_tree_view_append_column(tv_sheets, column);

  /* add names of available sheets */
  tree = gtk_tree_store_new(1, G_TYPE_STRING);
  gtk_tree_view_set_model(tv_sheets, GTK_TREE_MODEL(tree));
  SCM dataset = scm_call_0(forms[formid]->get_sheet_tree);
  taxbird_ws_fill_tree_store(tree, NULL, dataset);

  /* store selected form's id with the application window */
  g_object_set_data(G_OBJECT(appwin), "current_form", (void*) formid);

  /* create a new dataset and attach to the application window */
  dataset = scm_list_copy(scm_call_0(forms[formid]->dataset_create));
  g_object_set_data_full(G_OBJECT(appwin), "scm_data",
			 (void*) scm_gc_protect_object(dataset),
			 taxbird_ws_unprotect_scm);
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
    if(! SCM_STRINGP(SCM_CAR(data))) {
      g_print("get-sheet-tree returned non-string object: ");
      gh_write(SCM_CAR(data));
      g_print("\n");
      break;
    }

    gtk_tree_store_append(tree, &iter, parent);
    gtk_tree_store_set(tree, &iter, 0, SCM_STRING_CHARS(SCM_CAR(data)), -1);
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
taxbird_ws_sel_sheet(GtkWidget *appwin, const char *sheetname)
{
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  SCM sheet = scm_call_1(forms[current_form]->get_sheet,
			 scm_makfrom0str(sheetname));

  if(SCM_FALSEP(scm_list_p(sheet))) 
    return; /* user didn't select a real sheet but only a node representing
	     * a parent in the tree  */  

  g_return_if_fail(scm_ilength(sheet) == 2);

  const char *fn = SCM_STRING_CHARS(SCM_CAR(sheet));
  const char *root = SCM_STRING_CHARS(SCM_CADR(sheet));

  taxbird_ws_activate_sheet(appwin, fn, root);
}

void 
taxbird_ws_activate_sheet(GtkWidget *appwin, const char *fn, const char *root)
{
  if(! appwin) appwin = taxbird_active_win;
  g_return_if_fail(appwin);

  /* seek the guile search path for the xml file to use ... */
  char *lookup_fn = taxbird_guile_dirlist_lookup(fn);
  if(! lookup_fn) {
    g_printerr(PACKAGE_NAME ": cannot find file: %s\n", fn);
    return;
  }

  g_printerr(PACKAGE_NAME ": loading '%s' from '%s'\n", root, lookup_fn);

  /* remove old widget tree ... */
  GtkWidget *table;
  GtkBin *viewport = GTK_BIN(lookup_widget(appwin, "viewport"));
  
  if((table = GTK_WIDGET(gtk_bin_get_child(viewport))))
    gtk_widget_destroy(table);

  /* create new widget tree  ... */
  GladeXML *xml = glade_xml_new(lookup_fn, root, NULL);
  if(! xml) {
    g_printerr(PACKAGE_NAME ": cannot find root-element '%s' in '%s'\n",
	       root, lookup_fn);
    g_free(lookup_fn);
    return;
  }

  g_free(lookup_fn);
  table = glade_xml_get_widget(xml, root);

  /* keep a reference to the GladeXML structure */
  g_object_set_data_full(G_OBJECT(appwin), "gladexml", (void *) xml, NULL);

  /* make sure not to re-store field's values when loading them all */
  taxbird_ws_disable_storage_hook = 1;

  GList *widgets = glade_xml_get_widget_prefix(xml, "");
  GList *ptr = widgets;

  if(ptr) {
    do {
      GtkWidget *w = GTK_WIDGET(ptr->data);

      /* g_printerr("mangling widget: %s\n", glade_get_widget_name(w)); */
      GLADE_HOOKUP_OBJECT(appwin, w, glade_get_widget_name(w));

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

	taxbird_ws_retrieve_field(w, appwin, glade_get_widget_name(w));
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

  if(GTK_IS_ENTRY(w))
    sv = scm_makfrom0str(gtk_entry_get_text(GTK_ENTRY(w)));

  else if(GTK_IS_COMBO_BOX(w)) {
    int item = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
    g_return_if_fail(item >= 0);
    
    sv = scm_int2num(item);
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

  GtkWidget *appwin = lookup_widget(w, "taxbird");
  g_return_if_fail(appwin);
  taxbird_active_win = appwin;

  /* we need to lookup the help-widget this early, as the widget `w'
   * might disappear during the scheme call (in case the script decides to
   * display another sheet, leading in `w' to be destroyed) 
   */
  GtkWidget *helpw = lookup_widget(w, "helptext");
  g_return_if_fail(helpw);
  GtkTooltipsData *tooltip = gtk_tooltips_data_get(w);
  const char *plain_helptext = tooltip ? tooltip->tip_text : "";

  /* call storage function ... */
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  SCM result = scm_call_3(forms[current_form]->dataset_write,
			  (SCM)g_object_get_data(G_OBJECT(appwin), "scm_data"),
			  scm_makfrom0str(glade_get_widget_name(w)), sv);
  
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
  g_object_set_data(G_OBJECT(appwin), "changed", (void *) 1);

  /* now update all but the current field */
  const char *widget_name = glade_get_widget_name(w);
  if(widget_name) taxbird_ws_update_fields(appwin, widget_name);

  /* write out the current field's unmodified helptext,
   * this is to replace error-message enhanced versions, etc. */
  gtk_label_set_markup(GTK_LABEL(helpw), plain_helptext);
}



/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignored, may be NULL
 */
static void
taxbird_ws_update_fields(GtkWidget *appwin, const char *exception)
{
  g_return_if_fail(exception);

  assert(taxbird_ws_disable_storage_hook == 0);
  taxbird_ws_disable_storage_hook = 1;

  GladeXML *xml = g_object_get_data(G_OBJECT(appwin), "gladexml");
  g_return_if_fail(xml);

  GList *widgets = glade_xml_get_widget_prefix(xml, "");
  GList *ptr = widgets;

  if(ptr) {
    do {
      GtkWidget *w = GTK_WIDGET(ptr->data);

      if(strcmp(glade_get_widget_name(w), exception))
	taxbird_ws_retrieve_field(w, appwin, glade_get_widget_name(w));
    } while((ptr = ptr->next));
    
    g_list_free(widgets);
  }

  taxbird_ws_disable_storage_hook = 0;
}



static void
taxbird_ws_retrieve_field(GtkWidget *w, GtkWidget *appwin,
			  const char *field_name)
{
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  SCM v = scm_call_2(forms[current_form]->dataset_read,
		     g_object_get_data(G_OBJECT(appwin), "scm_data"),
		     scm_makfrom0str(field_name));
  
  if(GTK_IS_ENTRY(w)) {
    if(SCM_STRINGP(v)) {
      gtk_entry_set_text(GTK_ENTRY(w), SCM_STRING_CHARS(v));
    }
    else
      gtk_entry_set_text(GTK_ENTRY(w), "");
  } 

  else if(GTK_IS_COMBO_BOX(w)) {
    if(SCM_STRINGP(v))
      gtk_combo_box_set_active(GTK_COMBO_BOX(w), atoi(SCM_STRING_CHARS(v)));

    /* don't make a default choice, it's pretty much unlikely we will be
     * right anyway */
  }

  else if(GTK_IS_TOGGLE_BUTTON(w)) {
    if(SCM_STRINGP(v))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
				   atoi(SCM_STRING_CHARS(v)));

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
  GtkWidget *helpw = lookup_widget(widget, "helptext");
  gtk_label_set_markup(GTK_LABEL(helpw), tooltip ? tooltip->tip_text : "");
  return FALSE; /* call other handlers as well */
}



/* load taxbird workspace from file */
void
taxbird_ws_open(GtkWidget *appwin, const char *fname)
{
  SCM content;
  int formid;
  SCM handle = scm_open_file(scm_makfrom0str(fname),
			     scm_makfrom0str("r"));

  content = scm_read_and_eval_x(handle);

  if(! SCM_NFALSEP(scm_list_p(content)) || scm_ilength(content) != 2) {
    g_warning("unable to load file %s", fname);
    return;
  }

  formid = taxbird_form_get_by_name(SCM_STRING_CHARS(SCM_CAR(content)));
  taxbird_ws_sel_form(appwin, formid);

  g_object_set_data_full(G_OBJECT(appwin), "scm_data",
			 scm_gc_protect_object(SCM_CADR(content)),
			 taxbird_ws_unprotect_scm);

  if(g_object_get_data(G_OBJECT(appwin), "filename") != fname)
    g_object_set_data_full(G_OBJECT(appwin), "filename",
			   g_strdup(fname), g_free);

  g_object_set_data(G_OBJECT(appwin), "changed", (void *) 0);
}



/* store taxbird workspace to file */
void
taxbird_ws_save(GtkWidget *appwin, const char *fname)
{
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  if(current_form == -1) {
    taxbird_dialog_error(appwin, _("Current document contains no data. "
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
  SCM data = g_object_get_data(G_OBJECT(appwin), "scm_data");
  scm_write(scm_list_2(scm_makfrom0str(forms[current_form]->name), data),
	    handle);

  scm_close(handle);

  if(g_object_get_data(G_OBJECT(appwin), "filename") != fname)
    g_object_set_data_full(G_OBJECT(appwin), "filename",
			   g_strdup(fname), g_free);

  g_object_set_data(G_OBJECT(appwin), "changed", (void *) 0);
}

static void
taxbird_ws_unprotect_scm(gpointer d)
{
  scm_gc_unprotect_object((SCM)d);
}

SCM
taxbird_ws_chooser_additem(SCM chooser, SCM item)
{
  if(! SCM_STRINGP(chooser)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:chooser-additem"),
		  scm_makfrom0str("invalid first argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  if(! SCM_STRINGP(item)) {
    scm_error_scm(scm_c_lookup_ref("wrong-type-arg"),
		  scm_makfrom0str("tb:chooser-additem"),
		  scm_makfrom0str("invalid second argument, string expected"),
		  SCM_EOL, SCM_BOOL(0));
    return SCM_BOOL(0);
  }

  GtkTreeView *view = GTK_TREE_VIEW(lookup_widget(taxbird_active_win,
						  SCM_STRING_CHARS(chooser)));
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
  gtk_tree_store_set(tree, &iter, 0, SCM_STRING_CHARS(item), -1);
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
