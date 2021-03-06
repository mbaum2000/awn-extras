/*
 * Copyright (c) 2010 Sharkbaitbobby <sharkbaitbobby+awn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <libawn/libawn.h>
#include <libindicator/indicator-object.h>

typedef struct _IndicatorApplet IndicatorApplet;
struct _IndicatorApplet {
  AwnApplet *applet;
  GtkWidget *da;
  GtkWidget *icon_box;
  GtkWidget *awn_menu;
  GtkDialog *dialog;

  IndicatorObject *io;

  DesktopAgnosticConfigClient *config;
  gint config_rows_cols;
  gboolean config_ind_app;
  gboolean config_ind_app_ses;
  gboolean config_me;
  gboolean config_messaging;
  gboolean config_network;
  gboolean config_sound;
  gboolean config_other_menus;
  gboolean applet_mode;

  GList *images;
  GList *menus;
  GList *shown_images;
  GList *shown_menus;
  GList *awnicons;

  gint num;
  gint popup_num;
  gint last_num;
  gint dx;
  gint dy;
};

static gboolean icon_button_press(AwnIcon *icon, GdkEventButton *event, IndicatorApplet *iapplet);
static gboolean icon_right_click(AwnIcon *icon, GdkEventButton *event, IndicatorApplet *iapplet);
static gboolean icon_scroll(AwnIcon *icon, GdkEventScroll *event, IndicatorApplet *iapplet);
static void get_shown_entries(IndicatorApplet *iapplet);
static void resize_da(IndicatorApplet *iapplet);
static void update_config(IndicatorApplet *iapplet);
static void update_icon_mode(IndicatorApplet *iapplet);

void
show_about(GtkMenuItem *item, gpointer user_data)
{
  const gchar *license = "This program is free software; you can redistribute "
"it and/or modify it under the terms of the GNU General Public License as "
"published by the Free Software Foundation; either version 2 of the License, "
"or (at your option) any later version.\n\nThis program is distributed in the "
"hope that it will be useful, but WITHOUT ANY WARRANTY; without even the "
"implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
"See the GNU General Public License for more details.\n\nYou should have "
"received a copy of the GNU General Public License along with this program; "
"if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth "
"Floor, Boston, MA 02110-1301  USA.";
  const gchar *authors[] = {"Sharkbaitbobby <sharkbaitbobby+awn@gmail.com>",
                            NULL};

  GtkWidget *about = gtk_about_dialog_new();

  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), _("Indicator Applet"));
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about),
                                _("An applet to hold all of the system indicators"));
  gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about),
                                 "Copyright \xc2\xa9 2010 Sharkbaitbobby");
  gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about), "indicator-applet");
  gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about), license);
  gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about), TRUE);
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about),
                               "http://wiki.awn-project.org/Indicator_Applet");
  gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about),
                                     "wiki.awn-project.org");

  gtk_window_set_icon_name(GTK_WINDOW(about), "indicator-applet");

  gtk_dialog_run(GTK_DIALOG(about));
  gtk_widget_destroy(about);
}

/* Preferences dialog ... */
static gboolean
check_toggled(GtkToggleButton *button, IndicatorApplet *iapplet)
{
  desktop_agnostic_config_client_set_bool(iapplet->config,
                                          DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                          (gchar*)g_object_get_data(G_OBJECT(button), "ldakey"),
                                          gtk_toggle_button_get_active(button), NULL);
  update_config(iapplet);
  get_shown_entries(iapplet);

  update_icon_mode(iapplet);

  return FALSE;
}

static gboolean
applet_mode_check_toggled(GtkToggleButton *button, IndicatorApplet *iapplet)
{
  desktop_agnostic_config_client_set_bool(iapplet->config,
                                          DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                          "applet_icon_mode", gtk_toggle_button_get_active(button),
                                          NULL);

  update_config(iapplet);
  update_icon_mode(iapplet);
}

static GtkWidget*
make_check_button(IndicatorApplet *iapplet,
                  gchar *label,
                  gchar *key,
                  gboolean enabled,
                  GtkWidget *box)
{
  GtkWidget *check = gtk_check_button_new_with_label(label);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), enabled);
  g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(check_toggled), (gpointer)iapplet);
  g_object_set_data(G_OBJECT(check), "ldakey", key);
  gtk_box_pack_start(GTK_BOX(box), check, FALSE, FALSE, 0);
}

