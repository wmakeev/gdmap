/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/vfs.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "gui_main.h"
#include "dirselect.h"
#include "tree.h"
#include "colors.h"
#include "global.h"

GtkWidget* MainWin = NULL;
GtkWidget* TreeLabel = NULL;
GtkWidget* DirLabel = NULL;
GtkWidget* FileLabel = NULL;
GtkWidget* TreeSize = NULL;
GtkWidget* DirSize = NULL;
GtkWidget* FileSize = NULL;
GtkWidget* TreePer = NULL;
GtkWidget* DirPer = NULL;
GtkWidget* FilePer = NULL;

static char* Buffer = NULL;
static int Width = 0;
static int Height = 0;
static tree_t* Mark1 = NULL;
static tree_t* Mark2 = NULL;

static GtkWidget* Area = NULL;

static gboolean
on_main_win_delete_event() {
  global_exit();
  return TRUE;
}

static void
on_dir_select(char* dir) {
  if (dir) tree_load(dir);
}

static void
on_button_browse_clicked() {
  GtkWidget* win;
  
  win = create_dir_browser("Select Directory", NULL, on_dir_select);
  gtk_widget_show(win);
}

void gui_tree_display(tree_t* tree);

static gboolean
on_drawingarea_configure(GtkWidget* area) {

  if (Buffer) g_free(Buffer);

  Width = area->allocation.width;
  Height = area->allocation.height;

  Buffer = g_malloc0(sizeof(unsigned int)*Width*Height*3);

  if (CurrentTree) gui_tree_display(CurrentTree);
  return TRUE;
}

static void
gui_show_pix(GtkWidget* area) {
  if (Buffer) {
    gdk_draw_rgb_image(area->window,
		       area->style->white_gc,
		       0, 0, Width, Height,
		       GDK_RGB_DITHER_NONE,
		       Buffer, Width*3);
  }
}

static gboolean
on_area_expose(GtkWidget* area) {
  gui_show_pix(area);
  return TRUE;
}

static void gui_tree_unmark1(tree_t* tree) {
  color_t* color;
  
  color = color_search(".color1");
  gdk_draw_rectangle(Area->window, color->gc, 0,
		     tree->geo[0][0],tree->geo[1][0],
		     tree->geo[0][1]-tree->geo[0][0]-1,
		     tree->geo[1][1]-tree->geo[1][0]-1);
  Mark1 = NULL;
}

static void
gui_tree_mark1(tree_t* tree) {
  color_t* color;

  color = color_search(".color1");
  gdk_draw_rectangle(Area->window, color->gc, 0,
		     tree->geo[0][0],tree->geo[1][0],
		     tree->geo[0][1]-tree->geo[0][0]-1,
		     tree->geo[1][1]-tree->geo[1][0]-1);
  if (tree != Mark1) {
    gtk_label_set_text(GTK_LABEL(FileLabel), tree_full_name(tree));
    gtk_label_set_text(GTK_LABEL(FileSize), size_format(tree->size));
    gtk_label_set_text(GTK_LABEL(FilePer), 
		       per_format(tree->size, CurrentTree->size));
  }
  Mark1 = tree;
}

static void gui_tree_unmark2(tree_t* tree) {
  color_t* color;

  color = color_search(".color2");
  gdk_draw_rectangle(Area->window, color->gc, 0,
		     tree->geo[0][0]-1, tree->geo[1][0]-1,
		     tree->geo[0][1]-tree->geo[0][0]+1,
		     tree->geo[1][1]-tree->geo[1][0]+1);
  Mark2 = NULL;
}

void gui_tree_mark2(tree_t* tree) {
  color_t* color;
  
  color = color_search(".color2");
  gdk_draw_rectangle(Area->window, color->gc, 0,
		     tree->geo[0][0]-1, tree->geo[1][0]-1,
		     tree->geo[0][1]-tree->geo[0][0]+1,
		     tree->geo[1][1]-tree->geo[1][0]+1);

  if (tree != Mark2) {
    gtk_label_set_text(GTK_LABEL(DirLabel), tree_full_name(tree));
    gtk_label_set_text(GTK_LABEL(DirSize), size_format(tree->size));
    gtk_label_set_text(GTK_LABEL(DirPer), 
		       per_format(tree->size, CurrentTree->size));
  }
  Mark2 = tree;
}

