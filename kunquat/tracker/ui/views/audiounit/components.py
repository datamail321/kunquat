# -*- coding: utf-8 -*-

#
# Author: Tomi Jylhä-Ollila, Finland 2015-2017
#
# This file is part of Kunquat.
#
# CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
#
# To the extent possible under law, Kunquat Affirmers have waived all
# copyright and related or neighboring rights to Kunquat.
#

from kunquat.tracker.ui.qt import *

from kunquat.kunquat.limits import *
import kunquat.tracker.ui.model.tstamp as tstamp
from kunquat.tracker.ui.views.connectionseditor import ConnectionsEditor
from kunquat.tracker.ui.views.editorlist import EditorList
from kunquat.tracker.ui.views.headerline import HeaderLine
from kunquat.tracker.ui.views.kqtcombobox import KqtComboBox
from kunquat.tracker.ui.views.varnamevalidator import *
from kunquat.tracker.ui.views.varvalidators import *
from .audiounitupdater import AudioUnitUpdater


def _get_update_signal_type(au_id):
    return 'signal_au_control_vars_{}'.format(au_id)

def _get_rebuild_signal_type(au_id):
    return 'signal_au_control_vars_rebuild_{}'.format(au_id)

def _get_stream_update_signal_type(au_id):
    return 'signal_au_streams_{}'.format(au_id)

def _get_conns_update_signal_type(au_id):
    return 'signal_connections_{}'.format(au_id)


class Components(QSplitter, AudioUnitUpdater):

    def __init__(self):
        super().__init__(Qt.Vertical)

        self._conns_editor = ConnectionsEditor()
        self._streams = Streams()
        self._control_vars = ControlVariables()

        self.add_to_updaters(self._conns_editor, self._streams, self._control_vars)

        cl = QHBoxLayout()
        cl.setContentsMargins(0, 0, 0, 0)
        cl.setSpacing(4)
        cl.addWidget(self._streams, 1)
        #cl.addWidget(self._control_vars, 3)

        cw = QWidget()
        cw.setLayout(cl)

        self.addWidget(self._conns_editor)
        self.addWidget(cw)

        self.setStretchFactor(0, 4)


class NameEditor(QLineEdit, AudioUnitUpdater):

    def __init__(self, validator_cls):
        super().__init__()
        self._context = None
        self._validator = None
        self._validator_cls = validator_cls

        self.set_used_names(set())

    def set_context(self, context):
        self._context = context
        if self._ui_model:
            self._update_contents()

    def _on_setup(self):
        self.editingFinished.connect(self._change_name_handler)
        self._update_contents()

    def set_used_names(self, used_names):
        self._validator = self._validator_cls(used_names)
        self.setValidator(self._validator)

    def _change_name_handler(self):
        new_name = str(self.text())
        self._change_name(new_name)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape:
            event.accept()
            self.set_context(self._context)
        else:
            return super().keyPressEvent(event)

    # Protected interface

    def _update_contents(self):
        raise NotImplementedError

    def _change_name(self, new_name):
        raise NotImplementedError


