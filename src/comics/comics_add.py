# -*- coding: utf-8 -*-

# Copyright (c) 2008 Moses Palmér
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


# Libraries used
import gobject
import gtk
import gtk.glade
import os
import re
import tempfile

# Symbols used
from locale import gettext as _

# Local
from feed import Feed
from qfeed import QueryFeed, NAME, URL, TYPE, IMG_INDEX
from downloader import Downloader
from shared import SHARE_DIR, USER_DIR, SYS_FEEDS_DIR, USER_FEEDS_DIR, \
	GLADE_DIR, PIXMAPS_DIR, feeds
from settings import Settings

GLADE_FILE = os.path.join(GLADE_DIR, 'add.glade')

URL_RE = re.compile('.*?://.*?/.*', re.IGNORECASE)


class ComicsAdder:
	"""A program to add image feeds."""
	
	__name__ = 'Comics!'
	__version__ = '1.0'
	__author__ = 'Moses'

	########################################################################
	# Helper methods                                                       #
	########################################################################
	
	def process_error(self, result):
		if result == Feed.DOWNLOAD_FAILED:
			self.xml.get_widget('intro_label').set_text(
				_('Failed to download %s. Press "next" to try again.') \
					% self.url)
			self.assistant.set_current_page(0)
			return False
		
		elif result == Feed.DOWNLOAD_NOT_FEED:
			self.xml.get_widget('intro_label').set_text(
				_('The URL is not a valid comic feed. Press "next" to try again'))
			self.assistant.set_current_page(0)
			return False
		
		return True
		
	########################################################################
	# Standard python methods                                              #
	########################################################################
	
	def __init__(self):
		"""Create a new ComicsAdder instance."""
		self.name = ''
		self.url = ''
		self.feed = None
		self.__update = None
		
		# Connect dialogue events
		self.xml = gtk.glade.XML(GLADE_FILE)
		self.xml.signal_autoconnect(self)
		self.assistant = self.xml.get_widget('add_assistant')
		self.image_list = self.xml.get_widget('image_list')
		
		self.model = gtk.ListStore(gobject.TYPE_PYOBJECT, gtk.gdk.Pixbuf)
		self.image_list.set_model(self.model)
		
		# Show window
		self.assistant.set_page_complete(self.xml.get_widget('intro_page'),
			True)
		self.assistant.set_page_complete(self.xml.get_widget('last_page'),
			True)
		self.assistant.show()
	
	########################################################################
	# Event hooks                                                          #
	########################################################################
	
	def on_add_assistant_close(self, widget):
		if self.__update:
			self.feed.disconnect(self.__update)
		self.assistant.destroy()
	
	def on_add_assistant_cancel(self, widget):
		if self.__update:
			self.feed.disconnect(self.__update)
		self.assistant.destroy()
	
	def on_add_assistant_apply(self, widget):
		global feeds
		
		try:
			os.mkdir(USER_FEEDS_DIR)
		except:
			pass
		f, filename = tempfile.mkstemp('.feed', '', USER_FEEDS_DIR, True)
		os.close(f)
		settings = Settings(filename)
		settings.description  = 'Automatically generated by comics'
		settings[NAME] = self.name
		settings[URL] = self.url
		if self.image.is_indirect:
			settings[TYPE] = 'indirect'
			settings.comments[TYPE] = 'This is an indirect feed'
		
		if self.image.img_index > 1:
			settings[IMG_INDEX] = self.image.img_index + 1
			settings.comments[IMG_INDEX] = \
				'The index of the image in the HTML-code'
		
		settings.save()
		feeds.add_feed(filename)
	
	def on_add_assistant_prepare(self, widget, page):
		if page is self.xml.get_widget('url_page'):
			self.xml.get_widget('wait_page').show()
		
		elif page is self.xml.get_widget('wait_page'):
			self.assistant.set_page_complete(page, False)
			self.url = self.xml.get_widget('url_entry').get_text()
			
			self.xml.get_widget('wait_label').set_markup(
				_('Connecting to <i>%s</i>...') % self.url)
			
			# Download feed
			self.feed = QueryFeed(self.url)
			self.__update = self.feed.connect('updated', self.on_feed_updated)
			self.feed.update()
		
		elif page is self.xml.get_widget('image_page'):
			# Download images for the image list
			self.model.clear()
			for image in self.feed.images:
				iterator = self.model.append((image, None))
				downloader = Downloader(image.url)
				downloader.connect('completed',
					self.on_image_download_completed, iterator)
				downloader.download()
	
	def on_url_entry_changed(self, widget):
		text = widget.get_text()
		self.assistant.set_page_complete(self.xml.get_widget('url_page'),
			not URL_RE.match(text) is None)
	
	def on_image_list_unselect_all(self, widget):
		self.assistant.set_page_complete(self.xml.get_widget('image_page'),
			False)
	
	def on_image_list_selection_changed(self, widget):
		selection = self.image_list.get_selected_items()
		if len(selection) == 1:
			self.assistant.set_page_complete(self.xml.get_widget('image_page'),
				True)
			self.image = self.model.get_value(
				self.model.get_iter(selection[0]), 0)
		else:
			self.assistant.set_page_complete(self.xml.get_widget('image_page'),
				False)
	
	def on_feed_updated(self, feed, result):
		if not self.process_error(result):
			return
		
		self.name = feed.name
		
		last_label = self.xml.get_widget('last_label')
		last_label.set_markup(_('This guide has finished.\n\nPress "apply" to add <i>%s</i>.') \
			% self.name)
		
		# Can we skip the image page?
		if len(feed.images) > 1:
			self.xml.get_widget('image_page').show()
		else:
			self.xml.get_widget('image_page').hide()
			self.image = feed.images[0]
		
		# Complete page
		self.assistant.set_page_complete(self.xml.get_widget('wait_page'), True)
		self.xml.get_widget('wait_label').set_markup(
				_('Found <b>%(name)s</b>!\n\n<i>%(description)s</i>')
				% {'name': self.name,
					'description': self.feed.description})
	
	def on_image_download_completed(self, o, code, iterator):
		"""Update the image list with a newly downloaded image."""
		if code != Downloader.DOWNLOAD_OK:
			self.model.remove(iterator)
		else:
			pixbuf = gtk.gdk.pixbuf_new_from_file(o.filename)
			os.remove(o.filename)
			
			self.model.set_value(iterator, 1, pixbuf)

