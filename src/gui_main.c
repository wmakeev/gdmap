/* Copyright (C) 2005 sgop@users.sourceforge.net This is free software
 * distributed under the terms of the GNU Public License.  See the
 * file COPYING for details.
 * Modified for the GdMap fork on 2026-11-19.
 */
/* $Revision$
 * $Date$
 * $Author$
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include <cairo.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "gui_main.h"
#include "l_i18n.h"
#include "tree.h"
#include "colors.h"
#include "utils.h"
#include "preferences.h"
#include "about.h"


#include "treemap.h"
typedef struct
{
    GtkProgressBar* bar;
    GtkButton* abort;
    gboolean cancel;
} progress_data_t;

static GtkWidget* MainWin = NULL;

static cairo_surface_t* Surface = NULL;
static int Width = 0;
static int Height = 0;
static tree_info_t* Mark1 = NULL;
static tree_info_t* Mark2 = NULL;
static int RedrawTimer = 0;

static GtkWidget* Area = NULL;
static tree_info_t* CurrentItem = NULL;

static GtkLabel* FileLabel = NULL;
static GtkLabel* FileSizeLabel = NULL;
static GtkLabel* SubLabel = NULL;
static GtkLabel* SubSizeLabel = NULL;
static GtkLabel* SizeLabel = NULL;
static GtkBox* PathBox = NULL;
static GtkBox* StatusBar = NULL;

static GList* History = NULL;
static GList* HistoryPos = NULL;
static gboolean Loading = FALSE;

static GtkUIManager* UIManager = NULL;
static GtkActionGroup* ActionGroup = NULL;

static gboolean LUseColors = TRUE;
static gboolean LUseAverage = TRUE;
static unsigned LMaxDepth = 0;
static unsigned LDisplayMode = DISPLAY_SQUARE_CUSHION;

static void on_action_open();
static void on_action_scan_root(void);
static void on_action_back();
static void on_action_forward();
static void on_action_up();
static void on_action_top();
static void on_action_exit();
static void on_action_preferences();
static void on_action_about();
static void on_action_refresh();

static GtkActionEntry Actions[] =
{
   
    { "FileMenu", NULL, N_("_File"), 0,0,0 },
    { "ViewMenu", NULL, N_("_View"), 0,0,0 },
    { "HelpMenu", NULL, N_("_Help"), 0,0,0 },
    { "Open", "folder-open", N_("_Open..."),
      "<control>O", N_("Open a folder"),
      G_CALLBACK(on_action_open)
    },
    { "ScanRoot", "drive-harddisk", N_("_Scan Root (/)"),
      NULL, N_("Scan the filesystem root (/)"),
      G_CALLBACK(on_action_scan_root)
    },
    { "Quit", "application-exit", N_("_Quit"),
      "<control>Q", N_("Exit program"),
      G_CALLBACK(on_action_exit)
    },
    { "Back", "go-previous", N_("_Back"),
      NULL, N_("Go back one step"),
      G_CALLBACK(on_action_back)
    },
    { "Forward", "go-next", N_("_Forward"),
      NULL, N_("Go forward one step"),
      G_CALLBACK(on_action_forward)
    },
    { "Up", "go-up", N_("_Up"),
      NULL, N_("Go up one level"),
      G_CALLBACK(on_action_up)
    },
    { "Top", "go-top", N_("_Top"),
      NULL, N_("Goto toplevel folder"),
      G_CALLBACK(on_action_top)
    },
    { "Options", "preferences-system", N_("_Settings..."),
      NULL, N_("Edit settings"),
      G_CALLBACK(on_action_preferences)
    },
    { "About", "help-about", N_("_About..."),
      NULL, N_("About this program"),
      G_CALLBACK(on_action_about)
    },
    { "Refresh", "view-refresh", N_("_Refresh"),
      NULL, N_("Refresh current view"),
      G_CALLBACK(on_action_refresh)
    },
};

static const char* MenuDescription =
    "<ui>\n"
    "  <menubar action=\"MainMenu\">\n"
    "    <menu action=\"FileMenu\">\n"
    "      <menuitem action=\"Open\"/>\n"
    "      <menuitem action=\"ScanRoot\"/>\n"
    "      <separator/>\n"
    "      <menuitem action=\"Options\"/>\n"
    "      <separator/>\n"
    "      <menuitem action=\"Quit\"/>\n"
    "    </menu>\n"
    "    <menu action=\"ViewMenu\">\n"
    "      <menuitem action=\"Back\"/>\n"
    "      <menuitem action=\"Forward\"/>\n"
    "      <menuitem action=\"Up\"/>\n"
    "      <menuitem action=\"Top\"/>\n"
    "      <separator/>\n"
    "      <menuitem action=\"Refresh\"/>\n"
    "    </menu>\n"
    "    <menu action=\"HelpMenu\">\n"
    "      <menuitem action=\"About\"/>\n"
    "    </menu>\n"
    "  </menubar>\n"
    "  <toolbar action=\"ToolBar\">\n"
    "    <toolitem action=\"Open\"/>\n"
    "    <toolitem action=\"ScanRoot\"/>\n"
    "    <separator/>\n"
    "    <toolitem action=\"Back\"/>\n"
    "    <toolitem action=\"Forward\"/>\n"
    "    <toolitem action=\"Up\"/>\n"
    "    <toolitem action=\"Top\"/>\n"
    "    <separator/>\n"
    "    <toolitem action=\"Refresh\"/>\n"
    "    <separator/>\n"
    "    <toolitem action=\"Options\"/>\n"
    "  </toolbar>\n"
    "</ui>\n";

static void gui_tree_display(tree_info_t* info, gboolean destroy_old, gboolean new_history);
static tree_t* gui_tree_load(const char* folder, unsigned depth);
static void gui_buttons_update(void);


// HISTORY
static void history_clear(void)
{
    g_list_free(History);
    History = NULL;
    HistoryPos = NULL;
}

static void history_back(void)
{
    if (!HistoryPos) return;
    if (!HistoryPos->prev) return;
    
    HistoryPos = HistoryPos->prev;
    
    gui_tree_display(HistoryPos->data, FALSE, FALSE);
}

static void history_forward(void)
{
    if (!History) return;
    if (HistoryPos)
    {
        if (!HistoryPos->next) return;
        HistoryPos = HistoryPos->next;
    }
    else
    {
        HistoryPos = History;
    }
    gui_tree_display(HistoryPos->data, FALSE, FALSE);
}

static void history_append(tree_info_t* info)
{
    if (HistoryPos)
    {
        while (HistoryPos->next)
            History = g_list_delete_link(History, HistoryPos->next);
    }
    History = g_list_append(History, info);
    HistoryPos = g_list_last(History);
}



// TREE INFO
static tree_info_t* tree_info_find_path(tree_info_t* root, tree_info_t* child)
{
    tree_info_t* temp;
    
    for (temp = child; temp; temp = temp->parent)
    {
        if (temp->parent == root) return temp;
    }
    return NULL;
}


static void on_action_exit(void)
{
    gtk_main_quit();
}

static void on_action_preferences(void)
{
    gui_show_preferences(GTK_WINDOW(MainWin));
}

static void on_action_about(void)
{
    gui_show_about(GTK_WINDOW(MainWin));
}

static gint comp_func(gconstpointer p1, gconstpointer p2)
{
    const tree_t* t1 = (tree_t*)p1;
    const tree_t* t2 = (tree_t*)p2;
  
    if (t1->size < t2->size) return 1;
    if (t1->size > t2->size) return -1;
    return 0;
}

static gint comp_func2(gconstpointer p1, gconstpointer p2)
{
    const tree_info_t* t1 = (tree_info_t*)p1;
    const tree_info_t* t2 = (tree_info_t*)p2;
  
    if (t1->node->size < t2->node->size) return 1;
    if (t1->node->size > t2->node->size) return -1;
    return 0;
}

static void gui_refresh_current(void)
{
    char* folder;
    tree_t* new_tree;

    if (!CurrentItem) return;
    folder = tree_full_name(CurrentItem->node);
    new_tree = gui_tree_load(folder, CurrentItem->node->depth);
    if (new_tree)
    {
        tree_t* tree = CurrentItem->node;
        tree_t* parent = tree->parent;
        tree_info_t* info = tree_info_create(new_tree);
        char* t;

        t = new_tree->name;
        new_tree->name = tree->name;
        tree->name = t;

        if (parent)
        {
            tree_t* temp;

            // remove tree from parent
            parent->entries = g_list_remove(parent->entries, tree);
            CurrentItem->parent->children =
                g_list_remove(CurrentItem->parent->children, CurrentItem);

            // adjust sizes
            for (temp = parent; temp; temp = temp->parent)
            {
                temp->size -= tree->size;
                temp->size += new_tree->size;
            }
            // insert tree in parent
            parent->entries = g_list_insert_sorted(parent->entries, new_tree, comp_func);
            new_tree->parent = parent;

            CurrentItem->parent->children =
                g_list_insert_sorted(CurrentItem->parent->children, info, comp_func2);
            info->parent = CurrentItem->parent;
        }
        tree_destroy(tree);
        tree_info_destroy(CurrentItem);
        CurrentItem = NULL;
        gui_tree_display(info, TRUE, TRUE);
    }
    g_free(folder);
}

static void on_action_refresh(void)
{
    gui_refresh_current();
}

static gboolean _update_progress(void* data)
{
    progress_data_t* pdata = data;

    if (pdata->cancel) return FALSE;

    gtk_progress_bar_pulse(pdata->bar);
    while (gtk_events_pending()) if (gtk_main_iteration()) return FALSE;

    return !pdata->cancel;
}

static void on_abort_load(progress_data_t* data)
{
    data->cancel = TRUE;
}

static tree_t* gui_tree_load(const char* folder, unsigned depth)
{
    GtkWidget* item1;
    GtkWidget* item2;
    tree_t* tree;
    progress_data_t* data = g_malloc0(sizeof(*data));

    data->cancel = FALSE;
    Loading = TRUE;

    gui_buttons_update();
    gtk_widget_set_sensitive(Area, FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(PathBox), FALSE);

    item1 = gtk_progress_bar_new();
    data->bar = GTK_PROGRESS_BAR(item1);
    gtk_widget_show(item1);
    gtk_box_pack_start(StatusBar, item1, FALSE, FALSE, 0);
    gtk_progress_bar_set_pulse_step(data->bar, 0.05);
    gtk_progress_bar_set_text(data->bar, _("scanning..."));

    item2 = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    data->abort = GTK_BUTTON(item2);
    gtk_widget_show(item2);
    gtk_box_pack_start(StatusBar, item2, FALSE, FALSE, 0);
    g_signal_connect_swapped(G_OBJECT(item2), "clicked",
                             G_CALLBACK(on_abort_load), data);

    tree = tree_load(folder, _update_progress, data, depth);
  
    gtk_widget_destroy(item1);
    gtk_widget_destroy(item2);
    g_free(data);

    Loading = FALSE;
    gtk_widget_set_sensitive(Area, TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(PathBox), TRUE);
    gui_buttons_update();

    return tree;
}

void gui_tree_load_and_display(const char* folder)
{
    tree_t* tree = gui_tree_load(folder, 0);
    if (tree)
    {
        tree_info_t* info = tree_info_create(tree);
        gui_tree_display(info, TRUE, TRUE);
    }
}


static void on_action_open(void)
{
    GtkWidget* dialog;
    char* res;

    dialog = gtk_file_chooser_dialog_new
        (_("Choose folder"), GTK_WINDOW(MainWin),
         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        res = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
    else
    {
        res = NULL;
    }
    gtk_widget_destroy(dialog);

    if (res)
    {
        gui_tree_load_and_display(res);
        g_free(res);
    }
}

static void on_action_scan_root(void)
{
    gui_tree_load_and_display(G_DIR_SEPARATOR_S);
}

static void gui_buttons_update(void)
{
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/Open"), !Loading);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/ScanRoot"), !Loading);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/Back"),
                             !Loading && HistoryPos && HistoryPos->prev);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/Forward"),
                             !Loading && HistoryPos && HistoryPos->next);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/Up"),
                             !Loading && CurrentItem && CurrentItem->parent);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/Top"),
                             !Loading && CurrentItem && CurrentItem->parent);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(UIManager, "/ToolBar/Refresh"),
                             CurrentItem != NULL && !Loading);
}

static void on_action_top(void)
{
    tree_info_t* info = CurrentItem;

    if (!info || !info->parent) return;
    while (info->parent) info = info->parent;
    gui_tree_display(info, FALSE, TRUE);
}

static void on_action_up(void)
{
    tree_info_t* info = CurrentItem;

    if (!info || !info->parent) return;
    gui_tree_display(info->parent, FALSE, TRUE);
}

static void on_action_back(void)
{
    history_back();
}

static void on_action_forward(void)
{
    history_forward();
}



/* static void on_depth_change(GtkSpinButton* button) { */
/*   int val = gtk_spin_button_get_value_as_int(button); */
/*   if (val != MaxDepth) { */
/*     MaxDepth = val; */
/*     gui_tree_display(CurrentTree); */
/*   } */
/* } */

