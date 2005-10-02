/* Copyright (C) 2005 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

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

/* static int directory_exists(char* dir) { */
/*   struct stat st; */

/*   if (stat(dir, &st) < 0) return 0;  */
/*   else return 1; */
/* } */

/* static void create_dir(char* dir) { */
/*   char* pos; */
/*   char* slash; */

/*   if (!dir) return; */

/*   slash = dir; */
/*   while ((pos = strrchr(slash+1, '/')) != NULL) { */
/*     pos[0] = 0; */
/*     if (!directory_exists(dir)) { */
/*       mkdir(dir, 7*64+5*8+5); */
/*     } */
/*     pos[0] = '/'; */
/*     slash = pos; */
/*   } */
/*   if (!directory_exists(dir)) { */
/*     mkdir(dir, 7*64+5*8+5); */
/*   } */
/* } */

/* void global_init() { */
/*   char* home = g_strdup_printf("%s/.gdmap", g_get_home_dir()); */
/*   create_dir(home); */
/*   g_free(home); */

/*   colors_init(); */
/* } */

/* void global_exit() { */
/*   gtk_main_quit(); */
/* } */

/* char* size_format(gint64 size) { */
/*   static char str[2048]; */

/*   if (size < 0x400) sprintf(str, FF" B", size); */
/*   else if (size < 0x10000) sprintf(str, "%.1f kB", (double)size/(double)0x400); */
/*   else if (size < 0x100000) sprintf(str, FF" kB", size/0x400); */
/*   else if (size < 0x4000000) sprintf(str, "%.1f MB", (double)size/(double)0x100000); */
/*   else if (size < 0x40000000) sprintf(str, FF" MB", size/0x100000); */
/*   else if (size < 0x1000000000) sprintf(str, "%.1f GB", (double)size/(double)0x40000000); */
/*   else sprintf(str, FF" GB", size/0x40000000); */
/*   return str; */
/* } */

/* char* per_format(gint64 size1, gint64 size2) { */
/*   static char str[2048]; */
/*   sprintf(str, "%.2f %%", (double)size1/(double)size2*100); */
/*   return str; */
/* } */

/* char* to_utf8(const char* str) { */
/*   static char* result = NULL; */
  
/*   if (result) g_free(result); */
/*   result = g_locale_to_utf8(str, -1, NULL, NULL, NULL); */
/*   return result; */
/* } */