static gboolean
spin_changed(GtkSpinButton *spin, IndicatorApplet *iapplet)
{
  desktop_agnostic_config_client_set_int(iapplet->config,
                                         DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                         "rows_cols", (gint)gtk_spin_button_get_value(spin),
                                         NULL);
  update_config(iapplet);

  if (!iapplet->applet_mode)
  {
    resize_da(iapplet);
  }
}

static void
show_prefs(GtkMenuItem *item, IndicatorApplet *iapplet)
{
  GtkDialog *win = GTK_DIALOG(gtk_dialog_new_with_buttons(_("Indicator Applet Preferences"),
                              NULL, GTK_DIALOG_NO_SEPARATOR, GTK_STOCK_CLOSE, GTK_RESPONSE_ACCEPT,
                              NULL));

  iapplet->dialog = win;

  GtkWidget *label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), g_strdup_printf("<b>%s</b>", _("Enabled Entries")));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(win->vbox), label, FALSE, FALSE, 0);

  GtkWidget *vbox = gtk_vbox_new(FALSE, 3);
  make_check_button(iapplet, _("Indicator Applet"),
                    "indicator_applet", iapplet->config_ind_app, vbox);
  make_check_button(iapplet, _("Indicator Applet Session (Quit)"),
                    "indicator_applet_session", iapplet->config_ind_app_ses, vbox);
  make_check_button(iapplet, _("Me Menu"),
                    "me_menu", iapplet->config_me, vbox);
  make_check_button(iapplet, _("Messaging Menu"),
                    "messaging_menu", iapplet->config_messaging, vbox);
  make_check_button(iapplet, _("Network Menu"),
                    "network_menu", iapplet->config_network, vbox);
  make_check_button(iapplet, _("Sound Control Menu"),
                    "sound_menu", iapplet->config_sound, vbox);
  make_check_button(iapplet, _("Other Menus"),
                    "other_menus", iapplet->config_other_menus, vbox);

  GtkWidget *align = gtk_alignment_new(0.0, 0.5, 1.0, 0.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(align), vbox);

  gtk_box_pack_start(GTK_BOX(win->vbox), align, FALSE, FALSE, 0);

  label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), g_strdup_printf("<b>%s</b>", _("Options")));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(win->vbox), label, FALSE, FALSE, 0);

  vbox = gtk_vbox_new(FALSE, 3);
  GtkPositionType pos = awn_applet_get_pos_type(iapplet->applet);
  if (pos == GTK_POS_TOP || pos == GTK_POS_BOTTOM)
  {
    label = gtk_label_new(_("Number of rows:"));
  }
  else
  {
    label = gtk_label_new(_("Number of columns:"));
  }
  GtkWidget *spin = gtk_spin_button_new_with_range(1.0, 3.0, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), (gdouble)iapplet->config_rows_cols);
  g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(spin_changed), (gpointer)iapplet);

  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  GtkWidget *check = gtk_check_button_new_with_label(_("Enable Applet Icon mode"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), iapplet->applet_mode);
  g_signal_connect(G_OBJECT(check), "toggled",
                   G_CALLBACK(applet_mode_check_toggled), (gpointer)iapplet);
  gtk_box_pack_start(GTK_BOX(vbox), check, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.0, 0.5, 1.0, 0.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(align), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(align), vbox);
  gtk_box_pack_start(GTK_BOX(win->vbox), align, FALSE, FALSE, 0);

  gtk_container_set_border_width(GTK_CONTAINER(win), 12);
  gtk_window_set_icon_name(GTK_WINDOW(win), "indicator-applet");
  gtk_widget_show_all(GTK_WIDGET(win));
  gtk_dialog_run(win);
  gtk_widget_destroy(GTK_WIDGET(win));
}

static gboolean
get_bool(IndicatorApplet *iapplet, gchar *key)
{
  return desktop_agnostic_config_client_get_bool(iapplet->config,
                                                 DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                                 key, NULL);
}

static void
update_config(IndicatorApplet *iapplet)
{
  iapplet->config_rows_cols = desktop_agnostic_config_client_get_int(iapplet->config,
                                                              DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                                                     "rows_cols", NULL);
  if (iapplet->config_rows_cols < 1)
  {
    iapplet->config_rows_cols = 2;
  }
  iapplet->config_ind_app = get_bool(iapplet, "indicator_applet");
  iapplet->config_ind_app_ses = get_bool(iapplet, "indicator_applet_session");
  iapplet->config_me = get_bool(iapplet, "me_menu");
  iapplet->config_messaging = get_bool(iapplet, "messaging_menu");
  iapplet->config_network = get_bool(iapplet, "network_menu");
  iapplet->config_sound = get_bool(iapplet, "sound_menu");
  iapplet->config_other_menus = get_bool(iapplet, "other_menus");
  iapplet->applet_mode = get_bool(iapplet, "applet_icon_mode");
}