static gboolean on_pathbox_scroll(GtkWidget* scroll, GdkEventScroll event, G_GNUC_UNUSED gpointer user_data) {
    if (event.state || event.direction == GDK_SCROLL_LEFT || event.direction == GDK_SCROLL_RIGHT || event.delta_x)
        return FALSE;
    GtkAdjustment* adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll));
    // See https://gitlab.gnome.org/GNOME/gtk/blob/gtk-3-24/gtk/gtkscrolledwindow.c
#ifdef GDK_WINDOWING_QUARTZ
    gdouble unit = 1;
#else
    gdouble page = gtk_adjustment_get_page_size(adj);
    gdouble unit = fmin(page / 2, pow(page, 2.0 / 3));
#endif
    gdouble value = gtk_adjustment_get_value(adj);
    double delta_y;
    switch (event.direction) {
        case GDK_SCROLL_SMOOTH:
            delta_y = event.delta_y * unit;
            break;
        case GDK_SCROLL_UP:
            delta_y = -unit;
            break;
        case GDK_SCROLL_DOWN:
            delta_y = unit;
            break;
        default: // Unreachable
            delta_y = 0;
    }
    value += delta_y;
    gdouble lower = gtk_adjustment_get_lower(adj);
    gdouble upper = gtk_adjustment_get_upper(adj);
    value = fmin(fmax(value, lower), upper);
    gtk_adjustment_set_value(adj, value);
    return TRUE;
}

