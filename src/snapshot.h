/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
#ifndef GDMAP_SNAPSHOT_H
#define GDMAP_SNAPSHOT_H
#include "tree.h"

/* Version 1: (version, cross-device, reported-size, preorder records).
 * Record: parent index (root points to itself), raw name bytes, size, kind.
 * Kinds: 0 file, 1 directory, 2 symbolic link. Names contain no NUL bytes. */
gboolean snapshot_save(const char* filename, tree_t* root, const tree_scan_options* options, GError** error);
tree_t* snapshot_load(const char* filename, tree_scan_options* options, GError** error);
#endif
