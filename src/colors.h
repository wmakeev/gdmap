/* Copyright (C) 2003 sgop@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details. */

#ifndef _COLORS_H_
#define _COLORS_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

typedef struct _color_t color_t;

struct _color_t {
  char* suffix;
  char* color;
  GdkGC* gc;
};

void color_save(char* name);
int color_load(char* name);
void color_create_default();
color_t* color_search(char* filename);

#endif
