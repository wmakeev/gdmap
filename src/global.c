#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "tree.h"
#include "global.h"
#include "colors.h"

global_t global;

char* size_format(gint64 size) {
  static char str[2048];

  if (size < 0x400) sprintf(str, "%lld B", size);
  else if (size < 0x10000) sprintf(str, "%.1f kB", (double)size/(double)0x400);
  else if (size < 0x100000) sprintf(str, "%lld kB", size/0x400);
  else if (size < 0x4000000) sprintf(str, "%.1f MB", (double)size/(double)0x100000);
  else if (size < 0x40000000) sprintf(str, "%lld MB", size/0x100000);
  else if (size < 0x1000000000) sprintf(str, "%.1f GB", (double)size/(double)0x40000000);
  else sprintf(str, "%lld GB", size/0x40000000);
  return str;
}

int directory_exists(char* dir) {
  struct stat st;

  if (stat(dir, &st) < 0) return 0; 
  else return 1;
}

void create_dir(char* dir) {
  char* pos;
  char* slash;

  if (!dir) return;

  slash = dir;
  while ((pos = strrchr(slash+1, '/')) != NULL) {
    pos[0] = 0;
    if (!directory_exists(dir)) {
      mkdir(dir, 7*64+5*8+5);
    }
    pos[0] = '/';
    slash = pos;
  }
  if (!directory_exists(dir)) {
    mkdir(dir, 7*64+5*8+5);
  }
}

char* arg(char* data, int last) {
  static char* string;
  char* pos;
  char* pos2;

  if (data) string = data;
  if (!string) return NULL;
  
  if (!last) while (isspace(*string)) string++;

  if (last) {
    pos = string;
    string = NULL;
    if (*pos == 0) return NULL;
    else return pos;
  }
  if (*string == '\"') {
    pos = ++string;
    pos2 = strchr(string, '\"');
    if (pos2) {
      *pos2 = 0;
      string = pos2+1;
      if (*string == ' ') string++;
      return pos;
    } else {
      string = NULL;
      return NULL;
    }
  } else {
    pos = string;
    pos2 = strchr(string, ' ');
    if (pos2) {
      *pos2 = 0;
      string = pos2+1;
    } else {
      string = NULL;
    }
    if (*pos == 0) return NULL;
    else return pos;
  }

  return NULL;
}

void setup_widgets() {
}

void global_init() {
  char str[2048];

  global.current_tree = NULL;
  global.trees = NULL;
  global.mark = NULL;
  global.expose_timer = -1;
  global.paint_mode = P_SIMPLE;
  global.max_depth = 0;

  sprintf(str, "%s/.gdmap", getenv("HOME"));
  global.gdmap_home = strdup(str);
  create_dir(str);

  sprintf(str, "%s/colors", global.gdmap_home);
  create_dir(str);

  sprintf(str, "Graphical Disk Map %s", VERSION);
  gtk_window_set_title (GTK_WINDOW(global.win), str);

  if (!color_load("default")) {
    color_create_default();
    color_save(global.colors, "default");
  }
  
  setup_widgets();
}

void global_exit() {
  gtk_exit(0);
}

int expose_timeout_cb(gpointer data) {
  if (global.expose_timer >= 0) {
    gtk_timeout_remove(global.expose_timer);
    global.expose_timer = -1;
  }
  dir_tree_display(global.current_tree);

  return 0;
}
 
