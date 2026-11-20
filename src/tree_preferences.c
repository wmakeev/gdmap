/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
/* GUI compatibility entry point; the scanner itself never loads preferences. */
#include <gtk/gtk.h>
#include "tree.h"
#include "preferences.h"

tree_t* tree_load(const char* folder, ProgressFunc func, void* data, unsigned depth)
{
    tree_scan_options options = {pref_get_leave_device(), pref_get_use_reported_size()};
    return tree_load_with_options(folder, func, data, depth, &options);
}
