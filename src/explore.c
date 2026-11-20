/* GdMap fork: GPL-2.0-only. */
#include "explore.h"
#include "map_cli.h"
#include "colors.h"
#include "overview.h"
#include "snapshot.h"
#include <glib/gstdio.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    tree_t* root;
    GArray* index;
    int limit, width, height;
    treemap_options options;
    char* directory;
    char* png;
    char* legend;
    tree_info_t* layout;
} Explorer;

static const char commands[] =
    "overview | expand NODE | map NODE | hit X Y | help | quit";

static void clear_map(Explorer* session)
{
    if (session->layout) tree_info_destroy(session->layout);
    if (session->png) g_unlink(session->png);
    g_clear_pointer(&session->png, g_free);
    session->layout = NULL;
}

static void show_map(Explorer* session, tree_t* node, GError** error)
{
    if (!session->directory) {
        session->directory = g_dir_make_tmp("gdmap-explore-XXXXXX", error);
        if (!session->directory) return;
    }
    if (!session->layout || session->layout->node != node) {
        tree_info_t* layout = tree_info_create(node);
        treemap_layout(layout, session->width, session->height, &session->options);
        char* name = g_strdup_printf("node-%u.png", node->snapshot_index);
        char* png = g_build_filename(session->directory, name, NULL);
        g_free(name);
        if (!map_write_png(png, layout, &session->options, session->width, session->height, error)) {
            g_unlink(png);
            g_free(png);
            tree_info_destroy(layout);
            return;
        }
        clear_map(session);
        session->layout = layout;
        session->png = png;
    }
    printf("PNG %d %d\nimage %s\n", session->width, session->height, session->png);
    map_print_node("root", node);
    if (node->parent) map_print_node("parent", node->parent);
    fputs(session->legend, stdout);
}

static gboolean coordinate(const char* value, double* result)
{
    char* end;
    *result = g_ascii_strtod(value, &end);
    return end != value && !*end && isfinite(*result);
}

static void run_command(Explorer* session, int argc, char** argv)
{
    GError* error = NULL;
    const char* cmd = argv[0];
    if (argc == 1 && !strcmp(cmd, "overview")) {
        snapshot_overview(session->root, FALSE, NULL, -1, session->limit, &error);
    } else if (argc == 2 && (!strcmp(cmd, "expand") || !strcmp(cmd, "map"))) {
        tree_t* node = snapshot_index_lookup(session->index, argv[1]);
        if (!node) puts("error unknown node number");
        else if (!strcmp(cmd, "expand")) snapshot_index_expand(session->index, node, session->limit);
        else show_map(session, node, &error);
    } else if (argc == 3 && !strcmp(cmd, "hit")) {
        double x, y;
        if (!session->layout) puts("error open a map with map NODE first");
        else if (!coordinate(argv[1], &x) || !coordinate(argv[2], &y)) puts("error expected finite X Y");
        else map_hit_point(session->layout, x, y, session->width, session->height, 0, 0, 0, FALSE);
    } else if (argc == 1 && !strcmp(cmd, "help")) {
        puts(commands);
    } else {
        printf("error expected %s\n", commands);
    }
    if (error) {
        printf("error %s\n", error->message);
        g_error_free(error);
    }
}

static void explore_loop(Explorer* session)
{
    char line[256];
    puts("ready");
    puts(commands);
    char* initial[] = {"overview"};
    run_command(session, 1, initial);
    puts("end");
    fflush(stdout);
    while (fgets(line, sizeof(line), stdin)) {
        if (!strchr(line, '\n') && !feof(stdin)) {
            int ch;
            do { ch = getchar(); } while (ch != '\n' && ch != EOF);
            puts("error command too long");
        } else {
            g_strstrip(line);
            if (!strcmp(line, "quit")) break;
            int argc;
            char** argv;
            if (g_shell_parse_argv(line, &argc, &argv, NULL)) {
                run_command(session, argc, argv);
                g_strfreev(argv);
            } else puts("error expected a command (try help)");
        }
        puts("end");
        fflush(stdout);
    }
}

int explore_main(int argc, char** argv)
{
    Explorer session = {
        .limit = 10, .width = 1536, .height = 1024,
        .options = {.use_colors = TRUE, .use_average = TRUE, .mode = 1, .h = .5, .f = .8, .lighting = .55}
    };
    char* palette = NULL;
    GOptionEntry entries[] = {
        {"limit", 0, 0, G_OPTION_ARG_INT, &session.limit, "Maximum rows per section", "N"},
        {"width", 0, 0, G_OPTION_ARG_INT, &session.width, "Map width in pixels", "PX"},
        {"height", 0, 0, G_OPTION_ARG_INT, &session.height, "Map height in pixels", "PX"},
        {"lighting", 0, 0, G_OPTION_ARG_DOUBLE, &session.options.lighting, "Cushion lighting from 0 to 1", "FRACTION"},
        {"palette", 0, 0, G_OPTION_ARG_FILENAME, &palette, "Explicit GdMap colors.xml", "XML"},
        {NULL}
    };
    GOptionContext* context = g_option_context_new("explore SNAPSHOT");
    g_option_context_set_summary(context, commands);
    g_option_context_add_main_entries(context, entries, NULL);
    GError* error = NULL;
    gboolean parsed = g_option_context_parse(context, &argc, &argv, &error);
    g_option_context_free(context);
    int status = 1;
    if (!parsed || argc != 3 || strcmp(argv[1], "explore") || session.limit < 1 ||
        !map_size_valid(session.width, session.height) || !isfinite(session.options.lighting) ||
        session.options.lighting < 0 || session.options.lighting > 1) {
        g_printerr("%s\n", error ? error->message : "Invalid arguments; see gdmap-cli explore --help");
        status = 2;
        goto done;
    }
    if (!colors_init_headless(palette)) {
        g_printerr("Unable to load explicit palette\n");
        goto done;
    }
    tree_scan_options scan;
    session.root = snapshot_load(argv[2], &scan, &error);
    if (!session.root) {
        g_printerr("%s\n", error->message);
        goto done;
    }
    session.index = snapshot_index_new(session.root);
    GKeyFile* legend = g_key_file_new();
    map_add_legend(legend);
    session.legend = g_key_file_to_data(legend, NULL, NULL);
    g_key_file_free(legend);
    explore_loop(&session);
    clear_map(&session);
    if (session.directory) g_rmdir(session.directory);
    g_free(session.directory);
    g_free(session.legend);
    g_array_free(session.index, TRUE);
    tree_destroy(session.root);
    status = ferror(stdin) || ferror(stdout) ? 1 : 0;
done:
    g_clear_error(&error);
    g_free(palette);
    return status;
}
