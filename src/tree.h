/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifndef _TREE_H_
#define _TREE_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

typedef struct _tree_t tree_t;

struct _tree_t {
  char* name;
  gint64 size;
  gint64 dir_size;
  GList* entries;
  tree_t* parent;
  int geo[2][2];
  int depth;
};

extern int MaxDepth;
extern tree_t* CurrentTree;

void tree_load(char* folder);
char* tree_full_name(tree_t* tree);
tree_t* tree_search_file(tree_t* tree, int x, int y);
tree_t* tree_find_path(tree_t* root, tree_t* child);
tree_t* dir_tree_get_root(tree_t* dir);
void tree_refresh(tree_t* tree);

#endif
