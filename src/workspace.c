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

/* call validation function associated to the given field (w), ret 1 if valid */
static int taxbird_ws_validate(GtkWidget *w);

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

static GtkWidget *taxbird_ws_create_input(SCM specs);
static GtkWidget *taxbird_ws_create_output(SCM specs);
static GtkWidget *taxbird_ws_create_chooser(SCM specs);
static GtkWidget *taxbird_ws_create_label(SCM specs);
static GtkWidget *taxbird_ws_create_button(SCM specs);
static GtkWidget *taxbird_ws_create_checkbox(SCM specs);

/* unprotect referenced SCM (callback for destroy notifications) */
static void taxbird_ws_unprotect_scm(gpointer d);

/* generic callback function for tb:field:button's ... */
static void taxbird_ws_button_callback(GtkWidget *button, void *data);

static struct {
  GtkWidget *(*new)(SCM specs);

} taxbird_ws_field_creators[] = {
  { taxbird_ws_create_input },                      /* TEXT_INPUT  :: 0 */
  { taxbird_ws_create_output },                     /* TEXT_OUTPUT :: 1 */
  { taxbird_ws_create_chooser },                    /* CHOOSER     :: 2 */
  { taxbird_ws_create_checkbox },                   /* CHECKBOX    :: 3 */
  { taxbird_ws_create_label },                      /* LABEL       :: 4 */
  { taxbird_ws_create_button },                     /* BUTTON      :: 5 */
};


/* scan the provided dataset (guile list) for a sheet named 'sheet_name',
 * and return it's definition
 */
static SCM taxbird_ws_lookup_sheet(SCM dataset, const char *sheet_name);



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

  return taxbird;
}


/* prepare taxbird workspace for form with given id,
 * clearing any existing data (after asking)
 */
void
taxbird_ws_sel_form(GtkWidget *appwin, int formid)
{
  SCM dataset;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeStore *tree;
  GtkTreeView *tv_sheets = GTK_TREE_VIEW(lookup_widget(appwin, "tv_sheets"));

  g_return_if_fail(formid >= 0);
  g_return_if_fail(tv_sheets);

  dataset = forms[formid]->dataset;

  if(SCM_NFALSEP(scm_procedure_p(dataset)))
    /* real dataset (maybe individually) created by a (lambda) */
    dataset = scm_call_0(dataset);
    
  g_return_if_fail(SCM_NFALSEP(scm_list_p(dataset)));

  g_object_set_data_full(G_OBJECT(appwin), "sheetdef",
			 (void*) scm_gc_protect_object(dataset),
			 taxbird_ws_unprotect_scm);
  
  /* add column */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("Available Sheets"),
						    renderer, "text", 0, NULL);
  gtk_tree_view_append_column(tv_sheets, column);

  /* add names of available sheets */
  tree = gtk_tree_store_new(1, G_TYPE_STRING);
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

  if(SCM_SYMBOLP(dataset))
    dataset = scm_call_0(dataset);

  g_return_if_fail(SCM_NFALSEP(scm_list_p(dataset)));
  g_return_if_fail(scm_ilength(dataset));

  if(SCM_SYMBOLP(SCM_CAR(dataset))) {
    SCM symbol = scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(dataset)));

    /* ignore, symbols of number-type as those are the constants we use
     * to indicate fields, try to evaluate everything else ... */

    if(! SCM_NUMBERP(symbol))
      /* probably quoted list, or whatever else, interpret it and
       * call filling routine recursively */
      taxbird_ws_fill_tree_store(tree, parent, scm_call_0(dataset));

    return;
  }

  if(SCM_NUMBERP(SCM_CAR(dataset)))
    return; /* okay, this sheet is a leaf (holding fields) */

  while(scm_ilength(dataset)) {
    if(! SCM_STRINGP(SCM_CAR(dataset))) {
      g_print("sheet doesn't have sheet name at it's start: ");
      gh_write(SCM_CAR(dataset));
      g_print("\n");
      break;
    }

    gtk_tree_store_append(tree, &iter, parent);
    gtk_tree_store_set(tree, &iter, 0, SCM_STRING_CHARS(SCM_CAR(dataset)), -1);

    /* create sheet's nodes recursively */
    taxbird_ws_fill_tree_store(tree, &iter, SCM_CADR(dataset));

    dataset = SCM_CDDR(dataset);
  }
}



