/*
 * Anfängliche Datei main.c, erzeugt durch Glade. Bearbeiten Sie
 * Sie, wie Sie wollen. Glade wird diese Datei nicht überschreiben.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"
#include "tree.h"
#include "global.h"

int
main (int argc, char *argv[])
{

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  global.win = create_main_win ();
  gtk_widget_show (global.win);
  
  global_init();

  gtk_main ();
  return 0;
}

