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

typedef enum {
  X = 0,
  Y = 1
} dir_t;

typedef enum {
  Min = 0,
  Max = 1
} bound_t;

dir_tree_t* dir_tree_new(char* sname, int is_dir) {
  dir_tree_t* result;
  char str[2048];

  strcpy(str, sname);
  if (is_dir)
    if (str[strlen(str)-1] != '/') strcat(str, "/");

  result = (dir_tree_t*)malloc(sizeof(dir_tree_t));
  result->name = strdup(str);
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
  
  if (lstat(dirname, &buf) < 0) return NULL;
  if (buf.st_mode&S_IFDIR) {
    result = dir_tree_new(shortname, 1);
    result->dir_size += buf.st_size;
  } else {
    result = dir_tree_new(shortname, 0);
    result->size += buf.st_size;
    //    printf("[%s][%lu]\n", dirname, result->size);
    return result;
  }
  result->depth = depth;

  if ((dir = opendir(dirname)) == NULL) {
    printf("could not open dir [%s][%s] (add_shared)\n", 
	   dirname, shortname);
    return NULL;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    
    strcpy(name, dirname);
    if (name[strlen(name)-1] != '/') strcat(name, "/");
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
    pop = g_list_append(pop, strdup(dir_tree_full_name(tree)));
  }

  temp = lookup_widget(global.win, "combo1");
  if (pop && temp) {
    gtk_combo_set_popdown_strings (GTK_COMBO (temp), pop);
  }
  
  for (dlist = pop; dlist; dlist = dlist->next) {
    free(dlist->data);
  }
  if (pop) g_list_free(pop);
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
    if (!strcmp(dir_tree_full_name(tree), name)) return tree;
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

void dir_tree_display_file(GtkWidget* area, dir_tree_t* tree,
			   double r[2][2], double s[2][2]) {
  static int Ia = 40;
  static int Is = 215;
  static double Lx = 0.09759;
  static double Ly = 0.19518;
  static double Lz = 0.9759;
  int ix, iy;
  double nx, ny, cosa, val;
  int x1, y1, x2, y2;
  int width, height;
  long pos;
  int red, green, blue;
  color_t* color;

  x1 = (int)(r[X][Min]+0.5);
  y1 = (int)(r[Y][Min]+0.5);
  x2 = (int)(r[X][Max]+0.5);
  y2 = (int)(r[Y][Max]+0.5);

  width = x2 - x1 +1;
  height = y2 - y1 +1;
  if (width + height == 0) return;

  if (!tree->buf || (tree->width != width) ||
      (tree->height != height)) {
    tree->width = width;
    tree->height = height;
    if (tree->buf) free(tree->buf);
    tree->buf = malloc(width * height * 3);

    if (tree->entries) color = color_search(".dir");
    else color = color_search(tree->name);

    extract_rgb(color->color, &red, &green, &blue);

    pos = 0;
    for (iy = y1; iy <= y2; iy++) 
      for (ix = x1; ix <= x2; ix++) {
	nx = -(2*s[X][1]*(ix+0.5)+s[X][0]);
	ny = -(2*s[Y][1]*(iy+0.5)+s[Y][0]);
	cosa = (nx*Lx + ny*Ly + Lz)/sqrt(nx*nx + ny*ny +1.0);
	val = (Ia+MAX(0, Is*cosa));
	tree->buf[pos++] = (int)(red*val/255.);
	tree->buf[pos++] = (int)(green*val/255.);
	tree->buf[pos++] = (int)(blue*val/255.);
      }
  }
  gdk_draw_rgb_image (area->window,
		      area->style->white_gc,
		      x1, y1, width, height,
		      GDK_RGB_DITHER_NONE,
		      tree->buf, width*3);
  
}

void AddRidge(double x1, double x2, double h, double* s1, double* s2) {
  *s1 += 4*h*(x2+x1)/(x2-x1);
  *s2 -= 4*h/(x2-x1);
}


void dir_tree_display_rec(GtkWidget* area, dir_tree_t* tree,
			  double r[2][2], double h, double f,
			  dir_t d, double s[2][2]) {
  double w;
  GList* dlist;
  dir_tree_t* sub_tree;
  double new_r[2][2];
  double new_s[2][2];
  
  /*
  printf("display [%s] at [%.1f %.1f %.1f %.1f] [%lld]\n", dir_tree_full_name(tree),
	 r[0][0],r[1][0],r[0][1],r[1][1], tree->size);
  */
  memcpy(new_r, r, sizeof(double)*4);
  memcpy(new_s, s, sizeof(double)*4);
  memcpy(tree->geo, r, sizeof(double)*4);

  if (tree->parent) 
    AddRidge(new_r[d][Min], new_r[d][Max], h, &(new_s[d][0]), &(new_s[d][1]));

  if (tree->entries && ((global.max_depth==0) || (tree->depth < global.max_depth))) {
    if (d == X) d = Y;
    else d = X;

    if (tree->size)
      w = (new_r[d][Max] - new_r[d][Min])/tree->size;
    else w = 0;
    
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      sub_tree = (dir_tree_t*)(dlist->data);
      new_r[d][Max] = new_r[d][Min] + w*sub_tree->size;
      dir_tree_display_rec(area, sub_tree, new_r, h*f, f, d, new_s);
      new_r[d][Min] = new_r[d][Max];
    }
  } else {
    dir_tree_display_file(area, tree, new_r, new_s);
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
  GtkWidget* temp;
  char* str;
  double s[2][2];
  double r[2][2];

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
    r[X][Min] = .0;
    r[Y][Min] = .0;
    r[X][Max] = (double)(area->allocation.width);
    r[Y][Max] = (double)(area->allocation.height);

    printf("*** [%.1f %.1f %.1f %.1f]\n",
	   r[0][0],r[1][0],r[0][1],r[1][1]);

    /*
    gtk_paint_box(area->style, area->window,
		  GTK_STATE_NORMAL, GTK_SHADOW_NONE,
		  NULL, area, "trough",
		  alloc.x, alloc.y, alloc.width, alloc.height);
    */
    //  dir_tree_drec(tree, 0);
    s[X][0] = s[X][1] = s[Y][0] = s[Y][1] = 0;
    dir_tree_display_rec(area, tree, r, 0.7, 1.0, (tree->depth%2)?X:Y, s);
    
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

  if ((x < tree->geo[X][Min]) || (y < tree->geo[Y][Min]) ||
      (x >= tree->geo[X][Max]) || (y >= tree->geo[Y][Max]))
    return NULL;

  if (!global.max_depth || (global.max_depth > tree->depth)) {
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      sub_tree = (dir_tree_t*)(dlist->data);
      ttree = dir_tree_search_file(sub_tree, x, y);
      if (ttree) return ttree;
    }
  }
  
  return tree;
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
		     tree->geo[X][Min],tree->geo[Y][Min],
		     tree->geo[X][Max]-tree->geo[X][Min],
		     tree->geo[Y][Max]-tree->geo[Y][Min]);
  global.mark = NULL;
}