/* display sheet with provided id in the workspace of app_window */
void
taxbird_ws_sel_sheet(GtkWidget *appwin, const char *sheetname)
{
  GtkWidget *table;
  GtkBin *viewport = GTK_BIN(lookup_widget(appwin, "viewport"));
  SCM dataset = (SCM) g_object_get_data(G_OBJECT(appwin), "sheetdef");
  SCM sheet = taxbird_ws_lookup_sheet(dataset, sheetname);

  g_return_if_fail(SCM_NFALSEP(scm_list_p(sheet)));
  g_return_if_fail(scm_ilength(sheet));

  if(SCM_STRINGP(SCM_CAR(sheet)))
    return; /* user didn't select a real sheet but only a node representing
	     * a parent in the tree  */  

  /* remove old widget tree ... */
  if((table = GTK_WIDGET(gtk_bin_get_child(viewport))))
    gtk_widget_destroy(table);

  /* create new widget tree  ... */
  int columns = scm_num2int(SCM_CAR(sheet), 0, "taxbird_ws_sel_sheet");
  table = gtk_table_new(scm_ilength(sheet), columns, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);
  
  /* connect new table to the viewport - we need to do this so early,
   * as the storage callbacks (using glade's lookup code) require the whole
   * widget tree to be set up properly */
  gtk_container_add(GTK_CONTAINER(viewport), table);

  /* add headline
   * we want to modify the background color, however the GtkLabel is window-
   * less, thus we need to surround it with a GtkEventBox and modify that
   * one's color   */
  GtkWidget *headline_evbox = gtk_event_box_new();
  static GdkColor headline_bg = { 0, 0x6000, 0x6000, 0x6000 };
  gtk_widget_modify_bg(headline_evbox, GTK_STATE_NORMAL, &headline_bg);

  gtk_table_attach(GTK_TABLE(table), headline_evbox, 0, columns + 1, 0, 1,
		   (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		   (GtkAttachOptions) (0), 0, 0);

  /* now create the headline widget itself */
  char *headline_text = g_strdup_printf("<big><b>%s</b></big>", sheetname);
  GtkWidget *headline = gtk_label_new(headline_text);
  g_free(headline_text);

  gtk_label_set_use_markup(GTK_LABEL(headline), 1);
  gtk_misc_set_alignment(GTK_MISC(headline), 0, 0.5);
  gtk_misc_set_padding(GTK_MISC(headline), 4, 4);

  static GdkColor headline_fg = { 0, 0xFFFF, 0xFFFF, 0xFFFF };
  gtk_widget_modify_fg(headline, GTK_STATE_NORMAL, &headline_fg);

  gtk_container_add(GTK_CONTAINER(headline_evbox), headline);
  gtk_widget_show(headline);
  gtk_widget_show(headline_evbox);

  /* parse the sheet definition step by step,
   * and create the necessary widgets */
  int row;
  sheet = SCM_CDR(sheet);
  for(row = 1; scm_ilength(sheet); sheet = SCM_CDR(sheet), row ++) {
    SCM specs = SCM_CAR(sheet);
      
    if(SCM_SYMBOLP(specs))
      /* refrenced by variable, resolve it */
      specs = scm_c_lookup_ref(SCM_SYMBOL_CHARS(specs));

    /* resolve quotes */
    while(SCM_SYMBOLP(SCM_CAR(specs)))
      specs = scm_call_0(specs);


    /* first element of 'specs' contains the name of this row,
     * i.e. what we shall put into the first column label */
    g_return_if_fail(SCM_STRINGP(SCM_CAR(specs)));

    GtkWidget *label = gtk_label_new(SCM_STRING_CHARS(SCM_CAR(specs)));
    gtk_label_set_use_markup(GTK_LABEL(label), 1);
    gtk_label_set_line_wrap(GTK_LABEL(label), 1);
    gtk_widget_show(label);

    /* bind the label for the first column only, if there are further fields,
     * if this is some kind of caption label, bind to the full row */
    gtk_table_attach(GTK_TABLE(table), label, 0, 
		     scm_ilength(specs) == 1 ? columns + 1 : 1, row, row + 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    /* align label to the right, if there are any fields, to the left, if
     * there aren't, i.e. this is a caption label */
    gtk_misc_set_alignment(GTK_MISC(label),
			   scm_ilength(specs) == 1 ? 0 : 1, 0.5);
    gtk_label_set_justify(GTK_LABEL(label), scm_ilength(specs) == 1 ?
			  GTK_JUSTIFY_LEFT : GTK_JUSTIFY_RIGHT);

    /* generate the fields of this column now ... */
    int column;
    specs = SCM_CDR(specs); /* skip the label, we already cared for */
    for(column = 1; scm_ilength(specs); specs = SCM_CDDDDR(specs), column ++) {
      if(scm_ilength(specs) < 4) {
	g_print("field definition expected to be four fields long, got: ");
	gh_write(specs); g_print("\n");
	break;
      }

      if(! SCM_NUMBERP(SCM_CAR(specs))) {
	g_print("first spec-field expected to be numeric in: ");
	gh_write(specs); g_print("\n");
	break;
      }
      int input_type = scm_num2int(SCM_CAR(specs), 0, "taxbird_ws_sel_sheet");

      if(! SCM_STRINGP(SCM_CADR(specs))) {
	g_print("second spec-field expected to be identifier string, near: ");
	gh_write(specs); g_print("\n");
	break;
      }
      const char *input_name = SCM_STRING_CHARS(SCM_CADR(specs));

      GtkWidget *input = taxbird_ws_field_creators[input_type].new(specs);

      /* we need to attach the widget to the table (i.e. to the widget tree)
       * rather early (i.e. before calling the retrieval func) as it might
       * use Glade's lookup code
       */
      g_object_set_data_full(G_OBJECT(input), "scm_specs",
			     scm_gc_protect_object(specs),
			     taxbird_ws_unprotect_scm);
      gtk_table_attach(GTK_TABLE(table), input, column, column + 1, row, 
		       row + 1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		       (GtkAttachOptions) (0), 0, 0);
      GLADE_HOOKUP_OBJECT(appwin, input, input_name);
    
      /* set callback responsible for displaying help text in appbar */
      g_signal_connect((gpointer) input, "focus-in-event",
		       G_CALLBACK(taxbird_ws_show_appbar_help), NULL);
				  
      /* retrieve content of main field */
      taxbird_ws_retrieve_field(input, appwin, input_name);

      /* connect changed signal ... */
      if(! (input_type & FIELD_UNCHANGEABLE))
	g_signal_connect((gpointer) input,
			 GTK_IS_TOGGLE_BUTTON(input) ? "toggled" : "changed",
			 G_CALLBACK(taxbird_ws_store_event), NULL);

      gtk_widget_show(input);
    }
  }

  gtk_widget_show(table);
}



/* call validation function associated to the given field (w)
 * return 1 if contents is valid, 0 in case not
 */
static int
taxbird_ws_validate(GtkWidget *w)
{
  GtkWidget *appwin = lookup_widget(w, "taxbird");

  SCM field = (SCM) g_object_get_data(G_OBJECT(w), "scm_specs");
  /* field points to the ("type" "name" "desc" validate-func) list */

  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(field)), 0);

  if(! GTK_IS_ENTRY(w))
    return 1;    /* consider output fields, choosers etc. to be valid */

  /* validate field's value, in case it's a text entry field */
  SCM validatfunc = SCM_CADDDR(field);

  if(SCM_SYMBOLP(validatfunc))
    /* immediate symbol (i.e. defined function), resolve and execute then */
    validatfunc = scm_c_lookup_ref(SCM_SYMBOL_CHARS(validatfunc));

  if(SCM_NFALSEP(scm_list_p(validatfunc)))
    /* probably some kind of (lambda (v buf) (validator v)) thingy ... */
    validatfunc = scm_call_0(validatfunc);

  if(SCM_NFALSEP(scm_procedure_p(validatfunc))) {
    /* execute validator function */
    const char *content = gtk_entry_get_text(GTK_ENTRY(w));
    validatfunc = scm_call_2(validatfunc, scm_makfrom0str(content), (SCM)
			     g_object_get_data(G_OBJECT(appwin), "scm_data"));
  }

  if(SCM_BOOLP(validatfunc))
    return SCM_NFALSEP(validatfunc);

  g_warning("damn, who coded this piece of scheme code? don't know "
	    "what to do with validator func ...");
  gh_display(validatfunc);
  return 0;
}



