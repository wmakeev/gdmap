/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils.h"

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

char* print_filesize(gint64 bytes) {
  double size = bytes;
  if (size < 1024) return g_strdup_printf("%.0f B", size);
  size /= 1024.;
  if (size < 1024) return g_strdup_printf("%.2f KiB", size);
  if (size < 1024) return g_strdup_printf("%.1f KiB", size);
  size /= 1024.;
  if (size < 128) return g_strdup_printf("%.2f MiB", size);
  if (size < 1024) return g_strdup_printf("%.1f MiB", size);
  size /= 1024.;
  if (size < 128) return g_strdup_printf("%.2f GiB", size);
  if (size < 1024) return g_strdup_printf("%.1f GiB", size);
  size /= 1024.;
  if (size < 128) return g_strdup_printf("%.2f TiB", size);
  if (size < 1024) return g_strdup_printf("%.1f TiB", size);
  size /= 1024.;
  if (size < 128) return g_strdup_printf("%.2f PiB", size);
  if (size < 1024) return g_strdup_printf("%.1f PiB", size);
  size /= 1024.;
  return g_strdup_printf("%.2f EiB", size);
}

static int directory_exists(char* dir) {
  struct stat st;

  if (stat(dir, &st) < 0) return 0; 
  else return 1;
}

int create_dir(const char *dir) {
  char* ddir = g_strdup(dir);
  char *pos;
  char *slash;

  if (!ddir) return 0;
  pos = strrchr(ddir, DIR_SEP);
  if (!pos) return 0;
  *pos = 0;
  if (directory_exists(ddir)) {
    *pos = DIR_SEP;
    return 1;
  }
  *pos = DIR_SEP;
  
  slash = ddir;
  while ((pos = strchr(slash + 1, DIR_SEP)) != NULL) {
    *pos = 0;
    if (!directory_exists(ddir)) {
      if (mkdir(ddir, 7 * 64 + 5 * 8 + 5)) {
        g_warning("Could not create folder '%s'", ddir);
        *pos = DIR_SEP;
        return 0;
      }
    }
    *pos = DIR_SEP;
    slash = pos;
  }
  g_free(ddir);
  return 1;
}

char* to_utf8(const char* str) {
  static char* result = NULL;
  if (result) g_free(result);
  result = g_locale_to_utf8(str, -1, NULL, NULL, NULL);
  return result;
}
