/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "tree.h"
#include "utils.h"
#include "gui_main.h"

static ProgressFunc TheProgressFunc = NULL;
static unsigned TheProgress = 0;
static void* TheProgressData = NULL;
static unsigned TheWork = 0;
static unsigned MaxProgress = 2;
static gboolean Aborted = FALSE;

static void tree_print(tree_t* tree) {
  GList* dlist;
  printf("%*s%-20s "FF"\n", tree->depth*2, "", tree->name, tree->size);
  for (dlist = tree->entries; dlist; dlist = dlist->next) {
    tree = dlist->data;
    tree_print(tree);
  }
}

char* tree_full_name(tree_t* tree) {
  char* res;

  if (tree->parent) {
    char* p = tree_full_name(tree->parent);
    res = g_strdup_printf("%s/%s", p, tree->name);
    g_free(p);
  } else {
    res = g_strdup(tree->name);
  }

  return res;
}

void tree_destroy(tree_t* tree) {
  g_list_foreach(tree->entries, (GFunc)tree_destroy, NULL);
  g_list_free(tree->entries);
  g_free(tree->name);
  g_free(tree);
}

static tree_t* tree_new(const char* sname, gint64 size, unsigned depth) {
  static time_t last_t = 0;
  time_t t = 0;
  tree_t* result;

  result = g_malloc0(sizeof(tree_t));
  result->name = g_strdup(sname);
  result->size = size;
  result->entries = NULL;
  result->parent = NULL;
  result->depth = depth;

  if (depth <= MaxProgress) TheProgress++;

  if (TheProgressFunc && (t = time(NULL)) > last_t) {
    if (!TheProgressFunc((double)TheProgress/(double)TheWork, TheProgressData)) 
      Aborted = TRUE;
    last_t = t;
  }
  

  return result;
}

static gint comp_func(gconstpointer p1, gconstpointer p2) {
  const tree_t* t1 = (tree_t*)p1;
  const tree_t* t2 = (tree_t*)p2;
  
  if (t1->size < t2->size) return 1;
  if (t1->size > t2->size) return -1;
  return 0;
}

static tree_t* tree_scan_rec(const char* dirname, const char* shortname, int depth) {
  struct stat buf;
  const char* name;
  tree_t* result = NULL;
  tree_t* tree = NULL;
  GDir* dir;
  GError* error = NULL;

  if (lstat(dirname, &buf) < 0) return NULL;

  if (!(buf.st_mode&S_IFDIR)) {
    result = tree_new(shortname, buf.st_size, depth);
    return result;
  } else if ((dir = g_dir_open(dirname, 0, &error)) == NULL) {
    g_message("%s", error->message);
    g_error_free(error);
    return NULL;
  }

  result = tree_new(shortname, /*buf.st_size*/0, depth);

  while (result && !Aborted && (name = g_dir_read_name(dir)) != NULL) {
    char* nname;
    if (!strcmp(name, ".")) continue;
    if (!strcmp(name, "..")) continue;
    
    nname = g_strdup_printf("%s/%s", dirname, name);
    if ((tree = tree_scan_rec(nname, name, depth+1)) != NULL) {
      tree->parent = result;
      result->size += tree->size;
      result->entries = g_list_insert_sorted(result->entries, tree, comp_func);
    }
    g_free(nname);
  }
  g_dir_close(dir);

  return result;
}

static unsigned _tree_get_work(const char* dirname, unsigned d, unsigned res) {
  struct stat buf;
  const char* name;
  GDir* dir;
  
  if (lstat(dirname, &buf) < 0) return res;

  if (!(buf.st_mode&S_IFDIR)) {
    res++;
    return res;
  } else if ((dir = g_dir_open(dirname, 0, NULL)) == NULL) {
    return res;
  } else {
    res++;
  }
  
  if (d > 0) {
    while ((name = g_dir_read_name(dir)) != NULL) {
      char* nname;
      if (!strcmp(name, ".")) continue;
      if (!strcmp(name, "..")) continue;
      
      nname = g_strdup_printf("%s/%s", dirname, name);
      res = _tree_get_work(nname, d-1, res);
      g_free(nname);
    }
  }
  g_dir_close(dir);

  return res;
}

tree_t* tree_load(const char* folder, ProgressFunc func, void* data, unsigned depth) {
  tree_t* tree;
  
  TheProgressFunc = func;
  TheProgress = 0;
  TheProgressData = data;
  TheWork = 0;
  TheWork = _tree_get_work(folder, MaxProgress, TheWork);
  Aborted = FALSE;

  g_assert(folder);
  tree = tree_scan_rec(folder, folder, depth);

  TheProgressFunc = NULL;
  if (Aborted) {
    tree_destroy(tree);
    return NULL;
  } else {
    return tree;
  }
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

/* void tree_refresh(tree_t* tree) { */
/*   tree_t* parent; */
/*   tree_t* new_tree; */
/*   tree_t* temp; */
/*   char str[2048]; */

/*   if (!tree) return; */
/*   parent = tree->parent; */

/*   if (parent) { */
/*     parent->entries = g_list_remove(parent->entries, tree); */
/*     for (temp = parent; temp; temp = temp->parent) { */
/*       temp->size -= tree->size; */
/*     } */
/*     new_tree = tree_scan_rec(tree_full_name(tree), tree->name, tree->depth); */
/*     tree_destroy(tree); */
/*     if (new_tree) { */
/*       for (temp = parent; temp; temp = temp->parent) { */
/*         temp->size += new_tree->size; */
/*       } */
/*       parent->entries = */
/*         g_list_insert_sorted(parent->entries, new_tree, comp_func); */
/*       new_tree->parent = parent; */
/*       gui_tree_display(new_tree); */
/*     } */
/*   } else { */
/*     strcpy(str, tree_full_name(tree)); */
/*     tree_load(str); */
/*   } */
/* } */

