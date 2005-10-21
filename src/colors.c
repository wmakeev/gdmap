/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "tree.h"
#include "utils.h"
#include "colors.h"
#include "gui_main.h"

static GList* Colors = NULL;

static void color_save(const char* filename) {
  FILE* file;

  create_dir(filename);

  file = fopen(filename, "w");
  if (file) {
    GList* dlist;
    for (dlist = Colors; dlist; dlist = dlist->next) {
      color_t* color = dlist->data;
      fprintf(file, "%s %s\n", color->suffix, color->color);
    }
    fclose(file);
  } else {
    g_log("COLOR", G_LOG_LEVEL_WARNING,
          "Could not save colors to '%s'", filename);
  }
}

static gboolean color_load(const char* filename) {
  char temp[2048];
  FILE* file;
  char* suffix;
  char* c;
  color_t* color;
  GdkColor gcolor;
  int def = 0;
  GtkWidget* win = gui_get_main_win();

  file = fopen(filename, "r");
  if (file == NULL) return FALSE;

  while (fgets(temp, 1024, file)) {
    if (strlen(temp) <3) continue;
    if (temp[0] == '#') continue;
    temp[strlen(temp)-1] = 0;
    if (!win || !win->window) {
      g_warning("no window\n");
      continue;
    }
    suffix = arg(temp, 0);
    c = arg(NULL, 0);
    if (!suffix || ! c) continue;
    if (!strcasecmp(suffix, "Default")) def = 1;
    
    color = g_malloc(sizeof(color_t));
    color->suffix = g_strdup(suffix);
    color->color = g_strdup(c);
    color->gc = gdk_gc_new(win->window);
    gdk_color_parse(c, &gcolor);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    if (!strcasecmp(suffix, "color1") ||
        !strcasecmp(suffix, "color2")) {
      gdk_gc_set_function(color->gc, GDK_XOR);
    }
    Colors = g_list_append(Colors, color);
  }

  fclose(file);

  if (!def) {
    color = g_malloc(sizeof(color_t));
    color->suffix = g_strdup("Default");
    color->color = g_strdup("#aaaaaa");
    color->gc = gdk_gc_new(win->window);
    gdk_color_parse(color->color, &gcolor);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    Colors = g_list_append(Colors, color);
  }

  return 1;
}

#define N_COL  60

static void color_create_default() {
  color_t* color;
  GdkColor gcolor;
  int i1;
  static char dcolors[N_COL][2][20] = {
    {"DEFAULT","#CCCCCC"},              // Gray
    {"COLOR1", "#FFFF00"},              // Yellow
    {"COLOR2", "#0FFFFF"},              // Cyan
    {"DIR",    "#224400"},              // Dark Green
    {"ASF",    "#DD0000"},              // Red        
    {"AVI",    "#DD0000"},              // Red        
    {"MPG",    "#DD0000"},              // Red        
    {"MPEG",   "#DD0000"},              // Red        
    {"WMV",    "#DD0000"},              // Red        
    {"EXE",    "#FF00FF"},              // Magenta    
    {"DLL",    "#BB00BB"},              // Magenta    
    {"O",      "#BB00BB"},              // Magenta    
    {"LO",     "#BB00BB"},              // Magenta    
    {"A",      "#BB00BB"},              // Magenta    
    {"SO",     "#BB00BB"},              // Magenta    
    {"C",      "#00aa00"},              // Green      
    {"CC",     "#00aa00"},              // Green      
    {"CPP",    "#00aa00"},              // Green      
    {"H",      "#00ff00"},              // Light Green
    {"HH",     "#00ff00"},              // Light Green
    {"EPS",    "#EEEE00"},              // Yellow     
    {"BMP",    "#EEEE00"},              // Yellow     
    {"SVG",    "#EEEE00"},              // Yellow     
    {"GIF",    "#EEEE00"},              // Yellow     
    {"PNG",    "#EEEE00"},              // Yellow
    {"JPG",    "#EEEE00"},              // Yellow     
    {"JPEG",   "#EEEE00"},              // Yellow     
    {"TIF",    "#EEEE00"},              // Yellow     
    {"TIFF",   "#EEEE00"},              // Yellow     
    {"MP3",    "#00DDDD"},              // Cyan       
    {"OGG",    "#00DDDD"},              // Cyan       
    {"FLAC",   "#00DDDD"},              // Cyan       
    {"WAV",    "#00DDDD"},              // Cyan       
    {"ODP",    "#d8def1"},              // Power Blue 
    {"ODS",    "#d8def1"},              // Power Blue 
    {"ODT",    "#d8def1"},              // Power Blue 
    {"OTP",    "#d8def1"},              // Power Blue 
    {"OTS",    "#d8def1"},              // Power Blue 
    {"OTT",    "#d8def1"},              // Power Blue 
    {"STC",    "#d8def1"},              // Power Blue 
    {"STW",    "#d8def1"},              // Power Blue 
    {"sxc",    "#d8def1"},              // Power Blue 
    {"SXC",    "#d8def1"},              // Power Blue 
    {"SXI",    "#d8def1"},              // Power Blue 
    {"SXW",    "#d8def1"},              // Power Blue 
    {"MDB",    "#66aaff"},              // 
    {"DOC",    "#66aaff"},              // 
    {"PDF",    "#66aaff"},              // 
    {"PS",     "#66aaff"},              // 
    {"PPT",    "#66aaff"},              // 
    {"TXT",    "#66aaff"},              // 
    {"PO",     "#66aaff"},              // 
    {"XLS",    "#66aaff"},              // 
    {"XML",    "#66aaff"},              // 
    {"HTML",   "#66aaff"},              // 
    {"HTM",    "#66aaff"},              // 
    {"TGZ",    "#0000AA"},              // Dark Blue  
    {"bz2",    "#0000AA"},              // Dark Blue  
    {"GZ",     "#0000AA"},              // Dark Blue  
    {"ZIP",    "#0000AA"}               // Dark Blue  
  };
  GtkWidget* win = gui_get_main_win();

  for (i1 = 0; i1 < N_COL; i1++) {
    color = g_malloc(sizeof(color_t));
    color->suffix = g_strdup(dcolors[i1][0]);
    color->color = g_strdup(dcolors[i1][1]);
    color->gc = gdk_gc_new(win->window);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap(), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    if (!strcasecmp(dcolors[i1][0], "color1") ||
        !strcasecmp(dcolors[i1][0], "color2")) {
      gdk_gc_set_function(color->gc, GDK_XOR);
    }
    Colors = g_list_append(Colors, color);
  }
}

color_t* color_search(char* filename) {
  GList* dlist;
  color_t* color;
  char* suffix;
  color_t* temp = NULL;

  suffix = strrchr(filename, '.');
  if (suffix) suffix++;

  for (dlist = Colors; dlist; dlist = dlist->next) {
    color = dlist->data;
    if (suffix) if (!strcasecmp(color->suffix, suffix)) return color;
    if (!strcasecmp(color->suffix, "Default")) temp = color;
  }

  return temp;
}

void colors_init() {
  const char* home = g_get_home_dir();
  char* filename = g_strdup_printf("%s/.gdmap/default.colors", home);

  if (!color_load(filename)) {
    color_create_default();
    color_save(filename);
  }
  g_free(filename);
}
