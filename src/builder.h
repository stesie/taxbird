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

#include <gtk/gtk.h>

#ifndef TAXBIRD_BUILDER_H
#define TAXBIRD_BUILDER_H

/* GtkBuilder object of the application window */
extern GtkBuilder *taxbird_builder_app;

/* GtkBuilder object of the active sheet */
extern GtkBuilder *taxbird_builder_sheet;

/* GtkBuilder object of the active export.
   
   We use a seperate builder, so we can destroy and reinstantiate, if export
   is either finished or cancelled. */
extern GtkBuilder *taxbird_builder_export;

GtkWidget *taxbird_builder_create(GtkBuilder **xml, const char *widget_name,
				  const char *file_name);

GtkWidget *taxbird_builder_lookup(GtkBuilder *xml, const char *widget_name);

#endif /* TAXBIRD_GLADE_H */
