#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "tree.h"
#include "global.h"
#include "support.h"
#include "colors.h"

dir_tree_t* dir_tree_new(char* lname, char* sname, int is_dir) {
  dir_tree_t* result;
  char str[2048];

  strcpy(str, lname);
  if (is_dir)
    if (str[strlen(str)-1] != '/') strcat(str, "/");

  result = (dir_tree_t*)malloc(sizeof(dir_tree_t));
  result->full_name = strdup(str);
  result->name = strdup(sname);
  result->size = 0;
  result->dir_size = 0;
  result->entries = NULL;
  result->parent = NULL;
  result->depth = 0;

  result->width = 0;
  result->height = 0;
  result->buf = NULL;
  return result;
}

void dir_tree_destroy(dir_tree_t* tree) {
  GList* dlist;
  dir_tree_t* sub_tree;

  if (!tree) return;
  if (tree->entries) {
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      sub_tree = (dir_tree_t*)(dlist->data);
      dir_tree_destroy(sub_tree);
    }
    g_list_free(tree->entries);
  }
  if (tree->full_name) free(tree->full_name);
  if (tree->name) free(tree->name);
  free(tree);
}

dir_tree_t* dir_tree_rec(char* dirname, char* shortname, int depth) {
  DIR* dir;
  struct dirent* entry;
  struct stat buf;
  char name[2048];
  dir_tree_t* result = NULL;
  dir_tree_t* tree = NULL;
  
  lstat(dirname, &buf);
  if (buf.st_mode&S_IFDIR) {
    result = dir_tree_new(dirname, shortname, 1);
    result->dir_size += buf.st_size;
  } else {
    result = dir_tree_new(dirname, shortname, 0);
    result->size += buf.st_size;
    return result;
  }
  result->depth = depth;

  if ((dir = opendir(result->full_name)) == NULL) {
    printf("could not open dir [%s] (add_shared)\n", result->full_name);
    return 0;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    
    strcpy(name, result->full_name);
    strcat(name, entry->d_name);
    tree = dir_tree_rec(name, entry->d_name, depth+1);
    
    if (tree) {
      tree->parent = result;
      result->size += tree->size;
      result->entries = g_list_append(result->entries, tree);
    }
  }
  closedir(dir);

  return result;
}

void dir_tree_setup_popdown() {
  GList* pop = NULL;
  GList* dlist;
  dir_tree_t* tree;
  GtkWidget* temp;

  for (dlist = global.trees; dlist; dlist = dlist->next) {
    tree = (dir_tree_t*)(dlist->data);
    pop = g_list_append(pop, strdup(tree->full_name));
  }

  temp = lookup_widget(global.win, "combo1");
  if (pop && temp) {
    gtk_combo_set_popdown_strings (GTK_COMBO (temp), pop);
  }

  for (dlist = pop; dlist; dlist = dlist->next) {
    free(dlist->data);
  }
  g_list_free(pop);
}

dir_tree_t* dir_tree_read(char* name) {
  dir_tree_t* dir_tree;

  dir_tree = dir_tree_rec(name, name, 0);
  if (dir_tree) {
    global.trees = g_list_append(global.trees, dir_tree);
    dir_tree_setup_popdown();
  }
  return dir_tree;
}

dir_tree_t* dir_tree_search(char* name) {
  GList* dlist;
  dir_tree_t* tree;

  for (dlist = global.trees; dlist; dlist = dlist->next) {
    tree = (dir_tree_t*)(dlist->data);
    if (!strcmp(tree->full_name, name)) return tree;
  }
  return NULL;
}

void dir_tree_show(char* name) {
  dir_tree_t* tree;

  tree = dir_tree_search(name);
  if (!tree) tree = dir_tree_read(name);
  
  if (tree) dir_tree_display(tree);
}

