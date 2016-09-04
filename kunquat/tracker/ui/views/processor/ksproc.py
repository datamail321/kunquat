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

from PySide.QtCore import *
from PySide.QtGui import *

from kunquat.tracker.ui.views.audio_unit.time_env import TimeEnvelope
from kunquat.tracker.ui.views.envelope import Envelope
from kunquat.tracker.ui.views.headerline import HeaderLine
from .procnumslider import ProcNumSlider
from . import utils


class KsProc(QWidget):

    @staticmethod
    def get_name():
        return 'Karplus-Strong'

    def __init__(self):
        super().__init__()
        self._damp = DampSlider()
        self._init_env = InitEnvelope()
        self._shift_env = ShiftEnvelope()
        self._shift_threshold = ShiftThreshold()
        self._shift_var = ShiftVar()
        self._release_env = ReleaseEnvelope()
        self._release_var = ReleaseVar()

        sliders = QGridLayout()
        sliders.addWidget(QLabel('Damp:'), 0, 0)
        sliders.addWidget(self._damp, 0, 1)

        shift_params = QHBoxLayout()
        shift_params.setContentsMargins(0, 0, 0, 0)
        shift_params.addWidget(self._shift_threshold)
        shift_params.addWidget(self._shift_var)

        v = QVBoxLayout()
        v.addLayout(sliders)
        v.addWidget(self._init_env)
        v.addWidget(self._shift_env)
        v.addLayout(shift_params)
        v.addWidget(self._release_env)
        v.addWidget(self._release_var)
        self.setLayout(v)

    def set_au_id(self, au_id):
        self._damp.set_au_id(au_id)
        self._init_env.set_au_id(au_id)
        self._shift_env.set_au_id(au_id)
        self._shift_threshold.set_au_id(au_id)
        self._shift_var.set_au_id(au_id)
        self._release_env.set_au_id(au_id)
        self._release_var.set_au_id(au_id)

    def set_proc_id(self, proc_id):
        self._damp.set_proc_id(proc_id)
        self._init_env.set_proc_id(proc_id)
        self._shift_env.set_proc_id(proc_id)
        self._shift_threshold.set_proc_id(proc_id)
        self._shift_var.set_proc_id(proc_id)
        self._release_env.set_proc_id(proc_id)
        self._release_var.set_proc_id(proc_id)

    def set_ui_model(self, ui_model):
        self._damp.set_ui_model(ui_model)
        self._init_env.set_ui_model(ui_model)
        self._shift_env.set_ui_model(ui_model)
        self._shift_threshold.set_ui_model(ui_model)
        self._shift_var.set_ui_model(ui_model)
        self._release_env.set_ui_model(ui_model)
        self._release_var.set_ui_model(ui_model)

    def unregister_updaters(self):
        self._release_var.unregister_updaters()
        self._release_env.unregister_updaters()
        self._shift_var.unregister_updaters()
        self._shift_threshold.unregister_updaters()
        self._shift_env.unregister_updaters()
        self._init_env.unregister_updaters()
        self._damp.unregister_updaters()


class DampSlider(ProcNumSlider):

    def __init__(self):
        super().__init__(1, 0.0, 100.0, '')

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_update_signal_type(self):
        return 'signal_ks_damp_{}'.format(self._proc_id)

    def _update_value(self):
        ks_params = self._get_ks_params()
        self.set_number(ks_params.get_damp())

    def _value_changed(self, value):
        ks_params = self._get_ks_params()
        ks_params.set_damp(value)
        self._updater.signal_update(set([self._get_update_signal_type()]))


class InitEnvelope(TimeEnvelope):

    def __init__(self):
        super().__init__()
        self._proc_id = None

        self.setSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.MinimumExpanding)

    def set_proc_id(self, proc_id):
        self._proc_id = proc_id

    def _get_title(self):
        return 'Initial excitation envelope'

    def _allow_toggle_enabled(self):
        return False

    def _allow_loop(self):
        return True

    def _allow_release_toggle(self):
        return False

    def _make_envelope_widget(self):
        envelope = Envelope()
        envelope.set_node_count_max(32)
        envelope.set_y_range(0, 1)
        envelope.set_x_range(0, 0.2)
        envelope.set_first_lock(True, False)
        envelope.set_x_range_adjust(False, True)
        return envelope

    def _get_update_signal_type(self):
        return 'signal_ks_init_env_{}'.format(self._proc_id)

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_loop_enabled(self):
        return self._get_ks_params().get_init_env_loop_enabled()

    def _set_loop_enabled(self, enabled):
        self._get_ks_params().set_init_env_loop_enabled(enabled)

    def _get_scale_amount(self):
        return self._get_ks_params().get_init_env_scale_amount()

    def _set_scale_amount(self, value):
        self._get_ks_params().set_init_env_scale_amount(value)

    def _get_scale_center(self):
        return self._get_ks_params().get_init_env_scale_center()

    def _set_scale_center(self, value):
        self._get_ks_params().set_init_env_scale_center(value)

    def _get_envelope_data(self):
        return self._get_ks_params().get_init_env()

    def _set_envelope_data(self, envelope):
        self._get_ks_params().set_init_env(envelope)