static GtkWidget* gui_create_path_box(void)
{
    GtkWidget* scroll;
    GtkWidget* hbox;
    GtkWidget* hbox2;
    GtkWidget* label;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_show(hbox);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scroll);
    gtk_container_add(GTK_CONTAINER(hbox), scroll);
    gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scroll), TRUE);
    // We use "external" (scrollable but the scrollbar is never shown) because
    // otherwise the widget would expand vertically to make room for the
    // scrollbar
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_widget_set_hexpand(scroll, TRUE);
    g_signal_connect(scroll, "scroll-event", G_CALLBACK(on_pathbox_scroll), NULL);

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    PathBox = GTK_BOX(hbox2);
    gtk_widget_show(hbox2);
    gtk_container_add(GTK_CONTAINER(scroll), hbox2);

    label = gtk_label_new("");
    gtk_widget_show(label);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    SizeLabel = GTK_LABEL(label);

    return hbox;
}

static gboolean on_drawingarea_configure(GtkWidget* area)
{

    if (Surface) cairo_surface_destroy(Surface);
  
    GtkAllocation allocation;
    gtk_widget_get_allocation(area, &allocation);
    Width = allocation.width;
    Height = allocation.height;

    Surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, Width, Height);
    if (CurrentItem) gui_tree_display(CurrentItem, FALSE, FALSE);

    return TRUE;
}

