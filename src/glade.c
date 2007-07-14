/* Copyright(C) 2007 Stefan Siegl <stesie@brokenpipe.de>
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

#include <glade/glade.h>

#include "glade.h"

/* GladeXML object of the application window */
GladeXML *taxbird_gladexml_app = NULL;

/* GladeXML object of the active sheet */
GladeXML *taxbird_gladexml_sheet = NULL;


GtkWidget *
taxbird_glade_create(GladeXML **xml, const char *widget_name)
{
  GladeXML *xml_dummy = NULL;
  if(! xml) xml = &xml_dummy;

  if(! *xml) {
    *xml = glade_xml_new(PACKAGE_DATA_DIR "/taxbird.glade", widget_name, NULL);

    if(! *xml) {
      g_printerr(PACKAGE_NAME ": taxbird_glade_create failed for '%s'\n",
		 widget_name);
      return NULL;
    }

    glade_xml_signal_autoconnect(*xml);
  }

  return taxbird_glade_lookup(*xml, widget_name);
}


GtkWidget *
taxbird_glade_lookup(GladeXML *xml, const char *widget_name)
{
  g_return_val_if_fail(xml, NULL);
  g_return_val_if_fail(widget_name, NULL);

  return glade_xml_get_widget(xml, widget_name);
}