void dir_tree_unmark2(GtkWidget* area, dir_tree_t* tree) {
  color_t* color;

  color = color_search(".color2");
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo[X][Min],tree->geo[Y][Min],
		     tree->geo[X][Max]-tree->geo[X][Min],
		     tree->geo[Y][Max]-tree->geo[Y][Min]);
  global.mark2 = NULL;
}

void dir_tree_mark1(GtkWidget* area, dir_tree_t* tree, dir_tree_t*  parent) {
  GtkWidget* temp;
  color_t* color;
  char* str;

  color = color_search(".color1");
  gdk_draw_rectangle(area->window, color->gc, 0,
		     tree->geo[X][Min],tree->geo[Y][Min],
		     tree->geo[X][Max]-tree->geo[X][Min],
		     tree->geo[Y][Max]-tree->geo[Y][Min]);
  global.mark = tree;
  
  temp = lookup_widget(global.win, "entry1");
  gtk_entry_set_text(GTK_ENTRY(temp), dir_tree_full_name(tree));
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
		     tree->geo[X][Min],tree->geo[Y][Min],
		     tree->geo[X][Max]-tree->geo[X][Min],
		     tree->geo[Y][Max]-tree->geo[Y][Min]);

  if (tree != global.mark2) {
    temp = lookup_widget(global.win, "entry2");
    gtk_entry_set_text(GTK_ENTRY(temp), dir_tree_full_name(tree));
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
  dir_tree_t* temp;
  char str[2048];

  if (!tree) return;
  parent = tree->parent;

  if (parent) {
    parent->entries = g_list_remove(parent->entries, tree);
    for (temp = parent; temp; temp = temp->parent) {
      temp->size -= tree->size;
      //    temp->dir_size -= tree->dir_size;
    }
    new_tree = dir_tree_rec(dir_tree_full_name(tree), tree->name, tree->depth);
    dir_tree_destroy(tree);
    if (new_tree) {
      for (temp = parent; temp; temp = temp->parent) {
	temp->size += new_tree->size;
      }
      parent->entries = g_list_append(parent->entries, new_tree);
      new_tree->parent = parent;
      dir_tree_display(new_tree);
    }
  } else {
    strcpy(str, dir_tree_full_name(tree));
    dir_tree_destroy(tree);
    global.trees = g_list_remove(global.trees, tree);
    new_tree = dir_tree_read(str);
    if (new_tree) dir_tree_display(new_tree);
  }
}

