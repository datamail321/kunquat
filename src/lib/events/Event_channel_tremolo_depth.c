

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
#include <stdbool.h>
#include <math.h>

#include <Event_common.h>
#include <Event_channel_tremolo_depth.h>
#include <Reltime.h>
#include <Voice.h>
#include <math_common.h>
#include <xassert.h>
#include <xmemory.h>


static Event_field_desc tremolo_depth_desc[] =
{
    {
        .type = EVENT_FIELD_DOUBLE,
        .min.field.double_type = 0,
        .max.field.double_type = 24
    },
    {
        .type = EVENT_FIELD_NONE
    }
};


Event_create_constructor(Event_channel,
                         EVENT_CHANNEL_TREMOLO_DEPTH,
                         tremolo_depth);


bool Event_channel_tremolo_depth_process(Channel_state* ch_state, char* fields)
{
    assert(ch_state != NULL);
    if (fields == NULL)
    {
        return false;
    }
    Event_field data[1];
    Read_state* state = READ_STATE_AUTO;
    Event_type_get_fields(fields, tremolo_depth_desc, data, state);
    if (state->error)
    {
        return false;
    }
    double actual_depth = data[0].field.double_type / 6;
    ch_state->tremolo_depth = actual_depth;
    for (int i = 0; i < KQT_GENERATORS_MAX; ++i)
    {
        Event_check_voice(ch_state, i);
        Voice_state* vs = ch_state->fg[i]->state;
        if (ch_state->tremolo_speed > 0)
        {
            LFO_set_speed(&vs->tremolo, ch_state->tremolo_speed);
        }
        LFO_set_depth(&vs->tremolo, actual_depth);
        LFO_turn_on(&vs->tremolo);
    }
    return true;
}