/* callback function called when entry fields are changed,
 * need to validate and store the entered data 
 */
static void
taxbird_ws_store_event(GtkWidget *w, gpointer user_data)
{
  (void) user_data;

  /* validate field's content */
  if(! taxbird_ws_validate(w)) {
    /* draw red border (background) around textbox, to show the chosen value is
     * not valid. 
     */
    static GdkColor taxbird_color_red = { 0, 65535, 0, 0 };
    gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &taxbird_color_red);

    return;
  }


  /* field is valid ... 
   * draw green border around the field */
  static GdkColor taxbird_color_okay = { 0, 32767, 32767, 32767 };
  gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &taxbird_color_okay);


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
  SCM specs = (SCM) g_object_get_data(G_OBJECT(w), "scm_specs");
  g_return_if_fail(SCM_NFALSEP(scm_list_p(specs)));

  SCM field = SCM_CADR(specs);
  g_return_if_fail(SCM_STRINGP(field));

  GtkWidget *appwin = lookup_widget(w, "taxbird");
  int current_form = (int) g_object_get_data(G_OBJECT(appwin), "current_form");

  scm_call_3(forms[current_form]->dataset_write,
	     (SCM) g_object_get_data(G_OBJECT(appwin), "scm_data"),
	     field, store_value);


  /* set changed flag */
  g_object_set_data(G_OBJECT(appwin), "changed", (void *) 1);


  /* now update all but the current field */
  taxbird_ws_update_fields(appwin, SCM_STRING_CHARS(field));
}



