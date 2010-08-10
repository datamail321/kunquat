# -*- coding: utf-8 -*-

#
# Author: Tomi Jylhä-Ollila, Finland 2010
#
# This file is part of Kunquat.
#
# CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
#
# To the extent possible under law, Kunquat Affirmers have waived all
# copyright and related or neighboring rights to Kunquat.
#

import math

from PyQt4 import Qt, QtGui, QtCore

from column import Column
from cursor import Cursor
import kqt_limits as lim
import timestamp as ts


class Pattern(QtGui.QWidget):

    def __init__(self, handle, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.setSizePolicy(QtGui.QSizePolicy.Ignored,
                           QtGui.QSizePolicy.Ignored)
        self.handle = handle
        self.first_column = -1
        self.colours = {
                'bg': QtGui.QColor(0, 0, 0),
                'column_border': QtGui.QColor(0xcc, 0xcc, 0xcc),
                'column_head_bg': QtGui.QColor(0x33, 0x77, 0x22),
                'column_head_fg': QtGui.QColor(0xff, 0xee, 0xee),
                'cursor_bg': QtGui.QColor(0xff, 0x44, 0x22, 0x77),
                'cursor_line': QtGui.QColor(0xff, 0xee, 0x88),
                'ruler_bg': QtGui.QColor(0x11, 0x22, 0x55),
                'ruler_fg': QtGui.QColor(0xaa, 0xcc, 0xff),
                }
        self.fonts = {
                'column_head': QtGui.QFont('Decorative', 10),
                'ruler': QtGui.QFont('Decorative', 8),
                'trigger': QtGui.QFont('Decorative', 10),
                }
        self.length = ts.Timestamp(8)
        self.beat_len = 96
        self.view_start = ts.Timestamp(0)
        self.ruler = Ruler((self.colours, self.fonts))
        self.ruler.set_length(self.length)
        self.ruler.set_beat_len(self.beat_len)
        self.ruler.set_view_start(self.view_start)
        self.columns = [Column(num, None, (self.colours, self.fonts))
                        for num in xrange(-1, lim.COLUMNS_MAX)]
        for col in self.columns:
            col.set_length(self.length)
            col.set_beat_len(self.beat_len)
            col.set_view_start(self.view_start)
        self.cursor = Cursor(self.length, self.beat_len)
        self.columns[0].set_cursor(self.cursor)
        self.view_columns = []

    def set_path(self, path):
        pass

#    def sizeHint(self):
#        return QtCore.QSize(100, 100)

    def paintEvent(self, ev):
        paint = QtGui.QPainter()
        paint.begin(self)
        paint.setBackground(self.colours['bg'])
        paint.eraseRect(ev.rect())
        self.ruler.paint(ev, paint)
        col_pos = self.ruler.width()
        for column in self.view_columns:
            column.paint(ev, paint, col_pos)
            col_pos += column.width()
        paint.end()

    def resizeEvent(self, ev):
        self.view_columns = list(self.get_viewable_columns(ev))
        self.ruler.resize(ev)
        for column in self.view_columns:
            column.resize(ev)

    def get_viewable_columns(self, ev):
        used = self.ruler.width()
        for (width, column) in \
                ((c.width(), c) for c in self.columns[self.first_column + 1:]):
            used += width
            if used > ev.size().width():
                break
            yield column


class Ruler(object):

    def __init__(self, theme):
        self.colours = theme[0]
        self.fonts = theme[1]
        self.height = 0
        self.view_start = ts.Timestamp()
        self.beat_div_base = 2
        self.set_dimensions()

    def get_viewable_positions(self, interval):
        view_end = float(self.view_start) + (self.ruler_height +
                                             self.num_height) / self.beat_len
        view_end = min(view_end, float(self.length))
        error = interval / 2
        pos = math.ceil(float(self.view_start) / interval) * interval
        while pos < view_end:
            if abs(pos - round(pos)) < error:
                pos = round(pos)
            yield pos
            pos += interval

    def paint(self, ev, paint):
        ruler_area = QtCore.QRect(0, 0, self._width, self.height)
        real_area = ev.rect().intersect(ruler_area)
        if real_area.isEmpty() or self.ruler_height <= 0:
            return

        view_start = float(self.view_start)
        view_len = self.ruler_height / self.beat_len
        view_end = view_start + view_len

        # paint background, including start and end borders if visible
        bg_start = self.col_head_height
        bg_end = min(self.height, (float(self.length) - view_start) *
                                   self.beat_len + self.col_head_height)
        paint.setBrush(self.colours['ruler_bg'])
        paint.setPen(QtCore.Qt.NoPen)
        paint.drawRect(0, bg_start, self._width, bg_end - bg_start)
        paint.setPen(self.colours['ruler_fg'])
        if self.view_start == 0:
            paint.drawLine(0, self.col_head_height,
                           self._width - 2, self.col_head_height)
        if bg_end < self.height:
            paint.drawLine(0, bg_end, self._width - 2, bg_end)

        # resolve intervals
        line_min_time = self.line_min_dist / self.beat_len
        line_interval = self.beat_div_base**math.ceil(
                                math.log(line_min_time, self.beat_div_base))
        num_min_time = self.num_min_dist / self.beat_len
        num_interval = self.beat_div_base**math.ceil(
                               math.log(num_min_time, self.beat_div_base))

        # paint ruler lines
        for line_pos in set(self.get_viewable_positions(
                line_interval)).difference(
                        self.get_viewable_positions(num_interval)):
            self.paint_line(line_pos, paint)

        # paint ruler beat numbers
        paint.setFont(self.fonts['ruler'])
        for num_pos in self.get_viewable_positions(num_interval):
            self.paint_number(num_pos, paint)

        # paint right border
        paint.setPen(self.colours['column_border'])
        paint.drawLine(self._width - 1, 0,
                       self._width - 1, self.height - 1)

    def paint_line(self, pos, paint):
        view_start = float(self.view_start)
        view_len = self.ruler_height / self.beat_len
        view_end = view_start + view_len
        y = self.col_head_height + (pos - view_start) * self.beat_len
        if y < self.col_head_height or y >= self.height:
            return
        if pos == 0 or pos == float(self.length):
            return
        x = self._width - 3
        paint.drawLine(x, y, self._width - 1, y)

    def paint_number(self, pos, paint):
        view_start = float(self.view_start)
        view_len = self.ruler_height / self.beat_len
        view_end = view_start + view_len
        y = self.col_head_height + (pos - view_start) * self.beat_len
        if y < self.col_head_height:
            return
        if pos == 0 or pos == float(self.length):
            return
        x = self._width - 6
        if y < self.height:
            paint.drawLine(x, y, self._width - 1, y)
        y -= self.num_height // 2
        if y >= self.height:
            return
        rect = QtCore.QRectF(0, y, self._width - 8, self.num_height)
        text = str(round(pos, 3))
        text = str(int(pos)) if pos == int(pos) else str(round(pos, 3))
        paint.drawText(rect, text, QtGui.QTextOption(QtCore.Qt.AlignRight))

    def resize(self, ev):
        self.height = ev.size().height()
        self.set_dimensions()

    def set_beat_len(self, length):
        self.beat_len = float(length)

    def set_dimensions(self):
        space = QtGui.QFontMetrics(
                    self.fonts['ruler']).boundingRect('00.000')
        self._width = space.width() + 8
        self.num_height = space.height()
        self.num_min_dist = space.height() * 2.0
        self.line_min_dist = 4.0
        self.col_head_height = QtGui.QFontMetrics(
                                   self.fonts['column_head']).height()
        self.ruler_height = self.height - self.col_head_height

    def set_length(self, length):
        self.length = length

    def set_view_start(self, start):
        self.view_start = start

    def width(self):
        return self._width


