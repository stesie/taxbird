#include <gnome.h>

void
on_file_new_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_file_open_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

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
on_choose_template_OK_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_tv_sheets_cursor_changed            (GtkTreeView     *treeview,
                                        gpointer         user_data);


void
on_choose_file_OK_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_file_send_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_file_close_activate                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_export_druid_cancel                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_export_druid_finish                 (GnomeDruidPage  *gnomedruidpage,
                                        GtkWidget       *widget,
                                        gpointer         user_data);
