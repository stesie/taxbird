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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "interface.h"
#include "workspace.h"
#include "form.h"
#include "guile.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name)              \
  g_object_set_data_full (G_OBJECT (component), name,		\
			  gtk_widget_ref (widget),              \
			  (GDestroyNotify) gtk_widget_unref)

#define gh_cadddr(a) (gh_car(gh_cdddr(a)))
#define gh_cddddr(a) (gh_cdr(gh_cdddr(a)))
#define gh_caddddr(a) (gh_cadr(gh_cdddr(a)))

/* add necessary items (taken from the dataset) to the
 * GtkTreeStore recursively */
static void taxbird_ws_fill_tree_store(GtkTreeStore *ts, GtkTreeIter *iter,
				       SCM dataset);

static void taxbird_ws_store_event(GtkWidget *w, gpointer user_data);

static void taxbird_ws_retrieve_field(GtkWidget *w, struct taxbird_ws *ws,
				      const char *field_name);

static gboolean taxbird_ws_show_appbar_help(GtkWidget *widget,
                                            GdkEventFocus *event,
                                            gpointer user_data);

static GtkWidget *taxbird_ws_create_input(SCM specs);
static GtkWidget *taxbird_ws_create_output(SCM specs);
static GtkWidget *taxbird_ws_create_chooser(SCM specs);

static struct {
  GtkWidget *(*new)(SCM specs);
} taxbird_ws_field_creators[] = {
  { taxbird_ws_create_input }, /* FIELD_TEXT_INPUT */
  { taxbird_ws_create_output }, /* FIELD_TEXT_OUTPUT */
  { taxbird_ws_create_chooser }, /* FIELD_CHOOSER */
  { NULL },
  { taxbird_ws_create_output }, /* FIELD_TEXT_INPUT_CALC (2nd field) */
};


/* scan the provided dataset (gh_list) for a sheet named 'sheet_name',
 * and return it's definition
 */
static SCM taxbird_ws_lookup_sheet(SCM dataset, const char *sheet_name);



/* create new taxbird workspace */
void
taxbird_ws_new(void)
{
  struct taxbird_ws *ws = g_malloc(sizeof(*ws));
  GtkWidget *taxbird = create_taxbird();

  if(! ws || !taxbird) {
    g_free(ws);
    if(taxbird) gtk_widget_destroy(taxbird);
  }

  gtk_widget_show(taxbird);

  gtk_object_set_user_data(GTK_OBJECT(taxbird), ws);

  ws->current_form = -1;
  ws->fname = NULL;
  ws->changed = 0;
}


/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void
taxbird_ws_sel_form(GtkWidget *appwin, int formid)
{
  SCM dataset;
  GtkTreeView *tv_sheets = GTK_TREE_VIEW(lookup_widget(appwin, "tv_sheets"));

  g_return_if_fail(formid >= 0);
  g_return_if_fail(tv_sheets);

  dataset = forms[formid]->dataset;
  g_return_if_fail(gh_list_p(dataset));
  
  /* add column */
  {
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    column = gtk_tree_view_column_new_with_attributes(_("Available Sheets"),
						      renderer, "text", 0,
						      NULL);
    gtk_tree_view_append_column(tv_sheets, column);
  }

  /* add names of available sheets */
  {
    GtkTreeStore *tree = gtk_tree_store_new(1, G_TYPE_STRING);

    taxbird_ws_fill_tree_store(tree, NULL, dataset);
    gtk_tree_view_set_model(tv_sheets, GTK_TREE_MODEL(tree));
  }

  taxbird_ws_get(appwin)->current_form = formid;
  taxbird_ws_get(appwin)->ws_data = gh_call0(forms[formid]->dataset_create);
}


/* add necessary items (taken from the dataset) to the
 * GtkTreeStore recursively */