static void
gui_tree_mark(tree_t* sub_tree) {
  tree_t* tree;

  if (!sub_tree || !CurrentTree) return;
  if (sub_tree == Mark1) return;
  tree = tree_find_path(CurrentTree, sub_tree);
  if (Mark1) gui_tree_unmark1(Mark1);
  gui_tree_mark1(sub_tree);

  if (tree == Mark2) return;
  if (Mark2) gui_tree_unmark2(Mark2);
  if (!tree) return;
  gui_tree_mark2(tree);
}

static gboolean
on_area_motion(GtkWidget       *widget,
	       GdkEventMotion  *event) {
  tree_t* sub_tree;

  (void)widget;
  if (!CurrentTree) return FALSE;

  sub_tree = 
    tree_search_file(CurrentTree, (int)(event->x), (int)(event->y));
  if (sub_tree) gui_tree_mark(sub_tree);
  return TRUE;
}

static void on_level_root(tree_t* tree) {
  tree_t* root = NULL;

  root = dir_tree_get_root(tree);
  if (CurrentTree != root) gui_tree_display(root);
}

static void on_level_up(tree_t* tree) {
  
  if (tree && tree->parent)
    gui_tree_display(tree->parent);
}

static void on_level_down(tree_t* tree) {
  tree_t* sub_tree;

  sub_tree = tree_find_path(CurrentTree, tree);
  if (sub_tree) gui_tree_display(sub_tree);
}

static void on_refresh(tree_t* tree) {
  tree_refresh(tree);
}

GtkWidget* create_right_popup(tree_t* current) {
  GtkWidget *popup;
  GtkWidget *item;
  GtkWidget *separator;
  tree_t* c;

  c = CurrentTree;
  if (!c) return NULL;

  popup = gtk_menu_new ();

  item = gtk_menu_item_new_with_label ("Up to Root");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  g_signal_connect_swapped(G_OBJECT(item), "activate",
			   G_CALLBACK (on_level_root),
			   (gpointer)c);
  if (!c->parent)
    gtk_widget_set_sensitive(item, FALSE);
  
  separator = gtk_menu_item_new ();
  gtk_widget_show (separator);
  gtk_container_add (GTK_CONTAINER (popup), separator);
  gtk_widget_set_sensitive (separator, FALSE);
  
  item = gtk_menu_item_new_with_label ("One Level up");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  g_signal_connect_swapped(G_OBJECT (item), "activate",
			   G_CALLBACK (on_level_up),
			   (gpointer)c);
  if (!c->parent)
    gtk_widget_set_sensitive(item, FALSE);
  
  item = gtk_menu_item_new_with_label ("One Level Down");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  g_signal_connect_swapped(G_OBJECT (item), "activate",
			   G_CALLBACK (on_level_down),
			   (gpointer)current);
  if (!c->entries)
    gtk_widget_set_sensitive(item, FALSE);

  separator = gtk_menu_item_new ();
  gtk_widget_show (separator);
  gtk_container_add (GTK_CONTAINER (popup), separator);
  gtk_widget_set_sensitive (separator, FALSE);

  item = gtk_menu_item_new_with_label ("Refresh");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  g_signal_connect_swapped(G_OBJECT(item), "activate",
			   G_CALLBACK(on_refresh),
			   (gpointer)c);

  /*
  separator = gtk_menu_item_new ();
  gtk_widget_show (separator);
  gtk_container_add (GTK_CONTAINER (popup), separator);
  gtk_widget_set_sensitive (separator, FALSE);

  item = gtk_menu_item_new_with_label ("Delete");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);
  for (; current; current = current->parent) {
    item = gtk_menu_item_new_with_label (dir_tree_full_name(current));
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (menu), item);
    gtk_signal_connect (GTK_OBJECT (item), "activate",
			GTK_SIGNAL_FUNC (on_delete),
			(gpointer)current);
  }
  */
  return popup;
}

static gboolean
on_area_button_press(GtkWidget       *widget,
		     GdkEventButton  *event) {
  tree_t* sub_tree;
  GtkWidget* pop;

  (void)widget;
  if (!CurrentTree) return FALSE;

  sub_tree = 
    tree_search_file(CurrentTree, 
		     (int)(event->x), (int)(event->y));
  if (sub_tree) {
    gui_tree_mark(sub_tree);
    if (event->button == 3) {
      pop = create_right_popup(sub_tree);
      gtk_menu_popup(GTK_MENU(pop), NULL, NULL, NULL, NULL,
		     event->button, event->time);
    }
  }

  return FALSE;
}