/* Dealing with libindicator ... */
static void
get_shown_entries(IndicatorApplet *iapplet)
{
  // Causes segfaults for various reasons... (none fully known)
  /*if (iapplet->shown_images != NULL)
  {
    g_list_free(iapplet->shown_images);
  }
  if (iapplet->shown_menus != NULL)
  {
    g_list_free(iapplet->shown_menus);
  }*/

  iapplet->shown_images = NULL;
  iapplet->shown_menus = NULL;

  gchar *name;
  gboolean add;

  gint i = 0;
  GList *l;
  for (l = iapplet->images; l; l = l->next)
  {
    add = FALSE;

    IndicatorObject *io = INDICATOR_OBJECT(g_object_get_data(G_OBJECT(l->data), "indicator"));
    if (INDICATOR_IS_OBJECT(io))
    {
      name = (gchar*)g_object_get_data(G_OBJECT(io), "filename");

      if (!g_strcmp0(name, "libapplication.so"))
      {
        if (iapplet->config_ind_app)
        {
          add = TRUE;
        }
      }

      else if (!g_strcmp0(name, "libme.so"))
      {
        if (iapplet->config_me)
        {
          add = TRUE;
        }
      }

      else if (!g_strcmp0(name, "libmessaging.so"))
      {
        if (iapplet->config_messaging)
        {
          add = TRUE;
        }
      }

      else if (!g_strcmp0(name, "libnetworkmenu.so"))
      {
        if (iapplet->config_network)
        {
          add = TRUE;
        }
      }

      else if (!g_strcmp0(name, "libsession.so"))
      {
        if (iapplet->config_ind_app_ses)
        {
          add = TRUE;
        }
      }

      else if (!g_strcmp0(name, "libsoundmenu.so"))
      {
        if (iapplet->config_sound)
        {
          add = TRUE;
        }
      }

      else
      {
        if (iapplet->config_other_menus)
        {
          add = TRUE;
        }
      }

      if (add)
      {
        iapplet->shown_images = g_list_append(iapplet->shown_images, l->data);
        iapplet->shown_menus = g_list_append(iapplet->shown_menus,
                                             g_list_nth_data(iapplet->menus, i));
      }
    }

    i++;
  }
}

static gboolean
pixbuf_changed(GObject *image, GParamSpec *spec, IndicatorApplet *iapplet)
{
  if (iapplet->applet_mode)
  {
    update_icon_mode(iapplet);
  }
  else
  {
    gtk_widget_queue_draw(iapplet->da);
  }

  return FALSE;
}

static void
entry_added(IndicatorObject *io, IndicatorObjectEntry *entry, IndicatorApplet *iapplet)
{
  if (entry->image == NULL || entry->menu == NULL)
  {
    /* If either of these is NULL, there will likely be problems when
     * the entry is removed */
    return;
  }

  g_object_set_data(G_OBJECT(entry->image), "indicator", io);
  iapplet->images = g_list_append(iapplet->images, entry->image);
  iapplet->menus = g_list_append(iapplet->menus, entry->menu);
  iapplet->num++;

  gulong handler = g_signal_connect(G_OBJECT(entry->image), "notify::pixbuf",
                                  G_CALLBACK(pixbuf_changed), (gpointer)iapplet);
  g_object_set_data(G_OBJECT(entry->image), "pixbufhandler", (gpointer)handler);

  gtk_widget_hide(GTK_WIDGET(entry->menu));

  get_shown_entries(iapplet);

  update_icon_mode(iapplet);

  return;
}

static void
entry_removed(IndicatorObject *io, IndicatorObjectEntry *entry, IndicatorApplet *iapplet)
{
  iapplet->images = g_list_remove(iapplet->images, entry->image);
  iapplet->menus = g_list_remove(iapplet->menus, entry->menu);
  iapplet->num--;

  gulong handler = (gulong)g_object_get_data(G_OBJECT(entry->image), "pixbufhandler");

  if (g_signal_handler_is_connected(G_OBJECT(entry->image), handler))
  {
    g_signal_handler_disconnect(G_OBJECT(entry->image), handler);
  }

  get_shown_entries(iapplet);

  update_icon_mode(iapplet);
}