static void
taxbird_ws_fill_tree_store(GtkTreeStore *tree, GtkTreeIter *parent, SCM dataset)
{
  /* dataset actually represents a whole tree of sheets with field definitions
   * included, destinguishing works this way:
   *
   * take the car of the dataset list
   *  - if it's a number it is a constant specifing a field type
   *    -> the cadr is the definition (label etc.) of the field
   *
   *  - if it's a string, it's the name of another sheet
   *    -> the cadr is the further definition of this sheet (i.e. subsheets
   *       and fields)
   */
  GtkTreeIter iter;

  if(gh_symbol_p(dataset))
    dataset = scm_primitive_eval(dataset);

  g_return_if_fail(gh_list_p(dataset));
  g_return_if_fail(gh_length(dataset));

  if(gh_symbol_p(gh_car(dataset))) {
    SCM symbol = gh_lookup(SCM_SYMBOL_CHARS(gh_car(dataset)));

    /* ignore, symbols of number-type as those are the constants we use
     * to indicate fields, try to evaluate everything else ... */

    if(! gh_number_p(symbol))
      /* probably quoted list, or whatever else, interpret it and
       * call filling routine recursively */
      taxbird_ws_fill_tree_store(tree, parent, scm_primitive_eval(dataset));

    return;
  }

  if(gh_number_p(gh_car(dataset)))
    return; /* okay, this sheet is a leaf (holding fields) */

  while(gh_length(dataset)) {
    g_return_if_fail(gh_string_p(gh_car(dataset)));

    gtk_tree_store_append(tree, &iter, parent);
    gtk_tree_store_set(tree, &iter, 0, SCM_STRING_CHARS(gh_car(dataset)), -1);

    /* create sheet's nodes recursively */
    taxbird_ws_fill_tree_store(tree, &iter, gh_cadr(dataset));

    dataset = gh_cddr(dataset);
  }
}