static void gui_show_pix(G_GNUC_UNUSED GtkWidget* area, cairo_t* cr)
{
    if (Surface)
    {
        cairo_set_source_surface(cr, Surface, 0, 0);
        cairo_paint(cr);
    }

    cairo_set_line_width(cr, 1);
    const GdkColor* color;

    if (Mark1) {
        color = color_get_by_type(COLOR_MARK1);
        cairo_set_source_rgb(cr, color->red / 255.0, color->green / 255.0, color->blue / 255.0);
        cairo_rectangle(cr,
            Mark1->geo[0][0], Mark1->geo[1][0],
            Mark1->geo[0][1]-Mark1->geo[0][0]-1, Mark1->geo[1][1]-Mark1->geo[1][0]-1);
        cairo_stroke(cr);
    }
    if (Mark2) {
        color = color_get_by_type(COLOR_MARK2);
        cairo_set_source_rgb(cr, color->red / 255.0, color->green / 255.0, color->blue / 255.0);
        cairo_rectangle(cr,
            Mark2->geo[0][0], Mark2->geo[1][0],
            Mark2->geo[0][1]-Mark2->geo[0][0]-1, Mark2->geo[1][1]-Mark2->geo[1][0]-1);
        cairo_stroke(cr);
    }
}

static gboolean on_area_draw(GtkWidget* area, cairo_t* cr)
{
    gui_show_pix(area, cr);
    return TRUE;
}

