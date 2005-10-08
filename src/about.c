/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include "l_i18n.h"

static GtkAboutDialog* AboutWin = NULL;

static GdkPixbuf* create_pixbuf(const gchar *filename) {
  char* temp;
  GdkPixbuf* pix;

#ifdef PACKAGE_SOURCE_DIR
  temp = g_strdup_printf("%s/data/%s", PACKAGE_SOURCE_DIR, filename);
#else
  temp = g_strdup_printf("%s/pixmaps/%s", PACKAGE_DATA_DIR, filename);
#endif
  pix = gdk_pixbuf_new_from_file(temp, NULL);
  g_free(temp);

  return pix;
}

void gui_show_about(void) {
  GtkWidget* win;
  const char* authors[] = {
    "Markus Lausser <sgop@users.sourceforge.net>",
    NULL
  };

  if (AboutWin) {
    gtk_window_present(GTK_WINDOW(AboutWin));
    return;
  }
  win = gtk_about_dialog_new();
  AboutWin = GTK_ABOUT_DIALOG(win);
  gtk_about_dialog_set_name(AboutWin, PACKAGE_NAME);
  gtk_about_dialog_set_version(AboutWin, PACKAGE_VERSION);
  gtk_about_dialog_set_copyright(AboutWin, "Copyright \302\251 2005 Markus Lausser");
  gtk_about_dialog_set_license(AboutWin, "GPL (GNU General Public License)\n\nsee http://www.gnu.org/licenses/gpl.html");
  gtk_about_dialog_set_website(AboutWin, "http://gdmap.sourceforge.net");
  gtk_about_dialog_set_authors(AboutWin, authors);
  gtk_about_dialog_set_comments(AboutWin, _("GdMap is a tool which allows to graphically explore your disks."));
  gtk_about_dialog_set_logo(AboutWin, create_pixbuf("gdmap_icon.png"));
  
  gtk_about_dialog_set_translator_credits(AboutWin, _("translator-credits"));

  g_object_add_weak_pointer(G_OBJECT(AboutWin), (void*)&AboutWin);

  gtk_widget_show(win);
}