/* display sheet with provided id in the workspace of app_window */
void
taxbird_ws_sel_sheet(GtkWidget *appwin, const char *sheetname)
{
  GtkBin *viewport = GTK_BIN(lookup_widget(appwin, "viewport"));
  int forms_id = taxbird_ws_get(appwin)->current_form;
  SCM sheet = taxbird_ws_lookup_sheet(forms[forms_id]->dataset, sheetname);

  g_return_if_fail(gh_list_p(sheet));
  g_return_if_fail(gh_length(sheet));

  if(gh_string_p(gh_car(sheet)))
    return; /* user didn't select a real sheet but only a node representing
	     * a parent in the tree  */  

  /* remove old widget tree ... */
  {
    GtkWidget *child = GTK_WIDGET(gtk_bin_get_child(viewport));
    if(child) 
      gtk_widget_destroy(child);
  }

  /* create new widget tree  ... */
  {
    int item = 0;

    GtkWidget *table = gtk_table_new(gh_length(sheet) >> 1, 3, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 10);

    /* connect new table to the viewport - we need to do this so early,
     * as the storage callbacks (using glade's lookup code) require the whole
     * widget tree to be set up properly */
    gtk_container_add(GTK_CONTAINER(viewport), table);

    while(! gh_null_p(sheet)) {
      SCM specs = gh_cadr(sheet);
      
      if(gh_symbol_p(specs))
	/* refrenced by variable, resolve it */
	specs = gh_lookup(SCM_SYMBOL_CHARS(specs));

      /* resolve quotes */
      while(gh_symbol_p(gh_car(specs)))
	specs = scm_primitive_eval(specs);

      char *ws_field_type_sym = SCM_SYMBOL_CHARS(gh_car(sheet));
      int ws_field_type = gh_scm2int(gh_lookup(ws_field_type_sym));
      GtkWidget *input =
	taxbird_ws_field_creators[ws_field_type & 3].new(specs);

      { /* add description label */
	char *buf = gh_scm2newstr(gh_cadr(specs), NULL);
	GtkWidget *label = gtk_label_new(buf);
	free(buf);

	gtk_label_set_line_wrap(GTK_LABEL(label), 1);
	gtk_widget_show(label);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, item, item + 1,
			 (GtkAttachOptions) (GTK_FILL),
			 (GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
      }

      /* we need to attach the widget to the table (thus the widget tree itself)
       * rather early (i.e. before calling the retrieval func) as it might
       * use Glade's lookup code
       */
      gtk_object_set_user_data(GTK_OBJECT(input), (void *) specs);
      gtk_table_attach(GTK_TABLE(table), input, 1,
		       ws_field_type & 4 ? 2 : 3, item, item + 1,
		       (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		       (GtkAttachOptions) (0), 0, 0);
      GLADE_HOOKUP_OBJECT(appwin, input, 
			  SCM_STRING_CHARS(gh_car(specs)));

      /* retrieve content */
      taxbird_ws_retrieve_field(input, taxbird_ws_get(appwin),
				SCM_STRING_CHARS(gh_car(specs)));

      /* set callback responsible for displaying help text in appbar */
      g_signal_connect((gpointer) input, "focus-in-event",
		       G_CALLBACK(taxbird_ws_show_appbar_help),
		       gh_caddr(specs));
				  
      gtk_widget_show(input);


      if(ws_field_type & 4) {
	/* second field on same row */
	char *field_name = SCM_STRING_CHARS(gh_caddddr(specs));
	GtkWidget *input = taxbird_ws_field_creators[ws_field_type].new(specs);

	/* insert the widget into the widget tree as early as possible */
	gtk_object_set_user_data(GTK_OBJECT(input), (void *) gh_cddddr(specs));
	gtk_table_attach(GTK_TABLE(table), input, 2, 3, item, item + 1,
			 (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
			 (GtkAttachOptions) (0), 0, 0);
	GLADE_HOOKUP_OBJECT(appwin, input, field_name);

	taxbird_ws_retrieve_field(input, taxbird_ws_get(appwin), field_name);
	gtk_widget_show(input);
	
      }

      sheet = gh_cddr(sheet);
      item ++;
    }

    gtk_widget_show(table);
  }
}



/* callback function called when entry fields are left,
 * need to validate and store the entered data 
 */
static void
taxbird_ws_store_event(GtkWidget *w, gpointer user_data)
{
  (void) user_data;

  GtkWidget *appwin = lookup_widget(w, "taxbird");
  SCM field = (SCM) gtk_object_get_user_data(GTK_OBJECT(w));
  SCM validatfunc;
  int valid = 0;
  g_return_if_fail(gh_list_p(field));

  /* field points to the ("intname" "name" "desc" validate-func) list */

  if(GTK_IS_ENTRY(w)) {
    /* validate field's value, in case it's a text entry field */
    validatfunc = gh_cadddr(field);

    if(gh_symbol_p(validatfunc))
      /* immediate symbol (i.e. defined function), resolve and execute then */
      validatfunc = gh_lookup(SCM_SYMBOL_CHARS(validatfunc));

    if(gh_list_p(validatfunc))
      /* probably some kind of (lambda (v buf) (validator v)) thingy ... */
      validatfunc = scm_primitive_eval(validatfunc);

    if(gh_procedure_p(validatfunc))
      /* execute validator function */
      validatfunc = gh_call2(validatfunc,
			     scm_makfrom0str(gtk_entry_get_text(GTK_ENTRY(w))),
			     taxbird_ws_get(appwin)->ws_data);

    if(gh_boolean_p(validatfunc))
      valid = gh_scm2bool(validatfunc);
    else {
      g_warning("damn, who coded this piece of scheme code? don't know "
		"what to do with validator func ...");
      gh_display(validatfunc);
    }
  }
  else {
    /* consider output fields, choosers etc. to be valid */
    valid = 1;
  }

  /* store data if it's valid */
  if(valid) {
    SCM store_value;

    static GdkColor taxbird_color_okay = { 0, 32767, 32767, 32767 };
    gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &taxbird_color_okay);

    if(GTK_IS_ENTRY(w))
      store_value = scm_makfrom0str(gtk_entry_get_text(GTK_ENTRY(w)));
    else if(GTK_IS_COMBO_BOX(w)) {
      /* I'm currently unsure whether storing the value itself (i.e. the
       * text representation) or the offset is more clever. For the time being
       * I think storing the offset is easier (especially since
       * gtk_combo_box_get_active_text is not available and would have to be
       * worked around) ... */

      /* gtk_combo_box_get_active_text is available from Gtk 2.6 on, to work
       * with wide spread 2.4 as well, work around ... 
       *
       * perhaps conditionally compile and use 2.6 function if available!! 
       */
      
      /* store_value =
       *   scm_take0str(gtk_combo_box_get_active_text(GTK_COMBO_BOX(w)));
       */

      int item = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
      g_return_if_fail(item >= 0);

      store_value = gh_int2scm(item);

      /* SCM entries = gh_cadddr(field);
       *
       * while(item --)
       *   entries = gh_cdr(entries);
       *
       * g_printerr("selected item is '%s'.\n",
       *            SCM_STRING_CHARS(gh_car(entries)));
       */
    }
    else
      g_assert_not_reached();

    field = gh_car(field);
    g_return_if_fail(gh_string_p(field));

    gh_call3(forms[taxbird_ws_get(appwin)->current_form]->dataset_write,
	     taxbird_ws_get(appwin)->ws_data, field, store_value);
  }
  else {
    /* draw red border (background) around textbox, to show the chosen value is
     * not valid. 
     */
    static GdkColor taxbird_color_red = { 0, 65535, 0, 0 };
    gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &taxbird_color_red);
  }
}


static SCM
taxbird_ws_lookup_sheet(SCM dataset, const char *needle)
{
  if(gh_symbol_p(dataset))
    dataset = scm_primitive_eval(dataset);

  g_return_val_if_fail(gh_list_p(dataset), gh_bool2scm(0));
  g_return_val_if_fail(gh_length(dataset), gh_bool2scm(0));
  
  if(gh_symbol_p(gh_car(dataset))) {
    SCM symbol = gh_lookup(SCM_SYMBOL_CHARS(gh_car(dataset)));

    if(! gh_number_p(symbol))
      return taxbird_ws_lookup_sheet(scm_primitive_eval(dataset), needle);

    else
      return gh_bool2scm(0);
  }
    
  if(gh_number_p(gh_car(dataset)))
    return gh_bool2scm(0); /* sheet definition, no more sub-sheets */

  while(! gh_null_p(dataset)) {
    g_return_val_if_fail(gh_string_p(gh_car(dataset)), gh_bool2scm(0));

    if(! strcmp(SCM_STRING_CHARS(gh_car(dataset)), needle)) {
      SCM sheet = gh_cadr(dataset);

      while(gh_symbol_p(sheet))
	sheet = scm_primitive_eval(sheet);

      g_return_val_if_fail(gh_list_p(sheet), gh_bool2scm(0));

      /* now try to find out whether the list is quoted or requires a
       * procedure to be called (the first symbol may be a numeric
       * constant (telling the type of field)
       */
      if(gh_symbol_p(gh_car(sheet))) {
	SCM symbol = gh_lookup(SCM_SYMBOL_CHARS(gh_car(sheet)));
	if(! gh_number_p(symbol))
	  sheet = scm_primitive_eval(sheet);
      }

      return sheet;
    }
    else {
      /* try recursing down */
      SCM retval = taxbird_ws_lookup_sheet(gh_cadr(dataset), needle);

      if(gh_list_p(retval))
	return retval; /* got it, pass it back up */
    }

    dataset = gh_cddr(dataset);
  }

  return gh_bool2scm(0);
}


static void
taxbird_ws_retrieve_field(GtkWidget *w, struct taxbird_ws *ws,
			  const char *field_name)
{
  SCM v = gh_call2(forms[ws->current_form]->dataset_read,
		   ws->ws_data, scm_makfrom0str(field_name));
  
  if(GTK_IS_ENTRY(w)) {
    if(gh_string_p(v)) {
      gtk_entry_set_text(GTK_ENTRY(w), SCM_STRING_CHARS(v));
    }
    else
      gtk_entry_set_text(GTK_ENTRY(w), "");
  } 
  else if(GTK_IS_COMBO_BOX(w)) {
    if(gh_number_p(v))
      gtk_combo_box_set_active(GTK_COMBO_BOX(w), gh_scm2int(v));
    /* don't make a default choice, it's pretty much unlikely we will be
     * right anyway */
  }
}



static GtkWidget *
taxbird_ws_create_input(SCM specs)
{
  (void) specs;

  GtkWidget *input = gtk_entry_new();
  g_signal_connect((gpointer) input, "changed",
		   G_CALLBACK(taxbird_ws_store_event), NULL);

  return input;
}



static GtkWidget *
taxbird_ws_create_output(SCM specs)
{
  (void) specs;

  GtkWidget *w = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(w), FALSE);

  return w;
}



