#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "tree.h"
#include "global.h"

gboolean
on_drawingarea1_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{
  if (global.expose_timer >= 0) {
    gtk_timeout_remove(global.expose_timer);
  }
  global.expose_timer = 
    gtk_timeout_add(200, expose_timeout_cb, NULL);

  return FALSE;
}


gboolean
on_main_win_delete_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_exit(0);
  
  return TRUE;
}


void
on_button3_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget* entry;
  char* text;

  entry = lookup_widget(global.win, "combo_entry1");
  text = gtk_entry_get_text(GTK_ENTRY(entry));
  dir_tree_show(text);
}


gboolean
on_drawingarea1_motion_notify_event    (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
  dir_tree_t* sub_tree;

  if (!global.current_tree) return FALSE;

  sub_tree = dir_tree_search_file(global.current_tree, 
				  (int)(event->x), (int)(event->y));
  if (sub_tree) dir_tree_mark(sub_tree);
  return FALSE;
}


gboolean
on_drawingarea1_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  dir_tree_t* sub_tree;
  GtkWidget* pop;

  if (!global.current_tree) return FALSE;

  sub_tree = dir_tree_search_file(global.current_tree, 
				  (int)(event->x), (int)(event->y));
  if (sub_tree) {
    dir_tree_mark(sub_tree);
    if (event->button == 3) {
      pop = create_right_popup(sub_tree);
      gtk_menu_popup(GTK_MENU(pop), NULL, NULL, NULL, NULL,
		     event->button, event->time);
    }
  }

  return FALSE;
}


void
on_combo_entry1_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
  char* text;
  dir_tree_t* tree;

  text = gtk_entry_get_text(GTK_ENTRY(editable));

  tree = dir_tree_search(text);
  if (!tree) return;

  if (tree != global.current_tree) dir_tree_display(tree);
}


void
on_combo_entry2_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
  char* text;

  text = gtk_entry_get_text(GTK_ENTRY(editable));
  if (!strcasecmp(text, "simple")) global.paint_mode = P_SIMPLE;
  else if (!strcasecmp(text, "3d")) global.paint_mode = P_3D;
  else if (!strcasecmp(text, "gradient")) global.paint_mode = P_GRADIENT;
  else if (!strcasecmp(text, "cushion")) global.paint_mode = P_CUSHION;
  else global.paint_mode = P_SIMPLE;

  dir_tree_display(global.current_tree);
}


void
on_spinbutton1_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
  global.max_depth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(editable));
  on_drawingarea1_expose_event(NULL, NULL, NULL);  
}