void dir_tree_drec(dir_tree_t* tree, int depth) {
  GList* dlist;
  int i1;
  dir_tree_t* sub_tree;

  for (i1 = 0; i1 < depth; i1++) printf("  ");
  if (tree->entries)
    printf("[%s] [%lld]\n", tree->name, tree->size);
  else
    printf("%s [%lld]\n", tree->name, tree->size);
  for (dlist = tree->entries; dlist; dlist = dlist->next) {
    sub_tree = (dir_tree_t*)(dlist->data);
    dir_tree_drec(sub_tree, depth+1);
  }
}

void extract_rgb(char* color, int* red, int* green, int* blue) {
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

char* calc_color(int red, int green, int blue, double percent) {
  static char color[8] = "#ffffff";
  static char hex[17] = "0123456789abcdef";
  int val;

  val = (int)(red*percent);
  color[1] = hex[val/16];
  color[2] = hex[val%16];
  val = (int)(green*percent);
  color[3] = hex[val/16];
  color[4] = hex[val%16];
  val = (int)(blue*percent);
  color[5] = hex[val/16];
  color[6] = hex[val%16];

  return color;
}

double cfunc(int x, int y, int width, int height) {
  double h2, w2;
  double h22, w22;
  double x1, x2;

  w2 = (double)width/2.0;
  h2 = (double)height/2.0;
  w22 = w2*w2;
  h22 = h2*h2;

  x1 = 1-((w2-x)*(w2-x))/(w22);
  x2 = 1-((h2-y)*(h2-y))/(h22);

  return (x1+x2)/2;
}

double cfunc2(int x, int y, int width, int height) {
  double h2, w2;
  double h22, w22;
  double x1, x2;

  w2 = (double)width/2.0;
  h2 = (double)height/2.0;
  w22 = w2-x+1;
  if (w22 < 0) w22 = -w22;
  h22 = h2-y;
  if (h22 < 0) h22 = -h22;

  x1 = 1-(w22)/(w2);
  x2 = 1-(h22)/(h2);

  return (x1+x2)/2;
}

void dir_tree_display_file(GtkWidget* area, dir_tree_t* tree) {
  color_t* color;
  int red, green, blue;
  char* scolor;
  int i1, i2;
  GdkColor gcolor;
  int x;
  double val;
  long pos;

  if (tree->entries) color = color_search(".dir");
  else color = color_search(tree->full_name);

  if (global.paint_mode == P_3D) {
    gdk_color_parse(color->color, &gcolor);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);

    gdk_draw_rectangle(area->window, area->style->white_gc, 0,
		       tree->geo.x, tree->geo.y, 
		       tree->geo.width-1, tree->geo.height-1);
    if ((tree->geo.width > 2) && (tree->geo.height > 2))
      gdk_draw_rectangle(area->window, area->style->black_gc, 0,
			 tree->geo.x+1, tree->geo.y+1, 
			 tree->geo.width-2, tree->geo.height-2);
    if ((tree->geo.width > 3) && (tree->geo.height > 3))
      gdk_draw_rectangle(area->window, color->gc, 1,
			 tree->geo.x+1, tree->geo.y+1, 
			 tree->geo.width-3, tree->geo.height-3);
  } else if (global.paint_mode == P_GRADIENT) {
    extract_rgb(color->color, &red, &green, &blue);
    if (tree->depth%2) x = tree->geo.width;
    else x = tree->geo.height;

    for (i1 = 0; i1 < x; i1++) {
      scolor = calc_color(red, green, blue, (double)(i1+1)/(double)x);
      gdk_color_parse(scolor, &gcolor);
      gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
      gdk_gc_set_foreground(color->gc, &gcolor);
      if (tree->depth%2) {
	gdk_draw_line(area->window, color->gc,
		      tree->geo.x+i1, tree->geo.y, 
		      tree->geo.x+i1, tree->geo.y+tree->geo.height);
      } else {
	gdk_draw_line(area->window, color->gc,
		      tree->geo.x, tree->geo.y+i1, 
		      tree->geo.x+tree->geo.width, tree->geo.y+i1);
      }
    }
  } else if (global.paint_mode == P_CUSHION) {
    if (!tree->buf || (tree->geo.width != tree->width) ||
	(tree->geo.height != tree->height)) {
      extract_rgb(color->color, &red, &green, &blue);
      
      tree->width = tree->geo.width;
      tree->height = tree->geo.height;

      pos = 0;
      if (tree->buf) free(tree->buf);
      tree->buf = (char*)malloc(3*tree->width * tree->height);

      for (i2 = 0; i2 < tree->height; i2++) {
	for (i1 = 0; i1 < tree->width; i1++) {
	  val = cfunc(i1+1, i2+1, tree->width, tree->height);
	  tree->buf[pos++] = (int)(red*val);
	  tree->buf[pos++] = (int)(green*val);
	  tree->buf[pos++] = (int)(blue*val);
	}
      }
    }
    gdk_draw_rgb_image (area->window,
			area->style->white_gc,
			tree->geo.x, tree->geo.y, 
			tree->width, tree->height,
			GDK_RGB_DITHER_NONE,
			tree->buf, tree->width*3);
  } else {
    gdk_color_parse(color->color, &gcolor);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    gdk_draw_rectangle(area->window, area->style->black_gc, 0,
		       tree->geo.x, tree->geo.y, 
		       tree->geo.width-1, tree->geo.height-1);
    gdk_draw_rectangle(area->window, color->gc, 1,
		       tree->geo.x+1, tree->geo.y+1, 
		       tree->geo.width-1, tree->geo.height-1);
  }
}

