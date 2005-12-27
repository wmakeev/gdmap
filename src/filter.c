/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#include "filter.h"

static GList* FilterName = NULL;
static file_size_t FilterMin = 0;
static file_size_t FilterMax = 0;

void filters_init() {
  //FIXME: load filter.
}

void filter_clear() {
  //FIXME:
}

static void filters_save() {
  //FIXME:
}

void filter_add_by_name(const char* item) {
  if (item && *item) 
    FilterName = g_list_append(FilterName, g_strdup(item));
  filters_save();
}

void filter_add_by_size(file_size_t min, file_size_t max) {
  FilterMin = min;
  FilterMax = max;
  filters_save();
}
