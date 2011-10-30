

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
#include <stdint.h>
#include <math.h>

#include <Event_common.h>
#include <Event_channel_tremolo_delay.h>
#include <Reltime.h>
#include <Voice.h>
#include <math_common.h>
#include <kunquat/limits.h>
#include <xassert.h>
#include <xmemory.h>


static Event_field_desc tremolo_delay_desc[] =
{
    {
        .type = EVENT_FIELD_RELTIME,
        .min.field.Reltime_type = { 0, 0 },
        .max.field.Reltime_type = { INT64_MAX, KQT_RELTIME_BEAT - 1 }
    },
    {
        .type = EVENT_FIELD_NONE
    }
};


Event_create_constructor(Event_channel,
                         EVENT_CHANNEL_TREMOLO_DELAY,
                         tremolo_delay);


bool Event_channel_tremolo_delay_process(Channel_state* ch_state, char* fields)
{
    assert(ch_state != NULL);
    if (fields == NULL)
    {
        return false;
    }
    Event_field data[1];
    Read_state* state = READ_STATE_AUTO;
    Event_type_get_fields(fields, tremolo_delay_desc, data, state);
    if (state->error)
    {
        return false;
    }
    Reltime_copy(&ch_state->tremolo_depth_delay, &data[0].field.Reltime_type);
    LFO_set_depth_delay(&ch_state->tremolo, &data[0].field.Reltime_type);
    for (int i = 0; i < KQT_GENERATORS_MAX; ++i)
    {
        Event_check_voice(ch_state, i);
        Voice_state* vs = ch_state->fg[i]->state;
        LFO_set_depth_delay(&vs->tremolo, &data[0].field.Reltime_type);
    }
    return true;
}

