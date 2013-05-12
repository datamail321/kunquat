# -*- coding: utf-8 -*-

#
# Authors: Tomi Jylhä-Ollila, Finland 2013
#          Toni Ruottu, Finland 2013
#
# This file is part of Kunquat.
#
# CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
#
# To the extent possible under law, Kunquat Affirmers have waived all
# copyright and related or neighboring rights to Kunquat.
#

import threading

from commandqueue import CommandQueue

HALT = None

class BackendThread(threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self, name="Backend")
        self._q = CommandQueue()
        self._backend = None

    # Backend interface

    def set_audio_output(self, audio_output):
        self._q.push('set_audio_output', audio_output)

    def set_frontend(self, frontend):
        self._q.push('set_frontend', frontend)

    def set_data(self, key, value):
        self._q.push('set_data', key, value)

    def commit_data(self):
        self._q.push('commit_data')

    def generate_audio(self, nframes):
        self._q.push('generate_audio', nframes)

    def update_selected_driver(self, name):
        self._q.push('update_selected_driver', name)

    def load_module(self):
        self._q.push('load_module')

    # Threading interface

    def set_handler(self, backend):
        self._backend = backend

    def halt(self):
        self._q.push(HALT)

    def run(self):
        self._q.block()
        command = self._q.get()
        while command.name != HALT:
            getattr(self._backend, command.name)(*command.args)
            self._q.block()
            command = self._q.get()