class ShiftEnvelope(TimeEnvelope):

    def __init__(self):
        super().__init__()
        self._proc_id = None

        self.setSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.MinimumExpanding)

    def set_proc_id(self, proc_id):
        self._proc_id = proc_id

    def _get_title(self):
        return 'Pitch shift excitation envelope'

    def _allow_loop(self):
        return False

    def _allow_release_toggle(self):
        return False

    def _make_envelope_widget(self):
        envelope = Envelope()
        envelope.set_node_count_max(32)
        envelope.set_y_range(0, 1)
        envelope.set_x_range(0, 0.05)
        envelope.set_first_lock(True, False)
        envelope.set_last_lock(False, True)
        envelope.set_x_range_adjust(False, True)
        return envelope

    def _get_update_signal_type(self):
        return 'signal_ks_shift_env_{}'.format(self._proc_id)

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_enabled(self):
        return self._get_ks_params().get_shift_env_enabled()

    def _set_enabled(self, enabled):
        self._get_ks_params().set_shift_env_enabled(enabled)

    def _get_scale_amount(self):
        return self._get_ks_params().get_shift_env_scale_amount()

    def _set_scale_amount(self, value):
        self._get_ks_params().set_shift_env_scale_amount(value)

    def _get_scale_center(self):
        return self._get_ks_params().get_shift_env_scale_center()

    def _set_scale_center(self, value):
        self._get_ks_params().set_shift_env_scale_center(value)

    def _get_envelope_data(self):
        return self._get_ks_params().get_shift_env()

    def _set_envelope_data(self, envelope):
        self._get_ks_params().set_shift_env(envelope)


class ShiftThreshold(ProcNumSlider):

    def __init__(self):
        super().__init__(0, 1.0, 1200, 'Shift threshold')

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_update_signal_type(self):
        return 'signal_ks_shift_threshold_{}'.format(self._proc_id)

    def _update_value(self):
        ks_params = self._get_ks_params()
        self.set_number(ks_params.get_shift_env_trigger_threshold())

    def _value_changed(self, value):
        ks_params = self._get_ks_params()
        ks_params.set_shift_env_trigger_threshold(value)
        self._updater.signal_update(set([self._get_update_signal_type()]))


class ShiftVar(ProcNumSlider):

    def __init__(self):
        super().__init__(1, 0.0, 24.0, 'Shift strength variation')

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_update_signal_type(self):
        return 'signal_ks_shift_var_{}'.format(self._proc_id)

    def _update_value(self):
        ks_params = self._get_ks_params()
        self.set_number(ks_params.get_shift_env_strength_var())

    def _value_changed(self, value):
        ks_params = self._get_ks_params()
        ks_params.set_shift_env_strength_var(value)
        self._updater.signal_update(set([self._get_update_signal_type()]))


class ReleaseEnvelope(TimeEnvelope):

    def __init__(self):
        super().__init__()
        self._proc_id = None

        self.setSizePolicy(QSizePolicy.MinimumExpanding, QSizePolicy.MinimumExpanding)

    def set_proc_id(self, proc_id):
        self._proc_id = proc_id

    def _get_title(self):
        return 'Release excitation envelope'

    def _allow_loop(self):
        return False

    def _allow_release_toggle(self):
        return False

    def _make_envelope_widget(self):
        envelope = Envelope()
        envelope.set_node_count_max(32)
        envelope.set_y_range(0, 1)
        envelope.set_x_range(0, 0.2)
        envelope.set_first_lock(True, False)
        envelope.set_last_lock(False, True)
        envelope.set_x_range_adjust(False, True)
        return envelope

    def _get_update_signal_type(self):
        return 'signal_ks_release_env_{}'.format(self._proc_id)

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_enabled(self):
        return self._get_ks_params().get_release_env_enabled()

    def _set_enabled(self, enabled):
        self._get_ks_params().set_release_env_enabled(enabled)

    def _get_scale_amount(self):
        return self._get_ks_params().get_release_env_scale_amount()

    def _set_scale_amount(self, value):
        self._get_ks_params().set_release_env_scale_amount(value)

    def _get_scale_center(self):
        return self._get_ks_params().get_release_env_scale_center()

    def _set_scale_center(self, value):
        self._get_ks_params().set_release_env_scale_center(value)

    def _get_envelope_data(self):
        return self._get_ks_params().get_release_env()

    def _set_envelope_data(self, envelope):
        self._get_ks_params().set_release_env(envelope)


class ReleaseVar(ProcNumSlider):

    def __init__(self):
        super().__init__(1, 0.0, 24.0, 'Release strength variation')

    def _get_ks_params(self):
        return utils.get_proc_params(self._ui_model, self._au_id, self._proc_id)

    def _get_update_signal_type(self):
        return 'signal_ks_release_var_{}'.format(self._proc_id)

    def _update_value(self):
        ks_params = self._get_ks_params()
        self.set_number(ks_params.get_release_env_strength_var())

    def _value_changed(self, value):
        ks_params = self._get_ks_params()
        ks_params.set_release_env_strength_var(value)
        self._updater.signal_update(set([self._get_update_signal_type()]))

