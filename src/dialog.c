/* Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>
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

/* display a common error dialog with the given message (in modal mode) */
void 
taxbird_dialog_error(GtkWidget *parent, const char *message)
{
  GtkWidget *dialog = gtk_message_dialog_new(parent,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_MESSAGE_ERROR,
					     GTK_BUTTONS_CLOSE, message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
