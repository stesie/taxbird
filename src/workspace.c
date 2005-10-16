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

#define SCM_CADDDDR(a) SCM_CAR(SCM_CDDDDR(a))

/* add necessary items (taken from the dataset) to the
 * GtkTreeStore recursively */
static void taxbird_ws_fill_tree_store(GtkTreeStore *ts, GtkTreeIter *iter,
				       SCM dataset);

static void taxbird_ws_store_event(GtkWidget *w, gpointer user_data);

/* static void taxbird_ws_retrieve_field(GtkWidget *w, GtkWidget *appwin, 
 *				      const char *field_name);
 */

/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignore, may be NULL  */
static void taxbird_ws_update_fields(GtkWidget *appwin, const char *exception);


static gboolean taxbird_ws_show_appbar_help(GtkWidget *widget,
					    GdkEventFocus *event,
					    gpointer user_data);

/* unprotect referenced SCM (callback for destroy notifications) */
static void taxbird_ws_unprotect_scm(gpointer d);

/* generic callback function for tb:field:button's ... */
static void taxbird_ws_button_callback(GtkWidget *button, void *data);

/* set this to true to disable the storage hook,
 * this is thought to be set while the fields are loaded */
static int taxbird_ws_disable_storage_hook = 0;

/* list of all application windows */
GSList *taxbird_windows = NULL;


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

  return taxbird;
}


/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void
taxbird_ws_sel_form(GtkWidget *appwin, int formid)
{
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
  SCM dataset = scm_call_0(forms[formid]->get_sheet_tree);
  taxbird_ws_fill_tree_store(tree, NULL, dataset);
  gtk_tree_view_set_model(tv_sheets, GTK_TREE_MODEL(tree));

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
  GtkWidget *table;
  GtkBin *viewport = GTK_BIN(lookup_widget(appwin, "viewport"));
  
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  SCM sheet = scm_call_1(forms[current_form]->get_sheet,
			 scm_makfrom0str(sheetname));

  if(SCM_FALSEP(scm_list_p(sheet))) 
    return; /* user didn't select a real sheet but only a node representing
	     * a parent in the tree  */  

  g_return_if_fail(scm_ilength(sheet) == 2);

  const char *fn = SCM_STRING_CHARS(SCM_CAR(sheet));
  const char *root = SCM_STRING_CHARS(SCM_CADR(sheet));

  /* seek the guile search path for the xml file to use ... */
  char *lookup_fn = taxbird_guile_dirlist_lookup(fn);
  if(! lookup_fn) {
    g_printerr(PACKAGE_NAME ": cannot find file: %s\n", fn);
    return;
  }

  g_printerr(PACKAGE_NAME ": loading '%s' from '%s'\n", root, lookup_fn);

  /* remove old widget tree ... */
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

  GList *widgets = glade_xml_get_widget_prefix(xml, "");
  GList *ptr = widgets;

  if(ptr) do {
    GtkWidget *w = GTK_WIDGET(ptr->data);
    if(! GTK_IS_LABEL(w))
      g_signal_connect(w, "focus-in-event",
		       G_CALLBACK(taxbird_ws_show_appbar_help), NULL);
  } while((ptr = ptr->next));
  
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_container_add(GTK_CONTAINER(viewport), table);
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

  /* fetch specs for the selected field (GtkWidget *w) */
  SCM specs = (SCM) g_object_get_data(G_OBJECT(w), "scm_specs");
  g_return_if_fail(SCM_NFALSEP(scm_list_p(specs)));

  GtkWidget *appwin = lookup_widget(w, "taxbird");

  /* get the plain helptext to display in the bottom bar */
  //SCM plain_helptext = taxbird_ws_get_helptext(appwin, specs);
  GtkWidget *helpw = lookup_widget(w, "helptext");

  //g_return_if_fail(SCM_STRINGP(plain_helptext));


#if 0
  /* validate field's content */
  if(1) {
    /* the field's content is invalid, prepend warning to the bottom bar */
    char *helptext = 
      g_strdup_printf("<span foreground=\"red\" weight=\"bold\">%s</span>"
		      "\n\n%s",
		      _("The current field's content is not valid. "
			"Sorry."), SCM_STRING_CHARS(plain_helptext));
    g_return_if_fail(helptext);

    gtk_label_set_markup(GTK_LABEL(helpw), helptext);

    g_free(helptext);
    return;
  }
#endif     
  /* figure out, what value to store */
  SCM store_value;

  if(GTK_IS_ENTRY(w))
    store_value = scm_makfrom0str(gtk_entry_get_text(GTK_ENTRY(w)));

  else if(GTK_IS_COMBO_BOX(w)) {
    int item = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
    g_return_if_fail(item >= 0);
    
    store_value = scm_int2num(item);
  }

  else if(GTK_IS_TOGGLE_BUTTON(w)) {
    int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
    store_value = scm_makfrom0str(state ? "1" : "0");

  } else {
    g_assert_not_reached();
    return;
  }

  
  /* call storage function ... */
  SCM field = SCM_CADR(specs);
  g_return_if_fail(SCM_STRINGP(field));

  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");

  scm_call_3(forms[current_form]->dataset_write,
	     (SCM) g_object_get_data(G_OBJECT(appwin), "scm_data"),
	     field, store_value);


  /* set changed flag */
  g_object_set_data(G_OBJECT(appwin), "changed", (void *) 1);


  /* now update all but the current field */
  taxbird_ws_update_fields(appwin, SCM_STRING_CHARS(field));


  /* write out the current field's unmodified helptext,
   * this is to replace error-message enhanced versions, etc. */
  //gtk_label_set_markup(GTK_LABEL(helpw), SCM_STRING_CHARS(plain_helptext));
}