static GtkWidget *
taxbird_ws_create_chooser(SCM specs)
{
  GtkWidget *w; 
  SCM entries = gh_cadddr(specs);
  g_return_val_if_fail(gh_list_p(entries), NULL);

  w = gtk_combo_box_new_text();

  while(gh_length(entries)) {
    /* g_printerr("adding option '%s' to chooser.\n",
     *            SCM_STRING_CHARS(gh_car(entries)));
     */
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), 
			      SCM_STRING_CHARS(gh_car(entries)));
    entries = gh_cdr(entries);
  }

  g_signal_connect((gpointer) w, "changed",
		   G_CALLBACK(taxbird_ws_store_event), NULL);

  return w;
}


static gboolean
taxbird_ws_show_appbar_help(GtkWidget *widget, GdkEventFocus *event,
			    gpointer user_data)
{
  (void) event;

  SCM helptext = user_data;
  GtkWidget *appbar = lookup_widget(widget, "appbar");

  if(gh_list_p(helptext))
    /* probably a command, execute it */
    helptext = scm_primitive_eval(helptext);

  if(gh_string_p(helptext))
    gnome_appbar_set_status(GNOME_APPBAR(appbar), SCM_STRING_CHARS(helptext));

  else {
    g_warning("stumbling over invalid help text, see gh_display's output.\n");
    gh_display(helptext);
  }

  return FALSE; /* call other handlers as well */
}



