/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <unistd.h>

#include <strings.h>
#include <string.h>

#include <gtk/gtk.h>
#include <stdio.h>

static char *folder[] = {
  "16 16 8 1",
  " 	c None",
  ".	c #909000",
  "+	c #000000",
  "@	c #EFE8EF",
  "#	c #FFF8CF",
  "$	c #FFF890",
  "%	c #CFC860",
  "&	c #FFC890",
  "                ",
  "  .....+        ",
  " .@##$$.+       ",
  ".%%%%%%%......  ",
  ".###########$%+ ",
  ".#$$$$$$$$$$&%+ ",
  ".#$$$$$$$&$&$%+ ",
  ".#$$$$$$$$&$&%+ ",
  ".#$$$$$&$&$&$%+ ",
  ".#$$$$$$&$&$&%+ ",
  ".#$$$&$&$&$&&%+ ",
  ".#&$&$&$&$&&&%+ ",
  ".%%%%%%%%%%%%%+ ",
  " ++++++++++++++ ",
  "                ",
  "                "
};

static char *ofolder[] = {
  "16 16 12 1",
  " 	c None",
  ".	c #808080",
  "+	c #E0E0D0",
  "@	c #4F484F",
  "#	c #909000",
  "$	c #FFF8EF",
  "%	c #CFC860",
  "&	c #003090",
  "*	c #7F7800",
  "=	c #FFC890",
  "-	c #FFF890",
  ";	c #2F3000",
  "        .       ",
  "       .+@      ",
  "   ###.$$+@     ",
  "  #%%.$$$$+@    ",
  "  #%.$$$&$$+@** ",
  "  #.+++&+&+++@* ",
  "############++@ ",
  "#$$$$$$$$$=%#++@",
  "#$-------=-=#+; ",
  " #---=--=-==%#; ",
  " #-----=-=-==#; ",
  " #-=--=-=-=-=#; ",
  "  #=-=-=-=-==#; ",
  "  ############; ",
  "   ;;;;;;;;;;;  ",
  "                "
};

static GdkPixmap *folder_pixmap = NULL, *ofolder_pixmap;
static GdkBitmap *folder_mask, *ofolder_mask;

typedef struct {
  gboolean scanned;
  gchar *path;
} DirNode;

/*
static void destroy_cb(gpointer data) {
  DirNode *node = data;

  g_free(node->path);
  g_free(node);
}
*/

static void 
expand_cb(GtkTreeView * tv, GtkTreeIter* iter) {
  GtkTreeModel* model;
  DirNode *parent_node, *dirnode;
  GtkTreeIter child, dummy, child2;
  DIR *dir;
  struct dirent *dirent;
  gboolean has_subdir = FALSE;
  gchar *gpath, *text;
  struct stat statbuf;

  model = gtk_tree_view_get_model (tv);
  gtk_tree_model_get (model, iter, 0, &parent_node, -1);
  if (!parent_node) {
    printf("no node\n");
    return;
  }

  if (!parent_node->scanned) {
    gtk_tree_model_iter_children(model, &dummy, iter);
    if ((dir = opendir(parent_node->path)) != NULL) {
      while ((dirent = readdir(dir)) != NULL) {
	gpath = g_strconcat(parent_node->path, dirent->d_name, NULL);
	if (stat(gpath, &statbuf) != -1 && S_ISDIR(statbuf.st_mode) && dirent->d_name[0] != '.') {
	  dirnode = g_malloc0(sizeof (DirNode));
	  dirnode->path = g_strconcat(gpath, "/", NULL);
	  text = dirent->d_name;
	  if (statbuf.st_nlink > 2)
	    //	  if (check_for_subdir(dirnode->path))
	    has_subdir = TRUE;
	  else
	    has_subdir = FALSE;
	  gtk_tree_store_append (GTK_TREE_STORE(model), 
				 &child, iter);
	  gtk_tree_store_set (GTK_TREE_STORE(model), &child,
			      0, dirnode,
			      1, dirent->d_name,
			      -1);
	  if (has_subdir) {
	    gtk_tree_store_append (GTK_TREE_STORE(model),
				   &child2, &child);
	    gtk_tree_store_set (GTK_TREE_STORE(model), &child2,
				0, NULL,
				1, "dummy",
				-1);
	  }
	}
	g_free(gpath);
      }
      closedir(dir);
      //      gtk_ctree_sort_node(GTK_CTREE(widget), parent_node);
    }
    gtk_tree_store_remove(GTK_TREE_STORE(model), &dummy);
    parent_node->scanned = TRUE;
  }
}

static void browse_destroy(GtkWidget* win) {
  gtk_widget_destroy(win);
}