static gboolean
load_module(const gchar * name, IndicatorApplet *iapplet)
{
  g_return_val_if_fail(name != NULL, FALSE);

  if (!g_str_has_suffix(name, G_MODULE_SUFFIX))
  {
    return FALSE;
  }

  gchar *fullpath = g_build_filename(INDICATOR_DIR, name, NULL);
  IndicatorObject *io = iapplet->io = indicator_object_new_from_file(fullpath);
  g_free(fullpath);

  g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,
    G_CALLBACK(entry_added), iapplet);
  g_signal_connect(G_OBJECT(io), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED,
    G_CALLBACK(entry_removed), iapplet);

  GList *entries = indicator_object_get_entries(io);
  GList *entry = NULL;

  g_object_set_data(G_OBJECT(io), "filename", (gpointer)g_strdup(name));

  for (entry = entries; entry != NULL; entry = g_list_next(entry))
  {
    entry_added(io, (IndicatorObjectEntry*)entry->data, iapplet);
  }

  g_list_free(entries);

  return TRUE;
}

/* Drawing, widgets, etc ... */
static void
resize_da(IndicatorApplet *iapplet)
{
  gint size = awn_applet_get_size(iapplet->applet);
  GtkPositionType pos = awn_applet_get_pos_type(iapplet->applet);
  gint rc = iapplet->config_rows_cols;

  gint pb_size = size * 1.1 / rc;

  gint num = g_list_length(iapplet->shown_images);

  if (pos == GTK_POS_TOP || pos == GTK_POS_BOTTOM)
  {
    gtk_widget_set_size_request(iapplet->da,
      ((int)(num / rc) + num % rc) * pb_size, -1);
  }
  else
  {
    gtk_widget_set_size_request(iapplet->da, -1,
      ((int)(num / rc) + num % rc) * pb_size);
  }

  gtk_widget_queue_draw(iapplet->da);
}

static gboolean
determine_position(IndicatorApplet *iapplet, gint x, gint y)
{
  AwnApplet *applet = iapplet->applet;
  GtkPositionType pos = awn_applet_get_pos_type(applet);
  gint size = awn_applet_get_size(applet);
  gint offset = awn_applet_get_offset(applet);
  gint width = iapplet->da->allocation.width;
  gint height = iapplet->da->allocation.height;
  gint rc = iapplet->config_rows_cols;
  gint pb_size = size * 1.1 / rc;

  gint col = -1, row = -1, num = -1, dx = -1, dy = -1;

  switch (pos)
  {
    case GTK_POS_BOTTOM:
      row = (height - y - offset) / pb_size;
      if (row == -1)
      {
        row = 0;
      }
      dy = y - height + offset + pb_size * (row + 1);

      col = (gint)(x / pb_size);
      dx = x - col * pb_size;
      num = col * rc + row;
      break;

    case GTK_POS_TOP:
      row = (y - offset) / pb_size;
      if (row == -1)
      {
        row = 0;
      }
      dy = y - (offset + pb_size * row);

      col = (gint)(x / pb_size);
      dx = x - col * pb_size;
      num = col * rc + row;
      break;

    case GTK_POS_LEFT:
      col = (x - offset) / pb_size;
      if (col == -1)
      {
        col = 0;
      }
      dx = x - (offset + pb_size * col);

      row = (gint)(y / pb_size);
      dy = y - row * pb_size;
      num = row * rc + col;
      break;

    default:
      col = (width - x - offset) / pb_size;
      if (col == -1)
      {
        col = 0;
      }
      dx = x - width + offset + pb_size * (col + 1);

      row = (gint)(y / pb_size);
      dy = y - row * pb_size;
      num = row * rc + col;
      break;
  }

  if (row == -1 || col == -1 || num == -1 || num >= g_list_length(iapplet->shown_menus))
  {
    return FALSE;
  }

  iapplet->popup_num = num;
  iapplet->dx = dx;
  iapplet->dy = dy;

  return TRUE;
}

