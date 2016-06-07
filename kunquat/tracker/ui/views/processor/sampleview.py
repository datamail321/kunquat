# -*- coding: utf-8 -*-

#
# Author: Tomi Jylhä-Ollila, Finland 2016
#
# This file is part of Kunquat.
#
# CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
#
# To the extent possible under law, Kunquat Affirmers have waived all
# copyright and related or neighboring rights to Kunquat.
#

import math
import time

from PySide.QtCore import *
from PySide.QtGui import *


class SampleView(QWidget):

    def __init__(self):
        super().__init__()

        self._toolbar = SampleViewToolBar()
        self._area = SampleViewArea()

        v = QVBoxLayout()
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(0)
        v.addWidget(self._toolbar)
        v.addWidget(self._area)
        self.setLayout(v)

        QObject.connect(self._toolbar, SIGNAL('zoomIn()'), self._area.zoom_in)
        QObject.connect(self._toolbar, SIGNAL('zoomOut()'), self._area.zoom_out)

    def set_icon_bank(self, icon_bank):
        self._toolbar.set_icon_bank(icon_bank)

    def set_length(self, length):
        self._area.set_length(length)


class SampleViewToolBar(QToolBar):

    zoomIn = Signal(name='zoomIn')
    zoomOut = Signal(name='zoomOut')

    def __init__(self):
        super().__init__()

        self._zoom_in = QToolButton()
        self._zoom_in.setText('Zoom In')
        self._zoom_in.setToolTip(self._zoom_in.text())

        self._zoom_out = QToolButton()
        self._zoom_out.setText('Zoom Out')
        self._zoom_out.setToolTip(self._zoom_out.text())

        self.addWidget(self._zoom_in)
        self.addWidget(self._zoom_out)

        QObject.connect(self._zoom_in, SIGNAL('clicked()'), self._signal_zoom_in)
        QObject.connect(self._zoom_out, SIGNAL('clicked()'), self._signal_zoom_out)

    def set_icon_bank(self, icon_bank):
        self._zoom_in.setIcon(QIcon(icon_bank.get_icon_path('zoom_in')))
        self._zoom_out.setIcon(QIcon(icon_bank.get_icon_path('zoom_out')))

    def _signal_zoom_in(self):
        QObject.emit(self, SIGNAL('zoomIn()'))

    def _signal_zoom_out(self):
        QObject.emit(self, SIGNAL('zoomOut()'))


class SampleViewArea(QAbstractScrollArea):

    def __init__(self):
        super().__init__()

        self.setViewport(SampleViewCanvas())
        self.viewport().setFocusProxy(None)

        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)

    def set_length(self, length):
        self.viewport().set_length(length)

    def zoom_in(self):
        start, stop = self.viewport().get_range()
        width = stop - start - 1
        shrink = width // 4
        shrink_l, shrink_r = shrink, shrink

        # Make sure that at least 8 frames are visible
        max_shrink_total = width - 7
        if shrink * 2 > max_shrink_total:
            shrink_l = max_shrink_total // 2
            shrink_r = max_shrink_total - shrink_l

        self.set_range(start + shrink_l, stop - shrink_r)

    def zoom_out(self):
        start, stop = self.viewport().get_range()
        length = self.viewport().get_length()
        width = stop - start - 1
        expand = width // 2
        start -= expand
        stop += expand

        # Shift the expanded range if we went out of sample boundaries
        if start < 0:
            shift = -start
            start += shift
            stop += shift
        elif stop > length:
            shift = stop - length
            start -= shift
            stop -= shift

        self.set_range(start, stop)

    def set_range(self, start, stop):
        assert start < stop
        start = max(0, start)
        stop = min(self.viewport().get_length(), stop)
        self.viewport().set_range(start, stop)
        self._update_scrollbars()

    def _update_scrollbars(self):
        scrollbar = self.horizontalScrollBar()

        view = self.viewport()
        length = view.get_length()

        if length == 0:
            scrollbar.setRange(0, 0)
            return

        start, stop = view.get_range()
        width = stop - start
        scrollbar.setPageStep(width)
        scrollbar.setRange(0, length - width)
        scrollbar.setValue(start)

    def paintEvent(self, event):
        self.viewport().paintEvent(event)

    def scrollContentsBy(self, dx, dy):
        value = self.horizontalScrollBar().value()

        view = self.viewport()
        view.set_position(value)

    def mouseMoveEvent(self, event):
        self.viewport().mouseMoveEvent(event)

    def mousePressEvent(self, event):
        self.viewport().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        self.viewport().mouseReleaseEvent(event)

    def resizeEvent(self, event):
        pass


class SampleViewCanvas(QWidget):

    def __init__(self):
        super().__init__()

        self._length = 0

        self._range = [0, 0]

        self.setAutoFillBackground(False)
        self.setAttribute(Qt.WA_OpaquePaintEvent)
        self.setAttribute(Qt.WA_NoSystemBackground)

        self.setMouseTracking(True)

    def set_length(self, length):
        self._length = length
        self.set_range(0, length)
        self.update()

    def get_length(self):
        return self._length

    def set_range(self, start, stop):
        self._range = [start, stop]
        self.update()

    def get_range(self):
        return self._range

    def set_position(self, pos):
        start, stop = self._range
        pos_delta = pos - start
        start += pos_delta
        stop += pos_delta
        assert start >= 0
        assert stop <= self._length
        self._range = [start, stop]
        self.update()

    def paintEvent(self, event):
        start = time.time()

        painter = QPainter(self)
        painter.setBackground(QColor(0, 0, 0))
        painter.eraseRect(0, 0, self.width(), self.height())

        if self._length == 0:
            return

        print(self._range)

        end = time.time()
        elapsed = end - start
        #print('Sample view updated in {:.2f} ms'.format(elapsed * 1000))


