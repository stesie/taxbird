#include <gnome.h>

void
on_file_new_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_file_open_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void on_file_new_from_last_year_activate(GtkMenuItem *mi, gpointer u);

void
on_file_save_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_file_saveas_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_file_quit_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_edit_preferences_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_help_about_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tv_sheets_cursor_changed            (GtkTreeView     *treeview,
                                        gpointer         user_data);

void on_choose_template_OK_clicked(GtkTreeView *tv, gpointer user_data);

void on_choose_file_open(GtkButton *button, gpointer user_data);
void on_choose_file_copy_last_year(GtkButton *button, gpointer user_data);
void on_choose_file_save(GtkButton *button, gpointer user_data);


void
on_file_send_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_file_close_activate                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_taxbird_configure                   (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

GtkWidget*
htmlview_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

gboolean
on_templates_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_file_send_testcase_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
