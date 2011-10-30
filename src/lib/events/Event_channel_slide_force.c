

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
#include <limits.h>

#include <Event_common.h>
#include <Event_channel_slide_force.h>
#include <Reltime.h>
#include <Voice.h>
#include <xassert.h>
#include <xmemory.h>


static Event_field_desc slide_force_desc[] =
{
    {
        .type = EVENT_FIELD_DOUBLE,
        .min.field.double_type = -INFINITY,
        .max.field.double_type = 18
    },
    {
        .type = EVENT_FIELD_NONE
    }
};


Event_create_constructor(Event_channel,
                         EVENT_CHANNEL_SLIDE_FORCE,
                         slide_force);


bool Event_channel_slide_force_process(Channel_state* ch_state, char* fields)
{
    assert(ch_state != NULL);
    if (fields == NULL)
    {
        return false;
    }
    Event_field data[1];
    Read_state* state = READ_STATE_AUTO;
    Event_type_get_fields(fields, slide_force_desc, data, state);
    if (state->error)
    {
        return false;
    }
    double slide_target = exp2(data[0].field.double_type / 6);
    for (int i = 0; i < KQT_GENERATORS_MAX; ++i)
    {
        Event_check_voice(ch_state, i);
        Voice_state* vs = ch_state->fg[i]->state;
        if (Slider_in_progress(&vs->force_slider))
        {
            Slider_change_target(&vs->force_slider, slide_target);
        }
        else
        {
            Slider_start(&vs->force_slider, slide_target, vs->force);
        }
    }
    return true;
}