static GtkWidget* gui_create_display_box() {
  GtkWidget* frame;
  GtkWidget* drawingarea;

  frame = gtk_frame_new(NULL);
  gtk_widget_show(frame);
  gtk_frame_set_shadow_type(GTK_FRAME (frame), GTK_SHADOW_IN);

  drawingarea = gtk_drawing_area_new();
  Area = drawingarea;
  gtk_widget_show (drawingarea);
  gtk_container_add (GTK_CONTAINER (frame), drawingarea);
  gtk_widget_set_events (drawingarea, 
			 GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
  // no need for offscreen support of gtk, we do it manually
  gtk_widget_set_double_buffered(drawingarea, FALSE);

  g_signal_connect (G_OBJECT(drawingarea), "configure_event",
		    G_CALLBACK (on_drawingarea_configure),
		    NULL);
  g_signal_connect (G_OBJECT(drawingarea), "expose_event",
		    G_CALLBACK (on_area_expose),
		    NULL);
  g_signal_connect (G_OBJECT (Area), "motion_notify_event",
		    G_CALLBACK (on_area_motion),
		    NULL);
  g_signal_connect (G_OBJECT (Area), "button_press_event",
		    G_CALLBACK (on_area_button_press),
		    NULL);
  return frame;
}

GtkWidget* gui_get_main_win() {
  return MainWin;
}

static void on_depth_change(GtkSpinButton* button) {
  int val = gtk_spin_button_get_value_as_int(button);
  if (val != MaxDepth) {
    MaxDepth = val;
    gui_tree_display(CurrentTree);
  }
}

static GtkWidget* gui_create_info_box() {
  GtkWidget* table;
  GtkWidget* label;
  GtkWidget* frame;
  GtkWidget* button;
  GtkWidget* spinbutton;
  GtkWidget* sep;
  GtkWidget* vbox;

  table = gtk_table_new (5, 3, FALSE);
  gtk_widget_show (table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 0);
  
  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  TreeSize = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  TreePer = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
  gtk_table_attach (GTK_TABLE (table), frame, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
                    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND), 0, 0);

  label = gtk_label_new("");
  TreeLabel = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 5, 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_table_attach (GTK_TABLE (table), vbox, 3, 4, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  sep = gtk_hseparator_new();
  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);

  label = gtk_label_new("display depth");
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_misc_set_padding(GTK_MISC(label), 5, 0);

  sep = gtk_hseparator_new();
  gtk_widget_show (sep);
  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 0);

  spinbutton = gtk_spin_button_new
    (GTK_ADJUSTMENT (gtk_adjustment_new (MaxDepth, 0, 1000, 1, 10, 10)),
     1, 0);
  gtk_widget_show (spinbutton);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 4, 5, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  g_signal_connect(G_OBJECT (spinbutton), "value-changed",
		   G_CALLBACK (on_depth_change),
		   NULL);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  DirSize = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
  
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  DirPer = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
  
  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  DirLabel = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

  gtk_table_attach (GTK_TABLE (table), frame, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
                    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND), 0, 0);


  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  FileSize = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  FilePer = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

  gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  frame = gtk_frame_new(NULL);
  gtk_widget_show (frame);
  label = gtk_label_new("");
  FileLabel = label;
  gtk_widget_show (label);
  gtk_container_add(GTK_CONTAINER(frame), label);
  gtk_misc_set_padding(GTK_MISC(label), 5, 2);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);

  gtk_table_attach (GTK_TABLE (table), frame, 2, 3, 2, 3,
                    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND),
                    (GtkAttachOptions) (GTK_FILL|GTK_EXPAND), 0, 0);


  button = gtk_button_new_with_label ("Browse");
  gtk_widget_show (button);
  gtk_table_attach (GTK_TABLE (table), button, 3, 5, 1, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (on_button_browse_clicked),
		    NULL);



  return table;
}

GtkWidget* gui_create_main_win (void) {
  GtkWidget *main_win;
  GtkWidget *vbox;
  GtkWidget* widget;
  
  main_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  MainWin = main_win;
  gtk_window_set_title (GTK_WINDOW (main_win), "Graphical Disk Map "VERSION);
  gtk_window_set_default_size (GTK_WINDOW (main_win), 700, 450);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (main_win), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

  if ((widget = gui_create_info_box()) != NULL)
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

  if ((widget = gui_create_display_box()) != NULL)
    gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (main_win), "delete_event",
		    G_CALLBACK(on_main_win_delete_event),
		    NULL);
  
  gtk_widget_show(main_win);

  //  global.win = main_win;

  return main_win;
}