static void gui_tree_mark1(tree_info_t* info)
{
    if (info != Mark1)
    {
        char* temp;
        char* text = g_strdup(info->node->name);
        Mark1 = info;
        while (info != CurrentItem && info->parent != CurrentItem &&
               info->parent->parent != CurrentItem)
        {
            info = info->parent;
            temp = g_strdup_printf("%s/%s", info->node->name, text);
            g_free(text);
            text = temp;
        }
        gtk_label_set_text(GTK_LABEL(FileLabel), to_utf8(text));
        g_free(text);

        temp = print_filesize(Mark1->node->size);
        text = g_markup_printf_escaped("<b>%s</b>", temp);
        gtk_label_set_markup(FileSizeLabel, text);
        g_free(temp);
        g_free(text);
    }

}

static void gui_tree_unmark1(G_GNUC_UNUSED tree_info_t* info)
{
    Mark1 = NULL;
}

static void gui_tree_unmark2(G_GNUC_UNUSED tree_info_t* info)
{
    Mark2 = NULL;
}

static void gui_tree_mark2(tree_info_t* info)
{
    if (info != Mark2)
    {
        Mark2 = info;
        if (Mark2 != Mark1)
        {
            char* temp;
            char* text;
      
            gtk_label_set_text(SubLabel, to_utf8(info->node->name));
      
            temp = print_filesize(Mark2->node->size);
            text = g_markup_printf_escaped("<b>%s</b>", temp);
            gtk_label_set_markup(SubSizeLabel, text);
            g_free(temp);
            g_free(text);
        }
        else
        {
            gtk_label_set_markup(SubSizeLabel, "<b>-</b>");
            gtk_label_set_text(SubLabel, "-");
        }
    }

}

static void gui_tree_mark(tree_info_t* info)
{
    tree_info_t* tree;

    if (!info || info == Mark1) return;
    if (Mark1) gui_tree_unmark1(Mark1);
    gui_tree_mark1(info);

    tree = tree_info_find_path(CurrentItem, info);

    if (tree == Mark2) return;
    if (Mark2) gui_tree_unmark2(Mark2);
    if (!tree) return;
    gui_tree_mark2(tree);
}

static gboolean on_area_motion(GtkWidget* widget, GdkEventMotion* event)
{
    tree_info_t* info;

    (void)widget;
    if (!CurrentItem) return FALSE;

    info = tree_info_search(CurrentItem, (int)(event->x), (int)(event->y), LMaxDepth);
    if (info) {
        gui_tree_mark(info);
        gtk_widget_queue_draw(Area);
    }
    return TRUE;
}


static void show_path_error(const char* message, const GError* error)
{
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(MainWin),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", error->message);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    gtk_widget_show(dialog);
}

