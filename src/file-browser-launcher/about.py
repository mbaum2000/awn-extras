#!/usr/bin/env python
# -*- coding: utf-8 -*-
import pygtk
pygtk.require('2.0')
import gtk

#Simple about dialog window
class About:
	def __init__(self):
		win = gtk.AboutDialog()
		win.set_name("File Browser Launcher")
		win.set_copyright("Copyright 2007 sharkbaitbobby <sharkbaitbobby+awn@gmail.com>")
		win.set_authors(["sharkbaitbobby <sharkbaitbobby+awn@gmail.com>"])
		win.set_comments("A customizable launcher for browsing your files.")
		win.set_license("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.")
		win.set_wrap_license(True)
		win.set_website("http://ultimate-theme-manager.googlecode.com")
		win.set_documenters(["sharkbaitbobby <sharkbaitbobby+awn@gmail.com>"])
		win.set_artists(["GTK Stock Icons"])
		win.run()
		win.destroy()