/* Figure out, which sheet's displayed currently and update all the
 * widgets (calling retrieval function). The widget with the name 
 * 'exception' is ignore, may be NULL
 */
static void
taxbird_ws_update_fields(GtkWidget *appwin, const char *exception)
{
  /* figure out, which sheet's selected */
  GtkTreeView *tv = GTK_TREE_VIEW(lookup_widget(appwin, "tv_sheets"));
  GtkTreeSelection *sel = gtk_tree_view_get_selection(tv);

  GtkTreeModel *model;
  GtkTreeIter iter;
  if(! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  char *sheetname;
  gtk_tree_model_get(model, &iter, 0, &sheetname, -1);

  /* lookup the associated SCM structure */
  SCM dataset = (SCM) g_object_get_data(G_OBJECT(appwin), "sheetdef");
  SCM sheet = taxbird_ws_lookup_sheet(dataset, sheetname);

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
}




static SCM
taxbird_ws_lookup_sheet(SCM dataset, const char *needle)
{
  if(SCM_SYMBOLP(dataset))
    dataset = scm_call_0(dataset);

  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(dataset)), SCM_BOOL(0));
  g_return_val_if_fail(scm_ilength(dataset), SCM_BOOL(0));
  
  if(SCM_SYMBOLP(SCM_CAR(dataset))) {
    SCM symbol = scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(dataset)));

    if(! SCM_NUMBERP(symbol))
      return taxbird_ws_lookup_sheet(scm_call_0(dataset), needle);

    else
      return SCM_BOOL(0);
  }
    
  if(SCM_NUMBERP(SCM_CAR(dataset)))
    return SCM_BOOL(0); /* sheet definition, no more sub-sheets */

  while(scm_ilength(dataset)) {
    g_return_val_if_fail(SCM_STRINGP(SCM_CAR(dataset)), SCM_BOOL(0));

    if(! strcmp(SCM_STRING_CHARS(SCM_CAR(dataset)), needle)) {
      SCM sheet = SCM_CADR(dataset);

      while(SCM_SYMBOLP(sheet))
	sheet = scm_call_0(sheet);

      g_return_val_if_fail(SCM_NFALSEP(scm_list_p(sheet)), SCM_BOOL(0));

      /* now try to find out whether the list is quoted or requires a
       * procedure to be called (the first symbol may be a numeric
       * constant (telling the type of field)
       */
      if(SCM_SYMBOLP(SCM_CAR(sheet))) {
	SCM symbol = scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(sheet)));
	if(! SCM_NUMBERP(symbol))
	  sheet = scm_call_0(sheet);
      }

      return sheet;
    }
    else {
      /* try recursing down */
      SCM retval = taxbird_ws_lookup_sheet(SCM_CADR(dataset), needle);

      if(SCM_NFALSEP(scm_list_p(retval)))
	return retval; /* got it, pass it back up */
    }

    dataset = SCM_CDDR(dataset);
  }

  return SCM_BOOL(0);
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