void on_level_down (GtkMenuItem     *menuitem,
		    gpointer         user_data)
{
  dir_tree_t* dir = (dir_tree_t*)user_data;
  dir_tree_t* sub_tree;

  sub_tree = dir_tree_find_path(global.current_tree, dir);
  if (sub_tree) dir_tree_display(sub_tree);
}

void on_level_up (GtkMenuItem     *menuitem, gpointer         user_data)
{
  if (!global.current_tree) return;
  if (!global.current_tree->parent) return;

  dir_tree_display(global.current_tree->parent);
}

void on_level_root (GtkMenuItem     *menuitem, gpointer         user_data)
{
  dir_tree_t* dir = (dir_tree_t*)user_data;
  dir_tree_t* root = NULL;

  if (!dir) return;

  root = dir_tree_get_root(dir);
  if (global.current_tree != root) dir_tree_display(root);
}

void on_refresh (GtkMenuItem     *menuitem, gpointer         user_data) {
  if (!global.current_tree) return;
  dir_tree_refresh(global.current_tree);
}

void on_delete (GtkMenuItem     *menuitem, gpointer         user_data) {
  dir_tree_t* dir = (dir_tree_t*)user_data;
  char str[2048];

  if (dir->entries) {
    snprintf(str, 2047, "rm -r \"%s\"", dir_tree_full_name(dir));
    system(str);
  } else {
    unlink(dir_tree_full_name(dir));
  }
  on_refresh(NULL, NULL);
}


GtkWidget* create_right_popup(dir_tree_t* current) {
  GtkWidget *popup;
  GtkAccelGroup *popup_accels;
  GtkWidget *item;
  GtkWidget *separator;
  GtkWidget *menu;
  dir_tree_t* c;

  c = global.current_tree;
  if (!c) return NULL;

  popup = gtk_menu_new ();
  popup_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (popup));

  item = gtk_menu_item_new_with_label ("Up to Root");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_level_root),
                      (gpointer)current);
  if (!c->parent)
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
  if (!c->parent)
    gtk_widget_set_sensitive(item, FALSE);
  
  item = gtk_menu_item_new_with_label ("One Level Down");
  gtk_widget_show (item);
  gtk_container_add (GTK_CONTAINER (popup), item);
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_level_down),
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
  gtk_signal_connect (GTK_OBJECT (item), "activate",
                      GTK_SIGNAL_FUNC (on_refresh),
                      (gpointer)current);

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
  return popup;
}

char* dir_tree_full_name(dir_tree_t* tree) {
  static char dir[4096];

  if (tree->parent) {
    strcpy(dir, dir_tree_full_name(tree->parent));
    strcat(dir, tree->name);
  } else {
    strcpy(dir, tree->name);
  }

  return dir;
}