class RemoveButton(QPushButton, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None
        self.setToolTip('Remove')
        self.setStyleSheet('padding: 0 -2px;')

    def set_context(self, context):
        self._context = context

    def _on_setup(self):
        icon_bank = self._ui_model.get_icon_bank()
        self.setIcon(QIcon(icon_bank.get_icon_path('delete_small')))

        self.clicked.connect(self._remove)

    # Protected interface

    def _get_button_text(self):
        raise NotImplementedError

    def _remove(self):
        raise NotImplementedError


class Streams(QWidget, AudioUnitUpdater):

    def __init__(self):
        super().__init__()

        self._stream_list = StreamList()
        self.add_to_updaters(self._stream_list)

        v = QVBoxLayout()
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(2)
        v.addWidget(HeaderLine('Event stream interfaces'))
        v.addWidget(self._stream_list)
        self.setLayout(v)


class StreamList(EditorList, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._stream_names = None
        self._stream_names_set = None

    def _on_setup(self):
        self.register_action(
                _get_stream_update_signal_type(self._au_id), self._update_stream_names)
        self.register_action(
                _get_conns_update_signal_type(self._au_id), self._update_stream_names)
        self._update_stream_names()

    def _on_teardown(self):
        self.disconnect_widgets()

    def _update_stream_names(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        self._stream_names = au.get_stream_names()
        self._stream_names_set = set(self._stream_names)

        self.update_list()

    def _make_adder_widget(self):
        adder = StreamAdder()
        self.add_to_updaters(adder)
        return adder

    def _get_updated_editor_count(self):
        stream_count = len(self._stream_names)
        return stream_count

    def _make_editor_widget(self, index):
        stream_name = self._stream_names[index]

        editor = StreamEditor()
        editor.set_context(stream_name)
        self.add_to_updaters(editor)
        return editor

    def _update_editor(self, index, editor):
        stream_name = self._stream_names[index]

        editor.set_context(stream_name)
        editor.set_used_names(self._stream_names_set)

    def _disconnect_widget(self, widget):
        self.remove_from_updaters(widget)


class StreamAdder(QPushButton, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self.setText('Add new event stream interface')

    def _on_setup(self):
        self.register_action(
                _get_conns_update_signal_type(self._au_id), self._update_enabled)
        self.clicked.connect(self._add_new_entry)

        self._update_enabled()

    def _update_enabled(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        self.setEnabled(len(au.get_stream_processor_ids()) > 0)

    def _add_new_entry(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        au.add_stream()
        self._updater.signal_update(_get_stream_update_signal_type(self._au_id))


class StreamEditor(QWidget, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None

        self._name_editor = StreamNameEditor()
        self._target_proc_editor = StreamTargetProcEditor()
        self._remove_button = StreamRemoveButton()

        self.add_to_updaters(
                self._name_editor, self._target_proc_editor, self._remove_button)

        h = QHBoxLayout()
        h.setContentsMargins(4, 0, 0, 0)
        h.setSpacing(4)
        h.addWidget(QLabel('Interface name:'))
        h.addWidget(self._name_editor)
        h.addWidget(QLabel('Stream processor:'))
        h.addWidget(self._target_proc_editor)
        h.addWidget(self._remove_button)
        self.setLayout(h)

    def set_context(self, context):
        self._context = context
        self._name_editor.set_context(context)
        self._target_proc_editor.set_context(context)
        self._remove_button.set_context(context)

    def set_used_names(self, used_names):
        self._name_editor.set_used_names(used_names)


class StreamNameEditor(NameEditor):

    def __init__(self):
        super().__init__(VarNameValidator)

    def _update_contents(self):
        old_block = self.blockSignals(True)
        self.setText(self._context)
        self.blockSignals(old_block)

    def _change_name(self, new_name):
        if new_name == self._context:
            return

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.change_stream_name(self._context, new_name)
        self._updater.signal_update(_get_stream_update_signal_type(self._au_id))


class StreamTargetProcEditor(KqtComboBox, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None
        self.setSizeAdjustPolicy(QComboBox.AdjustToContents)

    def set_context(self, context):
        self._context = context
        if self._ui_model:
            self._update_contents()

    def _on_setup(self):
        self.register_action(
                'signal_connections_{}'.format(self._au_id), self._update_contents)
        self.register_action('signal_controls', self._update_contents)
        self.currentIndexChanged.connect(self._change_target_proc_id)
        self._update_contents()

    def _update_contents(self):
        if not self._context:
            return

        stream_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        if stream_name not in au.get_stream_names():
            return # We are being removed

        target_proc_num = au.get_stream_target_processor(stream_name)

        old_block = self.blockSignals(True)

        items = []
        for proc_id in au.get_stream_processor_ids():
            proc = au.get_processor(proc_id)
            if proc.get_existence():
                name = proc.get_name() or '-'
                cur_target_proc_id = proc_id.split('/')[-1]
                cur_target_proc_num = int(cur_target_proc_id.split('_')[-1], 16)
                items.append((name, cur_target_proc_num))
        self.set_items(items)

        selected_index = self.findData(target_proc_num)
        if selected_index >= 0:
            self.setCurrentIndex(selected_index)

        self.blockSignals(old_block)

    def _change_target_proc_id(self, index):
        if index < 0:
            return

        new_target_proc_num = self.itemData(index)

        stream_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.set_stream_target_processor(stream_name, new_target_proc_num)

        self._updater.signal_update(_get_stream_update_signal_type(self._au_id))


class StreamRemoveButton(RemoveButton):

    def __init__(self):
        super().__init__()

    def _remove(self):
        stream_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.remove_stream(stream_name)
        self._updater.signal_update(_get_stream_update_signal_type(self._au_id))


class ControlVariables(QWidget, AudioUnitUpdater):

    def __init__(self):
        super().__init__()

        self._var_list = ControlVariableList()
        self.add_to_updaters(self._var_list)

        v = QVBoxLayout()
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(2)
        v.addWidget(HeaderLine('Control variables'))
        v.addWidget(self._var_list)
        self.setLayout(v)


class ControlVariableList(EditorList):

    def __init__(self):
        super().__init__()
        self._au_id = None
        self._ui_model = None
        self._updater = None

        self._var_names = None
        self._var_names_set = None

    def set_au_id(self, au_id):
        self._au_id = au_id

    def set_ui_model(self, ui_model):
        self._ui_model = ui_model
        self._updater = ui_model.get_updater()
        self._updater.register_updater(self._perform_updates)

        self._update_var_names()

    def unregister_updaters(self):
        self.disconnect_widgets()
        self._updater.unregister_updater(self._perform_updates)

    def _perform_updates(self, signals):
        force_rebuild = _get_rebuild_signal_type(self._au_id) in signals
        if _get_update_signal_type(self._au_id) in signals:
            self._update_var_names(force_rebuild)

    def _update_var_names(self, force_rebuild=False):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        self._var_names = au.get_control_var_names()
        self._var_names_set = set(self._var_names)

        self.update_list(force_rebuild)

    def _make_adder_widget(self):
        adder = ControlVariableAdder()
        adder.set_au_id(self._au_id)
        adder.set_ui_model(self._ui_model)
        return adder

    def _get_updated_editor_count(self):
        var_count = len(self._var_names)
        return var_count

    def _make_editor_widget(self, index):
        var_name = self._var_names[index]

        editor = ControlVariableEditor()
        editor.set_au_id(self._au_id)
        editor.set_context(var_name)
        editor.set_ui_model(self._ui_model)
        return editor

    def _update_editor(self, index, editor):
        var_name = self._var_names[index]

        editor.set_context(var_name)
        editor.set_used_names(self._var_names_set)

    def _disconnect_widget(self, widget):
        widget.unregister_updaters()


class ControlVariableEditor(QWidget, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None

        self._expander = ControlVariableTypeExpander()
        self._name_editor = ControlVariableNameEditor()
        self._type_editor = ControlVariableTypeEditor()
        self._init_value_editor = ControlVariableInitValueEditor()
        self._remove_button = ControlVariableRemoveButton()
        self._bindings = ControlVariableBindings()

        self.add_to_updaters(
                self._name_editor,
                self._type_editor,
                self._init_value_editor,
                self._remove_button,
                self._bindings)

        h = QHBoxLayout()
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(4)
        h.addWidget(self._name_editor)
        h.addWidget(self._type_editor)
        h.addWidget(self._init_value_editor)
        h.addWidget(self._remove_button)

        g = QGridLayout()
        g.setContentsMargins(0, 0, 0, 0)
        g.setSpacing(0)
        g.addWidget(self._expander, 0, 0)
        g.addLayout(h, 0, 1)
        g.addWidget(self._bindings, 1, 1)
        self.setLayout(g)

    def set_context(self, context):
        self._context = context
        self._name_editor.set_context(context)
        self._type_editor.set_context(context)
        self._init_value_editor.set_context(context)
        self._remove_button.set_context(context)
        self._bindings.set_context(context)

        if self._ui_model:
            self._update_contents()

    def _on_setup(self):
        self._expander.clicked.connect(self._toggle_expand)
        self._update_contents()

    def set_used_names(self, used_names):
        self._name_editor.set_used_names(used_names)

    def _update_contents(self):
        var_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        is_expanded = au.is_control_var_expanded(var_name)

        old_block = self._expander.blockSignals(True)
        self._expander.setChecked(is_expanded)
        self._expander.blockSignals(old_block)

    def _toggle_expand(self, expand):
        var_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.set_control_var_expanded(var_name, expand)

        if expand:
            self._bindings.setVisible(True)
        else:
            self._updater.signal_update(
                _get_update_signal_type(self._au_id),
                _get_rebuild_signal_type(self._au_id))


class ControlVariableTypeExpander(QPushButton):

    def __init__(self):
        super().__init__()
        self.setCheckable(True)

    def sizeHint(self):
        sh = super().sizeHint()
        return QSize(sh.height(), sh.height())

    def paintEvent(self, event):
        painter = QPainter(self)

        colour = self.palette().color(QPalette.Text)
        painter.setPen(colour)
        painter.setBrush(colour)

        centre_x = self.width() // 2
        centre_y = self.height() // 2

        triangle_extent = 5

        painter.translate(QPoint(centre_x, centre_y))
        if self.isChecked():
            painter.drawPolygon(QPolygon([
                    QPoint(-triangle_extent, 0),
                    QPoint(0, triangle_extent),
                    QPoint(triangle_extent, 0)]))
        else:
            painter.drawPolygon(QPolygon([
                    QPoint(0, -triangle_extent),
                    QPoint(triangle_extent, 0),
                    QPoint(0, triangle_extent)]))


class ControlVariableNameEditor(NameEditor):

    def __init__(self):
        super().__init__(VarNameValidator)

    def _update_contents(self):
        old_block = self.blockSignals(True)
        self.setText(self._context)
        self.blockSignals(old_block)

    def _change_name(self, new_name):
        if new_name == self._context:
            return

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.change_control_var_name(self._context, new_name)
        self._updater.signal_update(_get_update_signal_type(self._au_id))


class ControlVariableTypeEditor(KqtComboBox, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None

    def set_context(self, context):
        self._context = context
        if self._ui_model:
            self._update_contents()

    def _on_setup(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_types = au.get_control_var_types()

        var_type_names = {
            bool: 'Boolean',
            int: 'Integer',
            float: 'Floating point',
            tstamp.Tstamp: 'Timestamp',
        }

        for t in var_types:
            type_name = var_type_names[t]
            self.addItem(type_name)

        self.currentIndexChanged.connect(self._change_type)

        self._update_contents()

    def _update_contents(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_types = au.get_control_var_types()
        var_type = au.get_control_var_type(self._context)
        var_type_index = var_types.index(var_type)

        old_block = self.blockSignals(True)
        self.setCurrentIndex(var_type_index)
        self.blockSignals(old_block)

    def _change_type(self, index):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_types = au.get_control_var_types()
        au.change_control_var_type(self._context, var_types[index])
        self._updater.signal_update(
            _get_update_signal_type(self._au_id),
            _get_rebuild_signal_type(self._au_id))


class ControlVariableValueEditor(QWidget):

    def __init__(self, label):
        super().__init__()
        self._au_id = None
        self._context = None
        self._ui_model = None
        self._updater = None

        self._label = label
        self._editor = None

        """
        h = QHBoxLayout()
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(2)
        h.addWidget(QLabel(label))
        self.setLayout(h)
        """

    def set_au_id(self, au_id):
        self._au_id = au_id

    def set_context(self, context):
        self._context = context

        if self._ui_model:
            self._update_contents()

    def set_ui_model(self, ui_model):
        if not self._context:
            return

        self._ui_model = ui_model
        self._updater = ui_model.get_updater()

        var_name = self._context if type(self._context) != tuple else self._context[0]

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_type = au.get_control_var_type(var_name)

        if var_type == bool:
            self._editor = QCheckBox()
            self._editor.stateChanged.connect(self._change_bool_value)
        elif var_type == int:
            self._editor = QLineEdit()
            self._editor.setValidator(IntValidator())
            self._editor.editingFinished.connect(self._change_int_value)
        elif var_type == float:
            self._editor = QLineEdit()
            self._editor.setValidator(FloatValidator())
            self._editor.editingFinished.connect(self._change_float_value)
        elif var_type == tstamp.Tstamp:
            self._editor = QLineEdit()
            self._editor.setValidator(FloatValidator())
            self._editor.editingFinished.connect(self._change_tstamp_value)
        else:
            assert False
        """
        self._editor_layout = QStackedLayout()
        for t in var_types:
            self._editor_layout.addWidget(self._editors[t])
        self.layout().addLayout(self._editor_layout)
        """

        h = QHBoxLayout()
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(2)
        h.addWidget(QLabel(self._label))
        h.addWidget(self._editor)
        self.setLayout(h)

        self._update_contents()

    def unregister_updaters(self):
        pass

    def _change_value(self, new_value):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        self._set_value(au, new_value)
        self._updater.signal_update(_get_update_signal_type(self._au_id))

    def _change_bool_value(self, new_state):
        new_value = (new_state == Qt.Checked)
        self._change_value(new_value)

    def _change_int_value(self):
        new_str = self._editor.text()
        new_value = int(new_str)
        self._change_value(new_value)

    def _change_float_value(self):
        new_str = self._editor.text()
        new_value = float(new_str)
        self._change_value(new_value)

    def _change_tstamp_value(self):
        new_str = self._editor.text()
        new_value = tstamp.Tstamp(float(new_str))
        self._change_value(new_value)

    # Protected interface

    def _update_contents(self):
        # TODO: this callback contains stuff that doesn't belong here, fix
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        var_types = au.get_control_var_types()
        var_type = au.get_control_var_type(self._context)
        var_types = au.get_control_var_types()
        var_type_index = var_types.index(var_type)
        """
        self._editor_layout.setCurrentIndex(var_type_index)
        """

        var_value = self._get_value(au)

        old_block = self._editor.blockSignals(True)
        if var_type == bool:
            self._editor.setCheckState(Qt.Checked if var_value else Qt.Unchecked)
        elif var_type in (int, float):
            self._editor.setText(str(var_value))
        elif var_type == tstamp.Tstamp:
            self._editor.setText(str(float(var_value)))
        else:
            assert False
        self._editor.blockSignals(old_block)

    def _get_value(self, au):
        raise NotImplementedError

    def _set_value(self, au, new_value):
        raise NotImplementedError


class ControlVariableInitValueEditor(ControlVariableValueEditor):

    def __init__(self):
        super().__init__('Initial value:')

    def _get_value(self, au):
        return au.get_control_var_init_value(self._context)

    def _set_value(self, au, new_value):
        au.change_control_var_init_value(self._context, new_value)


class ControlVariableAdder(QPushButton, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self.setText('Add new variable')

    def _on_setup(self):
        self.clicked.connect(self._add_new_entry)

    def _add_new_entry(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        au.add_control_var_bool()
        self._updater.signal_update(_get_update_signal_type(self._au_id))


class ControlVariableRemoveButton(RemoveButton):

    def __init__(self):
        super().__init__()

    def _remove(self):
        var_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.remove_control_var(var_name)
        self._updater.signal_update(_get_update_signal_type(self._au_id))


class ControlVariableBindings(QWidget):

    def __init__(self):
        super().__init__()
        self._au_id = None
        self._context = None
        self._ui_model = None
        self._updater = None

        self._adder = BindTargetAdder()

        self.setVisible(False)

        v = QVBoxLayout()
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(2)
        v.addWidget(self._adder)
        self.setLayout(v)

    def _get_adder(self):
        layout = self.layout()
        return layout.itemAt(layout.count() - 1).widget()

    def _get_widgets(self):
        for i in range(self.layout().count()):
            yield self.layout().itemAt(i).widget()

    def _get_editors(self):
        for i in range(self.layout().count() - 1):
            yield self.layout().itemAt(i).widget()

    def set_au_id(self, au_id):
        self._au_id = au_id
        for widget in self._get_widgets():
            widget.set_au_id(au_id)

    def set_context(self, context):
        self._context = context

        if self._ui_model:
            self._update_contents()

    def set_ui_model(self, ui_model):
        self._ui_model = ui_model
        self._updater = ui_model.get_updater()
        self._updater.register_updater(self._perform_updates)

        for widget in self._get_widgets():
            widget.set_ui_model(ui_model)

        self._update_contents()

    def unregister_updaters(self):
        self._updater.unregister_updater(self._perform_updates)
        for widget in self._get_widgets():
            widget.unregister_updaters()

    def _update_contents(self):
        var_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        self.setVisible(au.is_control_var_expanded(var_name))

        binding_targets = au.get_control_var_binding_targets(var_name)

        cur_binding_count = self.layout().count() - 1

        # Add missing editors
        layout = self.layout()
        for i in range(cur_binding_count, len(binding_targets)):
            editor = BindTargetEditor()
            editor.set_au_id(self._au_id)
            layout.insertWidget(i, editor)

        self._get_adder().set_context(var_name)

        for target_info, editor in zip(binding_targets, self._get_editors()):
            target_dev_id, target_var_name = target_info
            context = (var_name, target_dev_id, target_var_name)
            editor.set_context(context)
            editor.set_ui_model(self._ui_model)

    def _perform_updates(self, signals):
        if _get_update_signal_type(self._au_id) in signals:
            pass


class BindTargetEditor(QWidget):

    def __init__(self):
        super().__init__()
        self._au_id = None
        self._context = None
        self._ui_model = None

        self._target_dev_selector = BindTargetDeviceSelector()
        self._name_editor = BindTargetNameEditor()
        self._ext_editors = []
        self._remove_button = BindTargetRemoveButton()

    def set_au_id(self, au_id):
        self._au_id = au_id
        self._target_dev_selector.set_au_id(au_id)
        self._name_editor.set_au_id(au_id)
        self._remove_button.set_au_id(au_id)

    def set_context(self, context):
        self._context = context
        self._target_dev_selector.set_context(context)
        self._name_editor.set_context(context)
        self._remove_button.set_context(context)

        for editor in self._ext_editors:
            editor.set_context(context)

    def set_ui_model(self, ui_model):
        assert self._context

        if self._ui_model:
            return
        self._ui_model = ui_model

        self._target_dev_selector.set_ui_model(ui_model)
        self._name_editor.set_ui_model(ui_model)
        self._remove_button.set_ui_model(ui_model)

        var_name, target_dev_id, target_var_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_type = au.get_control_var_type(var_name)

        self._ext_editors = [
                BindTargetVariableTypeEditor(), BindTargetExpressionEditor()]

        for editor in self._ext_editors:
            editor.set_au_id(self._au_id)
            editor.set_context(self._context)
            editor.set_ui_model(self._ui_model)

        h = QHBoxLayout()
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(2)
        h.addWidget(self._target_dev_selector)
        h.addWidget(self._name_editor)
        for editor in self._ext_editors:
            h.addWidget(editor)
        h.addWidget(self._remove_button)
        self.setLayout(h)

    def unregister_updaters(self):
        for editor in self._ext_editors:
            editor.unregister_updaters()
        self._remove_button.unregister_updaters()
        self._name_editor.unregister_updaters()
        self._target_dev_selector.unregister_updaters()

    def set_used_names(self, used_names):
        self._name_editor.set_used_names(used_names)


class BindTargetDeviceSelector(KqtComboBox, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None
        self.setSizeAdjustPolicy(QComboBox.AdjustToContents)

    def set_context(self, context):
        self._context = context
        if self._ui_model:
            self._update_contents()

    def _on_setup(self):
        self.register_action(
                'signal_connections_{}'.format(self._au_id), self._update_contents)
        self.register_action('signal_controls', self._update_contents)
        self.currentIndexChanged.connect(self._change_target_dev_id)
        self._update_contents()

    def _get_internal_dev_ids(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        for au_id in au.get_au_ids():
            yield au_id

        for proc_id in au.get_processor_ids():
            yield proc_id

    def _get_internal_dev(self, dev_id):
        module = self._ui_model.get_module()

        if 'proc' in dev_id:
            dev_id_parts = dev_id.split('/')
            internal_au_id_parts = dev_id_parts[:-1]
            internal_au_id = '/'.join(internal_au_id_parts)
            internal_au = module.get_audio_unit(internal_au_id)
            dev = internal_au.get_processor(dev_id)
        else:
            dev = module.get_audio_unit(dev_id)

        return dev

    def _update_contents(self):
        if not self._context:
            return

        target_dev_id = self._context[1]

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        old_block = self.blockSignals(True)

        items = []
        for dev_id in self._get_internal_dev_ids():
            dev = self._get_internal_dev(dev_id)

            if dev.get_existence():
                name = dev.get_name() or '-'
                cur_target_dev_id = dev_id.split('/')[-1]
                items.append((name, cur_target_dev_id))
        self.set_items(items)

        selected_index = self.findData(target_dev_id)
        if selected_index >= 0:
            self.setCurrentIndex(selected_index)

        self.blockSignals(old_block)

    def _change_target_dev_id(self, index):
        if index < 0:
            return

        new_target_dev_id = self.itemData(index)

        var_name, target_dev_id, target_var_name = self._context

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.change_control_var_binding_target_dev(
                var_name, target_dev_id, target_var_name, new_target_dev_id)

        self._updater.signal_update(_get_update_signal_type(self._au_id))


class BindTargetNameValidator(QValidator):

    def __init__(self, used_names):
        super().__init__()
        self._used_names = used_names

    def validate(self, contents, pos):
        in_str = str(contents)
        parts = in_str.split('/')
        status = min(get_var_name_validation_status(part) for part in parts)
        if (status == QValidator.Acceptable) and (in_str in self._used_names):
            return (QValidator.Intermediate, contents, pos)
        return (status, contents, pos)


class BindTargetNameEditor(NameEditor):

    def __init__(self):
        super().__init__(BindTargetNameValidator)

    def _change_name(self, new_name):
        var_name, target_dev_id, target_var_name = self._context

        if new_name == target_var_name:
            return

        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.change_control_var_binding_target_name(
                var_name, target_dev_id, target_var_name, new_name)
        self._updater.signal_update(_get_update_signal_type(self._au_id))

    def _update_contents(self):
        if not self._context:
            return

        target_name = self._context[2]

        old_block = self.blockSignals(True)
        self.setText(target_name)
        self.blockSignals(old_block)


class BindTargetVariableTypeEditor(KqtComboBox, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None

    def set_context(self, context):
        self._context = context
        if self._ui_model:
            self._update_contents()

    def _on_setup(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_types = au.get_control_var_binding_target_types()

        var_type_names = {
            bool: 'Boolean',
            int: 'Integer',
            float: 'Floating point',
            tstamp.Tstamp: 'Timestamp',
        }

        for t in var_types:
            type_name = var_type_names[t]
            self.addItem(type_name)

        self.currentIndexChanged.connect(self._change_type)

        self._update_contents()

    def _update_contents(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_types = au.get_control_var_binding_target_types()

        var_name, target_dev_id, target_var_name = self._context
        var_type = au.get_control_var_binding_target_type(
                var_name, target_dev_id, target_var_name)
        var_type_index = var_types.index(var_type)

        old_block = self.blockSignals(True)
        self.setCurrentIndex(var_type_index)
        self.blockSignals(old_block)

    def _change_type(self, index):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        var_types = au.get_control_var_binding_target_types()

        var_name, target_dev_id, target_var_name = self._context
        au.change_control_var_binding_target_type(
                var_name, target_dev_id, target_var_name, var_types[index])

        self._updater.signal_update(_get_update_signal_type(self._au_id))


class ExpressionValidator(QValidator):

    def __init__(self):
        super().__init__()

    def validate(self, contents, pos):
        in_str = str(contents)

        ranges_ok = all(0x20 <= ord(ch) <= 0x7e for ch in in_str)
        if not ranges_ok:
            return (QValidator.Invalid, contents, pos)

        return (QValidator.Acceptable, contents, pos)


class BindTargetExpressionEditor(QWidget):

    def __init__(self):
        super().__init__()
        self._au_id = None
        self._context = None
        self._ui_model = None
        self._updater = None

        self._editor = QLineEdit()
        self._editor.setValidator(ExpressionValidator())

        h = QHBoxLayout()
        h.setContentsMargins(0, 0, 0, 0)
        h.setSpacing(2)
        h.addWidget(QLabel('Value expression:'))
        h.addWidget(self._editor)
        self.setLayout(h)

    def set_au_id(self, au_id):
        self._au_id = au_id

    def set_context(self, context):
        self._context = context

    def set_ui_model(self, ui_model):
        self._ui_model = ui_model
        self._updater = ui_model.get_updater()
        self._updater.register_updater(self._perform_updates)

        self._editor.textChanged.connect(self._change_expression)

        self._update_expression()

    def unregister_updaters(self):
        self._updater.unregister_updater(self._perform_updates)

    def _perform_updates(self, signals):
        if (_get_update_signal_type(self._au_id) in signals and
                _get_update_signal_type(self._au_id) not in signals):
            self._update_expression()

    def _update_expression(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        var_name, target_dev_id, target_var_name = self._context

        expr = au.get_control_var_binding_expression(
                var_name, target_dev_id, target_var_name)

        old_block = self._editor.blockSignals(True)
        if expr != self._editor.text():
            self._editor.setText(expr)
        self._editor.blockSignals(old_block)

    def _change_expression(self, expr):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        var_name, target_dev_id, target_var_name = self._context

        au.change_control_var_binding_expression(
                var_name, target_dev_id, target_var_name, expr)
        self._updater.signal_update(_get_update_signal_type(self._au_id))


class BindTargetRemoveButton(RemoveButton):

    def __init__(self):
        super().__init__()

    def _remove(self):
        var_name, target_dev_id, target_var_name = self._context
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)
        au.remove_control_var_binding(var_name, target_dev_id, target_var_name)
        self._updater.signal_update(
            _get_update_signal_type(self._au_id),
            _get_rebuild_signal_type(self._au_id))


class BindTargetAdder(QPushButton, AudioUnitUpdater):

    def __init__(self):
        super().__init__()
        self._context = None
        self.setText('Add new binding')

    def set_context(self, context):
        self._context = context

    def _on_setup(self):
        self.clicked.connect(self._add_new_entry)

    def _add_new_entry(self):
        module = self._ui_model.get_module()
        au = module.get_audio_unit(self._au_id)

        var_name = self._context
        var_type = au.get_control_var_type(var_name)

        au_ids = au.get_au_ids()
        if au_ids:
            dev_id = min(au_ids)
        else:
            proc_ids = au.get_processor_ids()
            if proc_ids:
                dev_id = min(proc_ids)
            else:
                assert False, 'Trying to add binding target without internal devices'

        internal_dev_id = dev_id.split('/')[-1]

        au.add_control_var_binding(var_name, internal_dev_id, var_type)

        self._updater.signal_update(_get_update_signal_type(self._au_id))


