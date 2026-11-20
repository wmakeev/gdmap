/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
#include "snapshot.h"
#include <string.h>

#define SNAPSHOT_TYPE "(ubba(tayxy))"

static gboolean add_node(GVariantBuilder* records, tree_t* node, guint64 parent, guint64* next, unsigned depth)
{
    if (depth > 1024) return FALSE;
    guint64 index = (*next)++;
    GVariant* name = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, node->name, strlen(node->name), 1);
    g_variant_builder_add(records, "(t@ayxy)", parent, name, node->size,
                          node->is_directory ? 1 : node->is_symlink ? 2 : 0);
    for (GList* item = node->entries; item; item = item->next)
        if (!add_node(records, item->data, index, next, depth + 1)) return FALSE;
    return TRUE;
}

gboolean snapshot_save(const char* filename, tree_t* root, const tree_scan_options* options, GError** error)
{
    GVariantBuilder records;
    guint64 next = 0;
    g_variant_builder_init(&records, G_VARIANT_TYPE("a(tayxy)"));
    if (!add_node(&records, root, 0, &next, 0)) {
        g_variant_builder_clear(&records);
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Snapshot exceeds maximum depth of 1024");
        return FALSE;
    }
    GVariant* value = g_variant_ref_sink(g_variant_new("(ubb@a(tayxy))", 1,
        options->leave_device, options->use_reported_size, g_variant_builder_end(&records)));
    /* Fixed little-endian encoding, including on big-endian hosts. */
    if (G_BYTE_ORDER == G_BIG_ENDIAN) {
        GVariant* swapped = g_variant_byteswap(value);
        g_variant_unref(value);
        value = swapped;
    }
    gboolean ok = g_file_set_contents(filename, g_variant_get_data(value), g_variant_get_size(value), error);
    g_variant_unref(value);
    return ok;
}

tree_t* snapshot_load(const char* filename, tree_scan_options* options, GError** error)
{
    gchar* data;
    gsize length;
    if (!g_file_get_contents(filename, &data, &length, error)) return NULL;
    GVariant* value = g_variant_ref_sink(g_variant_new_from_data(G_VARIANT_TYPE(SNAPSHOT_TYPE),
        data, length, FALSE, g_free, data));
    GVariant* records = NULL;
    GPtrArray* nodes = g_ptr_array_new();
    tree_t* root = NULL;
    guint version;
    if (!g_variant_is_normal_form(value)) goto invalid;
    if (G_BYTE_ORDER == G_BIG_ENDIAN) {
        GVariant* swapped = g_variant_byteswap(value);
        g_variant_unref(value);
        value = swapped;
    }
    g_variant_get(value, "(ubb@a(tayxy))", &version, &options->leave_device,
                  &options->use_reported_size, &records);
    if (version != 1 || g_variant_n_children(records) == 0) goto invalid;
    for (gsize i = 0; i < g_variant_n_children(records); i++) {
        guint64 parent;
        gint64 size;
        guchar kind;
        GVariant* name_value;
        g_variant_get_child(records, i, "(t@ayxy)", &parent, &name_value, &size, &kind);
        gsize n;
        const gchar* name = g_variant_get_fixed_array(name_value, &n, 1);
        gboolean valid = n > 0 && !memchr(name, 0, n) && size >= 0 && kind <= 2;
        if (i == 0) valid = valid && parent == 0 && name[0] == '/';
        else valid = valid && parent < i && !memchr(name, '/', n) &&
            !(n == 1 && name[0] == '.') && !(n == 2 && !memcmp(name, "..", 2));
        tree_t* ancestor = i && parent < i ? g_ptr_array_index(nodes, parent) : NULL;
        if (i && (!ancestor || !ancestor->is_directory || ancestor->depth >= 1024)) valid = FALSE;
        if (!valid) { g_variant_unref(name_value); goto invalid; }
        tree_t* node = g_new0(tree_t, 1);
        node->snapshot_index = i;
        node->name = g_strndup(name, n);
        node->size = size;
        node->is_directory = kind == 1;
        node->is_symlink = kind == 2;
        node->parent = ancestor;
        node->depth = ancestor ? ancestor->depth + 1 : 0;
        if (ancestor) ancestor->entries = g_list_prepend(ancestor->entries, node);
        else root = node;
        g_ptr_array_add(nodes, node);
        g_variant_unref(name_value);
    }
    for (gsize i = 0; i < nodes->len; i++) {
        tree_t* node = g_ptr_array_index(nodes, i);
        node->entries = g_list_reverse(node->entries);
        if (node->is_directory) {
            gint64 sum = 0;
            for (GList* entry = node->entries; entry; entry = entry->next) {
                tree_t* child = entry->data;
                if (child->size > G_MAXINT64 - sum) goto invalid;
                sum += child->size;
            }
            if (sum != node->size) goto invalid;
        }
    }
    g_ptr_array_free(nodes, TRUE);
    g_variant_unref(records);
    g_variant_unref(value);
    return root;
invalid:
    g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_INVAL, "Invalid or unsupported GdMap snapshot");
    tree_destroy(root);
    g_ptr_array_free(nodes, TRUE);
    if (records) g_variant_unref(records);
    g_variant_unref(value);
    return NULL;
}
