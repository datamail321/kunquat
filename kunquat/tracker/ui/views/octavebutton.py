# -*- coding: utf-8 -*-

#
# Author: Toni Ruottu, Finland 2013
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


class OctaveButton(QPushButton):

    def __init__(self, octave_id):
        QPushButton.__init__(self)
        self._octave_id = octave_id
        self._typewriter_manager = None

        self.setCheckable(True)
        self.setMinimumWidth(60)
        self.setMinimumHeight(60)
        layout = QVBoxLayout(self)
        octavename = QLabel()
        self._octavename = octavename
        octavename.setAlignment(Qt.AlignCenter)
        layout.addWidget(octavename)
        layout.setAlignment(Qt.AlignCenter)

        self.setStyleSheet("QLabel { background-color: #ccc; }")

        QObject.connect(self, SIGNAL('clicked()'), self._select_octave)

    def _select_octave(self):
        self._typewriter_manager.set_octave(self._octave_id)

    def set_ui_model(self, ui_model):
        updater = ui_model.get_updater()
        updater.register_updater(self.perform_updates)
        self._typewriter_manager = ui_model.get_typewriter_manager()
        octave_name = self._typewriter_manager.get_octave_name(self._octave_id)
        self._octavename.setText('%s' % octave_name)

    def _update_pressed(self):
        octave = self._typewriter_manager.get_octave()
        old_block = self.blockSignals(True)
        if octave == self._octave_id:
            self.setChecked(True)
        else:
            self.setChecked(False)
        self.blockSignals(old_block)

    def perform_updates(self, signals):
        if 'signal_octave' in signals:
            self._update_pressed()
