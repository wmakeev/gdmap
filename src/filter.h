/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifndef _FILTER_H_
#define _FILTER_H_

void filters_init();

void filter_clear();
void filter_add_by_name(const char* item);
void filter_add_by_size(file_size_t min, file_size_t max);

#endif
