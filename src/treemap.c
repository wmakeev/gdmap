/* Shared GTK and headless treemap; derived from GdMap gui_main.c, GPL-2.0. */
#include "treemap.h"
#include "colors.h"
#include "preferences.h"
#include <math.h>
#include <string.h>
void tree_info_destroy(tree_info_t* info)
{
    g_list_free_full(info->children, (GDestroyNotify)tree_info_destroy);
    g_free(info);
}

tree_info_t* tree_info_search(tree_info_t* info, int x, int y, unsigned depth)
{
    GList* dlist;

    
    if (depth != 0 && depth < info->node->depth) return NULL;
    
    if (x < info->geo[0][0] || x >= info->geo[0][1] ||
        y < info->geo[1][0] || y >= info->geo[1][1])
        return NULL;

    for (dlist = info->children; dlist; dlist = dlist->next)
    {
        tree_info_t* sinfo = dlist->data;
        sinfo = tree_info_search(sinfo, x, y, depth);
        if (sinfo) return sinfo;
    }
    return info;
}

tree_info_t* tree_info_create(tree_t* tree)
{
    tree_info_t* info = g_malloc0(sizeof(*info));
    GList* dlist;
    
    info->node = tree;
    for (dlist = tree->entries; dlist; dlist = dlist->next)
    {
        tree_t* child = dlist->data;
        tree_info_t* sub = tree_info_create(child);
        sub->parent = info;
        // Prepend instead of appending here, so that the time complexity is
        // O(n) instead of O(n^2).
        // The order will be the wrong way around, but we can fix that with a
        // g_list_reverse() at the end.
        info->children = g_list_prepend(info->children, sub);
    }
    info->children = g_list_reverse(info->children);
    return info;
}

static void treemap_reset(tree_info_t* info) {
    memset(info->geo, 0, sizeof(info->geo));
    for (GList* p=info->children; p; p=p->next) treemap_reset(p->data);
}

static void _get_average_color_rec(tree_t* tree, int* r, int* g, int* b)
{

    if (tree->entries && tree->size > 0)
    {
        GList* dlist;
        double dr=0, dg=0, db=0;

        for (dlist = tree->entries; dlist; dlist = dlist->next)
        {
            tree_t* sub = dlist->data;
            int lr=0, lg=0, lb=0;
            _get_average_color_rec(sub, &lr, &lg, &lb);
            dr += lr/(double)tree->size*(double)sub->size;
            dg += lg/(double)tree->size*(double)sub->size;
            db += lb/(double)tree->size*(double)sub->size;
        }
        *r += dr;
        *g += dg;
        *b += db;
    }
    else
    {
        const GdkColor* color = color_get_by_file(tree->name);
        *r += color->red;
        *g += color->green;
        *b += color->blue;
    }
}

static const GdkColor* _get_average_color(tree_t* tree)
{
    static GdkColor res;
    int r=0, g=0, b=0;

    _get_average_color_rec(tree, &r, &g, &b);
    res.red = r;
    res.green = g;
    res.blue = b;
    return &res;
}

static void gui_tree_show_item(tree_info_t* info, cairo_surface_t* surface, const treemap_options* options)
{
    static int Ia = 40;
    static int Is = 215;
    static double Lx = 0.09759;
    static double Ly = 0.19518;
    static double Lz = 0.9759;
    int ix, iy;
    double nx, ny, cosa, val;
    long pos;
    const GdkColor* color;

    int x1 = info->geo[0][0];
    int y1 = info->geo[1][0];
    int x2 = info->geo[0][1];
    int y2 = info->geo[1][1];
  
    if (x1 >= x2) return;
    if (y1 >= y2) return;

    if (options->use_colors)
    {
        if (info->node->entries)
        {
            if (options->use_average) color = _get_average_color(info->node); 
            else color = color_get_by_type(COLOR_FOLDER);
        }
        else
        {
            color = color_get_by_file(info->node->name);
        }
    }
    else
    {
        color = color_get_by_type(COLOR_DEFAULT);
    }
  
    unsigned char* buffer = cairo_image_surface_get_data(surface);
    pos = (y1*cairo_image_surface_get_stride(surface)/4+x1)*4;
    for (iy = y1; iy < y2; iy++)
    {
        for (ix = x1; ix < x2; ix++)
        {
            nx = -(2*info->s[0][1]*(ix+0.5)+info->s[0][0]);
            ny = -(2*info->s[1][1]*(iy+0.5)+info->s[1][0]);
            cosa = (nx*Lx + ny*Ly + Lz)/sqrt(nx*nx + ny*ny +1.0);
            val = 255*(1-options->lighting) + options->lighting*(Ia+MAX(0, Is*cosa));
            buffer[pos++] = (unsigned char)(color->blue  * val/65535.);
            buffer[pos++] = (unsigned char)(color->green * val/65535.);
            buffer[pos++] = (unsigned char)(color->red   * val/65535.);
            pos++;
        }
        pos -= (x2-x1)*4;  // cr
        pos += cairo_image_surface_get_stride(surface);    // lf
    }
}

