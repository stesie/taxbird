/* Copyright(C) 2011 Stefan Siegl <stesie@brokenpipe.de>
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

#include "builder.h"

/* GtkBuilder object of the application window */
GtkBuilder *taxbird_builder_app;

/* GtkBuilder object of the active sheet */
GtkBuilder *taxbird_builder_sheet;

/* GtkBuilder object of the active export.

   We use a seperate builder, so we can destroy and reinstantiate, if export
   is either finished or cancelled. */
GtkBuilder *taxbird_builder_export;


GtkWidget *
taxbird_builder_create(GtkBuilder **xml, const char *widget_name, const char *file_name)
{
  GtkBuilder *dummy = NULL;
  if(xml == NULL) xml = &dummy;

  if(! *xml) {
    GError* error = NULL;
    *xml = gtk_builder_new();

    if(!gtk_builder_add_from_file(*xml, file_name, &error)) {
      g_warning("taxbird_builder_create failed for '%s' in '%s': %s\n",
		widget_name, file_name, error->message);
      g_error_free(error);
      return NULL;
    }

    gtk_builder_connect_signals(*xml, NULL);
  }

  GtkWidget *w = taxbird_builder_lookup(*xml, widget_name);

  if(xml == &dummy) {
    g_object_unref(*xml);
  }

  return w;
}


GtkWidget *
taxbird_builder_lookup(GtkBuilder *xml, const char *widget_name)
{
  g_return_val_if_fail(xml, NULL);
  g_return_val_if_fail(widget_name, NULL);

  return GTK_WIDGET(gtk_builder_get_object(xml, widget_name));
}
