/* GdMap fork: GPL-2.0-only. See LICENSES/GPL-2.0.txt. */
#include "overview.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    tree_t* node;
    guint id;
    guint64 files, links;
    gint64 largest;
} Info;

typedef struct {
    GArray* rows;
    guint count;
    gint64 bytes;
    gboolean disjoint;
} Top;

typedef struct {
    const char* name;
    Top members;
} Group;

typedef struct {
    guint limit;
    GArray* index;
    gint selected_id;
    tree_t* selected_node;
    Info selected;
    gboolean found, expand;
    Top files, branches, children;
    GHashTable *groups, *active;
    char* root_name;
} View;

static Top top_new(gboolean disjoint)
{
    Top top = {g_array_new(FALSE, FALSE, sizeof(Info)), 0, 0, disjoint};
    return top;
}

/* Keep only the requested rows. Ties retain snapshot preorder. */
static void top_add(Top* top, const Info* info, guint limit)
{
    ++top->count;
    if (top->disjoint) top->bytes += info->node->size;
    guint position = 0;
    while (position < top->rows->len) {
        Info* row = &g_array_index(top->rows, Info, position);
        if (info->node->size > row->node->size ||
            (info->node->size == row->node->size && info->id < row->id)) break;
        ++position;
    }
    if (position >= limit) return;
    g_array_insert_val(top->rows, position, *info);
    if (top->rows->len > limit) g_array_set_size(top->rows, limit);
}

static void free_group(gpointer value)
{
    Group* group = value;
    g_array_free(group->members.rows, TRUE);
    g_free(group);
}

static Info collect(tree_t* node, View* view)
{
    Info info = {node, node->snapshot_index, 0, 0, 0};
    if ((view->selected_id >= 0 && info.id == (guint)view->selected_id) ||
        node == view->selected_node) {
        view->selected_node = node;
        view->found = TRUE;
    }
    if (view->index && info.id >= view->index->len)
        g_array_set_size(view->index, info.id + 1);
    const char* basename = node->parent ? node->name : view->root_name;
    gboolean outer = !view->expand && node->is_directory && !g_hash_table_contains(view->active, basename);
    if (outer) g_hash_table_add(view->active, (gpointer)basename);
    if (!node->is_directory) {
        info.files = !node->is_symlink;
        info.links = node->is_symlink;
        if (info.files) info.largest = node->size;
    }
    for (GList* item = node->entries; item; item = item->next) {
        Info child = collect(item->data, view);
        info.files += child.files;
        info.links += child.links;
        info.largest = MAX(info.largest, child.largest);
    }
    if (view->index) {
        g_array_index(view->index, Info, info.id) = info;
        return info;
    }
    if (node == view->selected_node) view->selected = info;
    if (view->expand) {
        if (node->parent == view->selected_node) top_add(&view->children, &info, view->limit);
    } else {
        if (info.files && !node->is_directory && !node->is_symlink)
            top_add(&view->files, &info, view->limit);
        if (node->parent && node->is_directory) top_add(&view->branches, &info, view->limit);
        if (outer && node->parent) {
            Group* group = g_hash_table_lookup(view->groups, node->name);
            if (!group) {
                group = g_new0(Group, 1);
                group->name = node->name;
                group->members = top_new(TRUE);
                g_hash_table_insert(view->groups, node->name, group);
            }
            top_add(&group->members, &info, view->limit);
        }
    }
    if (outer) g_hash_table_remove(view->active, basename);
    return info;
}

static void print_header(void)
{
    puts("# kind\tnode\tbytes\tfiles\tsymlinks\tlargest-file-percent\tpath (C-escaped bytes; use --node to expand literally)");
}

static void print_info(const char* kind, const Info* info)
{
    char* path = tree_full_name(info->node);
    char* escaped = g_strescape(path, NULL);
    char share[G_ASCII_DTOSTR_BUF_SIZE];
    if (info->node->size)
        g_ascii_formatd(share, sizeof(share), "%.2f", 100.0 * info->largest / info->node->size);
    else
        g_strlcpy(share, "none", sizeof(share));
    printf("%s\t%u\t%" G_GINT64_FORMAT "\t%" G_GUINT64_FORMAT "\t%" G_GUINT64_FORMAT "\t%s\t%s\n",
        kind, info->id, info->node->size, info->files, info->links, share, escaped);
    g_free(escaped);
    g_free(path);
}

static void print_top(const char* title, const Top* top)
{
    printf("# %s\n", title);
    gint64 remaining = top->bytes;
    for (guint i = 0; i < top->rows->len; ++i) {
        const Info* info = &g_array_index(top->rows, Info, i);
        print_info(info->node->is_directory ? "directory" : info->node->is_symlink ? "symlink" : "file", info);
        if (top->disjoint) remaining -= info->node->size;
    }
    printf("# remaining-nodes=%u", top->count - top->rows->len);
    if (top->disjoint) printf(" remaining-bytes=%" G_GINT64_FORMAT, remaining);
    else printf(" (branches can overlap; no additive remainder)");
    putchar('\n');
}

