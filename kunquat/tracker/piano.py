
import kunquat.tracker.kqt_limits as lim
from itertools import cycle

class Piano():
    def __init__(self, p):
        self.p = p 
        self._pressed = {}
        self._channel = cycle(xrange(lim.COLUMNS_MAX))
        self._inst_num = 0;
                
    def press(self, note, input_octave, cursor = None):
        octave = self.p._note_input.base_octave + input_octave
        cents = self.p._scale.get_cents(note, octave)
        ch = self._channel.next()
        if cents in self._pressed:
            return
        self._pressed[cents] = ch
        self.p._playback.play_event(ch, ['.i', self._inst_num])
        self.p._playback.play_event(ch, ['n+', cents])
        if cursor != None:
            print cursor

    def release(self, note, input_octave):
        octave = self.p._note_input.base_octave + input_octave
        cents = self.p._scale.get_cents(note, octave)
        if cents not in self._pressed:
            return
        ch = self._pressed.pop(cents)
        self.p._playback.play_event(ch, ['n-', None])

