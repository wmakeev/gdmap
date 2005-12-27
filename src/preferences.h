/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifndef _GUI_PREFERENCES_H_
#define _GUI_PREFERENCES_H_

#define DISPLAY_STANDARD_CUSHION    0
#define DISPLAY_SQUARE_CUSHION      1

typedef void (*RedrawFunc)(void);

void pref_set_redraw_callback(RedrawFunc func);
void pref_init();

unsigned pref_get_display_mode(void);
double pref_get_h(void);
double pref_get_f(void);
unsigned pref_get_max_depth(void);
gboolean pref_get_use_colors(void);
gboolean pref_get_use_average(void);

void gui_show_preferences(void);


#endif
