#ifndef GDMAP_TREEMAP_H
#define GDMAP_TREEMAP_H
#include <gtk/gtk.h>
#include "tree.h"
typedef struct _tree_info_t tree_info_t;

struct _tree_info_t
{
    tree_t* node;
    double s[2][2];
    int geo[2][2];
    GList* children;
    tree_info_t* parent;
};

typedef struct {
    gboolean use_colors, use_average;
    unsigned max_depth, mode;
    double h, f, lighting;
} treemap_options;
tree_info_t* tree_info_create(tree_t* tree);
void tree_info_destroy(tree_info_t* info);
tree_info_t* tree_info_search(tree_info_t* info, int x, int y, unsigned depth);
tree_info_t* treemap_layout(tree_info_t* info, int width, int height, const treemap_options* options);
void treemap_paint(tree_info_t* info, cairo_surface_t* surface, const treemap_options* options);
#endif
