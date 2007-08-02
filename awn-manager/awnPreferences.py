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
#  Author: Neil Jagdish Patel <njpatel@gmail.com>
#  Edited By: Ryan Rushton <ryan@rrdesign.ca>
#
#  Notes: Avant Window Navigator preferences window

import sys, os
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

APP = 'avant-window-navigator'
DIR = '/usr/share/locale'
I18N_DOMAIN = "avant-window-navigator"

import locale
import gettext
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain(APP, DIR)
gettext.textdomain(APP)
_ = gettext.gettext

def dec2hex(n):
	"""return the hexadecimal string representation of integer n"""
	n = int(n)
	if n == 0:
		return "00"
	return "%0.2X" % n

def hex2dec(s):
	"""return the integer value of a hexadecimal string s"""
	return int(s, 16)

def make_color(hexi):
	"""returns a gtk.gdk.Color from a hex string RRGGBBAA"""
	color = gtk.gdk.color_parse('#' + hexi[:6])
	alpha = hex2dec(hexi[6:])
	alpha = (float(alpha)/255)*65535
	return color, int(alpha)

def make_color_string(color, alpha):
	"""makes avant-readable string from gdk.color & alpha (0-65535) """
	string = ""

	string = string + dec2hex(int( (float(color.red) / 65535)*255))
	string = string + dec2hex(int( (float(color.green) / 65535)*255))
	string = string + dec2hex(int( (float(color.blue) / 65535)*255))
	string = string + dec2hex(int( (float(alpha) / 65535)*255))

	#hack
	return string

# GCONF KEYS
AWM_PATH		    = "/apps/avant-window-navigator"
AWM_AUTO_HIDE		= "/apps/avant-window-navigator/auto_hide"			#bool
AWM_PANEL_MODE		= "/apps/avant-window-navigator/panel_mode"			#bool

BAR_PATH		    = "/apps/avant-window-navigator/bar"
BAR_ROUNDED_CORNERS	= "/apps/avant-window-navigator/bar/rounded_corners"		# bool
BAR_CORNER_RADIUS 	= "/apps/avant-window-navigator/bar/corner_radius" 		# float
BAR_RENDER_PATTERN	= "/apps/avant-window-navigator/bar/render_pattern"		# bool
BAR_PATTERN_URI		= "/apps/avant-window-navigator/bar/pattern_uri" 		# string
BAR_PATTERN_ALPHA 	= "/apps/avant-window-navigator/bar/pattern_alpha" 		# float
BAR_GLASS_STEP_1	= "/apps/avant-window-navigator/bar/glass_step_1"		#string
BAR_GLASS_STEP_2	= "/apps/avant-window-navigator/bar/glass_step_2"		#string
BAR_GLASS_HISTEP_1	= "/apps/avant-window-navigator/bar/glass_histep_1"		#string
BAR_GLASS_HISTEP_2	= "/apps/avant-window-navigator/bar/glass_histep_2"		#string
BAR_BORDER_COLOR	= "/apps/avant-window-navigator/bar/border_color"		#string
BAR_HILIGHT_COLOR	= "/apps/avant-window-navigator/bar/hilight_color"		#string
BAR_SHOW_SEPARATOR	= "/apps/avant-window-navigator/bar/show_separator"		#bool
BAR_SEP_COLOR		= "/apps/avant-window-navigator/bar/sep_color"

BAR_HEIGHT		    = "/apps/avant-window-navigator/bar/bar_height"			#int
BAR_ANGLE		    = "/apps/avant-window-navigator/bar/bar_angle"			#int
BAR_ICON_OFFSET	    = "/apps/avant-window-navigator/bar/icon_offset"		#int

WINMAN_PATH		    = "/apps/avant-window-navigator/window_manager"
WINMAN_SHOW_ALL_WINS	= "/apps/avant-window-navigator/window_manager/show_all_windows" #bool

APP_PATH		    = "/apps/avant-window-navigator/app"
APP_ACTIVE_PNG		= "/apps/avant-window-navigator/app/active_png" 		#string
APP_ARROW_COLOR		= "/apps/avant-window-navigator/app/arrow_color" 		#color
APP_TASKS_H_ARROWS	= "/apps/avant-window-navigator/app/tasks_have_arrows" 		#bool
APP_FADE_EFFECT		= "/apps/avant-window-navigator/app/fade_effect" 		#bool