static void
da_expose_event(GtkWidget *da, GdkEventExpose *event, IndicatorApplet *iapplet)
{
  AwnApplet *applet = AWN_APPLET(iapplet->applet);

  cairo_t *cr = gdk_cairo_create(da->window);

  if (gdk_screen_is_composited(gtk_widget_get_screen(da)))
  {
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
  }

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  GtkPositionType pos = awn_applet_get_pos_type(applet);
  gint offset = awn_applet_get_offset(applet);
  gint size = awn_applet_get_size(applet);
  gint w = da->allocation.width;
  gint h = da->allocation.height;
  gfloat x = 0.0, y = 0.0;
  gint rc = iapplet->config_rows_cols;
  gint pb_size = size * 1.1 / rc;

  GtkImage *image;
  GdkPixbuf *pb;
  GIcon *icon;
  GtkIconInfo *icon_info;
  gchar *icon_path;
  gboolean free_pb;

  GtkIconTheme *theme = gtk_icon_theme_get_default();

  gint i;
  for (i = 0; i < g_list_length(iapplet->shown_images); i++)
  {
    /* Get the pixbuf */
    pb = NULL;
    free_pb = FALSE;
    image = GTK_IMAGE(g_list_nth_data(iapplet->shown_images, i));
    icon = g_object_get_data(G_OBJECT(image), "indicator-names-data");

    icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, pb_size,
      GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_USE_BUILTIN);
    if (icon_info == NULL)
    {
      icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, 22,
        GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_USE_BUILTIN);
    }

    if (icon_info == NULL)
    {
      if (gtk_image_get_storage_type(image) == GTK_IMAGE_PIXBUF)
      {
        pb = gtk_image_get_pixbuf(image);

        if (gdk_pixbuf_get_width(pb) != pb_size || gdk_pixbuf_get_height(pb) != pb_size)
        {
          pb = gdk_pixbuf_scale_simple(pb, pb_size, pb_size, GDK_INTERP_BILINEAR);
          free_pb = TRUE;
        }
      }
    }
    else
    {
      icon_path = (gchar*)gtk_icon_info_get_filename(icon_info);
      pb = gdk_pixbuf_new_from_file_at_size(icon_path, pb_size, pb_size, NULL);
      free_pb = TRUE;
      gtk_icon_info_free(icon_info);
    }

    cairo_save(cr);

    switch (pos)
    {
      case GTK_POS_BOTTOM:
        x = (i - i % rc) * pb_size / rc;
        y = h - pb_size * (1 + i % rc) - offset;
        break;

      case GTK_POS_TOP:
        x = (i - i % rc) * pb_size / rc;
        y = pb_size * (i % rc) + offset;
        break;

      case GTK_POS_LEFT:
        x = pb_size * (i % rc) + offset;
        y = (i - i % rc) * pb_size / rc;
        break;

      default:
        x = w - pb_size * (1 + i % rc) - offset;
        y = (i - i % rc) * pb_size / rc;
        break;
    }

    cairo_rectangle(cr, x, y, pb_size, pb_size);
    cairo_clip(cr);
    cairo_translate(cr, x, y);

    if (GDK_IS_PIXBUF(pb))
    {
      gdk_cairo_set_source_pixbuf(cr, pb, 0.0, 0.0);
      cairo_paint(cr);

      cairo_restore(cr);

      if (free_pb)
      {
        g_object_unref(G_OBJECT(pb));
      }
    }
  }
}

static gboolean
deactivate_event (GtkMenuShell *menushell, IndicatorApplet *iapplet)
{
  g_object_set (awn_overlayable_get_effects
    (AWN_OVERLAYABLE(AWN_ICON(g_list_nth_data(iapplet->awnicons, iapplet->popup_num)))),
     "depressed", FALSE, NULL);
}

