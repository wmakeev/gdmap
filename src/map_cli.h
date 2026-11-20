#ifndef GDMAP_MAP_CLI_H
#define GDMAP_MAP_CLI_H
#include "treemap.h"
gboolean map_size_valid(int width, int height);
gboolean map_write_png(const char* filename, tree_info_t* info, const treemap_options* options,
                       int width, int height, GError** error);
void map_add_legend(GKeyFile* frame);
void map_print_node(const char* label, tree_t* node);
void map_hit_point(tree_info_t* info, double x, double y, int width, int height,
                   int preview_width, int preview_height, unsigned depth, gboolean parent);
int map_cli_main(int argc, char** argv);
#endif