TITLE_PATH		    = "/apps/avant-window-navigator/title"
TITLE_TEXT_COLOR	= "/apps/avant-window-navigator/title/text_color" 		#color
TITLE_SHADOW_COLOR	= "/apps/avant-window-navigator/title/shadow_color" 		#color
TITLE_BACKGROUND	= "/apps/avant-window-navigator/title/background" 		#color
TITLE_ITALIC		= "/apps/avant-window-navigator/title/italic" 			#bool
TITLE_BOLD		    = "/apps/avant-window-navigator/title/bold" 			#bool
TITLE_FONT_SIZE		= "/apps/avant-window-navigator/title/font_size" 		#float

DATA_DIR = "/usr/share/avant-window-navigator"
EMPTY = "none";

#layout = { "Flat bar": {"barangle": [self.wTree.get_widget("barangle"), "hidden", 0], "roundedcornerscheck": [self.wTree.get_widget("roundedcornerscheck"),"visible"]},
#                   "3D look" : {"barangle": [self.wTree.get_widget("barangle"), "visible", 45], "roundedcornerscheck": [self.wTree.get_widget("roundedcornerscheck"),"hidden"]}}
#self.prefManager.setup_dropdown(self.wTree.get_widget("bartype"), layout)

class awnPreferences:
    """This is the main class, duh"""

    def __init__(self, wTree):
        self.wTree = wTree

        self.client = gconf.client_get_default()
        self.client.add_dir(BAR_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.add_dir(WINMAN_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.add_dir(APP_PATH, gconf.CLIENT_PRELOAD_NONE)
        self.client.add_dir(TITLE_PATH, gconf.CLIENT_PRELOAD_NONE)

        self.setup_bool (AWM_AUTO_HIDE, self.wTree.get_widget("autohide"))
        self.setup_bool (AWM_PANEL_MODE, self.wTree.get_widget("panelmode"))
        self.setup_bool (BAR_RENDER_PATTERN, self.wTree.get_widget("patterncheck"))
        self.setup_bool (BAR_ROUNDED_CORNERS, self.wTree.get_widget("roundedcornerscheck"))
        self.setup_bool (WINMAN_SHOW_ALL_WINS, self.wTree.get_widget("allwindowscheck"))
        #self.setup_bool (TITLE_ITALIC, self.wTree.get_widget ("italiccheck"))
        #self.setup_bool (TITLE_BOLD, self.wTree.get_widget("boldcheck"))
        self.setup_bool (BAR_SHOW_SEPARATOR, self.wTree.get_widget("separatorcheck"))
        self.setup_bool (APP_TASKS_H_ARROWS, self.wTree.get_widget("arrowcheck"))
        self.setup_bool (APP_FADE_EFFECT, self.wTree.get_widget("fadeeffect"))

        self.setup_chooser(APP_ACTIVE_PNG, self.wTree.get_widget("activefilechooser"))
        self.setup_chooser(BAR_PATTERN_URI, self.wTree.get_widget("patternchooserbutton"))

        #self.setup_spin(TITLE_FONT_SIZE, self.wTree.get_widget("fontsizespin"))
        #self.setup_font(TITLE_FONT_FACE, self.wTree.get_widget("selectfontface"))
        self.setup_scale (BAR_ICON_OFFSET, self.wTree.get_widget("bariconoffset"))
        self.setup_scale (BAR_HEIGHT, self.wTree.get_widget("barheight"))
        self.setup_scale (BAR_ANGLE, self.wTree.get_widget("barangle"))

        self.setup_scale(BAR_PATTERN_ALPHA, self.wTree.get_widget("patternscale"))

        self.setup_color(TITLE_TEXT_COLOR, self.wTree.get_widget("textcolor"))
        self.setup_color(TITLE_SHADOW_COLOR, self.wTree.get_widget("shadowcolor"))
        self.setup_color(TITLE_BACKGROUND, self.wTree.get_widget("backgroundcolor"))

        self.setup_color(BAR_BORDER_COLOR, self.wTree.get_widget("mainbordercolor"))
        self.setup_color(BAR_HILIGHT_COLOR, self.wTree.get_widget("internalbordercolor"))

        self.setup_color(BAR_GLASS_STEP_1, self.wTree.get_widget("gradientcolor1"))
        self.setup_color(BAR_GLASS_STEP_2, self.wTree.get_widget("gradientcolor2"))

        self.setup_color(BAR_GLASS_HISTEP_1, self.wTree.get_widget("highlightcolor1"))
        self.setup_color(BAR_GLASS_HISTEP_1, self.wTree.get_widget("highlightcolor2"))

        self.setup_color(BAR_SEP_COLOR, self.wTree.get_widget("sepcolor"))
        self.setup_color(APP_ARROW_COLOR, self.wTree.get_widget("arrowcolor"))

    def refresh(self, button):
        w = gtk.Window()
        i = gtk.IconTheme()
        w.set_icon(i.load_icon("gtk-refresh", 48, gtk.ICON_LOOKUP_FORCE_SVG))
        v = gtk.VBox()
        l = gtk.Label("Refreshed")
        b = gtk.Button(stock="gtk-close")
        b.connect("clicked", self.win_destroy, w)
        v.pack_start(l, True, True, 2)
        v.pack_start(b)
        w.add(v)
        w.resize(200, 100)
        w.show_all()

    def setup_color(self, key, colorbut):
        color, alpha = make_color(self.client.get_string(key))
        colorbut.set_color(color)
        colorbut.set_alpha(alpha)
        colorbut.connect("color-set", self.color_changed, key)

    def color_changed(self, colorbut, key):
        string =  make_color_string(colorbut.get_color(), colorbut.get_alpha())
        self.client.set_string(key, string)

    def setup_dropdown(self, combox, layout):
        liststore = gtk.ListStore(str)
        combox.set_model(liststore)
        cell = gtk.CellRendererText()
        combox.pack_start(cell, True)
        combox.add_attribute(cell, 'text', 0)
        combox.connect("changed", self.dropdown_changed, combox, layout)
        self.loading_done = False
        for item in layout:
            combox.append_text(item)
        if self.client.get_bool(self.BAR_ROUNDED_CORNERS):
            combox.set_active(1)
        self.loading_done = True

    def dropdown_changed(self, something, combox, layout):
        #pass
        '''
        if self.loading_done:
            active = combox.get_active_text()
            for item in layout:
                if active == item:
                    self.client.set_int(self.BAR_ANGLE, int(layout[item]["barangle"][2]))
                    self.scale_changed(layout[item]["barangle"][0],self.BAR_ANGLE)
                    if layout[item]["barangle"][1] == "hidden":
                        layout[item]["barangle"][0].hide()
                    else:
                        layout[item]["barangle"][0].show()
                    if layout[item]["roundedcornerscheck"][1] == "hidden":
                        print layout[item]["roundedcornerscheck"][0]
                        layout[item]["roundedcornerscheck"][0].hide()
                    else:
                        layout[item]["roundedcornerscheck"][0].show()
        '''

    def setup_scale(self, key, scale):
        type = self.client.get(key).type
        if type == gconf.VALUE_INT:
            val = self.client.get_int(key)
        elif type == gconf.VALUE_FLOAT:
            val = self.client.get_float(key)
            val = 100 - (val * 100)
        scale.set_value(val)
        scale.connect("value-changed", self.scale_changed, key)

    def scale_changed(self, scale, key):
        print scale
        print key
        type = self.client.get(key).type
        if type == gconf.VALUE_INT:
            self.client.set_int(key, int(scale.get_value()))
        elif type == gconf.VALUE_FLOAT:
            val = scale.get_value()
            val = 100 - val
            if (val):
                val = val/100
            self.client.set_float(key, val)

    def setup_spin(self, key, spin):
        spin.set_value(	self.client.get_float(key))

    def spin_changed(self, spin, key):
        self.client.set_float(key, spin.get_value())

    def setup_chooser(self, key, chooser):
        """sets up png choosers"""
        fil = gtk.FileFilter()
        fil.set_name("PNG Files")
        fil.add_pattern("*.png")
        fil.add_pattern("*.PNG")
        chooser.add_filter(fil)
        preview = gtk.Image()
        chooser.set_preview_widget(preview)
        chooser.connect("update-preview", self.update_preview, preview)
        chooser.set_filename(self.client.get_string(key))
        chooser.connect("selection-changed", self.chooser_changed, key)

    def chooser_changed(self, chooser, key):
        f = chooser.get_filename()
        if f == None:
            return
        self.client.set_string(key, f)

    def update_preview(self, chooser, preview):
        f = chooser.get_preview_filename()
        try:
            pixbuf = gtk.gdk.pixbuf_new_from_file_at_size(f, 128, 128)
            preview.set_from_pixbuf(pixbuf)
            have_preview = True
        except:
            have_preview = False
        chooser.set_preview_widget_active(have_preview)

    def setup_bool(self, key, check):
        """sets up checkboxes"""
        check.set_active(self.client.get_bool(key))
        check.connect("toggled", self.bool_changed, key)

    def bool_changed(self, check, key):
        self.client.set_bool(key, check.get_active())
        print "toggled"

    def setup_font(self, key, font_btn):
        """sets up font chooser"""
        font_btn.set_font_name(self.client.get_string(key))
        font_btn.connect("font-set", self.font_changed, key)

    def font_changed(self, font_btn, key):
        self.client.set_string(key, font_btn.get_font_name())