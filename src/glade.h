/* Copyright(C) 2007 Stefan Siegl <stesie@brokenpipe.de>
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

#include <glade/glade.h>

#ifndef TAXBIRD_GLADE_H
#define TAXBIRD_GLADE_H

/* GladeXML object of the application window */
extern GladeXML *taxbird_gladexml_app;

/* GladeXML object of the active sheet */
extern GladeXML *taxbird_gladexml_sheet;

/* GladeXML object of the export dialog */
extern GladeXML *taxbird_gladexml_export;

GtkWidget *taxbird_glade_create(GladeXML **xml, const char *widget_name);

GtkWidget *taxbird_glade_lookup(GladeXML *xml, const char *widget_name);


#endif /* TAXBIRD_GLADE_H */