static void AddRidge(double x1, double x2, double h, double* s1, double* s2)
{
    if (x2 <= x1) return;
    *s1 += 4*h*(x2+x1)/(x2-x1);
    *s2 -= 4*h/(x2-x1);
}


/* void AddRidge(tree_info_t* node, int geo[2][2], int d, double h) { */
/*   int x1 = geo[d][0]; */
/*   int x2 = geo[d][1]; */

/*   node->s[d][0] += 4*h*(x2+x1)/(x2-x1); */
/*   node->s[d][1] -= 4*h/(x2-x1); */
/* } */

static void gui_tree_evaluate_childs1(tree_info_t* info, double h, double f)
{
    tree_t* tree = info->node;
    int d = (tree->depth % 2);
    int pos = info->geo[d][0];
    int width = (info->geo[d][1] - info->geo[d][0]);
    gint64 size = 0;
    GList* dlist;

/*   g_message("%d evaluate '%s' with %u children", tree->depth, tree->name, */
/*             g_list_length(tree->entries)); */
/*   printf("  s: [%.1f,%.1f][%.1f,%.1f]\n", */
/*          info->s[0][0],info->s[0][1],info->s[1][0],info->s[1][1]); */

    for (dlist = info->children; dlist; dlist = dlist->next)
    {
        tree_info_t* sinfo = dlist->data;
        tree_t* sub = sinfo->node;
        int end_pos;
    
        sinfo->geo[0][0] = sinfo->geo[0][1] = 0;
        sinfo->geo[1][0] = sinfo->geo[1][1] = 0;

        if (sub->size <= 0) continue;

        size += sub->size;
        end_pos = info->geo[d][0] + (long double)size*width/tree->size;

        if (end_pos > pos+1 || (!dlist->next && end_pos > pos))
        {
            sinfo->geo[d][0] = pos;
            sinfo->geo[d][1] = end_pos;
            sinfo->geo[1-d][0] = info->geo[1-d][0];
            sinfo->geo[1-d][1] = info->geo[1-d][1];
      
            memcpy(sinfo->s, info->s, sizeof(sinfo->s));
            AddRidge(sinfo->geo[d][0], sinfo->geo[d][1], h,
                     &sinfo->s[d][0], &sinfo->s[d][1]);

            pos = end_pos;
            if (sub->entries) gui_tree_evaluate_childs1(sinfo, h*f, f);
        }
    }

}

static gint64
_find_next_size(GList* children, gint64 rsize, int width, int height)
{
    GList* dlist;
    gint64 size = 0;
    double best_ratio = 0.0;
    gint64 best_size = 0;

    g_assert(width >= height);

/*   g_message("   have %ld in (%d,%d) (%u children)", rsize, width, height, */
/*             g_list_length(children)); */

    for (dlist = children; dlist; dlist = dlist->next)
    {
        tree_info_t* sinfo = dlist->data;
        tree_t* sub = sinfo->node;
        int dx, dy;
        double ratio;

        size += sub->size;
        if (size == 0) continue;

        // get the width of the col
        dx = (long double)width * size / rsize;
        // get the height of the last item.
        dy = (long double)height * sub->size / size;
        // calculate the aspect ratio
/*     g_message("   -size is %ld (%d %d)", sub->size, dx, dy); */
        if (dx == 0 && dy == 0) continue;

  
        if (dx < dy) ratio = (double)dx/(double)dy;
        else ratio = (double)dy/(double)dx;

/*     g_message("   -ratio (%d,%d) %.1f (size %ld)", dx, dy, ratio, size); */

        if (ratio >= best_ratio)
        {
            best_ratio = ratio;
            best_size = size;
        }
        else
        {
            break;
        }
    }
    return best_size;
}

