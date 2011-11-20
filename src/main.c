/* Copyright(C) 2004,2005,2007,2008,2011 Stefan Siegl <stesie@brokenpipe.de>
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

#include <geier.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "workspace.h"
#include "guile.h"
#include "dialog.h"


/* forwarded main, i.e. with guile support initialized */
static void main_forward(void *closure, int argc, char **argv);

gchar **args_remaining = NULL;

int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  g_set_application_name("Taxbird");

  GError *error = NULL;
  gboolean show_version = FALSE;
  GOptionEntry options[] = {
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &args_remaining, NULL, N_("FILE") },
    { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Show version"), NULL },
    { NULL }
  };

  if(!gtk_init_with_args(&argc, &argv, _("- The first free Elster client"),
			 options, GETTEXT_PACKAGE, &error)) {
    g_printerr (_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
		error->message, argv[0]);
    g_error_free (error);
    return 1;
  }

  if(show_version) {
    g_print(PACKAGE_STRING "\n");
    return 0;
  }

  /* initialize Guile backend */
  scm_boot_guile(argc, argv, main_forward, NULL);

  /* leaving taxbird, deinitialize libgeier */
  geier_exit();

  return 0;
}



/* forwarded main, i.e. with guile support initialized */
static void
main_forward(void *closure, int argc, char **argv) 
{
  /* keep GCC from complaining that 'closure' is not used */
  (void) closure;

  /* TODO parse command line arguments, left for us ... 
   * Especially read the name of a file we should open right after startup.
   * Some output Elster-XML command like Glade's compile instruction would
   * be cool as well */
  (void) argc;
  (void) argv;

  SCM main_forward_bootstrap(void *body_data) {
    (void) body_data;

    /* load taxbird's Guile extension stuff */
    taxbird_guile_init();
    taxbird_guile_eval_file("startup.scm");

    if(! taxbird_ws_new())
      exit(1); /* abort start */
    
    if(args_remaining)
      taxbird_ws_open(*args_remaining, FALSE);

    /* initialize GEIER library, no debug output */
    if(geier_init(0))
      taxbird_dialog_error(NULL, _("Failed to initialize GEIER data transfer library. "
				   "Data transfers to fiscal authorities will not be possible."));

    return SCM_BOOL(0);
  }

  if(SCM_NFALSEP(scm_internal_stack_catch(SCM_BOOL(1),
					  main_forward_bootstrap, NULL,
					  taxbird_guile_global_err_handler,
					  NULL)))
    exit(1); /* abort */



  SCM main_forward_catchy(void *body_data) {
    (void) body_data;

    gtk_main (); /* run common application */
    return SCM_BOOL(0);
  }

  while(SCM_NFALSEP(scm_internal_stack_catch(SCM_BOOL(1),
					     main_forward_catchy, NULL,
					     taxbird_guile_global_err_handler,
					     NULL)));
}

