#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <unistd.h>

#include <strings.h>
#include <string.h>

#include <gtk/gtk.h>
#include <stdio.h>
#include "support.h"

static char *folder[] =
{
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
	"                "};

static char *ofolder[] =
{
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
	"                "};

static GdkPixmap *folder_pixmap = NULL, *ofolder_pixmap;
static GdkBitmap *folder_mask, *ofolder_mask;

typedef struct {
  gboolean scanned;
  gchar *path;
} DirNode;

static gboolean check_for_subdir(gchar * path) {
  DIR *dir;
  struct dirent *dirent;
  struct stat statbuf;
  gchar *npath;
  
  if ((dir = opendir(path)) != NULL) {
    while ((dirent = readdir(dir)) != NULL) {
      if (dirent->d_name[0] != '.') {
	npath = g_strconcat(path, dirent->d_name, NULL);
	if (stat(npath, &statbuf) != -1 && S_ISDIR(statbuf.st_mode)) {
	  g_free(npath);
	  closedir(dir);
	  return TRUE;
	}
	g_free(npath);
      }
    }
    closedir(dir);
  }
  return FALSE;
}

static void destroy_cb(gpointer data) {
  DirNode *node = data;
  
  g_free(node->path);
  g_free(node);
}

static void expand_cb(GtkWidget * widget, GtkCTreeNode * parent_node) {
  DIR *dir;
  struct dirent *dirent;
  gchar *path, *text, *dummy = "dummy";
  struct stat statbuf;
  GtkCTreeNode *node, *sub_node;
  DirNode *parent_dirnode, *dirnode;
  gboolean has_subdir = FALSE;
  
  parent_dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(widget), parent_node);
  if (!parent_dirnode->scanned) {
    gtk_clist_freeze(GTK_CLIST(widget));
    node = gtk_ctree_find_by_row_data(GTK_CTREE(widget), parent_node, NULL);
    gtk_ctree_remove_node(GTK_CTREE(widget), node);
    if ((dir = opendir(parent_dirnode->path)) != NULL) {
      while ((dirent = readdir(dir)) != NULL) {
	path = g_strconcat(parent_dirnode->path, dirent->d_name, NULL);
	if (stat(path, &statbuf) != -1 && S_ISDIR(statbuf.st_mode) && dirent->d_name[0] != '.') {
	  dirnode = g_malloc0(sizeof (DirNode));
	  dirnode->path = g_strconcat(path, "/", NULL);
	  text = dirent->d_name;
	  if (check_for_subdir(dirnode->path))
	    has_subdir = TRUE;
	  else
	    has_subdir = FALSE;
	  node = gtk_ctree_insert_node(GTK_CTREE(widget), parent_node, NULL, &text, 4, folder_pixmap, folder_mask, ofolder_pixmap, ofolder_mask, !has_subdir, FALSE);
	  gtk_ctree_node_set_row_data_full(GTK_CTREE(widget), node, dirnode, destroy_cb);
	  if (has_subdir)
	    sub_node = gtk_ctree_insert_node(GTK_CTREE(widget), node, NULL, &dummy, 4, NULL, NULL, NULL, NULL, FALSE, FALSE);
	}
	g_free(path);
      }
      closedir(dir);
      gtk_ctree_sort_node(GTK_CTREE(widget), parent_node);
    }
    gtk_clist_thaw(GTK_CLIST(widget));
    parent_dirnode->scanned = TRUE;
  }
}

static void select_row_cb(GtkWidget * widget, gint row, gint column, GdkEventButton * bevent, gpointer data) {
  DirNode *dirnode;
  GtkCTreeNode *node;
  void (*handler) (gchar *);
  
  if (bevent) {
    if (bevent->type == GDK_2BUTTON_PRESS) {
      node = gtk_ctree_node_nth(GTK_CTREE(widget), row);
      dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(widget), node);
      handler = (void (*)(gchar *)) gtk_object_get_user_data(GTK_OBJECT(widget));
      if (handler)
	handler(dirnode->path);
    }
  }
}

