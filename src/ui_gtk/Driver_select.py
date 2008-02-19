# coding=utf-8


# Copyright 2008 Tomi Jylhä-Ollila
#
# This file is part of Kunquat.
#
# Kunquat is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Kunquat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Kunquat.  If not, see <http://www.gnu.org/licenses/>.


import pygtk
pygtk.require('2.0')
import gtk
import gobject

import liblo


class Driver_select(gtk.HBox):

	def set_drivers(self, path, args):
		self.driver_list.clear()
		for arg in ['No sound'] + args:
			iter = self.driver_list.append()
			self.driver_list.set(iter, 0, arg)
		selection = self.driver_view.get_selection()
		selection.select_path(0)
		self.cur_driver = -1
		liblo.send(self.engine, '/kunquat/active_driver')

	def driver_init(self, path, args, types):
		if types[0] == 's' and args[0] == 'Error:':
			self.cur_driver = -1
			selection = self.driver_view.get_selection()
			selection.select_path(0)
			return
		self.cur_driver = args[0]
		self.hz.set_text(str(args[1]))

	def active_driver(self, path, args):
		cur = args[0] + 1
		selection = self.driver_view.get_selection()
		if not selection.path_is_selected(cur):
			self.update_table = True
		selection.select_path(cur)
		self.cur_driver = args[0]
		self.hz.set_text(str(args[1]))

	def set_driver(self, widget, data = None):
		_, cur = widget.get_selected_rows()
		cur = cur[0][0] - 1
		if self.cur_driver >= 0 and not self.update_table:
			liblo.send(self.engine, '/kunquat/driver_close')
		self.cur_driver = -1
		self.hz.set_text('0')
		if cur >= 0 and not self.update_table:
			liblo.send(self.engine, '/kunquat/driver_init', cur)
		self.cur_driver = cur
		if self.update_table:
			self.update_table = False

	def __init__(self, engine, server):
		self.engine = engine
		self.server = server

		self.update_table = False
		
		self.server.add_method('/kunquat_gtk/drivers', None, self.set_drivers)
		self.server.add_method('/kunquat_gtk/active_driver', 'ii', self.active_driver)
		self.server.add_method('/kunquat_gtk/driver_init', None, self.driver_init)

		gtk.HBox.__init__(self)

		self.driver_list = gtk.ListStore(gobject.TYPE_STRING)
		self.driver_view = gtk.TreeView(self.driver_list)
		selection = self.driver_view.get_selection()
		selection.connect('changed', self.set_driver)
		
		iter = self.driver_list.append()
		self.driver_list.set(iter, 0, 'No sound')
		self.cur_driver = -1

		cell = gtk.CellRendererText()
		column = gtk.TreeViewColumn('Drivers', cell, text = 0)
		self.driver_view.append_column(column)

		driver_scroll = gtk.ScrolledWindow()
		driver_scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		driver_scroll.add(self.driver_view)
		self.driver_view.show()

		self.pack_start(driver_scroll)
		driver_scroll.show()

		self.hz = gtk.Label('0')
		self.pack_end(self.hz)
		self.hz.show()

