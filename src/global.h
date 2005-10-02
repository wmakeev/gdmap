#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define P_SIMPLE   0
#define P_3D       1
#define P_GRADIENT 2
#define P_CUSHION  3
#define P_SCUSHION 4

#define BORDER   1

#include <glib.h>

#include "tree.h"

#define FF "%"G_GINT64_FORMAT  // format string for filesizes

void global_init();
void global_exit();
char* arg(char* data, int last);
char* size_format(gint64 size);
char* per_format(gint64 size1, gint64 size2);


#endif
