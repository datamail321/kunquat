

/*
 * Author: Tomi Jylhä-Ollila, Finland 2010
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <stdlib.h>
#include <assert.h>

#include <Event_handler.h>
#include <Event_type.h>
#include <Channel_state.h>
#include <Ins_table.h>
#include <Playdata.h>
#include <kunquat/limits.h>

#include <events/Event_global_pattern_delay.h>
#include <events/Event_global_set_jump_counter.h>
#include <events/Event_global_set_jump_row.h>
#include <events/Event_global_set_jump_section.h>
#include <events/Event_global_set_jump_subsong.h>

#include <events/Event_global_set_scale.h>
#include <events/Event_global_set_scale_offset.h>
#include <events/Event_global_mimic_scale.h>
#include <events/Event_global_shift_scale_intervals.h>

#include <events/Event_global_set_tempo.h>
#include <events/Event_global_set_volume.h>
#include <events/Event_global_slide_tempo.h>
#include <events/Event_global_slide_tempo_length.h>
#include <events/Event_global_slide_volume.h>
#include <events/Event_global_slide_volume_length.h>

#include <events/Event_channel_set_instrument.h>

#include <events/Event_channel_note_on.h>
#include <events/Event_channel_note_off.h>

#include <events/Event_channel_set_force.h>
#include <events/Event_channel_slide_force.h>
#include <events/Event_channel_slide_force_length.h>
#include <events/Event_channel_tremolo_speed.h>
#include <events/Event_channel_tremolo_depth.h>
#include <events/Event_channel_tremolo_delay.h>

#include <events/Event_channel_slide_pitch.h>
#include <events/Event_channel_slide_pitch_length.h>
#include <events/Event_channel_vibrato_speed.h>
#include <events/Event_channel_vibrato_depth.h>
#include <events/Event_channel_vibrato_delay.h>
#include <events/Event_channel_arpeggio.h>

#include <events/Event_channel_set_filter.h>
#include <events/Event_channel_slide_filter.h>
#include <events/Event_channel_slide_filter_length.h>
#include <events/Event_channel_autowah_speed.h>
#include <events/Event_channel_autowah_depth.h>
#include <events/Event_channel_autowah_delay.h>

#include <events/Event_channel_set_resonance.h>

#include <events/Event_channel_set_panning.h>
#include <events/Event_channel_slide_panning.h>
#include <events/Event_channel_slide_panning_length.h>

#include <events/Event_ins_set_pedal.h>

#include <xmemory.h>


struct Event_handler
{
    bool mute; // FIXME: this is just to make the stupid Channel_state_init happy
    Channel_state* ch_states[KQT_COLUMNS_MAX];
    Ins_table* insts;
    Playdata* global_state;
    bool (*ch_process[EVENT_CHANNEL_UPPER])(Channel_state* state,
                                           char* fields);
    bool (*global_process[EVENT_GLOBAL_UPPER])(Playdata* state,
                                               char* fields);
    bool (*ins_process[EVENT_INS_UPPER])(Instrument_params* state, char* fields);
    // TODO: generator and effect process collections
};


Event_handler* new_Event_handler(Playdata* global_state,
                                 Channel_state** ch_states,
                                 Ins_table* insts)
{
    Event_handler* eh = xalloc(Event_handler);
    if (eh == NULL)
    {
        return NULL;
    }
    eh->global_state = global_state;
/*    if (eh->global_state == NULL)
    {
        del_Event_handler(eh);
        return NULL;
    } */
    for (int i = 0; i < KQT_COLUMNS_MAX; ++i)
    {
        eh->ch_states[i] = ch_states[i];
//        Channel_state_init(&eh->ch_states[i], i, &eh->mute);
    }
    eh->insts = insts;

    Event_handler_set_global_process(eh, EVENT_GLOBAL_PATTERN_DELAY,
                                     Event_global_pattern_delay_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_JUMP_COUNTER,
                                     Event_global_set_jump_counter_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_JUMP_ROW,
                                     Event_global_set_jump_row_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_JUMP_SECTION,
                                     Event_global_set_jump_section_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_JUMP_SUBSONG,
                                     Event_global_set_jump_subsong_process);

    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_SCALE,
                                     Event_global_set_scale_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_SCALE_OFFSET,
                                     Event_global_set_scale_offset_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_MIMIC_SCALE,
                                     Event_global_mimic_scale_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SHIFT_SCALE_INTERVALS,
                                     Event_global_shift_scale_intervals_process);

    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_TEMPO,
                                     Event_global_set_tempo_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SET_VOLUME,
                                     Event_global_set_volume_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SLIDE_TEMPO,
                                     Event_global_slide_tempo_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SLIDE_TEMPO_LENGTH,
                                     Event_global_slide_tempo_length_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SLIDE_VOLUME,
                                     Event_global_slide_volume_process);
    Event_handler_set_global_process(eh, EVENT_GLOBAL_SLIDE_VOLUME_LENGTH,
                                     Event_global_slide_volume_length_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SET_INSTRUMENT,
                                 Event_channel_set_instrument_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_NOTE_ON,
                                 Event_channel_note_on_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_NOTE_OFF,
                                 Event_channel_note_off_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SET_FORCE,
                                 Event_channel_set_force_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_FORCE,
                                 Event_channel_slide_force_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_FORCE_LENGTH,
                                 Event_channel_slide_force_length_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_TREMOLO_SPEED,
                                 Event_channel_tremolo_speed_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_TREMOLO_DEPTH,
                                 Event_channel_tremolo_depth_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_TREMOLO_DELAY,
                                 Event_channel_tremolo_delay_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_PITCH,
                                 Event_channel_slide_pitch_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_PITCH_LENGTH,
                                 Event_channel_slide_pitch_length_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_VIBRATO_SPEED,
                                 Event_channel_vibrato_speed_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_VIBRATO_DEPTH,
                                 Event_channel_vibrato_depth_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_VIBRATO_DELAY,
                                 Event_channel_vibrato_delay_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_ARPEGGIO,
                                 Event_channel_arpeggio_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SET_FILTER,
                                 Event_channel_set_filter_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_FILTER,
                                 Event_channel_slide_filter_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_FILTER_LENGTH,
                                 Event_channel_slide_filter_length_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_AUTOWAH_SPEED,
                                 Event_channel_autowah_speed_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_AUTOWAH_DEPTH,
                                 Event_channel_autowah_depth_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_AUTOWAH_DELAY,
                                 Event_channel_autowah_delay_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SET_RESONANCE,
                                 Event_channel_set_resonance_process);

    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SET_PANNING,
                                 Event_channel_set_panning_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_PANNING,
                                 Event_channel_slide_panning_process);
    Event_handler_set_ch_process(eh, EVENT_CHANNEL_SLIDE_PANNING_LENGTH,
                                 Event_channel_slide_panning_length_process);

    Event_handler_set_ins_process(eh, EVENT_INS_SET_PEDAL,
                                  Event_ins_set_pedal_process);

    return eh;
}