static void
update_icon_mode(IndicatorApplet *iapplet)
{
  if (iapplet->applet_mode)
  {
    gtk_widget_hide(iapplet->da);
  }
  else
  {
    resize_da(iapplet);
    gtk_widget_show(iapplet->da);

    GList *l = NULL;
    for (l = gtk_container_get_children(GTK_CONTAINER(iapplet->icon_box)); l; l = l->next)
    {
      if (GTK_WIDGET(l->data) != iapplet->da)
      {
        gtk_widget_hide(GTK_WIDGET(l->data));
      }
    }

    return;
  }

  if (iapplet->shown_images == NULL)
  {
    return;
  }

  gint size = awn_applet_get_size(iapplet->applet);
  gboolean free_pb;
  GtkImage *image;
  GIcon *icon;
  AwnIcon *awnicon;
  GdkPixbuf *pb;
  GtkIconTheme *theme;
  GtkIconInfo *icon_info;

  gint nshown = g_list_length(iapplet->shown_images);
  gulong i;
  for (i = 0; i < nshown; i++)
  {
    image = GTK_IMAGE(g_list_nth_data(iapplet->shown_images, i));

    if (g_list_length(iapplet->awnicons) <= i)
    {
      /* Make new AwnIcon... */
      awnicon = AWN_ICON(awn_themed_icon_new());
      g_signal_connect(G_OBJECT(awnicon), "button-press-event",
                       G_CALLBACK(icon_button_press), (gpointer)iapplet);
      g_signal_connect(G_OBJECT(awnicon), "context-menu-popup",
                       G_CALLBACK(icon_right_click), (gpointer)iapplet);
      g_signal_connect(G_OBJECT(awnicon), "scroll-event",
                       G_CALLBACK(icon_scroll), (gpointer)iapplet);
      g_signal_connect(GTK_WIDGET (g_list_nth_data(iapplet->shown_menus, i)), "deactivate",
                       G_CALLBACK(deactivate_event), (gpointer)iapplet);
      g_object_set_data(G_OBJECT(awnicon), "num", (gpointer)i);

      gtk_box_pack_start(GTK_BOX(iapplet->icon_box), GTK_WIDGET(awnicon), FALSE, FALSE, 0);
      gtk_widget_show(GTK_WIDGET(awnicon));

      iapplet->awnicons = g_list_append(iapplet->awnicons, (gpointer)awnicon);
    }
    else
    {
      awnicon = AWN_ICON(g_list_nth_data(iapplet->awnicons, i));
    }

    awn_icon_set_pos_type(awnicon, awn_applet_get_pos_type(iapplet->applet));
    awn_icon_set_offset(awnicon, awn_applet_get_offset(iapplet->applet));

    IndicatorObject *io = INDICATOR_OBJECT(g_object_get_data(G_OBJECT(image), "indicator"));
    if (INDICATOR_IS_OBJECT(io))
    {
      const gchar *name = g_object_get_data(G_OBJECT(io), "filename");

      if (!g_strcmp0(name, "libme.so"))
      {
        awn_icon_set_tooltip_text(awnicon, _("Me menu"));
      }
      else if (!g_strcmp0(name, "libmessaging.so"))
      {
        awn_icon_set_tooltip_text(awnicon, _("Messaging menu"));
      }
      else if (!g_strcmp0(name, "libnetworkmenu.so"))
      {
        awn_icon_set_tooltip_text(awnicon, _("Network menu"));
      }
      else if (!g_strcmp0(name, "libsession.so"))
      {
        awn_icon_set_tooltip_text(awnicon, _("Indicator Applet Session"));
      }
      else if (!g_strcmp0(name, "libsoundmenu.so"))
      {
        awn_icon_set_tooltip_text(awnicon, _("Sound Menu"));
      }
      else
      {
        awn_icon_set_tooltip_text(awnicon, _("Indicator Applet"));
      }
    }

    free_pb = FALSE;
    icon = g_object_get_data(G_OBJECT(image), "indicator-names-data");
    pb = NULL;
    theme = gtk_icon_theme_get_default();
    icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, size,
      GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_USE_BUILTIN);
    if (icon_info == NULL)
    {
      icon_info = gtk_icon_theme_lookup_by_gicon(theme, icon, 22,
        GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_USE_BUILTIN);
    }

    if (icon_info == NULL)
    {
      if (gtk_image_get_storage_type(image) == GTK_IMAGE_PIXBUF)
      {
        pb = gtk_image_get_pixbuf(image);

        if (gdk_pixbuf_get_width(pb) != size || gdk_pixbuf_get_height(pb) != size)
        {
          pb = gdk_pixbuf_scale_simple(pb, size, size, GDK_INTERP_BILINEAR);
          free_pb = TRUE;
        }
      }
    }
    else
    {
      const gchar *icon_path = gtk_icon_info_get_filename(icon_info);
      pb = gdk_pixbuf_new_from_file_at_size(icon_path, size, size, NULL);
      free_pb = TRUE;
      gtk_icon_info_free(icon_info);
    }

    awn_icon_set_from_pixbuf(awnicon, pb);
    if (free_pb)
    {
      g_object_unref(G_OBJECT(pb));
    }

    gtk_widget_show(GTK_WIDGET(awnicon));
  }

  if (g_list_length(iapplet->awnicons) > i)
  {
    gint j;
    gpointer rm;
    for (j = g_list_length(iapplet->awnicons); j > i; j--)
    {
      rm = g_list_nth_data(iapplet->awnicons, j - 1);
      iapplet->awnicons = g_list_remove(iapplet->awnicons, rm);
      gtk_widget_destroy(GTK_WIDGET(rm));
    }
  }
}