void dir_tree_display_rec(dir_tree_t* tree, GtkWidget* area,
			  GtkAllocation alloc) {
  GList* dlist;
  dir_tree_t* sub_tree;
  GtkAllocation alloc2;
  int x1, x2;
  long size;

  /*
  printf("display [%s] at [%d %d %d %d] [%ld]\n", tree->full_name,
	 alloc.x, alloc.y, alloc.width, alloc.height, tree->size);
  */
  tree->geo = alloc;
  if (tree->entries && ((global.max_depth==0) || (tree->depth < global.max_depth))) {
    if (tree->depth%2) {
      alloc2.x = alloc.x;
      alloc2.width = alloc.width;
    } else {
      alloc2.y = alloc.y;
      alloc2.height = alloc.height;
    }
    x1 = 0;
    size = 0;
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      sub_tree = (dir_tree_t*)(dlist->data);
      size += sub_tree->size;
      if (tree->depth%2)
	x2 = (int)(alloc.height * ((double)size/(double)tree->size));
      else
	x2 = (int)(alloc.width * ((double)size/(double)tree->size));
      if (tree->depth%2) {
	alloc2.y = alloc.y+x1;
	alloc2.height = x2-x1;
      } else {
	alloc2.x = alloc.x+x1;
	alloc2.width = x2-x1;
      }
      dir_tree_display_rec(sub_tree, area, alloc2);
      x1 = x2;
    }
  } else {
    if ((alloc.width >= BORDER) && (alloc.height >= BORDER)) {
      dir_tree_display_file(area, tree);
    }
  }
}

dir_tree_t* dir_tree_get_root(dir_tree_t* dir) {
  dir_tree_t* root = dir;

  if (!root) return root;
  while (root->parent) root = root->parent;

  return root;
}

void dir_tree_display(dir_tree_t* tree) {
  GtkWidget* area;
  GtkAllocation alloc;
  GtkWidget* temp;
  char* str;

  if (!tree) return;
  if (global.mark) global.mark = NULL;
  if (global.mark2) global.mark2 = NULL;

  global.current_tree = tree;
  /*
  temp = lookup_widget(global.win, "combo_entry1");
  gtk_entry_set_text(GTK_ENTRY(temp), tree->full_name);
  */
  if (tree) {
    area = lookup_widget(global.win, "drawingarea1");
    alloc = area->allocation;
    alloc.x = 0;
    alloc.y = 0;
    gtk_paint_box(area->style, area->window,
		  GTK_STATE_NORMAL, GTK_SHADOW_NONE,
		  NULL, area, "trough",
		  alloc.x, alloc.y, alloc.width, alloc.height);
    //  dir_tree_drec(tree, 0);
    dir_tree_display_rec(tree, area, alloc);

    temp = lookup_widget(global.win, "label10");
    str = size_format(tree->size);
    gtk_label_set_text(GTK_LABEL(temp), str);
  }
  //  dir_tree_mark(NULL);
}