static void ok_clicked(GtkWidget * widget, GtkWidget * tree) {
  GtkCTreeNode *node;
  DirNode *dirnode;
  GList *list_node;
  GtkWidget *window;
  void (*handler) (gchar *);
  
  window = gtk_object_get_user_data(GTK_OBJECT(widget));
  gtk_widget_hide(window);
  list_node = GTK_CLIST(tree)->selection;
  while (list_node) {
    node = list_node->data;
    dirnode = gtk_ctree_node_get_row_data(GTK_CTREE(tree), node);
    handler = (void (*)(gchar *)) gtk_object_get_user_data(GTK_OBJECT(tree));
    if (handler)
      handler(dirnode->path);
    list_node = g_list_next(list_node);
  }
  gtk_widget_destroy(window);
}

static int dir_compare_func(const void *a, const void *b) {
  if (!a || !b || !((DirNode *) a)->path)
    return -1;
  return strcasecmp(((DirNode *) a)->path, (gchar *) b);
}

GtkWidget *create_dir_browser(gchar * title, gchar * current_path, GtkSelectionMode mode, void (*handler) (gchar *)) {
  GtkWidget *win;
  GtkWidget *frame40;
  GtkWidget *vbox29;
  GtkWidget *scrolledwindow51;
  GtkWidget *ctree1;
  GtkWidget *hseparator6;
  GtkWidget *frame42;
  GtkWidget *hbox60;
  GtkWidget *button80;
  GtkWidget *button81;
  gchar *root_text = "/", *node_text = "dummy";
  GtkCTreeNode *root_node, *node, *nextnode;
  DirNode *dirnode;
  gchar *currentdir, *pos, *tpath, *tpathnew;
  gboolean leaf;
 
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (win), "win", win);
  gtk_widget_set_usize (win, 230, 320);
  gtk_window_set_title (GTK_WINDOW (win), title);
  gtk_window_set_policy (GTK_WINDOW (win), TRUE, TRUE, FALSE);

  frame40 = gtk_frame_new (NULL);
  gtk_widget_ref (frame40);
  gtk_object_set_data_full (GTK_OBJECT (win), "frame40", frame40,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame40);
  gtk_container_add (GTK_CONTAINER (win), frame40);
  gtk_container_set_border_width (GTK_CONTAINER (frame40), 5);

  vbox29 = gtk_vbox_new (FALSE, 5);
  gtk_widget_ref (vbox29);
  gtk_object_set_data_full (GTK_OBJECT (win), "vbox29", vbox29,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox29);
  gtk_container_add (GTK_CONTAINER (frame40), vbox29);
  gtk_container_set_border_width (GTK_CONTAINER (vbox29), 5);

  scrolledwindow51 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow51);
  gtk_object_set_data_full (GTK_OBJECT (win), "scrolledwindow51", scrolledwindow51,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow51);
  gtk_box_pack_start (GTK_BOX (vbox29), scrolledwindow51, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow51), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_realize(win);
  if (!folder_pixmap) {
    folder_pixmap = gdk_pixmap_create_from_xpm_d(win->window, &folder_mask, NULL, folder);
    ofolder_pixmap = gdk_pixmap_create_from_xpm_d(win->window, &ofolder_mask, NULL, ofolder);
  }

  ctree1 = gtk_ctree_new (1, 0);
  gtk_clist_set_column_auto_resize(GTK_CLIST(ctree1), 0, TRUE);
  gtk_ctree_set_line_style(GTK_CTREE(ctree1), GTK_CTREE_LINES_DOTTED);
  gtk_clist_set_selection_mode(GTK_CLIST(ctree1), mode);
  gtk_widget_ref (ctree1);
  gtk_object_set_data_full (GTK_OBJECT (win), "ctree1", ctree1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_clist_column_titles_hide (GTK_CLIST (ctree1));
  gtk_signal_connect(GTK_OBJECT(ctree1), "tree_expand", GTK_SIGNAL_FUNC(expand_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(ctree1), "select_row", GTK_SIGNAL_FUNC(select_row_cb), NULL);
  gtk_container_add (GTK_CONTAINER (scrolledwindow51), ctree1);
  gtk_object_set_user_data(GTK_OBJECT(ctree1), (gpointer) handler);

  root_node = gtk_ctree_insert_node(GTK_CTREE(ctree1), NULL, NULL, &root_text, 4, folder_pixmap, folder_mask, ofolder_pixmap, ofolder_mask, FALSE, FALSE);
  dirnode = g_malloc0(sizeof (DirNode));
  dirnode->path = g_strdup("/");
  gtk_ctree_node_set_row_data_full(GTK_CTREE(ctree1), root_node, dirnode, destroy_cb);
  node = gtk_ctree_insert_node(GTK_CTREE(ctree1), root_node, NULL, &node_text, 4, NULL, NULL, NULL, NULL, TRUE, TRUE);
  gtk_ctree_expand(GTK_CTREE(ctree1), root_node);

  gtk_widget_show (ctree1);

  hseparator6 = gtk_hseparator_new ();
  gtk_widget_ref (hseparator6);
  gtk_object_set_data_full (GTK_OBJECT (win), "hseparator6", hseparator6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator6);
  gtk_box_pack_start (GTK_BOX (vbox29), hseparator6, FALSE, TRUE, 0);

  frame42 = gtk_frame_new (NULL);
  gtk_widget_ref (frame42);
  gtk_object_set_data_full (GTK_OBJECT (win), "frame42", frame42,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame42);
  gtk_box_pack_start (GTK_BOX (vbox29), frame42, FALSE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame42), GTK_SHADOW_IN);

  hbox60 = gtk_hbox_new (TRUE, 5);
  gtk_widget_ref (hbox60);
  gtk_object_set_data_full (GTK_OBJECT (win), "hbox60", hbox60,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox60);
  gtk_container_add (GTK_CONTAINER (frame42), hbox60);
  gtk_container_set_border_width (GTK_CONTAINER (hbox60), 5);

  button80 = gtk_button_new_with_label ("Ok");
  gtk_widget_ref (button80);
  gtk_object_set_user_data(GTK_OBJECT(button80), win);
  gtk_object_set_data_full (GTK_OBJECT (win), "button80", button80,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button80);
  gtk_signal_connect(GTK_OBJECT(button80), "clicked", 
		     GTK_SIGNAL_FUNC(ok_clicked), ctree1);
  gtk_box_pack_start (GTK_BOX (hbox60), button80, FALSE, TRUE, 0);
  GTK_WIDGET_UNSET_FLAGS (button80, GTK_CAN_FOCUS);

  button81 = gtk_button_new_with_label ("Cancel");
  gtk_widget_ref (button81);
  gtk_object_set_data_full (GTK_OBJECT (win), "button81", button81,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_signal_connect_object(GTK_OBJECT(button81), "clicked", 
			    GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(win));
  gtk_widget_show (button81);
  gtk_box_pack_start (GTK_BOX (hbox60), button81, FALSE, TRUE, 0);
  GTK_WIDGET_UNSET_FLAGS (button81, GTK_CAN_FOCUS);

  gtk_signal_connect(GTK_OBJECT(ctree1), "tree_expand", GTK_SIGNAL_FUNC(expand_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(ctree1), "select_row", GTK_SIGNAL_FUNC(select_row_cb), NULL);

  if (current_path && *current_path) {
    currentdir = g_strdup(current_path);
    tpath = g_strdup("/");
    pos = strtok(currentdir, "/");
    node = gtk_ctree_find_by_row_data_custom(GTK_CTREE(ctree1), NULL, "/", dir_compare_func);
    do {
      tpathnew = g_strconcat(tpath, pos, "/", NULL);
      g_free(tpath);
      tpath = tpathnew;
      nextnode = 
	gtk_ctree_find_by_row_data_custom(GTK_CTREE(ctree1), 
					  node, tpath, 
					  dir_compare_func);
      if (!nextnode)
	break;
      node = nextnode;
      pos = strtok(NULL, "/");
      gtk_ctree_get_node_info(GTK_CTREE(ctree1), node, 
			      NULL, NULL, NULL, NULL, NULL, NULL, 
			      &leaf, NULL);
      if (!leaf && pos)
	gtk_ctree_expand(GTK_CTREE(ctree1), node);
      else {
	gtk_ctree_select(GTK_CTREE(ctree1), node);
	break;
      }
    } while (pos);
    g_free(tpath);
    g_free(currentdir);
  } else
    gtk_ctree_select(GTK_CTREE(ctree1), root_node);

  return win;
}