/* AwnIcon-related code ... */
static gboolean
icon_button_press(AwnIcon *icon, GdkEventButton *event, IndicatorApplet *iapplet)
{
  if (iapplet->shown_menus == NULL || event->button == 3)
  {
    return FALSE;
  }

  iapplet->popup_num = (gulong)g_object_get_data(G_OBJECT(icon), "num");

  awn_icon_popup_gtk_menu (icon, GTK_WIDGET (g_list_nth_data(iapplet->shown_menus, iapplet->popup_num)), 1, event->time);

  return FALSE;
}

static gboolean
icon_right_click(AwnIcon *icon, GdkEventButton *event, IndicatorApplet *iapplet)
{
  if (!iapplet->awn_menu)
  {
    iapplet->awn_menu = awn_applet_create_default_menu(iapplet->applet);

    GtkWidget *item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(show_prefs), (gpointer)iapplet);
    gtk_menu_shell_append(GTK_MENU_SHELL(iapplet->awn_menu), GTK_WIDGET(item));

    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(show_about), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(iapplet->awn_menu), GTK_WIDGET(item));

    gtk_widget_show_all(iapplet->awn_menu);
  }

  awn_icon_popup_gtk_menu (icon, GTK_WIDGET (iapplet->awn_menu), event->button, event->time);

  return FALSE;
}

static gboolean
icon_scroll(AwnIcon *icon, GdkEventScroll *event, IndicatorApplet *iapplet)
{
  gulong num = (gulong)g_object_get_data(G_OBJECT(icon), "num");

  GtkWidget *image = g_list_nth_data(iapplet->shown_images, num);
  IndicatorObject *io = g_object_get_data(G_OBJECT(image), "indicator");
  g_signal_emit_by_name(io, "scroll", 1, event->direction);

  return FALSE;
}

/* DrawingArea-related code ... */
static void
da_menu_position(GtkMenu *menu, gint *x, gint *y, gboolean *move, IndicatorApplet *iapplet)
{
  AwnApplet *applet = AWN_APPLET(iapplet->applet);
  GtkPositionType pos = awn_applet_get_pos_type(applet);
  gint size = awn_applet_get_size(applet);
  gint offset = awn_applet_get_offset(applet);
  gint mwidth = GTK_WIDGET(menu)->requisition.width;
  gint mheight = GTK_WIDGET(menu)->requisition.height;
  gint rc = iapplet->config_rows_cols;
  gint pb_size = size * 1.1 / rc;

  switch (pos)
  {
    case GTK_POS_BOTTOM:
      *x -= iapplet->dx;
      *y -= iapplet->dy + mheight;
      break;
    case GTK_POS_TOP:
      *x -= iapplet->dx;
      *y += pb_size - iapplet->dy;
      break;
    case GTK_POS_LEFT:
      *x += pb_size - iapplet->dx;
      *y -= iapplet->dy;
      break;
    default:
      *x -= iapplet->dx + mwidth;
      *y -= iapplet->dy;
      break;
  }

  /* fits to screen? */
  GdkScreen *screen = NULL;
  if (gtk_widget_has_screen (GTK_WIDGET (menu)))
  {
    screen = gtk_widget_get_screen (GTK_WIDGET (menu));
  }
  else
  {
    screen = gdk_screen_get_default ();
  }
  if (screen)
  {
    gint screen_w = gdk_screen_get_width (screen);
    gint screen_h = gdk_screen_get_height (screen);
    *x = MIN (*x, screen_w - mwidth);
    *y = MIN (*y, screen_h - mheight);
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
  }

  *move = TRUE;
}