/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignored, may be NULL
 */
static void
taxbird_ws_update_fields(GtkWidget *appwin, const char *exception)
{
  assert(taxbird_ws_disable_storage_hook == 0);
  taxbird_ws_disable_storage_hook = 1;

  /* figure out, which sheet's selected */
  GtkTreeView *tv = GTK_TREE_VIEW(lookup_widget(appwin, "tv_sheets"));
  GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);

  GtkTreeModel *model;
  GtkTreeIter iter;
  if(! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  char *sheetname;
  gtk_tree_model_get(model, &iter, 0, &sheetname, -1);

  /* retrieve associated SCM structure */
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");
  SCM sheet = scm_call_1(forms[current_form]->get_sheet,
			 scm_makfrom0str(sheetname));

  /* get rid of the name */
  g_free(sheetname);

  g_return_if_fail(SCM_NFALSEP(scm_list_p(sheet)));
  g_return_if_fail(scm_ilength(sheet));

  g_return_if_fail(! SCM_STRINGP(SCM_CAR(sheet)));

  /* now recurse through the whole sheet (all rows and colums and 
   * call the retrieval function), to update each field's content
   */
  sheet = SCM_CDR(sheet); /* skip the number of columns field */
  for(; scm_ilength(sheet); sheet = SCM_CDR(sheet)) {
    SCM specs = SCM_CAR(sheet);
      
    if(SCM_SYMBOLP(specs))
      /* refrenced by variable, resolve it */
      specs = scm_c_lookup_ref(SCM_SYMBOL_CHARS(specs));

    /* resolve quotes */
    while(SCM_SYMBOLP(SCM_CAR(specs)))
      specs = scm_call_0(specs);

    specs = SCM_CDR(specs); /* skip the line name */
    for(; scm_ilength(specs); specs = SCM_CDDDDR(specs)) {
      g_return_if_fail(SCM_NUMBERP(SCM_CAR(specs)));

      g_return_if_fail(SCM_STRINGP(SCM_CADR(specs)));
      const char *input_name = SCM_STRING_CHARS(SCM_CADR(specs));

      /* retrieve content of field ... */
      if(!exception || strcmp(input_name, exception)) {
	GtkWidget *input = lookup_widget(appwin, input_name);
	taxbird_ws_retrieve_field(input, appwin, input_name);
      }
    }
  }

  taxbird_ws_disable_storage_hook = 0;
}



static void
taxbird_ws_button_callback(GtkWidget *button, void *data)
{
  (void) data;

  SCM specs = (SCM) g_object_get_data(G_OBJECT(button), "scm_specs");
  g_return_if_fail(SCM_NFALSEP(scm_list_p(specs)));

  /* validate field's value, in case it's a text entry field */
  SCM validatfunc = SCM_CADDDR(specs);

  if(SCM_SYMBOLP(validatfunc))
    /* immediate symbol (i.e. defined function), resolve and execute then */
    validatfunc = scm_c_lookup_ref(SCM_SYMBOL_CHARS(validatfunc));

  if(SCM_NFALSEP(scm_list_p(validatfunc)))
    /* probably some kind of (lambda (v buf) (validator v)) thingy ... */
    validatfunc = scm_call_0(validatfunc);

  if(SCM_FALSEP(scm_procedure_p(validatfunc))) {
    /* not a function */
    g_warning("something strange found, where a procedure was expected: ");
    gh_display(validatfunc);
    return;
  }

  GtkWidget *appwin = lookup_widget(button, "taxbird");
  scm_call_2(validatfunc, SCM_BOOL(0),
	     (SCM) g_object_get_data(G_OBJECT(appwin), "scm_data"));
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
