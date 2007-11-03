#!/usr/bin/env python

# Copyright (c) 2007 Randal Barlow
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

import sys
import os
import gtk
from gtk import gdk
import gobject
import pango
import gconf
import awn
import cairo
import gnome.ui
import gnomedesktop
import time
import locale
import gettext

from stacks_backend import *
from stacks_backend_file import *
from stacks_backend_folder import *
from plugger_backend import *
from trasher_backend import *
from stacks_config import StacksConfig
from stacks_launcher import LaunchManager
from stacks_icons import IconFactory
from stacks_vfs import VfsUri


APP="Stacks"
DIR="locale"
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP, DIR)
gettext.textdomain(APP)
_ = gettext.gettext


def _to_full_path(path):
    head, tail = os.path.split(__file__)
    return os.path.join(head, path)


"""
Main Applet class
"""
class StacksApplet (awn.AppletSimple):

    # Awn applet
    uid = None
    orient = None
    height = None
    title = None
    effetect = None

    # GConf
    gconf_path = None
    gconf_client = None

    # Structures
    backend = None
    dialog = None
    hbox = None
    table = None
    navbuttons = None

    # Status values
    dialog_visible = False
    context_menu_visible = False
    just_dragged = False
    current_page = -1

    # Basically drop everything to everything
    dnd_targets = [("text/uri-list", 0, 0), ("text/plain", 0, 1)]

    # Default configuration values, are overruled while reading config
    config_cols = 5
    config_rows = 4
    config_fileops = gtk.gdk.ACTION_LINK
    config_icon_size = 48
    config_composite_icon = True
    config_browsing = True
    config_icon_empty = _to_full_path("icons/stacks-drop.svg")
    config_icon_full = _to_full_path("icons/stacks-full.svg")
    config_item_count = True
    config_backend = "file://" +    os.path.join(
                                    os.path.expanduser("~"), 
                                    ".config", "awn", "applets", "stacks")


    def __init__ (self, uid, orient, height):
        awn.AppletSimple.__init__(self, uid, orient, height)

        # initalize variables
        self.uid = uid
        self.orient = orient
        self.height = height
        self.title = awn.awn_title_get_default()
        self.effects = self.get_effects()

        # get GConf client and read configuration
        self.gconf_path = "/apps/avant-window-navigator/applets/" + self.uid
        self.gconf_client = gconf.client_get_default()
        self.gconf_client.notify_add(self.gconf_path, self.backend_gconf_cb)

        # set Backend
        # ensure config path (dir) exists
        try:
            os.mkdir(self.config_backend)
        except OSError: # if file exists
            pass
        self.backend_get_config()

        # connect to events
        self.connect("button-release-event", self.applet_button_cb)
        self.connect("enter-notify-event", self.applet_enter_cb)
        self.connect("leave-notify-event", self.applet_leave_cb)
        self.connect("drag-data-received", self.applet_drop_cb)
        self.connect("drag-motion", self.applet_drag_motion_cb)
        self.connect("drag-leave", self.applet_drag_leave_cb)
        self.connect("orientation-changed", self.applet_orient_changed_cb)
        self.connect("height-changed", self.applet_height_changed_cb)


    """
    Functions concerning the Applet
    """
    # Launch the preferences dialog
    def applet_menu_pref_cb(self, widget):
        cfg = StacksConfig(self)


    # Launch the about dialog
    def applet_menu_about_cb(self, widget):
        cfg = StacksConfig(self)
        cfg.set_current_page(-1)


    # not supported yet, maybe in future
    def applet_orient_changed_cb(self, widget, orient):
        return


    # Bar height changed
    def applet_height_changed_cb(self, widget, height):
        self.height = height
        gobject.idle_add(self.backend_restructure_cb, None, None)


    # On enter -> show the title of the stack
    def applet_enter_cb (self, widget, event):
        title = self.backend.get_title()
        if self.config_item_count:
            n_items = self.backend.get_number_items()
            if n_items > 0:
                title += " (" + str(n_items) + ")"
        self.title.show(self, title)


    # On leave -> hide the title of the stack
    def applet_leave_cb (self, widget, event):
        self.title.hide(self)


    # On mouseclick on applet ->
    # * hide the dialog and show the context menu on button 3
    # * open the backend on button 2
    # * show/hide the dialog on button 1 (if backend not empty) 
    def applet_button_cb(self, widget, event):
        if event.button == 3:
            # right click
            self.dialog_hide()
            # create popup menu
            popup_menu = gtk.Menu()
            # get list of backend specified menu items
            items = self.backend.get_menu_items()
            if items:
                for i in items:
                  popup_menu.append(i)
            popup_menu.append(gtk.SeparatorMenuItem())
            pref_item = gtk.ImageMenuItem(stock_id=gtk.STOCK_PREFERENCES)
            popup_menu.append(pref_item)
            about_item = gtk.ImageMenuItem(stock_id=gtk.STOCK_ABOUT)
            popup_menu.append(about_item)
            pref_item.connect_object("activate",self.applet_menu_pref_cb,self)
            about_item.connect_object("activate",self.applet_menu_about_cb,self)
            popup_menu.show_all()
            popup_menu.popup(None, None, None, event.button, event.time)
        elif event.button == 2:
            # middle click
            self.backend.open()
        else:
            # left click
            if self.dialog_visible:
               self.dialog_hide()
            else:
               if not self.backend.is_empty():
                    self.dialog_show()


    def applet_drag_leave_cb(self, widget, context, time):
        awn.awn_effect_stop(self.effects, "hover")


    def applet_drag_motion_cb(self, widget, context, x, y, time):
        awn.awn_effect_start(self.effects, "hover")
        return True


    # On drag-drop on applet icon ->
    # * add each uri in the list to the backend
    # * set "full" icon as applet icon
    # --
    # For direct feedback "feeling"
    # add drop source to stack immediately,
    # and prevent duplicates @ monitor callback
    def applet_drop_cb(self, widget, context, x, y,
                            selection, targetType, time):
        pixbuf = None
        vfs_uris = []
        for uri in selection.data.split():
            try:
                vfs_uris.append(VfsUri(uri))
            except TypeError:
                pass
        if vfs_uris:
            self.backend.add(vfs_uris, context.action)
        context.finish(True, False, time)
        return True


    # Set the empty icon as applet icon
    def applet_set_empty_icon(self):
        icon = IconFactory().load_icon(self.config_icon_empty, self.height)
        self.set_temp_icon(icon)


    # Set the full icon as applet icon
    def applet_set_full_icon(self, pixbuf):
        icon = IconFactory().load_icon(self.config_icon_full, self.height)
        if self.config_composite_icon and pixbuf:
            pixbuf = IconFactory().scale_to_bounded(pixbuf, self.height)
            cx = (self.height-pixbuf.get_width())/2
            cy = (self.height-pixbuf.get_height())
            trans = gdk.Pixbuf(
                    pixbuf.get_colorspace(),
                    True,
                    pixbuf.get_bits_per_sample(),
                    self.height,
                    self.height)
            trans.fill(0x00000000)
            pixbuf.composite(
                    trans,
                    cx, cy,
                    pixbuf.get_width(),
                    pixbuf.get_height(),
                    cx, cy, 1, 1,
                    gtk.gdk.INTERP_BILINEAR, 255)
            icon.composite(
                    trans, 0,0,
                    self.height, self.height,
                    0, 0, 1, 1,
                    gtk.gdk.INTERP_BILINEAR, 255)
            icon = trans
        self.set_temp_icon(icon)


    # only enable link action if we have a FILE type backend
    def applet_setup_drag_drop(self):
        self.drag_dest_set( gtk.DEST_DEFAULT_ALL,
                            self.dnd_targets,
                            self.config_fileops)


    """
    Functions concerning items in the stack
    """
    def item_clear_cb(self, widget, user_data):
        self.backend.remove(user_data)


    def item_menu_hide_cb(self, widget):
        self.context_menu_visible = False


    # launches the command for a stack icon
    # -distinguishes desktop items
    def item_button_cb(self, widget, event, user_data):
        uri, mimetype = user_data
        if event.button == 3:
            #self.item_context_menu(uri).popup(None, None, None, event.button, event.time)
            pass # for now
        elif event.button == 1:
            if self.just_dragged:
                self.just_dragged = False
            else:
                self.item_activated_cb(None, user_data)


    def item_activated_cb(self, widget, user_data):
        uri, mimetype = user_data
        if uri.as_string().endswith(".desktop"):
            item = gnomedesktop.item_new_from_uri(
                    uri.as_string(), gnomedesktop.LOAD_ONLY_IF_EXISTS)
            if item:
                command = item.get_string(gnomedesktop.KEY_EXEC)
                LaunchManager().launch_command(command, uri.as_string())
        else:
            LaunchManager().launch_uri(uri.as_string(), mimetype)


    def item_drag_data_get(
            self, widget, context, selection, info, time, vfs_uri):
        selection.set_uris([vfs_uri.as_string()])


    def item_drag_begin(self, widget, context):
        self.just_dragged = True


    def item_context_menu(self, uri):
        self.context_menu_visible = True
        context_menu = gtk.Menu()
        del_item = gtk.ImageMenuItem(stock_id=gtk.STOCK_CLEAR)
        context_menu.append(del_item)
        del_item.connect_object("activate", self.item_clear_cb, self, uri)
        context_menu.connect("hide", self.item_menu_hide_cb)
        context_menu.show_all()
        return context_menu


    """
    Functions concerning the Dialog
    """
    # hide the dialog
    def dialog_hide(self):
        if self.dialog_visible is True:
            self.title.hide(self)
            if self.dialog:
                self.dialog.hide()
            self.dialog_visible = False


    # show the dialog
    def dialog_show(self):
        if self.dialog_visible is False:
            self.dialog_visible = True
            self.title.hide(self)
            if self.current_page >= 0:
               self.dialog_show_new(self.current_page)
            else:
               self.dialog_show_new(0)


    def dialog_focus_out(self, widget, event):
        if self.context_menu_visible is False:
            self.dialog_hide()

    def _item_created_cb(self, widget, iter):
        # get values from store
        store = self.backend.get_store()
        vfs_uri, lbl_text, mime_type, icon, button = store.get(
                iter, COL_URI, COL_LABEL, COL_MIMETYPE, COL_ICON, COL_BUTTON)
        if button:
            return button
        # create new button
        button = gtk.Button()
        button.set_relief(gtk.RELIEF_NONE)
        button.drag_source_set( gtk.gdk.BUTTON1_MASK,
                                self.dnd_targets,
                                self.config_fileops)
        button.drag_source_set_icon_pixbuf(icon)
        button.connect( "button-release-event",
                        self.item_button_cb,
                        (vfs_uri, mime_type))
        button.connect( "activate",
                        self.item_activated_cb,
                        (vfs_uri, mime_type))
        button.connect( "drag-data-get",
                        self.item_drag_data_get,
                        vfs_uri)
        button.connect( "drag-begin",
                        self.item_drag_begin)
        # add to vbox
        vbox = gtk.VBox(False, 4)
        button.add(vbox)
        # icon -> button.image
        image = gtk.Image()
        image.set_from_pixbuf(icon)
        image.set_size_request(self.config_icon_size,
                self.config_icon_size)
        vbox.pack_start(image, False, False, 0)
        # label
        label = gtk.Label(lbl_text)
        label.set_justify(gtk.JUSTIFY_CENTER)
        label.set_line_wrap(True)
        # pango layout
        layout = label.get_layout()
        lw, lh = layout.get_size()
        layout.set_width(int(1.5 * self.config_icon_size) * pango.SCALE)
        layout.set_wrap(pango.WRAP_WORD_CHAR)
        layout.set_alignment(pango.ALIGN_CENTER)
        _lbltxt = label.get_text()
        lbltxt = ""
        for i in range(layout.get_line_count()):
            length = layout.get_line(i).length
            lbltxt += str(_lbltxt[0:length]) + '\n'
            _lbltxt = _lbltxt[length:]
        label.set_text(lbltxt)
        label.set_size_request(-1, lh*2/pango.SCALE)
        # add to vbox
        vbox.pack_start(label, True, True, 0)
        vbox.set_size_request(int(1.5 * self.config_icon_size), -1)
        store.set_value(iter, COL_BUTTON, button)
        return button

    def _dialog_restructure_cb(self, widget, page=0):
        store = self.backend.get_store()
        iter = store.iter_nth_child(None, page * self.config_rows * self.config_cols)

        if self.table:
            for item in self.table.get_children():
                self.table.remove(item)
            self.table.destroy()

        if page > 0:
            self.table = gtk.Table(self.config_rows, self.config_cols, True)
        else:
            self.table = gtk.Table(1, 1, True)
        self.table.set_resize_mode(gtk.RESIZE_PARENT)
        self.table.set_row_spacings(0)
        self.table.set_col_spacings(0)

        x=y=0
        theres_more = False
        while iter:
            button = store.get_value(iter, COL_BUTTON)
            t = button.get_parent()
            if t:
                t.remove(button)
            self.table.attach(button, x, x+1, y, y+1)

            x += 1
            if x == self.config_cols:
                x = 0
                y += 1
            if y == self.config_rows:
                theres_more = True
                break
            iter = store.iter_next(iter)

        self.hbox.add(self.table)
        return theres_more


    def dialog_show_prev_page(self, widget):
        self.dialog_show_new(self.current_page-1)


    def dialog_show_next_page(self, widget):
        self.dialog_show_new(self.current_page+1)


    def dialog_show_new(self, page=0):
        assert page >= 0
        self.current_page = page

        # if nothing to show, then return
        if self.backend.is_empty():
            return

        # create new dialog if it does not exists yet
        if not self.dialog:
            self.dialog = awn.AppletDialog (self)
            self.dialog.set_focus_on_map(True)
            self.dialog.connect("focus-out-event", self.dialog_focus_out)
            # TODO: preference -> set title?
            self.dialog.set_title(self.backend.get_title())
            self.hbox = gtk.HBox(False, 0)
            self.dialog.add(self.hbox)

        theres_more = self._dialog_restructure_cb(None, page)
        # if we have more than 1 page and browsing is enabled
        if self.config_browsing and (theres_more or page > 0):
            if self.navbuttons is None:
                buttonbox = gtk.HButtonBox()
                buttonbox.set_layout(gtk.BUTTONBOX_EDGE)
                self.dialog.add(buttonbox)
                bt_left = gtk.Button(stock=gtk.STOCK_GO_BACK)
                bt_left.set_use_stock(True)
                bt_left.set_relief(gtk.RELIEF_NONE)
                bt_left.connect("clicked", self.dialog_show_prev_page)
                buttonbox.add(bt_left)
                bt_right = gtk.Button(stock=gtk.STOCK_GO_FORWARD)
                bt_right.set_use_stock(True)
                bt_right.set_relief(gtk.RELIEF_NONE)
                bt_right.connect("clicked", self.dialog_show_next_page)
                buttonbox.add(bt_right)
                self.navbuttons = (bt_left, bt_right)

            # enable appropriate navigation buttons
            if page > 0:
                self.navbuttons[0].set_sensitive(True)
            else:
                self.navbuttons[0].set_sensitive(False)
            if theres_more:
                self.navbuttons[1].set_sensitive(True)
            else:
                self.navbuttons[1].set_sensitive(False)

        # show everything on the dialog
        self.dialog.show_all()


    """
    Functions concerning the Backend
    """
    def backend_gconf_cb(self, gconf_client, *args, **kwargs):
        self.dialog_hide()
        self.backend_get_config()


    def backend_attention_cb(self, widget, backend_type):
        awn.awn_effect_start(self.effects, "attention")
        time.sleep(1.0)
        awn.awn_effect_stop(self.effects, "attention")


    def backend_restructure_cb(self, widget, pixbuf):
        # setting applet icon
        if self.backend.is_empty():
            self.applet_set_empty_icon()
        else:
            if not pixbuf:
                pixbuf = self.backend.get_random_pixbuf()
            self.applet_set_full_icon(pixbuf)

    def backend_get_config(self):
        # try to get backend from gconf
        _config_backend = self.gconf_client.get_string(
                self.gconf_path + "/backend")
        if _config_backend:
            try: self.config_backend = VfsUri(_config_backend)
            except: pass
        # assume file backend
        if not isinstance(self.config_backend, VfsUri):
            back_uri = VfsUri(self.config_backend).as_uri()
            self.config_backend = VfsUri(back_uri.append_path(self.uid))

        # get dimension
        _config_cols = self.gconf_client.get_int(self.gconf_path + "/cols")
        if _config_cols > 0:
            self.config_cols = _config_cols
        _config_rows = self.gconf_client.get_int(self.gconf_path + "/rows")
        if _config_rows > 0:
            self.config_rows = _config_rows

        # get icon size
        _config_icon_size = self.gconf_client.get_int(
                self.gconf_path + "/icon_size")
        if _config_icon_size > 0:
            self.config_icon_size = _config_icon_size

        # get file operations
        _config_fileops = self.gconf_client.get_int(
                self.gconf_path + "/file_operations")
        if _config_fileops > 0:
            self.config_fileops = _config_fileops

        # get composite icon
        if self.gconf_client.get_bool(self.gconf_path + "/composite_icon"):
            self.config_composite_icon = True
        else:
            self.config_composite_icon = False

        # get browsing
        if self.gconf_client.get_bool(self.gconf_path + "/browsing"):
            self.config_browsing = True
        else:
            self.config_browsing = False

        # get icons
        _config_icon_empty = self.gconf_client.get_string(
                self.gconf_path + "/applet_icon_empty")
        if _config_icon_empty:
            self.config_icon_empty = _config_icon_empty
        _config_icon_full = self.gconf_client.get_string(
                self.gconf_path + "/applet_icon_full")
        if _config_icon_full:
            self.config_icon_full = _config_icon_full

        # get item count
        if self.gconf_client.get_bool(self.gconf_path + "/item_count"):
            self.config_item_count = True
        else:
            self.config_item_count = False

        # setup dnd area
        self.applet_setup_drag_drop()

        # destroy backend
        if self.backend:
            self.backend.destroy()

        # create new backend of specified type
        _config_backend_type = self.gconf_client.get_int(
                self.gconf_path + "/backend_type")
        if _config_backend_type == BACKEND_TYPE_FOLDER:
            self.backend = FolderBackend(self,
                    self.config_backend, self.config_icon_size)
        elif _config_backend_type == BACKEND_TYPE_PLUGGER:
            self.backend = PluggerBackend(self,
                    self.config_backend, self.config_icon_size)
        elif _config_backend_type == BACKEND_TYPE_TRASHER:
            self.backend = TrashBackend(self,
                    self.config_backend, self.config_icon_size)
        else:   # BACKEND_TYPE_FILE:
            self.backend = FileBackend(self,
                    self.config_backend, self.config_icon_size)

        # read the backends contents and connect to its signals
        self.backend.connect("item_created", self._item_created_cb)
        self.backend.read()
        self.backend_restructure_cb(None, None)
        self.backend.connect("attention", self.backend_attention_cb)
        self.backend.connect("restructure", self.backend_restructure_cb)


if __name__ == "__main__":
    awn.init (sys.argv[1:])
    # might needed to request passwords from user
    gnome.ui.authentication_manager_init()
    applet = StacksApplet (awn.uid, awn.orient, awn.height)
    awn.init_applet (applet)
    applet.show_all()
    gtk.main()
