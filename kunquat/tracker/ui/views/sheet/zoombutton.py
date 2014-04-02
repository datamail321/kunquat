# -*- coding: utf-8 -*-

#
# Author: Tomi Jylhä-Ollila, Finland 2014
#
# This file is part of Kunquat.
#
# CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
#
# To the extent possible under law, Kunquat Affirmers have waived all
# copyright and related or neighboring rights to Kunquat.
#

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ZoomButton(QToolButton):

    INFO = {
            'in': ('Zoom In', 'zoom-in', 'Ctrl +'),
            'out': ('Zoom Out', 'zoom-out', 'Ctrl -'),
            'original': ('Zoom to Original', 'zoom-original', 'Ctrl+0'),
        }

    def __init__(self, mode):
        QToolButton.__init__(self)
        self._ui_model = None
        self._updater = None
        self._sheet_manager = None

        self._mode = mode
        self.setText(self._get_text(mode))
        self.setIcon(self._get_icon(mode))
        self.setToolTip(self._get_tooltip(mode))
        self.setAutoRaise(True)

    def set_ui_model(self, ui_model):
        self._ui_model = ui_model
        self._updater = ui_model.get_updater()
        self._updater.register_updater(self._perform_updates)
        self._sheet_manager = ui_model.get_sheet_manager()

        self._update_enabled()
        QObject.connect(self, SIGNAL('clicked()'), self._clicked)

    def unregister_updaters(self):
        self._updater.unregister_updater(self._perform_updates)

    def _perform_updates(self, signals):
        if 'signal_sheet_zoom' in signals:
            self._update_enabled()
        if 'signal_sheet_zoom_range' in signals:
            self._update_enabled()

    def _update_enabled(self):
        zoom = self._sheet_manager.get_zoom()
        if self._mode == 'in':
            _, maximum = self._sheet_manager.get_zoom_range()
            is_enabled = zoom < maximum
        elif self._mode == 'out':
            minimum, _ = self._sheet_manager.get_zoom_range()
            is_enabled = zoom > minimum
        else:
            is_enabled = zoom != 0

        self.setEnabled(is_enabled)

    def _get_text(self, mode):
        return ZoomButton.INFO[mode][0]

    def _get_icon(self, mode):
        return QIcon.fromTheme(ZoomButton.INFO[mode][1])

    def _get_shortcut(self, mode):
        return ZoomButton.INFO[mode][2]

    def _get_tooltip(self, mode):
        return '{} ({})'.format(self._get_text(mode), self._get_shortcut(mode))

    def _clicked(self):
        new_zoom = 0
        if self._mode == 'in':
            new_zoom = self._sheet_manager.get_zoom() + 1
        elif self._mode == 'out':
            new_zoom = self._sheet_manager.get_zoom() - 1

        self._sheet_manager.set_zoom(new_zoom)


