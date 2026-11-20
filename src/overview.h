/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
#ifndef GDMAP_OVERVIEW_H
#define GDMAP_OVERVIEW_H
#include "tree.h"
gboolean snapshot_overview(tree_t* root, gboolean expand, const char* path, gint node_id, guint limit, GError** error);
void snapshot_expand_batch(tree_t* root, guint limit);
#endif
