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

static void taxbird_ws_retrieve_field(GtkWidget *w, struct taxbird_ws *ws,
				      const char *field_name);

static gboolean taxbird_ws_show_appbar_help(GtkWidget *widget,
                                            GdkEventFocus *event,
                                            gpointer user_data);

static GtkWidget *taxbird_ws_create_input(SCM specs);
static GtkWidget *taxbird_ws_create_output(SCM specs);
static GtkWidget *taxbird_ws_create_chooser(SCM specs);

/* unprotect referenced SCM (callback for destroy notifications) */
static void taxbird_ws_unprotect_scm(gpointer d);

static struct {
  GtkWidget *(*new)(SCM specs);
  GtkWidget *(*new_2nd)(SCM specs);
} taxbird_ws_field_creators[] = {
  { taxbird_ws_create_input, NULL },                      /* TEXT_INPUT   */
  { taxbird_ws_create_output, NULL },                     /* TEXT_OUTPUT  */
  { taxbird_ws_create_chooser, NULL },                    /* CHOOSER      */
  { NULL, NULL },
  { taxbird_ws_create_input, taxbird_ws_create_output },  /* INPUT_CALC   */
  { taxbird_ws_create_input, taxbird_ws_create_input },   /* INPUT_INPUT  */
};


/* scan the provided dataset (guile list) for a sheet named 'sheet_name',
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
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeStore *tree;
  GtkTreeView *tv_sheets = GTK_TREE_VIEW(lookup_widget(appwin, "tv_sheets"));

  g_return_if_fail(formid >= 0);
  g_return_if_fail(tv_sheets);

  dataset = forms[formid]->dataset;
  g_return_if_fail(SCM_NFALSEP(scm_list_p(dataset)));
  
  /* add column */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("Available Sheets"),
						    renderer, "text", 0, NULL);
  gtk_tree_view_append_column(tv_sheets, column);

  /* add names of available sheets */
  tree = gtk_tree_store_new(1, G_TYPE_STRING);
  taxbird_ws_fill_tree_store(tree, NULL, dataset);
  gtk_tree_view_set_model(tv_sheets, GTK_TREE_MODEL(tree));

  taxbird_ws_get(appwin)->current_form = formid;
  taxbird_ws_get(appwin)->ws_data =
    scm_list_copy(scm_call_0(forms[formid]->dataset_create));
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
    dataset = scm_primitive_eval(dataset);

  g_return_if_fail(SCM_NFALSEP(scm_list_p(dataset)));
  g_return_if_fail(scm_ilength(dataset));

  if(SCM_SYMBOLP(SCM_CAR(dataset))) {
    SCM symbol = scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(dataset)));

    /* ignore, symbols of number-type as those are the constants we use
     * to indicate fields, try to evaluate everything else ... */

    if(! SCM_NUMBERP(symbol))
      /* probably quoted list, or whatever else, interpret it and
       * call filling routine recursively */
      taxbird_ws_fill_tree_store(tree, parent, scm_primitive_eval(dataset));

    return;
  }

  if(SCM_NUMBERP(SCM_CAR(dataset)))
    return; /* okay, this sheet is a leaf (holding fields) */

  while(scm_ilength(dataset)) {
    g_return_if_fail(SCM_STRINGP(SCM_CAR(dataset)));

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
  int item = 0;
  GtkBin *viewport = GTK_BIN(lookup_widget(appwin, "viewport"));
  int forms_id = taxbird_ws_get(appwin)->current_form;
  SCM sheet = taxbird_ws_lookup_sheet(forms[forms_id]->dataset, sheetname);

  g_return_if_fail(SCM_NFALSEP(scm_list_p(sheet)));
  g_return_if_fail(scm_ilength(sheet));

  if(SCM_STRINGP(SCM_CAR(sheet)))
    return; /* user didn't select a real sheet but only a node representing
	     * a parent in the tree  */  

  /* remove old widget tree ... */
  if((table = GTK_WIDGET(gtk_bin_get_child(viewport))))
    gtk_widget_destroy(table);

  /* create new widget tree  ... */
  table = gtk_table_new(scm_ilength(sheet) >> 1, 3, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 5);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);
  
  /* connect new table to the viewport - we need to do this so early,
   * as the storage callbacks (using glade's lookup code) require the whole
   * widget tree to be set up properly */
  gtk_container_add(GTK_CONTAINER(viewport), table);

  /* parse the sheet definition step by step and create the necessary widgets */
  g_return_if_fail(scm_ilength(sheet)%2==0); /* sheet definition must have an 
					      * even number of elements! */
  while(scm_ilength(sheet)) {
    int ws_field_t;
    GtkWidget *input, *label;
    SCM specs = SCM_CADR(sheet);
      
    if(SCM_SYMBOLP(specs))
      /* refrenced by variable, resolve it */
      specs = scm_c_lookup_ref(SCM_SYMBOL_CHARS(specs));

    /* resolve quotes */
    while(SCM_SYMBOLP(SCM_CAR(specs)))
      specs = scm_primitive_eval(specs);

    ws_field_t = scm_num2int(scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(sheet))),
			     0, "taxbird_ws_sel_sheet");
    input = taxbird_ws_field_creators[ws_field_t].new(specs);

    /* we need to attach the widget to the table (thus the widget tree itself)
     * rather early (i.e. before calling the retrieval func) as it might
     * use Glade's lookup code
     */
    g_object_set_data_full(G_OBJECT(input), "scm_specs",
			   scm_gc_protect_object(specs),
			   taxbird_ws_unprotect_scm);
    gtk_table_attach(GTK_TABLE(table), input, 1,
		     ws_field_t > FIELD_COMBINED_SPLIT ? 2 : 3, item, item + 1,
		     (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		     (GtkAttachOptions) (0), 0, 0);
    GLADE_HOOKUP_OBJECT(appwin, input, 
			SCM_STRING_CHARS(SCM_CAR(specs)));
    
    /* set callback responsible for displaying help text in appbar */
    g_signal_connect((gpointer) input, "focus-in-event",
		     G_CALLBACK(taxbird_ws_show_appbar_help), NULL);
				  
    /* add description label */
    g_return_if_fail(SCM_STRINGP(SCM_CADR(specs)));
    label = gtk_label_new(SCM_STRING_CHARS(SCM_CADR(specs)));
    gtk_label_set_line_wrap(GTK_LABEL(label), 1);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, item, item + 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);

    if(ws_field_t > FIELD_COMBINED_SPLIT) {
      /* second field on same row */
      char *field_name = SCM_STRING_CHARS(SCM_CADDDDR(specs));
      GtkWidget *input = taxbird_ws_field_creators[ws_field_t].new_2nd(specs);

      /* insert the widget into the widget tree as early as possible */
      g_object_set_data_full(G_OBJECT(input), "scm_specs",
			     scm_gc_protect_object(SCM_CDDDDR(specs)),
			     taxbird_ws_unprotect_scm);
      gtk_table_attach(GTK_TABLE(table), input, 2, 3, item, item + 1,
		       (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		       (GtkAttachOptions) (0), 0, 0);
      GLADE_HOOKUP_OBJECT(appwin, input, field_name);
      
      taxbird_ws_retrieve_field(input, taxbird_ws_get(appwin), field_name);

      /* set callback responsible for displaying help text in appbar */
      g_signal_connect((gpointer) input, "focus-in-event",
		       G_CALLBACK(taxbird_ws_show_appbar_help), NULL);

      gtk_widget_show(input);
    }

    /* retrieve content of main field, we need to do this after creating
     * the first one since the changed-handler of the main field may try
     * to change the first one */
    taxbird_ws_retrieve_field(input, taxbird_ws_get(appwin),
			      SCM_STRING_CHARS(SCM_CAR(specs)));
    gtk_widget_show(input);

    /* care for next item */
    sheet = SCM_CDDR(sheet);
    item ++;
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
  /* field points to the ("intname" "name" "desc" validate-func) list */
  SCM validatfunc;
  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(field)), 0);

  if(! GTK_IS_ENTRY(w))
    return 1;    /* consider output fields, choosers etc. to be valid */

  /* validate field's value, in case it's a text entry field */
  validatfunc = SCM_CADDDR(field);

  if(SCM_SYMBOLP(validatfunc))
    /* immediate symbol (i.e. defined function), resolve and execute then */
    validatfunc = scm_c_lookup_ref(SCM_SYMBOL_CHARS(validatfunc));

  if(SCM_NFALSEP(scm_list_p(validatfunc)))
    /* probably some kind of (lambda (v buf) (validator v)) thingy ... */
    validatfunc = scm_primitive_eval(validatfunc);

  if(SCM_NFALSEP(scm_procedure_p(validatfunc))) {
    /* execute validator function */
    const char *content = gtk_entry_get_text(GTK_ENTRY(w));
    validatfunc = scm_call_2(validatfunc, scm_makfrom0str(content),
			     taxbird_ws_get(appwin)->ws_data);
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

  GtkWidget *appwin = lookup_widget(w, "taxbird");

  SCM specs = (SCM) g_object_get_data(G_OBJECT(w), "scm_specs");
  g_return_if_fail(SCM_NFALSEP(scm_list_p(specs)));

  /* store data if it's valid */
  if(taxbird_ws_validate(w)) {
    SCM store_value;
    SCM field = SCM_CAR(specs);
    g_return_if_fail(SCM_STRINGP(field));

    static GdkColor taxbird_color_okay = { 0, 32767, 32767, 32767 };
    gtk_widget_modify_bg(w, GTK_STATE_NORMAL, &taxbird_color_okay);

    if(GTK_IS_ENTRY(w))
      store_value = scm_makfrom0str(gtk_entry_get_text(GTK_ENTRY(w)));

    else if(GTK_IS_COMBO_BOX(w)) {
      int item = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
      g_return_if_fail(item >= 0);

      store_value = scm_int2num(item);
    }
    else
      g_assert_not_reached();

    scm_call_3(forms[taxbird_ws_get(appwin)->current_form]->dataset_write,
	       taxbird_ws_get(appwin)->ws_data, field, store_value);

    if(scm_ilength(specs) > 4) {
      /* there is an associated field, which possibly is some kind of
       * calculated field, re-retrieve it's value
       */
      GtkWidget *w2;

      field = SCM_CADDDDR(specs);
      g_return_if_fail(SCM_STRINGP(field));

      w2 = lookup_widget(appwin, SCM_STRING_CHARS(field));
      g_return_if_fail(w2);

      taxbird_ws_retrieve_field(w2, taxbird_ws_get(appwin),
				SCM_STRING_CHARS(field));
    }      
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
  if(SCM_SYMBOLP(dataset))
    dataset = scm_primitive_eval(dataset);

  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(dataset)), SCM_BOOL(0));
  g_return_val_if_fail(scm_ilength(dataset), SCM_BOOL(0));
  
  if(SCM_SYMBOLP(SCM_CAR(dataset))) {
    SCM symbol = scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(dataset)));

    if(! SCM_NUMBERP(symbol))
      return taxbird_ws_lookup_sheet(scm_primitive_eval(dataset), needle);

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
	sheet = scm_primitive_eval(sheet);

      g_return_val_if_fail(SCM_NFALSEP(scm_list_p(sheet)), SCM_BOOL(0));

      /* now try to find out whether the list is quoted or requires a
       * procedure to be called (the first symbol may be a numeric
       * constant (telling the type of field)
       */
      if(SCM_SYMBOLP(SCM_CAR(sheet))) {
	SCM symbol = scm_c_lookup_ref(SCM_SYMBOL_CHARS(SCM_CAR(sheet)));
	if(! SCM_NUMBERP(symbol))
	  sheet = scm_primitive_eval(sheet);
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
taxbird_ws_retrieve_field(GtkWidget *w, struct taxbird_ws *ws,
			  const char *field_name)
{
  SCM v = scm_call_2(forms[ws->current_form]->dataset_read,
		     ws->ws_data, scm_makfrom0str(field_name));
  
  if(GTK_IS_ENTRY(w)) {
    if(SCM_STRINGP(v)) {
      gtk_entry_set_text(GTK_ENTRY(w), SCM_STRING_CHARS(v));
    }
    else
      gtk_entry_set_text(GTK_ENTRY(w), "");
  } 
  else if(GTK_IS_COMBO_BOX(w)) {
    if(SCM_NUMBERP(v))
      gtk_combo_box_set_active(GTK_COMBO_BOX(w), scm_num2int(v, 0,"taxbird_ws_"
							     "retrieve_field"));
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
  SCM entries = SCM_CADDDR(specs);
  g_return_val_if_fail(SCM_NFALSEP(scm_list_p(entries)), NULL);

  w = gtk_combo_box_new_text();

  while(scm_ilength(entries)) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), 
			      SCM_STRING_CHARS(SCM_CAR(entries)));
    entries = SCM_CDR(entries);
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
    helptext = scm_primitive_eval(helptext);

  if(SCM_STRINGP(helptext))
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

  if(! SCM_NFALSEP(scm_list_p(content)) || scm_ilength(content) != 2) {
    g_warning("unable to load file %s", fname);
    return;
  }

  formid = taxbird_form_get_by_name(SCM_STRING_CHARS(SCM_CAR(content)));
  taxbird_ws_sel_form(appwin, formid);
		      
  ws->ws_data = SCM_CADR(content);

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

static void
taxbird_ws_unprotect_scm(gpointer d)
{
  scm_gc_unprotect_object((SCM)d);
}