static GtkWidget *
taxbird_ws_create_input(SCM specs)
{
  (void) specs;

  GtkWidget *input = gtk_entry_new();
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
taxbird_ws_create_label(SCM specs)
{
  (void) specs;

  GtkWidget *w = gtk_label_new(SCM_STRING_CHARS(SCM_CADR(specs)));
  gtk_label_set_use_markup(GTK_LABEL(w), 1);
  return w;
}



static GtkWidget *
taxbird_ws_create_checkbox(SCM specs)
{
  (void) specs;

  GtkWidget *w = gtk_check_button_new_with_label(_("Yes!"));

  return w;
}



static GtkWidget *
taxbird_ws_create_button(SCM specs)
{
  (void) specs;

  GtkWidget *w = gtk_button_new_with_label(SCM_STRING_CHARS(SCM_CADR(specs)));

  g_signal_connect((gpointer) w, "clicked",
		   G_CALLBACK(taxbird_ws_button_callback), NULL);

  return w;
}



static void
taxbird_ws_button_callback(GtkWidget *button, void *data)
{
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


static GtkWidget *
taxbird_ws_create_chooser(SCM specs)
{
  GtkWidget *w; 
  SCM entries = SCM_CADDDR(specs);
  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(entries)), NULL);

  w = gtk_combo_box_new_text();

  while(scm_ilength(entries)) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), 
			      SCM_STRING_CHARS(SCM_CAR(entries)));
    entries = SCM_CDR(entries);
  }

  return w;
}


static gboolean
taxbird_ws_show_appbar_help(GtkWidget *widget, GdkEventFocus *event,
			    gpointer user_data)
{
  (void) event;
  (void) user_data;

  SCM helptext;
  SCM specs = (SCM) g_object_get_data(G_OBJECT(widget), "scm_specs");
  GtkWidget *appbar = lookup_widget(widget, "appbar");

  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(specs)), 0);
  if(scm_ilength(specs) < 3) {
    /* no help text assigned, clear status line */
    gnome_appbar_set_status(GNOME_APPBAR(appbar), "");
    return FALSE; /* continue with next handler, if any. */
  }

  helptext = SCM_CADDR(specs);   

  if(SCM_NFALSEP(scm_list_p(helptext)))
    /* probably a command, execute it */
    helptext = scm_call_0(helptext);

  gnome_appbar_set_status(GNOME_APPBAR(appbar), SCM_STRINGP(helptext) ?
			  SCM_STRING_CHARS(helptext) : "");

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
