/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifndef _GUI_MAIN_H_
#define _GUI_MAIN_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "tree.h"

GtkWidget* gui_create_main_win (void);
GtkWidget* gui_get_main_win();
void gui_tree_display(tree_t* tree);

#endif
