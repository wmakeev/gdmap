/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <glib.h>

#define DIR_SEP G_DIR_SEPARATOR
#define FF "%"G_GINT64_FORMAT  // format string for filesizes

char* arg(char* data, int last);
char* print_filesize(gint64 bytes);
int create_dir(const char* dir);
char* to_utf8(const char* str);

#endif