dir_tree_t* dir_tree_search_file(dir_tree_t* tree, int x, int y) {
  GList* dlist;
  dir_tree_t* sub_tree;
  dir_tree_t* ttree;

  if (!tree) return NULL;

  if ((x < tree->geo.x) || (y < tree->geo.y) ||
      (x >= tree->geo.x + tree->geo.width) ||
      (y >= tree->geo.y + tree->geo.height))
    return NULL;

  if (!global.max_depth || (global.max_depth > tree->depth)) {
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      sub_tree = (dir_tree_t*)(dlist->data);
      ttree = dir_tree_search_file(sub_tree, x, y);
      if (ttree) return ttree;
    }
  }
  
  if ((tree->geo.width >= BORDER) && (tree->geo.height >= BORDER)) 
    return tree;
  else return NULL;
}

dir_tree_t* dir_tree_find_path(dir_tree_t* root, dir_tree_t* child) {
  dir_tree_t* temp;

  if (!child || !root) return NULL;

  for (temp = child; temp; temp = temp->parent) {
    if (temp->parent == root) return temp;
  }
  return NULL;
}

void dir_tree_unmark1(GtkWidget* area, dir_tree_t* tree) {
  color_t* color;

  color = color_search(".color1");
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo.x, tree->geo.y,
		     tree->geo.width-1, tree->geo.height-1);
  global.mark = NULL;
}

void dir_tree_unmark2(GtkWidget* area, dir_tree_t* tree) {
  color_t* color;

  color = color_search(".color2");
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo.x, tree->geo.y,
		     tree->geo.width-1, tree->geo.height-1);
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo.x-1, tree->geo.y-1,
		     tree->geo.width+1, tree->geo.height+1);
  global.mark2 = NULL;
}

void dir_tree_mark1(GtkWidget* area, dir_tree_t* tree, dir_tree_t*  parent) {
  GtkWidget* temp;
  color_t* color;
  char* str;

  color = color_search(".color1");
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo.x, tree->geo.y,
		     tree->geo.width-1, tree->geo.height-1);
  global.mark = tree;
  
  temp = lookup_widget(global.win, "entry1");
  gtk_entry_set_text(GTK_ENTRY(temp), tree->full_name);
  temp = lookup_widget(global.win, "label12");
  str = size_format(tree->size);
  gtk_label_set_text(GTK_LABEL(temp), str);
  if (parent) {
    temp = lookup_widget(global.win, "label7");
    str = g_strdup_printf("%.2f %%", 
			  100*((double)(tree->size)/(double)(parent->size)));
    gtk_label_set_text(GTK_LABEL(temp), str);
    g_free(str);
  }
}

void dir_tree_mark2(GtkWidget* area, dir_tree_t* tree) {
  color_t* color;
  GtkWidget* temp;
  char* str;

  color = color_search(".color2");
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo.x, tree->geo.y,
		     tree->geo.width-1, tree->geo.height-1);
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo.x-1, tree->geo.y-1,
		     tree->geo.width+1, tree->geo.height+1);
  
  if (tree != global.mark2) {
    temp = lookup_widget(global.win, "entry2");
    gtk_entry_set_text(GTK_ENTRY(temp), tree->full_name);
    temp = lookup_widget(global.win, "label3");
    str = size_format(tree->size);
    gtk_label_set_text(GTK_LABEL(temp), str);
    temp = lookup_widget(global.win, "label11");
    gtk_label_set_text(GTK_LABEL(temp), str);
    temp = lookup_widget(global.win, "label6");
    str = g_strdup_printf("%.2f %%", 
			  100*((double)(tree->size)/(double)(global.current_tree->size)));
    gtk_label_set_text(GTK_LABEL(temp), str);
  }
  global.mark2 = tree;
}