/* load taxbird workspace from file */
void
taxbird_ws_open(GtkWidget *appwin, const char *fname)
{
  SCM content;
  int formid;
  struct taxbird_ws *ws = taxbird_ws_get(appwin);
  SCM handle = scm_open_file(scm_makfrom0str(fname),
			     scm_makfrom0str("r"));

  content = scm_read_and_eval_x(handle);

  if(! gh_list_p(content) || gh_length(content) != 2) {
    g_warning("unable to load file %s", fname);
    return;
  }

  formid = taxbird_form_get_by_name(SCM_STRING_CHARS(gh_car(content)));
  taxbird_ws_sel_form(appwin, formid);
		      
  ws->ws_data = gh_cadr(content);

  if(ws->fname != fname) {
    free(ws->fname);
    ws->fname = strdup(fname);
  }

  ws->changed = 0;
}



/* store taxbird workspace to file */
void
taxbird_ws_save(GtkWidget *appwin, const char *fname)
{
  struct taxbird_ws *ws = taxbird_ws_get(appwin);

  SCM handle = scm_open_file(scm_makfrom0str(fname),
			     scm_makfrom0str("w"));

  /* write header */
  scm_display(scm_makfrom0str(";; This file was produced using taxbird. \n"
			      ";; You probably don't want to touch this \n"
			      ";; file. In case you do want it anyway, \n"
			      ";; be warned: BE CAREFUL!!\n;;\n\n'"), handle);

  /* write content */
  scm_write(scm_list_2(scm_makfrom0str(forms[ws->current_form]->name),
		       taxbird_ws_get(appwin)->ws_data), handle);

  scm_close(handle);

  if(ws->fname != fname) {
    free(ws->fname);
    ws->fname = strdup(fname);
  }

  ws->changed = 0;
}
