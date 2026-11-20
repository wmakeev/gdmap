/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
#include "snapshot.h"
#include <stdio.h>
#include <string.h>

static void inspect_node(tree_t* node, guint64 parent, guint64* next)
{
    guint64 index = (*next)++;
    char* path = tree_full_name(node);
    char* escaped = g_strescape(path, NULL);
    printf("%" G_GUINT64_FORMAT "\t%" G_GUINT64_FORMAT "\t%s\t%" G_GINT64_FORMAT "\t%s\n",
        index, parent, node->is_directory ? "directory" : node->is_symlink ? "symlink" : "file",
        node->size, escaped);
    g_free(escaped);
    g_free(path);
    for (GList* item = node->entries; item; item = item->next)
        inspect_node(item->data, index, next);
}

static void inspect(tree_t* root, const tree_scan_options* options)
{
    guint64 next = 0;
    printf("# gdmap-snapshot 1 leave-device=%s use-reported-size=%s\n",
        options->leave_device ? "true" : "false", options->use_reported_size ? "true" : "false");
    puts("# index\tparent\tkind\tbytes\tpath (C-escaped bytes)");
    inspect_node(root, 0, &next);
}

int main(int argc, char** argv)
{
    gboolean leave_device = FALSE, allocated_size = FALSE;
    GOptionEntry entries[] = {
        {"leave-device", 0, 0, G_OPTION_ARG_NONE, &leave_device, "Descend onto other filesystems when scanning", NULL},
        {"allocated-size", 0, 0, G_OPTION_ARG_NONE, &allocated_size, "Use allocated bytes when positive and smaller than file size", NULL},
        {NULL, 0, 0, 0, NULL, NULL, NULL}
    };
    GOptionContext* context = g_option_context_new("scan FOLDER SNAPSHOT | inspect SNAPSHOT");
    g_option_context_set_summary(context, "Scan once, then inspect the saved tree without accessing the source or user preferences.");
    g_option_context_add_main_entries(context, entries, NULL);
    GError* error = NULL;
    gboolean parsed = g_option_context_parse(context, &argc, &argv, &error);
    if (!parsed) {
        g_printerr("%s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);
        return 2;
    }
    g_option_context_free(context);
    tree_scan_options options = {leave_device, !allocated_size};
    tree_t* root = NULL;
    if (argc == 4 && !strcmp(argv[1], "scan")) {
        char* path = g_canonicalize_filename(argv[2], NULL);
        root = tree_load_with_options(path, NULL, NULL, 0, &options);
        g_free(path);
        if (!root) {
            g_printerr("Unable to scan root: %s\n", argv[2]);
            return 1;
        }
        if (!snapshot_save(argv[3], root, &options, &error)) goto failed;
    } else if (argc == 3 && !strcmp(argv[1], "inspect") && !leave_device && !allocated_size) {
        root = snapshot_load(argv[2], &options, &error);
        if (!root) goto failed;
    } else {
        g_printerr("Usage: gdmap-cli [--leave-device] [--allocated-size] scan FOLDER SNAPSHOT\n"
                   "       gdmap-cli inspect SNAPSHOT\n");
        return 2;
    }
    inspect(root, &options);
    tree_destroy(root);
    return 0;
failed:
    g_printerr("%s\n", error->message);
    g_error_free(error);
    tree_destroy(root);
    return 1;
}
