/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#include <gtk/gtk.h>
#include "l_i18n.h"
#include "preferences.h"

static GtkWidget* PrefWin = NULL;
static RedrawFunc RedrawCallback = NULL;

static unsigned DisplayMode = DISPLAY_STANDARD_CUSHION;
static double HValue = 0.5;
static double FValue = 0.8;

void pref_set_redraw_callback(RedrawFunc func) {
  RedrawCallback = func;
}

unsigned pref_get_display_mode(void) {
  return DisplayMode;
}
double pref_get_h(void) {
  return HValue;
}
double pref_get_f(void) {
  return FValue;
}


static void pref_set_display_mode(unsigned mode) {
  DisplayMode = mode;
  if (RedrawCallback) RedrawCallback();
}
static void pref_set_h(double value) {
  HValue = value;
  if (RedrawCallback) RedrawCallback();
}
static void pref_set_f(double value) {
  FValue = value;
  if (RedrawCallback) RedrawCallback();
}

static GtkWidget* gui_create_section(const char* header, GtkWidget** hbox) {
  GtkWidget* box;
  GtkWidget* label;
  char* temp;
  
  box = gtk_vbox_new(FALSE, 5);
  
  temp = g_strdup_printf("<b>%s</b>", header);
  label = gtk_label_new(temp);
  g_free(temp);
  gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

  *hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), *hbox, TRUE, TRUE, 0);
  label = gtk_label_new("");
  gtk_widget_set_size_request(label, 20, -1);
  gtk_box_pack_start(GTK_BOX(*hbox), label, FALSE, FALSE, 0);

  return box;
}

static void on_display_mode(GtkToggleButton* button, void* data) {
  unsigned mode = GPOINTER_TO_INT(data);
  if (gtk_toggle_button_get_active(button))
    pref_set_display_mode(mode);
}

static void on_h_changed(GtkSpinButton* button) {
  pref_set_h(gtk_spin_button_get_value(button));
}
static void on_f_changed(GtkSpinButton* button) {
  pref_set_f(gtk_spin_button_get_value(button));
}

void gui_show_preferences(void) {
  GtkWidget* win;
  GtkWidget* section;
  GtkWidget* table;
  GtkWidget* hbox;
  GtkWidget* vbox;
  GtkWidget* vbox0;
  GtkWidget* item;
  GtkWidget* item2;
  GSList* Group = NULL;

  if (PrefWin) {
    gtk_window_present(GTK_WINDOW(PrefWin));
    return;
  }

  PrefWin = win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), _("Preferences"));
  gtk_window_set_role(GTK_WINDOW(win), "Preferences");
/*   gtk_window_set_default_icon(create_pixbuf("gdmap_icon.png")); */
/*   gtk_window_set_default_size(GTK_WINDOW(win), 700, 450); */

  g_object_add_weak_pointer(G_OBJECT(PrefWin), (void*)&PrefWin);
  
  vbox0 = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width(GTK_CONTAINER(vbox0), 12);
  gtk_container_add(GTK_CONTAINER(win), vbox0);

  section = gui_create_section(_("Display type"), &hbox);
  gtk_box_pack_start(GTK_BOX(vbox0), section, FALSE, FALSE, 0);
  
  vbox = gtk_vbox_new(FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

  item = gtk_radio_button_new_with_label(Group, _("Standard cushioned treemaps"));
  Group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(item));
  gtk_box_pack_start(GTK_BOX(vbox), item, FALSE, FALSE, 0);

  item2 = gtk_radio_button_new_with_label(Group, _("Squarified cushioned treemaps"));
  Group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(item2));
  gtk_box_pack_start(GTK_BOX(vbox), item2, FALSE, FALSE, 0);

  if (DisplayMode == DISPLAY_STANDARD_CUSHION)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item2), TRUE);
  
  g_signal_connect(G_OBJECT(item), "toggled",
                   G_CALLBACK(on_display_mode), GINT_TO_POINTER(DISPLAY_STANDARD_CUSHION));
  g_signal_connect(G_OBJECT(item2), "toggled",
                   G_CALLBACK(on_display_mode), GINT_TO_POINTER(DISPLAY_SQUARE_CUSHION));

  section = gui_create_section(_("Cushion settings"), &hbox);
  gtk_box_pack_start(GTK_BOX(vbox0), section, FALSE, FALSE, 0);
  
  table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);
  gtk_table_set_row_spacings(GTK_TABLE(table), 2);

  item = gtk_label_new(_("Height factor:"));
  gtk_misc_set_alignment(GTK_MISC(item), 1, 0.5);
/*   gtk_label_set_use_markup(GTK_LABEL(item), TRUE); */
  gtk_table_attach(GTK_TABLE(table), item, 0, 1, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);

  item = gtk_spin_button_new_with_range(0.0, 1.0, 0.1);
  gtk_table_attach(GTK_TABLE(table), item, 1, 2, 0, 1,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(item), HValue);
  g_signal_connect(G_OBJECT(item), "value-changed",
                   G_CALLBACK(on_h_changed), NULL);
  
  item = gtk_label_new(_("Decreasing height:"));
  gtk_misc_set_alignment(GTK_MISC(item), 1, 0.5);
/*   gtk_label_set_use_markup(GTK_LABEL(item), TRUE); */
  gtk_table_attach(GTK_TABLE(table), item, 0, 1, 1, 2,
                   GTK_FILL, GTK_FILL, 0, 0);

  item = gtk_spin_button_new_with_range(0.0, 1.0, 0.1);
  gtk_table_attach(GTK_TABLE(table), item, 1, 2, 1, 2,
                   GTK_FILL, GTK_FILL, 0, 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(item), FValue);
  g_signal_connect(G_OBJECT(item), "value-changed",
                   G_CALLBACK(on_f_changed), NULL);

  
  
  // buttons
  item = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox0), item, FALSE, FALSE, 0);

  hbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing(GTK_BOX(hbox), 10);
  gtk_box_pack_start(GTK_BOX(vbox0), hbox, FALSE, FALSE, 0);

  item = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_container_add(GTK_CONTAINER(hbox), item);
  g_signal_connect_swapped(G_OBJECT(item), "clicked",
                           G_CALLBACK(gtk_widget_destroy), win);

  gtk_widget_show_all(win);
}