static void gui_tree_evaluate_childs2(tree_info_t* info, double h, double f)
{
    tree_t* tree = info->node;
    GList* children = info->children;
    gint64 rsize, size;
    int d;
    int geo[2][2];
    double s[2][2];
    
    memcpy(geo, info->geo, sizeof(geo));
    rsize = tree->size;
    
    while (children)
    {
        int width, height;
        int dx, pos;
        gint64 ssize;
        
        if (geo[0][1]-geo[0][0] < geo[1][1]-geo[1][0]) d = 1;
        else d = 0;
        
        width = geo[d][1]-geo[d][0];
        height = geo[1-d][1]-geo[1-d][0];
        
        size = _find_next_size(children, rsize, width, height);
        
        if (rsize) dx = (long double)width * size / rsize;
        else dx = 0;
        
        ssize = 0;
        pos = geo[1-d][0];
        
        memcpy(s, info->s, sizeof(s));
        AddRidge(geo[d][0], geo[d][0]+dx, h, &s[d][0], &s[d][1]);
        
        for ( ; children; children = children->next)
        {
            tree_info_t* sinfo = children->data;
            tree_t* sub = sinfo->node;
            int end_pos;
            
            sinfo->geo[0][0] = sinfo->geo[0][1] = 0;
            sinfo->geo[1][0] = sinfo->geo[1][1] = 0;
            
            if (sub->size <= 0) continue;
            ssize += sub->size;
            if (ssize > size) break;
            
            end_pos = geo[1-d][0] + (long double)ssize*height/size;
            
            if (end_pos > pos+1 || (ssize == size && end_pos > pos))
            {
                sinfo->geo[1-d][0] = pos;
                sinfo->geo[1-d][1] = end_pos;
                sinfo->geo[d][0] = geo[d][0];
                sinfo->geo[d][1] = geo[d][0]+dx;
                
                memcpy(sinfo->s, s, sizeof(sinfo->s));
                AddRidge(sinfo->geo[1-d][0], sinfo->geo[1-d][1], h,
                         &sinfo->s[1-d][0], &sinfo->s[1-d][1]);
                
                pos = end_pos;
                if (sub->entries) gui_tree_evaluate_childs2(sinfo, h*f, f);
            }
        }
        geo[d][0] += dx;
        rsize -= size;
    }
}
  
  
tree_info_t* treemap_layout(tree_info_t* info, int width, int height, const treemap_options* options)
{
    int Border = 1;
    treemap_reset(info);
    
    info->geo[0][0] = Border;
    info->geo[0][1] = width-Border;
    info->geo[1][0] = Border;
    info->geo[1][1] = height-Border;
    info->s[0][0] = .0;
    info->s[0][1] = .0;
    info->s[1][0] = .0;
    info->s[1][1] = .0;
    
    if (options->mode == DISPLAY_STANDARD_CUSHION)
    {
        gui_tree_evaluate_childs1(info, options->h, options->f);
    }
    else
    {
        gui_tree_evaluate_childs2(info, options->h, options->f);
    }
  
    return info;
}

static void paint_nodes(tree_info_t* info, cairo_surface_t* surface, const treemap_options* options)
{
    unsigned depth = options->max_depth;
    
    if (info->geo[0][0] >= info->geo[0][1]) return;
    if (info->geo[1][0] >= info->geo[1][1]) return;
    
    if (info->children && (depth == 0 || info->node->depth < depth))
    {
        GList* dlist;
        for (dlist = info->children; dlist; dlist = dlist->next)
        {
            tree_info_t* sub = dlist->data;
            paint_nodes(sub, surface, options);
        }
    }
    else
    {
        gui_tree_show_item(info, surface, options);
    }
}


void treemap_paint(tree_info_t* info, cairo_surface_t* surface, const treemap_options* options)
{
    cairo_surface_flush(surface);
    memset(cairo_image_surface_get_data(surface), 0,
           (gsize)cairo_image_surface_get_stride(surface)*cairo_image_surface_get_height(surface));
    paint_nodes(info, surface, options);
    cairo_surface_mark_dirty(surface);
}