static void extract_rgb(char* color,
			int* red, int* green, int* blue) {
  char str[5] = "0x00";

  str[2] = color[1];
  str[3] = color[2];
  *red = strtoul(str, NULL, 16);
  str[2] = color[3];
  str[3] = color[4];
  *green = strtoul(str, NULL, 16);
  str[2] = color[5];
  str[3] = color[6];
  *blue = strtoul(str, NULL, 16);
}

static void gui_tree_display_file(tree_t* tree,
				  int r[2][2], double s[2][2]) {
  static int Ia = 40;
  static int Is = 215;
  static double Lx = 0.09759;
  static double Ly = 0.19518;
  static double Lz = 0.9759;
  int ix, iy;
  double nx, ny, cosa, val;
  int x1, y1;
  int x2, y2;
  long pos;
  int red, green, blue;
  color_t* color;

  x1 = r[0][0];
  y1 = r[1][0];
  x2 = r[0][1];
  y2 = r[1][1];
  
  if (tree->entries) color = color_search(".dir");
  else color = color_search(tree->name);
  
  extract_rgb(color->color, &red, &green, &blue);
  
  pos = (y1*Width+x1)*3;
  for (iy = y1; iy < y2; iy++) {
    for (ix = x1; ix < x2; ix++) {
      nx = -(2*s[0][1]*(ix+0.5)+s[0][0]);
      ny = -(2*s[1][1]*(iy+0.5)+s[1][0]);
      cosa = (nx*Lx + ny*Ly + Lz)/sqrt(nx*nx + ny*ny +1.0);
      val = (Ia+MAX(0, Is*cosa));
      Buffer[pos++] = (int)(red*val/255.);
      Buffer[pos++] = (int)(green*val/255.);
      Buffer[pos++] = (int)(blue*val/255.);
    }
    pos -= (x2-x1)*3;  // cr
    pos += Width*3;    // lf
  }
}

void AddRidge(double x1, double x2, double h, double* s1, double* s2) {
  *s1 += 4*h*(x2+x1)/(x2-x1);
  *s2 -= 4*h/(x2-x1);
}

static int
gui_tree_display_rec(tree_t* tree, int allow_skip,
		     int r[2][2], double h, double f,
		     int d, double s[2][2]) {
  GList* dlist;
  int new_r[2][2];
  double new_s[2][2];
  int w;
  gint64 size;

  memcpy(new_r, r, sizeof(int)*4);
  memcpy(new_s, s, sizeof(double)*4);

  if (r[0][1]-r[0][0] <= allow_skip ||
      r[1][1]-r[1][0] <= allow_skip ||
      tree->size == 0) {
    memset(tree->geo, 0, sizeof(int)*4);
    return 0;
  }
  memcpy(tree->geo, r, sizeof(int)*4);
  
  if (tree->parent) 
    AddRidge(new_r[d][0], new_r[d][1], h,
	     &(new_s[d][0]), &(new_s[d][1]));

  if (tree->entries && (MaxDepth == 0 || tree->depth < MaxDepth)) {
    d = 1-d;

    w = (new_r[d][1] - new_r[d][0]);
    size = 0;
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      tree_t* sub_tree = dlist->data;
      size += sub_tree->size;
      new_r[d][1] = r[d][0] + size*w/tree->size;
      if (gui_tree_display_rec(sub_tree, dlist->next?allow_skip:0,
			       new_r, h*f, f, d, new_s))
	new_r[d][0] = new_r[d][1];
    }
  } else {
    gui_tree_display_file(tree, new_r, new_s);
  }
  return 1;
}

void gui_tree_display(tree_t* tree) {
  double s[2][2];
  int r[2][2];
  struct statfs buf;

  g_assert(tree);

  statfs(tree->name, &buf);
  CurrentTree = tree;
  r[0][0] = 1;
  r[0][1] = Width-1;
  r[1][0] = 1;
  r[1][1] = Height-1;
  s[0][0] = s[0][1] = s[1][0] = s[1][1] = .0;
  
  gui_tree_display_rec(tree, 1, 
		       r, 0.9, 1.0, (tree->depth%2)?0:1, s);
  
  //  gtk_entry_set_text(GTK_ENTRY(tree_name), tree_full_name(tree));
  gui_show_pix(Area);

  Mark1 = NULL;
  Mark2 = NULL;

  gtk_label_set_text(GTK_LABEL(TreeLabel), tree_full_name(tree));
  gtk_label_set_text(GTK_LABEL(TreeSize), size_format(tree->size));
  gtk_label_set_text(GTK_LABEL(TreePer), 
		     per_format(tree->size, CurrentTree->size));
}



/*

void gui_tree_display(tree_t* tree);


*/
