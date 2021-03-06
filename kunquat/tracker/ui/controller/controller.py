# -*- coding: utf-8 -*-

#
# Authors: Tomi Jylhä-Ollila, Finland 2013-2017
#          Toni Ruottu, Finland 2013-2014
#
# This file is part of Kunquat.
#
# CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
#
# To the extent possible under law, Kunquat Affirmers have waived all
# copyright and related or neighboring rights to Kunquat.
#

import re
import sys
import json
import time
import tempfile
from io import BytesIO
import os.path
import zipfile

from kunquat.kunquat.file import KqtFile, KunquatFileError, KQT_KEEP_NONE
from kunquat.kunquat.kunquat import get_default_value
from kunquat.kunquat.limits import *
import kunquat.tracker.cmdline as cmdline
from kunquat.tracker.ui.model.triggerposition import TriggerPosition
import kunquat.tracker.ui.model.tstamp as tstamp

from .kqtivalidator import KqtiValidator
from .store import Store
from .session import Session
from .share import Share
from .updater import Updater
from .notechannelmapper import NoteChannelMapper

#TODO: figure a place for the events
EVENT_SELECT_CONTROL = '.a'
EVENT_NOTE_ON = 'n+'
EVENT_HIT = 'h'
EVENT_NOTE_OFF = 'n-'
EVENT_SET_FORCE = '.f'
EVENT_SET_CH_EXPRESSION = '.xc'
EVENT_SET_NOTE_EXPRESSION = '.x'
EVENT_SET_TEST_PROCESSOR = 'c.tp'
EVENT_SET_TEST_PROCESSOR_PARAM = 'c.tpp'


