/* Copyright(C) 2005,2007 Stefan Siegl <stesie@brokenpipe.de>
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

#include <gnome.h>

#include "dialog.h"
#include "workspace.h"

/* display a common error dialog with the given message (in modal mode) */
void 
taxbird_dialog_error(GtkWidget *parent, const char *message)
{
  if(! parent)
    parent = taxbird_window;

  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_ERROR,
					     GTK_BUTTONS_CLOSE, message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}



/* display a common info dialog with the given message (in modal mode) */
void 
taxbird_dialog_info(GtkWidget *parent, const char *message)
{
  if(! parent)
    parent = taxbird_window;

  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_INFO,
					     GTK_BUTTONS_CLOSE, message);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}


/* display a yes/no/cancel dialog with the given message (in modal mode) */
int 
taxbird_dialog_yes_no_cancel(GtkWidget *parent, const char *message)
{
  if(! parent)
    parent = taxbird_window;

  GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
					     GTK_DIALOG_MODAL,
					     GTK_MESSAGE_QUESTION,
					     GTK_BUTTONS_NONE, message);

  GtkWidget *but;

  but = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(but);
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog), but, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(but, GTK_CAN_DEFAULT);

  but = gtk_button_new_from_stock("gtk-no");
  gtk_widget_show(but);
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog), but, GTK_RESPONSE_NO);
  GTK_WIDGET_SET_FLAGS(but, GTK_CAN_DEFAULT);

  but = gtk_button_new_from_stock("gtk-yes");
  gtk_widget_show(but);
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog), but, GTK_RESPONSE_YES);
  GTK_WIDGET_SET_FLAGS(but, GTK_CAN_DEFAULT);

  int resp = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return resp;
}