static void 
foreach_cb(GtkTreeModel* model, GtkTreePath* path,
	   GtkTreeIter* iter, gpointer data) {
  DirNode* node;
  void (*handler)(gchar*);

  (void)path;
  handler = data;
  gtk_tree_model_get (model, iter, 0, &node, -1);
  if (node) handler(node->path);
}

static void
ok_clicked(GtkTreeView* tv) {
  GtkTreeSelection *select;
  void (*handler)(char*);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  handler = g_object_get_data(G_OBJECT(tv), "handler");

  if (handler)
    gtk_tree_selection_selected_foreach(select, foreach_cb, handler);
}

int tree_search(GtkTreeView* tv, GtkTreeModel* model,
		const char* dir, GtkTreeIter* result) {
  DirNode* node;
  int len;
  GtkTreeIter parent;
  GtkTreeIter child;
  char* folder = g_strdup(dir);

  parent = *result;
  while (1) {
    gtk_tree_model_get (model, &parent, 0, &node, -1);
    len = strlen(node->path);
    if (!strncmp(node->path, folder, len)) {
      if (folder[len] == 0 ||
	  (folder[len] == '/' && folder[len+1] == 0)) {
	(*result) = parent;
	g_free(folder);
	return 1;
      } else if (folder[len] == '/' || folder[len-1] == '/') {
	if (!node->scanned) expand_cb(tv, &parent);
	gtk_tree_model_iter_children(model, &child, &parent);
	parent = child;
      } else {
	g_free(folder);
	return 0;
      }
    } else {
      if (!gtk_tree_model_iter_next(model, &parent)) break;
    }
  }
  g_free(folder);
  return 0;
}

GtkWidget*
create_dir_browser(const char* title, const char* path,
		   void (*handler)(char*)) {
  GtkWidget *win;
  GtkWidget *vbox;
  GtkWidget *scrolledwindow;
  GtkTreeStore *store;
  GtkWidget* tv;
  GtkTreeSelection *select;
  GtkCellRenderer *renderer;
  GtkTreeIter root_node;
  GtkTreeIter dummy_node;
  DirNode *dirnode;
  GtkWidget *hseparator;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkTreeViewColumn *column;
 
  if (!path) path = g_get_home_dir();

  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (win), title);
  gtk_window_set_default_size (GTK_WINDOW (win), 300, 450);
  
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (win), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_realize(win);

  if (!folder_pixmap) {
    folder_pixmap = gdk_pixmap_create_from_xpm_d(win->window, &folder_mask, NULL, folder);
    ofolder_pixmap = gdk_pixmap_create_from_xpm_d(win->window, &ofolder_mask, NULL, ofolder);
  }
  
  store = gtk_tree_store_new(2,
			     G_TYPE_POINTER,
			     G_TYPE_STRING);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tv), FALSE);
  gtk_tree_view_columns_autosize(GTK_TREE_VIEW(tv));
  //  gtk_tree_view_collapse_all(GTK_TREE_VIEW(tv));
  gtk_widget_show(tv);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes 
    ("Folder", renderer, "text", 1, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  gtk_container_add (GTK_CONTAINER (scrolledwindow), tv);
  g_object_set_data(G_OBJECT(tv), "handler", (gpointer) handler);

  g_signal_connect(G_OBJECT(tv), "row-expanded",
		   G_CALLBACK(expand_cb), NULL);

  gtk_tree_store_append (store, &root_node, NULL);
  dirnode = g_malloc0(sizeof (DirNode));
  dirnode->path = g_strdup("/");
  gtk_tree_store_set (store, &root_node,
		      0, dirnode,
		      1, dirnode->path,
		      -1);
  gtk_tree_store_append (store, &dummy_node, &root_node);
  gtk_tree_store_set (store, &dummy_node,
		      0, NULL,
		      1, "dummy",
		      -1);
  gtk_tree_view_expand_row(GTK_TREE_VIEW(tv),
			   gtk_tree_model_get_path(GTK_TREE_MODEL(store),
						   &root_node),
			   FALSE);

  hseparator = gtk_hseparator_new ();
  gtk_widget_show (hseparator);
  gtk_box_pack_start (GTK_BOX (vbox), hseparator, FALSE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  hbox = gtk_hbox_new (TRUE, 5);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  button = gtk_button_new_with_label ("Ok");
  //  g_object_set_data(G_OBJECT(button), win);
  gtk_widget_show (button);
  g_signal_connect_swapped(G_OBJECT(button), "clicked", 
			   G_CALLBACK(ok_clicked), tv);
  g_signal_connect_swapped(G_OBJECT(button), "clicked", 
			   G_CALLBACK(browse_destroy), win);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  button = gtk_button_new_with_label ("Cancel");
  g_signal_connect_swapped(G_OBJECT(button), "clicked", 
			   G_CALLBACK(browse_destroy), win);
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

  tree_search(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(store),
	      path, &root_node);

  gtk_tree_selection_select_iter(select, &root_node);

  return win;
}