class Controller():

    def __init__(self):
        self._push_amount = None
        self._audio_levels = (0, 0)
        self._store = None
        self._session = None
        self._share = None
        self._updater = None
        self._note_channel_mapper = None
        self._audio_engine = None
        self._ui_model = None
        self._ch_expr_counter = [-1] * CHANNELS_MAX
        self._format_error_handler = None

    def set_ui_model(self, ui_model):
        self._ui_model = ui_model

    def set_store(self, store):
        self._store = store

    def get_store(self):
        return self._store

    def set_session(self, session):
        self._session = session

    def get_session(self):
        return self._session

    def set_share(self, share):
        self._share = share

    def get_share(self):
        return self._share

    def set_updater(self, updater):
        self._updater = updater

    def get_updater(self):
        return self._updater

    def set_note_channel_mapper(self, note_channel_mapper):
        self._note_channel_mapper = note_channel_mapper
        self._note_channel_mapper.set_controller(self)

    def set_audio_engine(self, audio_engine):
        self._audio_engine = audio_engine
        self._store.set_audio_engine(audio_engine)

    def _remove_prefix(self, path, prefix):
        preparts = prefix.split('/')
        keyparts = path.split('/')
        for pp in preparts:
            kp = keyparts.pop(0)
            if pp != kp:
                 return None
        return '/'.join(keyparts)

    def create_sandbox(self):
        transaction = {
            'p_force_shift.json'                  : -30,
            'album/p_manifest.json'               : {},
            'album/p_tracks.json'                 : [0],
            'song_00/p_manifest.json'             : {},
            'song_00/p_order_list.json'           : [[0, 0]],
            'pat_000/p_manifest.json'             : {},
            'pat_000/instance_000/p_manifest.json': {},
            'out_00/p_manifest.json'              : {},
            'out_01/p_manifest.json'              : {},
        }
        self._store.put(transaction)

    def _set_module_load_transaction_error_handler(self, module_path):
        def _on_error(e):
            desc = e.args[0]
            self._session.set_module_load_error_info(module_path, desc['message'])
            self._updater.signal_update(
                    'signal_module_load_error', 'signal_progress_finished')

        self._format_error_handler = _on_error

    def _get_transaction_notifier(self, start_progress, on_finished):
        def on_transaction_progress_update(progress):
            left = 1 - start_progress
            self._session.set_progress_position(min(1, start_progress + left * progress))
            if progress < 1:
                self._updater.signal_update('signal_progress_step')
            else:
                self._updater.signal_update('signal_progress_finished')
                self._format_error_handler = None
                on_finished()

        return on_transaction_progress_update

    def _update_progress_step(self, progress):
        self._session.set_progress_position(progress)
        self._updater.signal_update('signal_progress_step')

    def get_task_load_module(self, module_path):
        values = dict()

        kqtfile = KqtFile(module_path, KQT_KEEP_NONE)
        self._session.set_progress_description('Loading {}...'.format(module_path))
        self._session.set_progress_position(0)
        self._updater.signal_update('signal_progress_start')

        try:
            for i, entry in enumerate(kqtfile.get_entries()):
                yield
                key, value = entry
                values[key] = value
                self._update_progress_step(kqtfile.get_loading_progress() * 0.5)
        except KunquatFileError as e:
            self._session.set_module_load_error_info(module_path, e.args[0])
            self._updater.signal_update(
                    'signal_module_load_error', 'signal_progress_finished')
            return

        self._set_module_load_transaction_error_handler(module_path)

        notifier = self._get_transaction_notifier(0.5,
                lambda: self._updater.signal_update('signal_module'))
        self._store.put(values, transaction_notifier=notifier)
        self._store.clear_modified_flag()

        sheet_mgr = self._ui_model.get_sheet_manager()
        if not sheet_mgr.is_grid_default_enabled():
            sheet_mgr.set_grid_enabled(False)

        notation_mgr = self._ui_model.get_notation_manager()
        stored_notation_id = notation_mgr.get_stored_notation_id()
        if stored_notation_id in notation_mgr.get_all_notation_ids():
            notation_mgr.set_selected_notation_id(stored_notation_id)
        else:
            notation_mgr.set_selected_notation_id(None)

        self._updater.signal_update('signal_controls')

        self._reset_expressions()

    def get_task_save_module(self, module_path):
        assert module_path
        tmpname = None

        self._session.set_progress_description(
                'Saving module to {}...'.format(module_path))
        self._session.set_progress_position(0)
        self._updater.signal_update('signal_progress_start')

        with tempfile.NamedTemporaryFile(delete=False) as f:
            with zipfile.ZipFile(f, mode='w', compression=zipfile.ZIP_STORED) as zfile:
                prefix = 'kqtc00'

                step_count = len(self._store.items()) + 1

                for i, (key, value) in enumerate(self._store.items()):
                    yield
                    path = '/'.join((prefix, key))
                    if key.endswith('.json'):
                        encoded = bytes(json.dumps(value), encoding='utf-8')
                    else:
                        encoded = value
                    compress = zipfile.ZIP_DEFLATED if key.endswith('.json') else None
                    zfile.writestr(path, encoded, compress)
                    self._update_progress_step(i / step_count)

                tmpname = f.name

        if tmpname:
            os.rename(tmpname, module_path)

        self._session.set_progress_position(1)
        self._updater.signal_update('signal_progress_finished')

        self._updater.signal_update('signal_save_module_finished')

    def get_task_export_audio_unit(self, au_id, au_path):
        assert au_path
        tmpname = None

        module = self._ui_model.get_module()
        au = module.get_audio_unit(au_id)
        au_type = 'instrument' if au.is_instrument() else 'effect'

        self._session.set_progress_description(
                'Exporting {} to {}...'.format(au_type, au_path))
        self._session.set_progress_position(0)
        self._updater.signal_update('signal_progress_start')

        with tempfile.NamedTemporaryFile(delete=False) as f:
            with zipfile.ZipFile(f, mode='w', compression=zipfile.ZIP_STORED) as zfile:
                prefix = 'kqti00'
                au_prefix = au_id + '/'
                au_keys = [k for k in self._store.keys() if k.startswith(au_prefix)]

                step_count = len(au_keys) + 1

                for i, key in enumerate(au_keys):
                    yield
                    value = self._store[key]
                    path = '{}/{}'.format(prefix, key[len(au_prefix):])
                    if key.endswith('.json'):
                        encoded = bytes(json.dumps(value), encoding='utf-8')
                    else:
                        encoded = value
                    compress = zipfile.ZIP_DEFLATED if key.endswith('.json') else None
                    zfile.writestr(path, encoded, compress)
                    self._update_progress_step(i / step_count)

                tmpname = f.name

        if tmpname:
            os.rename(tmpname, au_path)

        self._session.set_progress_position(1)
        self._updater.signal_update('signal_progress_finished')

        self._updater.signal_update('signal_export_au_finished')

    def get_task_load_audio_unit(
            self, kqtifile, au_id, control_id=None, is_sandbox=False):
        if not is_sandbox:
            path = kqtifile.get_path()
            self._session.set_progress_description(
                    'Importing audio unit {}...'.format(path))
            self._session.set_progress_position(0)
            self._updater.signal_update('signal_progress_start')
            yield

        try:
            for _ in kqtifile.get_read_steps():
                if not is_sandbox:
                    self._update_progress_step(kqtifile.get_loading_progress() / 3)
                yield
        except KunquatFileError as e:
            self._session.set_au_import_error_info(kqtifile.get_path(), str(e))
            self._updater.signal_update(
                    'signal_au_import_error', 'signal_au_import_finished')
            if not is_sandbox:
                self._updater.signal_update('signal_progress_finished')
            return
        contents = kqtifile.get_contents()

        # Validate contents
        validator = KqtiValidator(contents)
        for _ in validator.get_validation_steps():
            if not is_sandbox:
                self._update_progress_step((1 + validator.get_progress()) / 3)
            yield
        if not validator.is_valid():
            self._session.set_au_import_error_info(
                    kqtifile.get_path(), validator.get_validation_error())
            self._updater.signal_update(
                    'signal_au_import_error', 'signal_au_import_finished')
            if not is_sandbox:
                self._updater.signal_update('signal_progress_finished')
            return

        transaction = {}

        # Add audio unit data to the transaction
        for (key, value) in contents.items():
            dest_key = '{}/{}'.format(au_id, key)
            transaction[dest_key] = value

        # Connect instrument
        if transaction['{}/p_manifest.json'.format(au_id)]['type'] == 'instrument':
            # Add instrument control
            if ('/' not in au_id) and control_id:
                control_num = int(control_id.split('_')[1], 16)
                au_num = int(au_id.split('_')[1], 16)

                control_map = self._store.get('p_control_map.json', [])
                control_map.append([control_num, au_num])

                transaction['p_control_map.json'] = control_map
                transaction['{}/p_manifest.json'.format(control_id)] = {}

            # Get output ports of the containing device
            if '/' in au_id:
                parent_au_id = au_id.split('/')[0]
                module = self._ui_model.get_module()
                parent_au = module.get_audio_unit(parent_au_id)
                parent_out_ports = sorted(parent_au.get_out_ports())
            else:
                parent_out_ports = ['out_00', 'out_01']

            # Get instrument output ports (manually since the model has no access yet)
            ins_out_ports = []
            key_pattern = re.compile(
                    '{}/out_[0-9a-f]{{2}}/p_manifest.json'.format(au_id))
            for path in transaction:
                if key_pattern.match(path) and transaction[path] != None:
                    ins_out_port = path.split('/')[-2]
                    ins_out_ports.append(ins_out_port)
            ins_out_ports = sorted(ins_out_ports)

            # Connect if the number of output ports match
            if parent_out_ports and (len(parent_out_ports) == len(ins_out_ports)):
                sub_au_id = au_id.split('/')[-1]
                conns = self._store.get('p_connections.json', [])
                for (send_port, recv_port) in zip(ins_out_ports, parent_out_ports):
                    conns.append(['{}/{}'.format(sub_au_id, send_port), recv_port])
                transaction['p_connections.json'] = conns

        # Set up transaction notifier
        if not is_sandbox:
            start_progress = 2 / 3
            signaller = lambda: self._updater.signal_update('signal_au_import_finished')
            if '/' not in au_id:
                def on_finished():
                    signaller()
                    visibility_mgr = self._ui_model.get_visibility_manager()
                    visibility_mgr.show_connections()
                notifier = self._get_transaction_notifier(start_progress, on_finished)
            else:
                notifier = self._get_transaction_notifier(start_progress, signaller)
        else:
            notifier = None

        # Send data
        self._store.put(transaction, transaction_notifier=notifier)

        self._updater.signal_update('signal_controls')

    def _reset_runtime_env(self):
        self._session.reset_runtime_env()
        self._updater.signal_update('signal_runtime_env')

    def _reset_expressions(self):
        self._session.reset_active_ch_expressions()

        channel_defaults = self._ui_model.get_module().get_channel_defaults()
        for i in range(CHANNELS_MAX):
            ch_expr_name = channel_defaults.get_initial_expression(i)
            self._session.set_default_ch_expression(i, ch_expr_name)

    def _check_update_random_seed(self):
        rand_seed_update_key = 'i_random_seed_auto_update.json'
        rand_seed_key = 'p_random_seed.json'
        if self._store.get(rand_seed_update_key, False):
            old_seed = self._store.get(
                    rand_seed_key, get_default_value(rand_seed_key))
            new_seed = (old_seed + 1) % 2**63
            transaction = { rand_seed_key: new_seed }
            self._store.put(transaction, mark_modified=False)
            self._updater.signal_update('signal_random_seed')

    def call_post_action(self, action_name, args):
        getattr(self, action_name)(*args)

    def _reset_with_post_action(self, action, *args):
        assert hasattr(self, action.__name__)
        track_num = self._session.get_playback_track()
        self._audio_engine.reset_and_pause(track_num)
        self._audio_engine.sync_call_post_action(action.__name__, args)

    def play(self, track=None):
        if self._session.is_playback_active():
            self._check_update_random_seed()
        self._session.set_playback_track(track)
        self._reset_with_post_action(self._play)

    def _play(self):
        self._session.reset_max_audio_levels()
        self._session.reset_voice_stats()
        self._reset_expressions()
        self._reset_runtime_env()

        if self._session.get_infinite_mode():
            self._audio_engine.tfire_event(0, ('cinfinite+', None))

        self._audio_engine.tfire_event(0, ('cresume', None))
        self._session.set_playback_active(True)
        self._updater.signal_update('signal_play')

    def play_pattern(self, pattern_instance):
        if self._session.is_playback_active():
            self._check_update_random_seed()
        self._reset_with_post_action(self._play_pattern, pattern_instance)

    def _play_pattern(self, pattern_instance):
        self._session.reset_max_audio_levels()
        self._session.reset_voice_stats()
        self._reset_expressions()
        self._reset_runtime_env()

        if self._session.get_infinite_mode():
            self._audio_engine.tfire_event(0, ('cinfinite+', None))

        play_event = ('cpattern', pattern_instance)
        self._audio_engine.tfire_event(0, play_event)

        self._session.reset_max_audio_levels()
        self._reset_expressions()
        self._audio_engine.tfire_event(0, ('cresume', None))
        self._session.set_playback_active(True)
        self._updater.signal_update('signal_play')

    def play_from_cursor(self, pattern_instance, row_ts):
        if self._session.is_playback_active():
            self._check_update_random_seed()
        self._reset_with_post_action(self._play_from_cursor, pattern_instance, row_ts)

    def _play_from_cursor(self, pattern_instance, row_ts):
        self._session.reset_max_audio_levels()
        self._session.reset_voice_stats()
        self._reset_expressions()
        self._reset_runtime_env()

        if self._session.get_infinite_mode():
            self._audio_engine.tfire_event(0, ('cinfinite+', None))

        set_goto_pinst = ('c.gp', pattern_instance)
        set_goto_row = ('c.gr', row_ts)
        self._audio_engine.tfire_event(0, set_goto_pinst)
        self._audio_engine.tfire_event(0, set_goto_row)
        self._audio_engine.tfire_event(0, ('cg', None))

        self._session.reset_max_audio_levels()
        self._reset_expressions()
        self._audio_engine.tfire_event(0, ('cresume', None))
        self._session.set_playback_active(True)
        self._updater.signal_update('signal_play')

    def silence(self):
        self._reset_with_post_action(self._silence)

        if self._session.is_playback_active():
            self._check_update_random_seed()

        self._session.set_playback_active(False)
        self._updater.signal_update('signal_silence')

    def _silence(self):
        self._session.reset_max_audio_levels()
        self._session.reset_voice_stats()
        self._reset_runtime_env()
        self._reset_expressions()

        # Reset active notes
        for ch in range(CHANNELS_MAX):
            self._session.set_active_note(ch, 'n-', None)

        if self._session.get_infinite_mode():
            self._audio_engine.tfire_event(0, ('cinfinite+', None))

    def set_infinite_mode(self, enabled):
        self._session.set_infinite_mode(enabled)

        event_name = 'cinfinite+' if self._session.get_infinite_mode() else 'cinfinite-'
        self._audio_engine.tfire_event(0, (event_name, None))

    def get_infinite_mode(self):
        return self._session.get_infinite_mode()

    def set_channel_mute(self, channel, mute):
        self._audio_engine.set_channel_mute(channel, mute)

    def start_tracked_note(self, channel_number, control_id, event_type, param):
        assert event_type in (EVENT_NOTE_ON, EVENT_HIT)

        module = self._ui_model.get_module()
        control = module.get_control(control_id)
        au = control.get_audio_unit()
        force = None

        if self._session.are_au_test_params_enabled(au.get_id()):
            force = au.get_test_force() - module.get_force_shift()
            expressions = []
            for i in range(2):
                expr_name = au.get_test_expression(i)
                expressions.append(expr_name)
        else:
            channel_defaults = self._ui_model.get_module().get_channel_defaults()
            expressions = [channel_defaults.get_initial_expression(channel_number)]

        note = self._note_channel_mapper.get_tracked_note(channel_number, False)
        self.set_active_note(
                note.get_channel(), control_id, event_type, param, force, expressions)
        return note

    def set_active_note(
            self, channel_number, control_id, event_type, param, force, expressions):
        # Get control override event
        parts = control_id.split('_')
        second = parts[1]
        control_number = int(second, 16)
        control_event = (EVENT_SELECT_CONTROL, control_number)

        # Get expression override and restore events
        orig_ch_expr = self._session.get_active_ch_expression(channel_number)
        clear_expr_event = (EVENT_SET_CH_EXPRESSION, '')
        restore_expr_event = (EVENT_SET_CH_EXPRESSION, orig_ch_expr)

        # Fire events
        self._audio_engine.fire_event(channel_number, control_event)

        test_proc_id = self._session.get_test_processor(control_id)
        if test_proc_id:
            proc_num = int(test_proc_id.split('_')[-1], 16)
            test_proc_event = (EVENT_SET_TEST_PROCESSOR, proc_num)
            self._audio_engine.fire_event(channel_number, test_proc_event)
            proc_param = self._session.get_test_processor_param(test_proc_id)
            if proc_param != None:
                test_proc_param_event = (EVENT_SET_TEST_PROCESSOR_PARAM, proc_param)
                self._audio_engine.fire_event(channel_number, test_proc_param_event)
                self._session.set_test_processor_param(test_proc_id, None)

        note_on_or_hit_event = (event_type, param)
        self._audio_engine.fire_event(channel_number, note_on_or_hit_event)
        if force != None:
            self._audio_engine.fire_event(channel_number, (EVENT_SET_FORCE, force))
        expr_events = [EVENT_SET_CH_EXPRESSION, EVENT_SET_NOTE_EXPRESSION]
        ch_expr_count = 0
        for i, expression in enumerate(expressions):
            if expression != None:
                apply_expr_event = (expr_events[i], expression)
                self._audio_engine.fire_event(channel_number, apply_expr_event)
                if expression == EVENT_SET_CH_EXPRESSION:
                    ch_expr_count += 1
        self._ch_expr_counter[channel_number] = ch_expr_count
        if ch_expr_count > 0:
            self._audio_engine.sync_call_post_action(
                    self._reset_ch_expr.__name__, (channel_number, restore_expr_event))

    def _reset_ch_expr(self, ch_num, restore_event):
        if self._ch_expr_counter[ch_num] == 0:
            self._audio_engine.fire_event(ch_num, restore_event)
        else:
            print('expr reset skipped')

    def set_rest(self, channel_number):
        note_off_event = (EVENT_NOTE_OFF, None)
        self._audio_engine.fire_event(channel_number, note_off_event)

    def notify_kunquat_exception(self, e):
        handler = None
        desc = e.args[0]
        if type(desc) == dict:
            if desc.get('type') == 'FormatError':
                handler = self._format_error_handler

        if handler:
            handler(e)
        else:
            raise e

    def notify_libkunquat_error(self, e):
        raise e

    def update_output_speed(self, fps):
        self._session.set_output_speed(fps)
        self._updater.signal_update()

    def update_render_speed(self, fps):
        self._session.set_render_speed(fps)
        self._updater.signal_update()

    def update_render_load(self, load):
        self._session.set_render_load(load)
        self._updater.signal_update()

    def update_audio_levels(self, levels):
        self._session.set_audio_levels(levels)
        self._updater.signal_update()

    def update_ui_load(self, load):
        self._session.set_ui_load(load)
        self._updater.signal_update()

    def add_ui_load_average(self, load_avg):
        self._session.add_ui_load_average(load_avg)
        self._updater.signal_update('signal_load_history')

    def add_ui_load_peak(self, load_peak):
        self._session.add_ui_load_peak(load_peak)
        self._updater.signal_update('signal_load_history')

    def update_selected_control(self, channel, control_id):
        self._session.set_selected_control_id_by_channel(channel, control_id)
        self._updater.signal_update()

    def update_active_note(self, channel, event_type, pitch):
        self._session.set_active_note(channel, event_type, pitch)
        self._updater.signal_update()

    def update_active_var_name(self, ch, var_name):
        self._session.set_active_var_name(ch, var_name)

    def update_active_var_value(self, ch, var_value):
        self._session.set_active_var_value(ch, var_value)
        self._updater.signal_update('signal_runtime_env')

    def update_ch_expression(self, ch, expr_name):
        self._ch_expr_counter[ch] -= 1
        self._session.set_active_ch_expression(ch, expr_name)

    def set_runtime_var_value(self, var_name, var_value):
        # Get current active variable name
        old_name = self._session.get_active_var_name(0) or ''

        # Set new value
        name_event = ('c.evn', var_name)
        value_event = ('c.ev', var_value)
        self._audio_engine.tfire_event(0, name_event)
        self._audio_engine.tfire_event(0, value_event)

        # Restore old active variable name so that we don't mess up playback
        old_name_event = ('c.evn', old_name)
        self._audio_engine.tfire_event(0, old_name_event)

    def send_queries(self):
        location_feedback_event = ('qlocation', None)
        self._audio_engine.tfire_event(0, location_feedback_event)
        self._audio_engine.request_voice_info()

    def update_pending_playback_cursor_track(self, track):
        self._session.set_pending_playback_cursor_track(track)

    def update_pending_playback_cursor_system(self, system):
        self._session.set_pending_playback_cursor_system(system)

    def update_playback_pattern(self, piref):
        self._session.set_playback_pattern(piref)

    def update_playback_cursor(self, row):
        self._session.set_playback_cursor(row)
        if self._session.get_record_mode():
            self._move_edit_cursor_to_playback_cursor()
        self._updater.signal_update('signal_playback_cursor')

    def _move_edit_cursor_to_playback_cursor(self):
        (track, system, row) = self._session.get_playback_cursor_position()
        selection = self._ui_model.get_selection()
        current_location = selection.get_location()
        col_num = current_location.get_col_num()
        row_ts = tstamp.Tstamp(*row)
        new_location = TriggerPosition(track, system, col_num, row_ts, 0)
        selection.set_location(new_location)

    def update_active_voice_count(self, voice_count):
        self._session.set_active_voice_count(voice_count)

    def update_active_vgroup_count(self, vgroup_count):
        self._session.set_active_vgroup_count(vgroup_count)

    def update_event_log_with(self, channel_number, event_type, event_value, context):
        self._session.log_event(channel_number, event_type, event_value, context)
        self._updater.signal_update()

    def update_transaction_progress(self, transaction_id, progress):
        self._store.update_transaction_progress(transaction_id, progress)

    def confirm_valid_data(self, transaction_id):
        self._store.confirm_valid_data(transaction_id)


def create_controller():
    store = Store()
    session = Session()
    share_path = os.path.join(cmdline.get_install_prefix(), 'share', 'kunquat')
    share = Share(share_path)
    updater = Updater()
    note_channel_mapper = NoteChannelMapper()
    controller = Controller()
    controller.set_store(store)
    controller.set_session(session)
    controller.set_share(share)
    controller.set_updater(updater)
    controller.set_note_channel_mapper(note_channel_mapper)
    return controller