static gboolean
da_button_press(GtkWidget *widget, GdkEventButton *event, IndicatorApplet *iapplet)
{
  AwnApplet *applet = AWN_APPLET(iapplet->applet);
  if (event->button == 3)
  {
    if (!iapplet->awn_menu)
    {
      iapplet->awn_menu = awn_applet_create_default_menu(applet);

      GtkWidget *item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
      g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(show_prefs), (gpointer)iapplet);
      gtk_menu_shell_append(GTK_MENU_SHELL(iapplet->awn_menu), GTK_WIDGET(item));

      item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
      g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(show_about), NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(iapplet->awn_menu), GTK_WIDGET(item));

      gtk_widget_show_all(iapplet->awn_menu);
    }

    gtk_menu_popup(GTK_MENU(iapplet->awn_menu), NULL, NULL, NULL, NULL,
                   event->button, event->time);

    return FALSE;
  }

  if (iapplet->applet_mode || !determine_position(iapplet, (gint)event->x, (gint)event->y))
  {
    return FALSE;
  }

  gtk_menu_popup(GTK_MENU(g_list_nth_data(iapplet->shown_menus, iapplet->popup_num)), NULL, NULL,
    (GtkMenuPositionFunc)da_menu_position, (gpointer)iapplet, event->button, event->time);

  return FALSE;
}

static gboolean
da_scroll(GtkWidget *da, GdkEventScroll *event, IndicatorApplet *iapplet)
{
  if (!determine_position(iapplet, (gint)event->x, (gint)event->y))
  {
    return FALSE;
  }

  GtkWidget *image = g_list_nth_data(iapplet->shown_images, iapplet->popup_num);
  IndicatorObject *io = g_object_get_data(G_OBJECT(image), "indicator");
  g_signal_emit_by_name(io, "scroll", 1, event->direction);

  return FALSE;
}

static gboolean
applet_size_changed(AwnApplet *applet, gint size, IndicatorApplet *iapplet)
{
  update_icon_mode(iapplet);

  return FALSE;
}

static gboolean
applet_position_changed(AwnApplet *applet, GtkPositionType pos, IndicatorApplet *iapplet)
{
  update_icon_mode(iapplet);

  return FALSE;
}

AwnApplet*
awn_applet_factory_initp(const gchar *name, const gchar *uid, gint panel_id)
{
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  AwnApplet *applet = awn_applet_new(name, uid, panel_id);

  GtkWidget *da = gtk_drawing_area_new();
  gtk_widget_add_events(da, GDK_BUTTON_PRESS_MASK);
  gtk_widget_show(da);

  GtkWidget *icon_box = awn_icon_box_new_for_applet(applet);
  gtk_box_pack_start(GTK_BOX(icon_box), da, FALSE, FALSE, 0);
  gtk_widget_show_all(icon_box);
  gtk_container_add(GTK_CONTAINER(applet), icon_box);

  IndicatorApplet* iapplet = g_new0(IndicatorApplet, 1);
  iapplet->da = da;
  iapplet->num = 0;
  iapplet->applet = applet;
  iapplet->icon_box = icon_box;
  iapplet->images = NULL;
  iapplet->menus = NULL;
  iapplet->shown_images = NULL;
  iapplet->shown_menus = NULL;
  iapplet->awnicons = NULL;
  iapplet->popup_num = -1;
  iapplet->last_num = -1;

  iapplet->config = awn_config_get_default_for_applet(iapplet->applet, NULL);
  update_config(iapplet);

  g_signal_connect(G_OBJECT(applet), "position-changed",
                   G_CALLBACK(applet_position_changed), (gpointer)iapplet);
  g_signal_connect(G_OBJECT(applet), "size-changed",
                   G_CALLBACK(applet_size_changed), (gpointer)iapplet);

  g_signal_connect(G_OBJECT(da), "button-press-event",
                   G_CALLBACK(da_button_press), (gpointer)iapplet);
  g_signal_connect(G_OBJECT(da), "expose-event",
                   G_CALLBACK(da_expose_event), (gpointer)iapplet);
  g_signal_connect(G_OBJECT(da), "scroll-event",
                   G_CALLBACK(da_scroll), (gpointer)iapplet);

  gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), INDICATOR_ICONS_DIR);
  /* Code (mostly) from gnome-panel's indicator-applet-0.3.6/src/applet-main.c */
  if (g_file_test(INDICATOR_DIR, (G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)))
  {
    GDir *dir = g_dir_open(INDICATOR_DIR, 0, NULL);

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL)
    {
      /* Don't load the global menu used in Unity (Ubuntu Netbook Edition) */
      if (!g_strcmp0(name, "libappmenu.so"))
      {
        continue;
      }
      load_module(name, iapplet);
    }
    g_dir_close (dir);
  }
  /* End... */

  update_icon_mode(iapplet);

  return applet;
}
