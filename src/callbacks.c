#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>

#include "dirselect.h"
#include "callbacks.h"
#include "interface.h"
#include "support.h"

//#include "tree.h"
#include "global.h"
#include "gui_main.h"
#include "tree.h"

/*
void
on_combo_entry1_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
  char* text;

  text = gtk_entry_get_text(GTK_ENTRY(editable));

  dir_tree_show(text);
}


void
on_spinbutton1_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
  global.max_depth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  on_drawingarea1_expose_event(NULL, NULL, NULL);  
}


gboolean
on_drawingarea1_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{

  return FALSE;
}

*/