static void on_open_map_folder(G_GNUC_UNUSED GtkMenuItem* item, const char* folder)
{
    GError* error = NULL;
    char* uri = g_filename_to_uri(folder, NULL, &error);
    if (uri)
        gtk_show_uri_on_window(GTK_WINDOW(MainWin), uri, gtk_get_current_event_time(), &error);
    if (error)
    {
        show_path_error(_("Could not open the folder"), error);
        g_error_free(error);
    }
    g_free(uri);
}

static void on_copy_map_path(GtkMenuItem* item, const char* path)
{
    GError* error = NULL;
    char* text = g_filename_to_utf8(path, -1, NULL, NULL, &error);
    if (!text)
    {
        show_path_error(_("Could not copy the full path"), error);
        g_error_free(error);
        return;
    }

    GtkClipboard* clipboard = gtk_widget_get_clipboard(GTK_WIDGET(item), GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, text, -1);
    gtk_clipboard_set_can_store(clipboard, NULL, 0);
    g_free(text);
}

static void gui_tree_show_context_menu(tree_t* node, GdkEventButton* event)
{
    char* path = tree_full_name(node);
    if (!g_path_is_absolute(path))
    {
        char* cwd = g_get_current_dir();
        char* absolute_path = g_build_filename(cwd, path, NULL);
        g_free(cwd);
        g_free(path);
        path = absolute_path;
    }
    char* folder = node->is_directory ? g_strdup(path) : g_path_get_dirname(path);

    GtkWidget* menu = gtk_menu_new();
    gtk_menu_attach_to_widget(GTK_MENU(menu), Area, NULL);
    g_object_set_data_full(G_OBJECT(menu), "path", path, g_free);
    g_object_set_data_full(G_OBJECT(menu), "folder", folder, g_free);
    g_signal_connect(menu, "selection-done", G_CALLBACK(gtk_widget_destroy), NULL);

    GtkWidget* open = gtk_menu_item_new_with_label(node->is_directory
        ? _("Open in File Manager") : _("Open Containing Folder"));
    g_signal_connect(open, "activate", G_CALLBACK(on_open_map_folder), folder);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), open);

    GtkWidget* copy = gtk_menu_item_new_with_label(_("Copy Full Path"));
    g_signal_connect(copy, "activate", G_CALLBACK(on_copy_map_path), path);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
}

static gboolean on_area_button_press(G_GNUC_UNUSED GtkWidget* widget, GdkEventButton* event)
{
    tree_info_t* info;
    if (!CurrentItem) return FALSE;

    info = tree_info_search(CurrentItem, (int)(event->x), (int)(event->y), LMaxDepth);
    if (!info) return FALSE;

    if (event->type == GDK_BUTTON_PRESS && gdk_event_triggers_context_menu((GdkEvent*)event))
    {
        gui_tree_mark(info);
        gtk_widget_queue_draw(Area);
        gui_tree_show_context_menu(info->node, event);
        return TRUE;
    }

    if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
    {
        tree_info_t* sub = tree_info_find_path(CurrentItem, info);
        if (sub) gui_tree_display(sub, FALSE, TRUE);
    }

    return FALSE;
}