/* Resolve raw absolute bytes by components, without constructing every path. */
static tree_t* find_path(tree_t* root, const char* path)
{
    if (!strcmp(root->name, path)) return root;
    gsize length = strlen(root->name);
    if (strncmp(root->name, path, length)) return NULL;
    if (length > 1) {
        if (path[length] != '/') return NULL;
        ++length;
    }
    tree_t* current = root;
    const char* component = path + length;
    while (*component) {
        const char* end = strchr(component, '/');
        gsize size = end ? (gsize)(end - component) : strlen(component);
        tree_t* match = NULL;
        for (GList* item = current->entries; item; item = item->next) {
            tree_t* child = item->data;
            if (strlen(child->name) == size && !memcmp(child->name, component, size)) {
                match = child;
                break;
            }
        }
        if (!match) return NULL;
        current = match;
        if (!end) return current;
        component = end + 1;
        if (!*component) return NULL;
    }
    return NULL;
}

static gboolean group_before(const Group* left, const Group* right)
{
    if (left->members.bytes != right->members.bytes) return left->members.bytes > right->members.bytes;
    return strcmp(left->name, right->name) < 0;
}

gboolean snapshot_overview(tree_t* root, gboolean expand, const char* path, gint node_id, guint limit, GError** error)
{
    View view = {0};
    view.limit = limit;
    view.expand = expand;
    view.selected_id = node_id;
    view.selected_node = path ? find_path(root, path) : node_id < 0 ? root : NULL;
    if (path && !view.selected_node) {
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Path does not exist in the snapshot");
        return FALSE;
    }
    view.root_name = g_path_get_basename(root->name);
    view.files = top_new(TRUE);
    view.branches = top_new(FALSE);
    view.children = top_new(TRUE);
    view.groups = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free_group);
    view.active = g_hash_table_new(g_str_hash, g_str_equal);
    collect(root, &view);
    if (!view.found) {
        g_set_error_literal(error, G_FILE_ERROR, G_FILE_ERROR_NOENT, "Node does not exist in the snapshot");
    } else {
        print_header();
        print_info("root", &view.selected);
        if (expand) {
            print_top("children", &view.children);
        } else {
            print_top("largest files", &view.files);
            print_top("largest branches", &view.branches);
            GPtrArray* top_groups = g_ptr_array_new();
            guint count = 0;
            GHashTableIter iterator;
            gpointer value;
            g_hash_table_iter_init(&iterator, view.groups);
            while (g_hash_table_iter_next(&iterator, NULL, &value)) {
                Group* group = value;
                if (group->members.count < 2) continue;
                ++count;
                guint position = 0;
                while (position < top_groups->len && !group_before(group, g_ptr_array_index(top_groups, position))) ++position;
                if (position >= limit) continue;
                g_ptr_array_add(top_groups, NULL);
                memmove(top_groups->pdata + position + 1, top_groups->pdata + position,
                    (top_groups->len - position - 1) * sizeof(gpointer));
                top_groups->pdata[position] = group;
                if (top_groups->len > limit) g_ptr_array_set_size(top_groups, limit);
            }
            puts("# repeated directory groups: bytes\toccurrences\tname (C-escaped bytes)");
            for (guint i = 0; i < top_groups->len; ++i) {
                Group* group = g_ptr_array_index(top_groups, i);
                char* escaped = g_strescape(group->name, NULL);
                printf("group\t%" G_GINT64_FORMAT "\t%u\t%s\n", group->members.bytes, group->members.count, escaped);
                g_free(escaped);
                print_top("group members", &group->members);
            }
            printf("# remaining-groups=%u (groups can overlap; no additive remainder)\n", count - top_groups->len);
            g_ptr_array_free(top_groups, TRUE);
        }
    }
    g_hash_table_destroy(view.active);
    g_free(view.root_name);
    g_hash_table_destroy(view.groups);
    g_array_free(view.files.rows, TRUE);
    g_array_free(view.branches.rows, TRUE);
    g_array_free(view.children.rows, TRUE);
    return view.found;
}

/* Keep the same preorder IDs and subtree totals for the lifetime of the session. */
void snapshot_expand_batch(tree_t* root, guint limit)
{
    View view = {0};
    view.expand = TRUE;
    view.selected_id = -1;
    view.index = g_array_new(FALSE, FALSE, sizeof(Info));
    collect(root, &view);
    puts("ready");
    fflush(stdout);
    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        if (!strchr(line, '\n') && !feof(stdin)) {
            int ch;
            do { ch = getchar(); } while (ch != '\n' && ch != EOF);
            line[0] = '\0';
        }
        g_strstrip(line);
        if (!strcmp(line, "quit")) break;
        char* end;
        guint64 id = g_ascii_strtoull(line, &end, 10);
        if (!g_ascii_isdigit(line[0]) || *end || id >= view.index->len) {
            puts("error expected a node number from the snapshot");
        } else {
            const Info* selected = &g_array_index(view.index, Info, id);
            Top children = top_new(TRUE);
            for (GList* item = selected->node->entries; item; item = item->next) {
                tree_t* child = item->data;
                const Info* child_info = &g_array_index(view.index, Info, child->snapshot_index);
                top_add(&children, child_info, limit);
            }
            print_header();
            print_info("root", selected);
            print_top("children", &children);
            g_array_free(children.rows, TRUE);
        }
        puts("end");
        fflush(stdout);
    }
    g_array_free(view.index, TRUE);
}
