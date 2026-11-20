/* GdMap fork: GPL-2.0-only. */
#include "map_cli.h"
#include "snapshot.h"
#include "treemap.h"
#include "colors.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static char* digest(const char* path, GError** error)
{
    GMappedFile* file = g_mapped_file_new(path, FALSE, error);
    if (!file) return NULL;
    char* hash = g_compute_checksum_for_data(G_CHECKSUM_SHA256,
        (const guchar*)g_mapped_file_get_contents(file), g_mapped_file_get_length(file));
    g_mapped_file_unref(file);
    return hash;
}

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

static tree_t* select_id(tree_t* node, guint id)
{
    if(node->snapshot_index == id) return node;
    for(GList* p=node->entries; p; p=p->next) {
        tree_t* found=select_id(p->data,id);
        if(found) return found;
    }
    return NULL;
}

static void print_node(const char* label, tree_t* node)
{
    char* path = tree_full_name(node);
    char* escaped = g_strescape(path, NULL);
    printf("%s\t%s\t%" G_GINT64_FORMAT "\t%s\tnode=%u\n", label,
        node->is_directory ? "directory" : node->is_symlink ? "symlink" : "file", node->size, escaped, node->snapshot_index);
    g_free(path); g_free(escaped);
}

static void hit_point(tree_info_t* info, double x, double y, int width, int height,
                      int preview_width, int preview_height, unsigned depth, gboolean parent)
{
    if(preview_width) {x=x*width/preview_width; y=y*height/preview_height;}
    tree_info_t* found=(x>=0 && y>=0 && x<width && y<height) ? tree_info_search(info,(int)floor(x),(int)floor(y),depth) : NULL;
    printf("# original-coordinate %.6f %.6f; original-size %d %d\n",x,y,width,height);
    if(found) {
        if(parent && found->parent) found=found->parent;
        print_node("hit",found->node);
        if(found->parent) print_node("parent",found->parent->node);
    } else puts("miss");
}