void Event_handler_set_ch_process(Event_handler* eh,
                                  Event_type type,
                                  bool (*ch_process)(Channel_state*, char*))
{
    assert(eh != NULL);
    assert(EVENT_IS_CHANNEL(type));
    assert(ch_process != NULL);
    eh->ch_process[type] = ch_process;
    return;
}


void Event_handler_set_global_process(Event_handler* eh,
                                      Event_type type,
                                      bool (*global_process)(Playdata*,
                                                             char*))
{
    assert(eh != NULL);
    assert(EVENT_IS_GLOBAL(type));
    assert(global_process != NULL);
    eh->global_process[type] = global_process;
    return;
}


void Event_handler_set_ins_process(Event_handler* eh,
                                   Event_type type,
                                   bool (*ins_process)(Instrument_params*, char*))
{
    assert(eh != NULL);
    assert(EVENT_IS_INS(type));
    assert(ins_process != NULL);
    eh->ins_process[type] = ins_process;
    return;
}


bool Event_handler_handle(Event_handler* eh,
                          int index,
                          Event_type type,
                          char* fields)
{
    assert(eh != NULL);
    assert(EVENT_IS_VALID(type));
    if (EVENT_IS_CHANNEL(type))
    {
        assert(index >= 0);
        assert(index < KQT_COLUMNS_MAX);
        if (eh->ch_process[type] == NULL)
        {
            return false;
        }
        return eh->ch_process[type](eh->ch_states[index], fields);
    }
    else if (EVENT_IS_INS(type))
    {
        assert(index >= 0);
        assert(index < KQT_INSTRUMENTS_MAX);
        Instrument* ins = Ins_table_get(eh->insts, index);
        if (ins != NULL)
        {
            Instrument_params* ins_params = Instrument_get_params(ins);
            assert(ins_params != NULL);
            return eh->ins_process[type](ins_params, fields);
        }
    }
    else if (EVENT_IS_GLOBAL(type))
    {
        assert(index == -1);
        if (eh->global_process[type] == NULL)
        {
            return false;
        }
        return eh->global_process[type](eh->global_state, fields);
    }
    return false;
}


Playdata* Event_handler_get_global_state(Event_handler* eh)
{
    assert(eh != NULL);
    return eh->global_state;
}


bool Event_handler_add_channel_gen_state_key(Event_handler* eh,
                                             const char* key)
{
    assert(eh != NULL);
    assert(key != NULL);
    for (int i = 0; i < KQT_COLUMNS_MAX; ++i)
    {
        if (!Channel_gen_state_set_key(eh->ch_states[i]->cgstate, key))
        {
            return false;
        }
    }
    return true;
}


void del_Event_handler(Event_handler* eh)
{
    assert(eh != NULL);
    if (eh->global_state != NULL)
    {
//        del_Playdata(eh->global_state); // TODO: enable if Playdata becomes private
    }
    xfree(eh);
    return;
}


