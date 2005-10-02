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
#include "global.h"
#include "colors.h"
#include "gui_main.h"

static GList* Colors = NULL;

static void color_save(char* name) {
  char* filename = g_strdup_printf("%s/.gdmap/%s", g_get_home_dir(), name);
  FILE* file = fopen(filename, "w");
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
  g_free(filename);
}

static int color_load(char* name) {
  char filename[2048];
  char temp[2048];
  FILE* file;
  char* suffix;
  char* c;
  color_t* color;
  GdkColor gcolor;
  int def = 0;
  GtkWidget* win = gui_get_main_win();

  sprintf(filename, "%s/.gdmap/%s", g_get_home_dir(), name);

  file = fopen(filename, "r");
  if (file == NULL) {
    printf("Could not load colors!\n");
    return 0;
  }

  while (fgets(temp, 1024, file)) {
    if (strlen(temp) <3) continue;
    if (temp[0] == '#') continue;
    temp[strlen(temp)-1] = 0;
    if (!win || !win->window) {
      printf("no window\n");
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

#define N_COL  25

static void color_create_default() {
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
  if (!color_load("default.colors")) {
    color_create_default();
    color_save("default.colors");
  }
}