int map_cli_main(int argc, char** argv)
{
    gint width=1200, height=800, depth=0, id=-1, preview_width=0, preview_height=0;
    double x=-1, y=-1, lighting=1;
    char *path=NULL, *palette=NULL, *mode=NULL;
    gboolean parent=FALSE, batch=FALSE;
    GOptionEntry entries[] = {
        {"width",0,0,G_OPTION_ARG_INT,&width,"PNG width", "PX"},
        {"height",0,0,G_OPTION_ARG_INT,&height,"PNG height", "PX"},
        {"depth",0,0,G_OPTION_ARG_INT,&depth,"Maximum depth relative to rendered root; 0 unlimited", "N"},
        {"node",0,0,G_OPTION_ARG_INT,&id,"Snapshot node to render", "N"},
        {"path",0,0,G_OPTION_ARG_FILENAME,&path,"Original absolute path to render", "PATH"},
        {"palette",0,0,G_OPTION_ARG_FILENAME,&palette,"Explicit GdMap colors.xml; default built-in palette", "XML"},
        {"mode",0,0,G_OPTION_ARG_STRING,&mode,"square (default) or standard", "MODE"},
        {"lighting",0,0,G_OPTION_ARG_DOUBLE,&lighting,"Cushion lighting 0 (flat) to 1 (original)", "FRACTION"},
        {"x",0,0,G_OPTION_ARG_DOUBLE,&x,"Hit horizontal coordinate", "PX"},
        {"y",0,0,G_OPTION_ARG_DOUBLE,&y,"Hit vertical coordinate", "PX"},
        {"preview-width",0,0,G_OPTION_ARG_INT,&preview_width,"Displayed preview width for hit", "PX"},
        {"preview-height",0,0,G_OPTION_ARG_INT,&preview_height,"Displayed preview height for hit", "PX"},
        {"batch",0,0,G_OPTION_ARG_NONE,&batch,"Read X Y or parent X Y lines from stdin after one preparation", NULL},
        {"parent",0,0,G_OPTION_ARG_NONE,&parent,"Return hit node's parent (within rendered root)", NULL},
        {NULL,0,0,0,NULL,NULL,NULL}
    };
    GError* error=NULL;
    GOptionContext* context=g_option_context_new("render SNAPSHOT PNG | hit PNG.frame --x X --y Y");
    g_option_context_add_main_entries(context,entries,NULL);
    gboolean parsed=g_option_context_parse(context,&argc,&argv,&error);
    g_option_context_free(context);
    int result=1;
    tree_t* root=NULL;
    tree_info_t* info=NULL;
    GKeyFile* frame=g_key_file_new();
    char *snapshot=NULL, *frame_path=NULL, *hash=NULL;
    cairo_surface_t* surface=NULL;
    gboolean hit=argc>1 && !strcmp(argv[1],"hit");
    if (!parsed) goto failed;
    if ((!hit && (argc!=4 || strcmp(argv[1],"render"))) || (hit && argc!=3)) {
        g_set_error_literal(&error,G_OPTION_ERROR,G_OPTION_ERROR_BAD_VALUE,"Use render SNAPSHOT PNG or hit PNG.frame --x X --y Y"); goto failed;
    }
    if (hit) {
        if (!g_key_file_load_from_file(frame,argv[2],G_KEY_FILE_NONE,&error)) goto failed;
        gint version=g_key_file_get_integer(frame,"frame","version",&error);
        if(error) goto failed;
        if(version!=1) {g_set_error_literal(&error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"Unsupported frame version"); goto failed;}
        snapshot=g_key_file_get_string(frame,"frame","snapshot",&error);
        if (!snapshot) goto failed;
        hash=digest(snapshot,&error); if (!hash) goto failed;
        char* saved=g_key_file_get_string(frame,"frame","sha256",&error);
        if (!saved) goto failed;
        gboolean same=!strcmp(hash,saved); g_free(saved);
        if (!same) {g_set_error_literal(&error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"Snapshot changed since render"); goto failed;}
#define GET_INT(key, dest) dest=g_key_file_get_integer(frame,"frame",key,&error); if(error) goto failed
        GET_INT("width",width); GET_INT("height",height); GET_INT("depth",depth); GET_INT("node",id);
        g_free(mode); mode=g_key_file_get_string(frame,"frame","mode",&error); if(error) goto failed;
    } else {
        snapshot=g_canonicalize_filename(argv[2],NULL);
        hash=digest(snapshot,&error); if (!hash) goto failed;
    }
    if (width<3 || height<3 || width>32767 || height>32767 || (guint64)width*height>100000000 || depth<0 || id < -1 ||
        (path && id>=0) || !isfinite(lighting) || lighting<0 || lighting>1 ||
        (mode && strcmp(mode,"square") && strcmp(mode,"standard")) ||
        (batch && !hit) ||
        (hit && ((!batch && (!isfinite(x) || !isfinite(y) || x<0 || y<0)) || preview_width<0 || preview_height<0 || (!!preview_width != !!preview_height)))) {
        g_set_error_literal(&error,G_OPTION_ERROR,G_OPTION_ERROR_BAD_VALUE,"Invalid frame or coordinate options (dimensions 3..32767, at most 100 million pixels)"); goto failed;
    }
    tree_scan_options scan;
    root=snapshot_load(snapshot,&scan,&error); if(!root) goto failed;
    tree_t* selected=path ? find_path(root,path) : id>=0 ? select_id(root,id) : root;
    if(!selected) {g_set_error_literal(&error,G_OPTION_ERROR,G_OPTION_ERROR_BAD_VALUE,"Selected node not found"); goto failed;}
    info=tree_info_create(selected);
    treemap_options options={TRUE,TRUE,depth ? selected->depth+depth : 0, mode && !strcmp(mode,"standard") ? 0 : 1, .5,.8,lighting};
    treemap_layout(info,width,height,&options);
    if(hit) {
        if(batch) {
            printf("ready %d %d\n",width,height); fflush(stdout);
            char line[256];
            while(fgets(line,sizeof(line),stdin)) {
                if(!strcmp(line,"quit\n") || !strcmp(line,"quit")) break;
                gboolean line_parent=g_str_has_prefix(line,"parent ") || parent;
                const char* coordinates=g_str_has_prefix(line,"parent ") ? line+7 : line;
                char extra;
                if(sscanf(coordinates,"%lf %lf %c",&x,&y,&extra)!=2 || !isfinite(x) || !isfinite(y))
                    puts("error expected finite X Y or parent X Y");
                else hit_point(info,x,y,width,height,preview_width,preview_height,options.max_depth,line_parent);
                puts("end"); fflush(stdout);
            }
        } else hit_point(info,x,y,width,height,preview_width,preview_height,options.max_depth,parent);
    } else {
        if(!colors_init_headless(palette)) {g_set_error_literal(&error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"Unable to load explicit palette"); goto failed;}
        surface=cairo_image_surface_create(CAIRO_FORMAT_RGB24,width,height);
        if(cairo_surface_status(surface)!=CAIRO_STATUS_SUCCESS) {g_set_error_literal(&error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"Unable to allocate image"); goto failed;}
        treemap_paint(info,surface,&options); cairo_surface_mark_dirty(surface);
        cairo_status_t status=cairo_surface_write_to_png(surface,argv[3]);
        if(status!=CAIRO_STATUS_SUCCESS) {g_set_error(&error,G_FILE_ERROR,G_FILE_ERROR_FAILED,"PNG: %s",cairo_status_to_string(status)); goto failed;}
        g_key_file_set_integer(frame,"frame","version",1);
        g_key_file_set_string(frame,"frame","snapshot",snapshot);
        g_key_file_set_string(frame,"frame","sha256",hash);
        g_key_file_set_integer(frame,"frame","width",width); g_key_file_set_integer(frame,"frame","height",height);
        g_key_file_set_integer(frame,"frame","depth",depth);
        /* Resolve paths to a stable preorder number, without storing every rectangle. */
        g_key_file_set_integer(frame,"frame","node",selected->snapshot_index);
        g_key_file_set_string(frame,"frame","mode",mode ? mode : "square");
        g_key_file_set_double(frame,"frame","lighting",lighting);
        g_key_file_set_string(frame,"frame","coordinates","top-left origin; pixels; half-open rectangles; 1px outer border; scale x by original-width/preview-width and y likewise");
        guint number=0;
        for(GList* p=colors_get_list();p;p=p->next) {
            color_t* c=p->data;
            if(c->type==COLOR_MARK1 || c->type==COLOR_MARK2) continue;
            GString* legend=g_string_new(NULL);
            g_string_append_printf(legend,"#%02x%02x%02x ",c->c.red/257,c->c.green/257,c->c.blue/257);
            if(c->type==COLOR_EXTENSION) for(GList* e=c->ext;e;e=e->next) g_string_append_printf(legend,"%s%s",e==c->ext?"":" ",(char*)e->data);
            else g_string_append(legend,c->type==COLOR_FOLDER ? "folder" : "default / unlisted extensions");
            char* key=g_strdup_printf("color-%u",number++);
            g_key_file_set_string(frame,"legend",key,legend->str); g_free(key); g_string_free(legend,TRUE);
        }
        frame_path=g_strconcat(argv[3],".frame",NULL);
        gsize length; char* data=g_key_file_to_data(frame,&length,NULL);
        gboolean written=g_file_set_contents(frame_path,data,length,&error); g_free(data); if(!written) goto failed;
        printf("PNG %d %d\nframe %s\n",width,height,frame_path);
        print_node("root",selected);
    }
    result=0;
failed:
    if(error) {g_printerr("%s\n",error->message); g_error_free(error);}
    if(surface) cairo_surface_destroy(surface);
    if(info) tree_info_destroy(info);
    tree_destroy(root); g_key_file_free(frame);
    g_free(snapshot); g_free(frame_path); g_free(hash); g_free(path); g_free(palette); g_free(mode);
    return result;
}