static GtkWidget* gui_create_display_box(void)
{
    GtkWidget* da;

    da = gtk_drawing_area_new();
    Area = da;
    gtk_widget_show(da);
    gtk_widget_set_events(da, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
    // no need for offscreen support of gtk, we do it manually
    gtk_widget_set_double_buffered(da, FALSE);

    g_signal_connect(G_OBJECT(da), "configure_event",
                     G_CALLBACK(on_drawingarea_configure), NULL);
    g_signal_connect(G_OBJECT(da), "draw",
                     G_CALLBACK(on_area_draw), NULL);
    g_signal_connect(G_OBJECT(da), "motion_notify_event",
                     G_CALLBACK(on_area_motion), NULL);
    g_signal_connect(G_OBJECT(da), "button_press_event",
                     G_CALLBACK(on_area_button_press), NULL);
    return da;
}

static GtkWidget* gui_create_status_bar(void)
{
    GtkWidget* bar;
    GtkWidget* grid;
    GtkWidget* label;

    bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_show(bar);
    StatusBar = GTK_BOX(bar);

    grid = gtk_grid_new();
    gtk_widget_show(grid);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_margin_start(grid, 5);
    gtk_widget_set_margin_end(grid, 5);
    gtk_box_pack_start(GTK_BOX(bar), grid, TRUE, TRUE, 0);

    label = gtk_label_new("");
    gtk_widget_show(label);
    SubSizeLabel = GTK_LABEL(label);
    gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    label = gtk_label_new("");
    gtk_widget_show(label);
    SubLabel = GTK_LABEL(label);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE);
/*   gtk_widget_set_size_request(label, 50, -1); */
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_grid_attach(GTK_GRID(grid), label, 1, 0, 1, 1);

    label = gtk_label_new("");
    gtk_widget_show(label);
    FileSizeLabel = GTK_LABEL(label);
    gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 1, 1, 1);

    label = gtk_label_new("");
    gtk_widget_show(label);
    FileLabel = GTK_LABEL(label);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_label_set_single_line_mode(GTK_LABEL(label), TRUE);
    gtk_widget_set_size_request(label, 50, -1);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_grid_attach(GTK_GRID(grid), label, 1, 1, 1, 1);

    return bar;
}

static gboolean on_main_win_delete_event(void)
{
    gtk_main_quit();
    return TRUE;
}

