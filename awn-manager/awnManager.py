#!/usr/bin/env python
#
#  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
#
#  Author: Ryan Rushton <ryan@rrdesign.ca>
#
#  Notes: Avant Window Navigator Manager

import sys, os, time
try:
    import pygtk
    pygtk.require("2.0")
except:
  	pass
try:
    import gtk
    import gtk.glade
except:
	sys.exit(1)

import gconf
import gnomedesktop
import gtk.gdk as gdk

from awnTheme import AwnThemeManager
from awnPreferences import awnPreferences
from awnApplet import awnApplet
from awnLauncher import awnLauncher

APP = 'avant-window-navigator'
DIR = '/usr/share/locale'
I18N_DOMAIN = "avant-window-navigator"

import locale
import gettext
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP, DIR)
gettext.textdomain(APP)
_ = gettext.gettext

class AwnManager:

    def __init__(self):

        self.AWN_CONFIG_DIR = os.path.join(os.path.expanduser('~'),'.config/awn')
        if not os.path.exists(self.AWN_CONFIG_DIR):
            os.makedirs(self.AWN_CONFIG_DIR)

        self.GLADE_PATH = os.path.join(sys.path[0], "window.glade")
        gtk.glade.bindtextdomain(APP, DIR)
        gtk.glade.textdomain(APP)

        self.wTree = gtk.glade.XML(self.GLADE_PATH, domain=I18N_DOMAIN)

        self.window = self.wTree.get_widget("main_window")
        self.window.connect("delete-event", gtk.main_quit)

        close = self.wTree.get_widget("closebutton")
        close.connect("clicked", gtk.main_quit)

        #menu
        self.treeview =  self.wTree.get_widget("view_menu")
        self.treeview.get_selection().connect("changed", self.menu_row_clicked)
        self.make_menu_model()

        #theme
        self.themeManager = AwnThemeManager(self.wTree, self.AWN_CONFIG_DIR)

        #applet
        self.appletManager = awnApplet(self.wTree)

        #launcher
        self.launchManager = awnLauncher(self.wTree, self.AWN_CONFIG_DIR)

        #preferences
        self.prefManager = awnPreferences(self.wTree)

    def menu_row_clicked(self, data=None):
        selection = self.treeview.get_selection()
        (model, iter) = selection.get_selected()
        if iter != None:
            self.show_panel(model.get_path(iter)[0])

    def show_panel(self, index):
        for panel in ['panel_preferences','panel_applets','panel_launchers','panel_themes']:
            self.wTree.get_widget(panel).hide()
        if index == 0:
            self.wTree.get_widget('panel_preferences').show()
        elif index == 1:
            self.wTree.get_widget('panel_applets').show()
        elif index == 2:
            self.wTree.get_widget('panel_launchers').show()
        elif index == 3:
            self.wTree.get_widget('panel_themes').show()


    def make_menu_model (self):
        self.model = model = gtk.ListStore(gdk.Pixbuf, str, str, str)
        self.treeview.set_model (model)

        ren = gtk.CellRendererPixbuf()
        col = gtk.TreeViewColumn ("Pixbuf", ren, pixbuf=0)
        self.treeview.append_column (col)

        ren = gtk.CellRendererText()
        col = gtk.TreeViewColumn ("Name", ren, markup=1)
        self.treeview.append_column (col)

        theme = gtk.icon_theme_get_default()

        row = self.model.append ()
        self.model.set_value (row, 0, theme.load_icon (gtk.STOCK_PREFERENCES, 32, 0))
        self.model.set_value (row, 1, "General")

        row = self.model.append ()
        self.model.set_value (row, 0, theme.load_icon (gtk.STOCK_SORT_ASCENDING, 32, 0))
        self.model.set_value (row, 1, "Applets")

        row = self.model.append ()
        self.model.set_value (row, 0, theme.load_icon (gtk.STOCK_FULLSCREEN, 32, 0))
        self.model.set_value (row, 1, "Launchers")

        row = self.model.append ()
        self.model.set_value (row, 0, theme.load_icon (gtk.STOCK_HOME, 32, 0))
        self.model.set_value (row, 1, "Themes")

        path = self.model.get_path(self.model.get_iter_first())
        self.treeview.set_cursor(path, focus_column=None, start_editing=False)

    def main(self):
        gtk.main()

if __name__ == "__main__":
    gettext.textdomain(I18N_DOMAIN)
    gtk.glade.bindtextdomain(I18N_DOMAIN, "/usr/share/locale")
    awnmanager = AwnManager()
    awnmanager.main()