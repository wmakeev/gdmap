/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gui_main.h"
#include "global.h"

int main (int argc, char *argv[]) {

  gtk_set_locale ();
  gtk_init (&argc, &argv);
  
  gui_create_main_win ();
  
  global_init();
  
  if (argc >= 2) tree_load(argv[1]);

  gtk_main ();
  return 0;
}