static void gui_setup_actions(GtkWidget* win)
{
    GError* error = NULL;
    
    UIManager = gtk_ui_manager_new();
    ActionGroup = gtk_action_group_new("Actions");
    gtk_action_group_set_translation_domain(ActionGroup, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(ActionGroup, Actions, G_N_ELEMENTS(Actions), win);
    gtk_action_group_set_sensitive(ActionGroup, TRUE);

    gtk_ui_manager_insert_action_group(UIManager, ActionGroup, 0);
    if (!gtk_ui_manager_add_ui_from_string(UIManager, MenuDescription, -1, &error))
    {
        g_warning("building menus failed: %s", error->message);
        g_error_free(error);
    }
}

static gboolean on_redraw(tree_info_t* item)
{
    gui_tree_display(item, FALSE, FALSE);
    if (RedrawTimer) g_source_remove(RedrawTimer);
    RedrawTimer = 0;
    return TRUE;
}

static void gui_tree_redraw(void)
{
    LUseColors = pref_get_use_colors();
    LMaxDepth = pref_get_max_depth();
    LDisplayMode = pref_get_display_mode();
    LUseAverage = pref_get_use_average();
    
    if (CurrentItem) 
    {
        if (RedrawTimer) g_source_remove(RedrawTimer);
        RedrawTimer = g_timeout_add(300, (GSourceFunc)on_redraw, CurrentItem);
    }
}

static GdkPixbuf* create_pixbuf(const gchar *filename)
{
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

GtkWidget* gui_create_main_win(void)
{
    GtkWidget* win;
    GtkWidget* vbox;
    GtkWidget* item;
  
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    MainWin = win;
    gtk_window_set_title(GTK_WINDOW(win), "Graphical Disk Map "VERSION);
    gtk_window_set_role(GTK_WINDOW(win), "Main window");
    gtk_window_set_default_icon(create_pixbuf("gdmap_icon.png"));
    gtk_window_set_default_size(GTK_WINDOW(win), 700, 450);
    g_object_add_weak_pointer(G_OBJECT(win), (void*)&MainWin);


    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(win), vbox);
/*   gtk_container_set_border_width(GTK_CONTAINER (vbox), 0); */
  

    gui_setup_actions(win);
  
    if ((item = gtk_ui_manager_get_widget(UIManager, "/ui/MainMenu")) != NULL)
        gtk_box_pack_start(GTK_BOX (vbox), item, FALSE, FALSE, 0);

    if ((item = gtk_ui_manager_get_widget(UIManager, "/ui/ToolBar")) != NULL)
        gtk_box_pack_start(GTK_BOX (vbox), item, FALSE, FALSE, 0);

    if ((item = gui_create_path_box()) != NULL)
        gtk_box_pack_start(GTK_BOX (vbox), item, FALSE, FALSE, 0);

    if ((item = gui_create_display_box()) != NULL)
        gtk_box_pack_start(GTK_BOX (vbox), item, TRUE, TRUE, 0);

    if ((item = gui_create_status_bar()) != NULL)
        gtk_box_pack_start(GTK_BOX (vbox), item, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT (win), "delete_event",
                     G_CALLBACK(on_main_win_delete_event), NULL);
  
    gui_buttons_update();

    gtk_widget_show(win);

    pref_set_redraw_callback(gui_tree_redraw);
  
    return win;
}





GtkWidget* gui_get_main_win(void)
{
    return MainWin;
}

static void on_path_toggled(GtkToggleButton* button, tree_info_t* info)
{
    if (gtk_toggle_button_get_active(button))
    {
        gui_tree_display(info, FALSE, TRUE/*FALSE*/);
    }
}

static void gui_paths_update(void)
{
    static GList* Paths = NULL;
    static GSList* Group = NULL;
    GtkWidget* button;
    GList* dlist;
    GList* temp;
    tree_info_t* dtemp;
    tree_info_t* item;

    if (!CurrentItem) return;

    for (item = CurrentItem; item; item = item->parent)
    {

        for (dlist = Paths; dlist; dlist = dlist->next)
        {
            GtkWidget* button = dlist->data;
            tree_info_t* info = g_object_get_data(G_OBJECT(button), "info");
      
            if (info == item)
            {
                if (item == CurrentItem)
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
                break;
            }
        }
        if (dlist) break;
    }

    if (dlist)
    {
        // only delete if we have to add a new end path.
        if (item != CurrentItem) temp = dlist->next;
        else temp = NULL;
/*     temp = dlist->next; */
    }
    else
    {
        // delete all
        temp = Paths;
    }

    for (dlist = temp; dlist; )
    {
        GtkWidget* button = dlist->data;
        dlist = dlist->next;
        Paths = g_list_remove(Paths, button);
        gtk_widget_destroy(button);
    }

    // update radio button group
    if (Paths)
        Group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(Paths->data));
    else
        Group = NULL;

    temp = NULL;
    dtemp = CurrentItem;
    while (dtemp != item)
    {
        temp = g_list_prepend(temp, dtemp);
        dtemp = dtemp->parent;
    }
  
    for (dlist = temp; dlist; dlist = dlist->next)
    {
        tree_info_t* info = dlist->data;
        button = gtk_radio_button_new_with_label(Group, info->node->name);
        Group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
        gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(button), FALSE);
        g_object_set_data(G_OBJECT(button), "info", info);
        gtk_widget_show(button);
        gtk_box_pack_start(PathBox, button, FALSE, FALSE, 0);
        Paths = g_list_append(Paths, button);
        if (info == CurrentItem)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
        g_signal_connect(G_OBJECT(button), "toggled",
                         G_CALLBACK(on_path_toggled), info);
    }
}

static void gui_tree_display(tree_info_t* info, gboolean destroy_old, gboolean new_history)
{
    if (destroy_old)
    {
        if (CurrentItem)
        {
            while (CurrentItem->parent) CurrentItem = CurrentItem->parent;
            tree_destroy(CurrentItem->node);
            tree_info_destroy(CurrentItem);
        }
        CurrentItem = NULL;
        history_clear();
    }


    if (CurrentItem != info)
    {
        if (new_history) history_append(info);
        CurrentItem = info;
        gui_paths_update();
    }

    treemap_options options = {LUseColors, LUseAverage, LMaxDepth, LDisplayMode, pref_get_h(), pref_get_f(), 1.0};
    treemap_layout(info, Width, Height, &options);
    treemap_paint(info, Surface, &options);
    cairo_surface_mark_dirty(Surface);
    Mark1 = NULL;
    Mark2 = NULL;
    gtk_widget_queue_draw(Area);

    gui_buttons_update();
  
    {
        char* temp1 = print_filesize(info->node->size);
        char* temp2 = g_markup_printf_escaped("<b>%s</b>", temp1);
        gtk_label_set_markup(SizeLabel, temp2);
        g_free(temp1);
        g_free(temp2);
    }

}

