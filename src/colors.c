#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "support.h"
#include "tree.h"
#include "global.h"
#include "colors.h"

void color_save(GList* colors, char* name) {
  char filename[2048];
  color_t* color;
  GList* dlist;
  FILE* file;
  
  sprintf(filename, "%s/colors/%s", global.gdmap_home, name);

  file = fopen(filename, "w");
  if (file == NULL) {
    printf("Could not save colors!\n");
    return;
  }

  for (dlist = colors; dlist; dlist = dlist->next) {
    color = (color_t*)(dlist->data);
    fprintf(file, "%s %s\n", color->suffix, color->color);
  }
  fclose(file);
}

int color_load(char* name) {
  char filename[2048];
  char temp[2048];
  GtkWidget* area;
  FILE* file;
  char* suffix;
  char* c;
  color_t* color;
  GdkColor gcolor;
  int def = 0;

  sprintf(filename, "%s/colors/%s", global.gdmap_home, name);

  file = fopen(filename, "r");
  if (file == NULL) {
    printf("Could not load colors!\n");
    return 0;
  }

  area = lookup_widget(global.win, "drawingarea1");
  while (fgets(temp, 1024, file)) {
    if (strlen(temp) <3) continue;
    if (temp[0] == '#') continue;
    temp[strlen(temp)-1] = 0;
    if (!area->window) {
      printf("no window\n");
      continue;
    }
    suffix = arg(temp, 0);
    c = arg(NULL, 0);
    if (!suffix || ! c) continue;
    if (!strcasecmp(suffix, "Default")) def = 1;
    
    color = (color_t*)malloc(sizeof(color_t));
    color->suffix = strdup(suffix);
    color->color = strdup(c);
    color->gc = gdk_gc_new(area->window);
    gdk_color_parse(c, &gcolor);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    if (!strcasecmp(suffix, "color1") ||
	!strcasecmp(suffix, "color2")) {
      gdk_gc_set_function(color->gc, GDK_XOR);
    }
    global.colors = g_list_append(global.colors, color);
  }

  fclose(file);

  if (!def) {
    color = (color_t*)malloc(sizeof(color_t));
    color->suffix = strdup("Default");
    color->color = strdup("#aaaaaa");
    color->gc = gdk_gc_new(area->window);
    gdk_color_parse(color->color, &gcolor);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap (), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    global.colors = g_list_append(global.colors, color);
  }

  return 1;
}

#define N_COL  25

void color_create_default() {
  GtkWidget* area;
  color_t* color;
  GdkColor gcolor;
  int i1;
  static char dcolors[N_COL][2][20] = {
    {"DEFAULT", "#AAAAAA"},
    {"COLOR1", "#FFFF00"},
    {"COLOR2", "#0FFFFF"},
    {"DIR", "#666600"},
    {"ASF", "#EE0000"},
    {"MPG", "#dd0000"},
    {"MPEG", "#cc0000"},
    {"AVI", "#BB0000"},
    {"MP3", "#00DDDD"},
    {"WAV", "#00FFFF"},
    {"JPG", "#EEEE00"},
    {"JPEG", "#EEEE00"},
    {"GIF", "#DDDD00"},
    {"H", "#00ff00"},
    {"HH", "#00ff00"},
    {"C", "#00aa00"},
    {"CC", "#00aa00"},
    {"CPP", "#00aa00"},
    {"bz", "#0000AA"},
    {"ZIP", "#0000AA"},
    {"GZ", "#0000AA"},
    {"TGZ", "#0000CC"},
    {"O", "#BB00BB"},
    {"EXE", "#FF00FF"},
    {"DLL", "#DD00DD"}
  };

  area = lookup_widget(global.win, "drawingarea1");

  for (i1 = 0; i1 < N_COL; i1++) {
    color = (color_t*)malloc(sizeof(color_t));
    color->suffix = strdup(dcolors[i1][0]);
    color->color = strdup(dcolors[i1][1]);
    color->gc = gdk_gc_new(area->window);
    gdk_colormap_alloc_color(gdk_rgb_get_cmap(), &gcolor, FALSE, FALSE);
    gdk_gc_set_foreground(color->gc, &gcolor);
    if (!strcasecmp(dcolors[i1][0], "color1") ||
	!strcasecmp(dcolors[i1][0], "color2")) {
      gdk_gc_set_function(color->gc, GDK_XOR);
    }
    global.colors = g_list_append(global.colors, color);
  }
}

color_t* color_search(char* filename) {
  GList* dlist;
  color_t* color;
  char* suffix;
  color_t* temp = NULL;

  suffix = strrchr(filename, '.');
  if (suffix) suffix++;

  for (dlist = global.colors; dlist; dlist = dlist->next) {
    color = (color_t*)(dlist->data);
    if (suffix)
      if (!strcasecmp(color->suffix, suffix)) return color;
    if (!strcasecmp(color->suffix, "Default")) temp = color;
  }

  return temp;
}