void dir_tree_mark(dir_tree_t* sub_tree) {
  GtkWidget* area;
  dir_tree_t* tree;

  area = lookup_widget(global.win, "drawingarea1");
  if (!sub_tree) return;

  if (sub_tree == global.mark) return;
  tree = dir_tree_find_path(global.current_tree, sub_tree);

  if (global.mark) dir_tree_unmark1(area, global.mark);
  dir_tree_mark1(area, sub_tree, tree);

  if (!tree) return;

  if (tree == global.mark2) return;
  if (global.mark2) dir_tree_unmark2(area, global.mark2);
  dir_tree_mark2(area, tree);
}

void dir_tree_refresh(dir_tree_t* tree) {
  dir_tree_t* parent;
  dir_tree_t* new_tree;

  if (!tree) return;
  parent = tree->parent;

  if (parent) {
    parent->entries = g_list_remove(parent->entries, tree);
    parent->size -= tree->size;
    //    parent->dir_size -= tree->dir_size;
    new_tree = dir_tree_rec(tree->full_name, tree->name, tree->depth);
    dir_tree_destroy(tree);
    if (new_tree) {
      parent->size += new_tree->size;
      parent->entries = g_list_append(parent->entries, new_tree);
      new_tree->parent = parent;
      dir_tree_display(new_tree);
    }
  } else {
    new_tree = dir_tree_read(tree->full_name);
    dir_tree_destroy(tree);
    if (new_tree) dir_tree_display(new_tree);
  }
}

void on_level_down (GtkMenuItem     *menuitem,
		    gpointer         user_data)
{
  dir_tree_t* dir = (dir_tree_t*)user_data;
  dir_tree_t* sub_tree;

  sub_tree = dir_tree_find_path(dir, global.mark);
  if (sub_tree) {
    dir_tree_display(sub_tree);
  }
}

void on_level_up (GtkMenuItem     *menuitem, gpointer         user_data)
{
  dir_tree_t* dir = (dir_tree_t*)user_data;

  if (!dir) return;
  if (!dir->parent) return;

  dir_tree_display(dir->parent);
}

void on_level_root (GtkMenuItem     *menuitem, gpointer         user_data)
{
  dir_tree_t* dir = (dir_tree_t*)user_data;
  dir_tree_t* root = NULL;

  if (!dir) return;

  root = dir_tree_get_root(dir);
  if (dir != root) dir_tree_display(root);
}

void on_refresh (GtkMenuItem     *menuitem, gpointer         user_data) {
  dir_tree_t* dir = (dir_tree_t*)user_data;

  if (!dir) return;
  dir_tree_refresh(dir);
}


GtkWidget* create_right_popup(dir_tree_t* current) {
  GtkWidget *popup;
  GtkAccelGroup *popup_accels;
  GtkWidget *item;
  GtkWidget *separator;

  popup = gtk_menu_new ();
  popup_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (popup));

  item = gtk_menu_item_new_with_label ("Up to Root");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_level_root),
                      (gpointer)current);
  if (!current->parent)
    gtk_widget_set_sensitive(item, FALSE);
  
  separator = gtk_menu_item_new ();
  gtk_widget_show (separator);
  gtk_container_add (GTK_CONTAINER (popup), separator);
  gtk_widget_set_sensitive (separator, FALSE);
  

  item = gtk_menu_item_new_with_label ("One Level up");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_level_up),
                      (gpointer)current);
  if (!current->parent)
    gtk_widget_set_sensitive(item, FALSE);
  
  item = gtk_menu_item_new_with_label ("One Level Down");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_level_down),
                      (gpointer)current);
  if (!current->entries)
    gtk_widget_set_sensitive(item, FALSE);
  
  separator = gtk_menu_item_new ();
  gtk_widget_show (separator);
  gtk_container_add (GTK_CONTAINER (popup), separator);
  gtk_widget_set_sensitive (separator, FALSE);

  item = gtk_menu_item_new_with_label ("Refresh");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_refresh),
                      (gpointer)current);
  return popup;
}
