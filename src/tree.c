/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "tree.h"
#include "gui_main.h"

tree_t* CurrentTree = NULL;
int MaxDepth = 0;

static void tree_print(tree_t* tree, int d) {
  GList* dlist;
  printf("%*s%-20s [%4d,%4d][%4d,%4d] %lld\n", d*2, "", tree->name, 
	 tree->geo[0][0], tree->geo[1][0], 
	 tree->geo[0][1], tree->geo[1][1], tree->size);
  for (dlist = tree->entries; dlist; dlist = dlist->next) {
    tree = dlist->data;
    tree_print(tree, d+1);
  }
}

char* tree_full_name(tree_t* tree) {
  static char dir[4096];

  if (tree->parent) {
    strcpy(dir, tree_full_name(tree->parent));
    strcat(dir, tree->name);
  } else {
    strcpy(dir, tree->name);
  }

  return dir;
}

static void tree_destroy(tree_t* tree) {
  GList* dlist;

  g_assert(tree);
  for (dlist = tree->entries; dlist; dlist = dlist->next) {
    tree_t* sub_tree = dlist->data;
    tree_destroy(sub_tree);
  }
  g_list_free(tree->entries);
  g_free(tree->name);
  g_free(tree);
}

static tree_t*
tree_new(char* sname, int is_dir) {
  tree_t* result;
  char str[2048];

  strcpy(str, sname);
  if (is_dir)
    if (str[strlen(str)-1] != '/') strcat(str, "/");

  result = g_malloc(sizeof(tree_t));
  result->name = g_strdup(str);
  result->size = 0;
  result->dir_size = 0;
  result->entries = NULL;
  result->parent = NULL;
  result->depth = 0;

  return result;
}

static gint
comp_func (gconstpointer p1, gconstpointer p2) {
  const tree_t* t1 = (tree_t*)p1;
  const tree_t* t2 = (tree_t*)p2;
  
  if (t1->size < t2->size) return 1;
  if (t1->size > t2->size) return -1;
  return 0;
}

static tree_t*
tree_scan_rec(char* dirname, char* shortname, int depth) {
  DIR* dir;
  struct dirent* entry;
  struct stat buf;
  char name[2048];
  tree_t* result = NULL;
  tree_t* tree = NULL;
  
  if (lstat(dirname, &buf) < 0) return NULL;
  if (buf.st_mode&S_IFDIR) {
    result = tree_new(shortname, 1);
    result->dir_size += buf.st_size;
  } else {
    result = tree_new(shortname, 0);
    result->size += buf.st_size;
    return result;
  }
  result->depth = depth;

  if ((dir = opendir(dirname)) == NULL) {
    g_log("TREE", G_LOG_LEVEL_DEBUG,
	  "could not open dir '%s'", dirname);
    return NULL;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".")) continue;
    if (!strcmp(entry->d_name, "..")) continue;
    
    strcpy(name, dirname);
    if (name[strlen(name)-1] != '/') strcat(name, "/");
    strcat(name, entry->d_name);
    tree = tree_scan_rec(name, entry->d_name, depth+1);
    
    if (tree) {
      tree->parent = result;
      result->size += tree->size;
      result->entries =
	g_list_insert_sorted(result->entries, tree, comp_func);
    }
  }
  closedir(dir);

  return result;
}

void tree_load(char* folder) {
  tree_t* tree;

  g_assert(folder);

  g_log("TREE", G_LOG_LEVEL_DEBUG,
	"scanning folder '%s'", folder);
  tree = tree_scan_rec(folder, folder, 0);
  if (tree) {
    while (CurrentTree && CurrentTree->parent)
      CurrentTree = CurrentTree->parent;
    if (CurrentTree) tree_destroy(CurrentTree);
    CurrentTree = tree;
    gui_tree_display(CurrentTree);
  } else {
    g_log("TREE", G_LOG_LEVEL_DEBUG,
	  "could not load tree '%s'", folder);
  }
}

tree_t* tree_search_file(tree_t* tree, int x, int y) {
  GList* dlist;
  tree_t* ttree;

  g_assert(tree);

  if (x < tree->geo[0][0] ||
      x >= tree->geo[0][1] ||
      y < tree->geo[1][0] ||
      y >= tree->geo[1][1])
    return NULL;

  if (!MaxDepth || MaxDepth > tree->depth) {
    for (dlist = tree->entries; dlist; dlist = dlist->next) {
      tree_t* sub_tree = dlist->data;
      ttree = tree_search_file(sub_tree, x, y);
      if (ttree) return ttree;
    }
  }
  return tree;
}

tree_t* tree_find_path(tree_t* root, tree_t* child) {
  tree_t* temp;

  g_assert(child && root);

  for (temp = child; temp; temp = temp->parent) {
    if (temp->parent == root) return temp;
  }
  return NULL;
}

tree_t* dir_tree_get_root(tree_t* dir) {
  tree_t* root = dir;

  if (!root) return root;
  while (root->parent) root = root->parent;

  return root;
}

void tree_refresh(tree_t* tree) {
  tree_t* parent;
  tree_t* new_tree;
  tree_t* temp;
  char str[2048];

  if (!tree) return;
  parent = tree->parent;

  if (parent) {
    parent->entries = g_list_remove(parent->entries, tree);
    for (temp = parent; temp; temp = temp->parent) {
      temp->size -= tree->size;
      //    temp->dir_size -= tree->dir_size;
    }
    new_tree = tree_scan_rec(tree_full_name(tree), tree->name, tree->depth);
    tree_destroy(tree);
    if (new_tree) {
      for (temp = parent; temp; temp = temp->parent) {
	temp->size += new_tree->size;
      }
      parent->entries =
	g_list_insert_sorted(parent->entries, new_tree, comp_func);
      new_tree->parent = parent;
      gui_tree_display(new_tree);
    }
  } else {
    strcpy(str, tree_full_name(tree));
    tree_load(str);
  }
}

