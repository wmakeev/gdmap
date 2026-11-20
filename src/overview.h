/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
#ifndef GDMAP_OVERVIEW_H
#define GDMAP_OVERVIEW_H
#include "tree.h"
gboolean snapshot_overview(tree_t* root, gboolean expand, const char* path, gint node_id, guint limit, GError** error);
GArray* snapshot_index_new(tree_t* root);
tree_t* snapshot_index_lookup(GArray* index, const char* number);
void snapshot_index_expand(GArray* index, tree_t* node, guint limit);
void snapshot_expand_batch(tree_t* root, guint limit);
#endif
