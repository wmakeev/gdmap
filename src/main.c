/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "l_i18n.h"
#include "gui_main.h"
#include "colors.h"

int main (int argc, char *argv[]) {

	bindtextdomain(GETTEXT_PACKAGE, GDMAP_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

  gtk_set_locale();
  gtk_init(&argc, &argv);
  
  gui_create_main_win();
  colors_init();
  
/*   global_init(); */
/*   if (argc >= 2) tree_load(argv[1]); */

  gtk_main();

  return 0;
}

