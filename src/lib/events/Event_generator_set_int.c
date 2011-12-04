

/*
 * Author: Tomi Jylhä-Ollila, Finland 2010-2011
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#include <Active_names.h>
#include <Event_common.h>
#include <Event_generator_set_int.h>
#include <File_base.h>
#include <Generator.h>
#include <string_common.h>
#include <xassert.h>
#include <xmemory.h>


static Event_field_desc set_int_desc[] =
{
    {
        .type = EVENT_FIELD_INT,
        .min.field.integral_type = INT64_MIN,
        .max.field.integral_type = INT64_MAX
    },
    {
        .type = EVENT_FIELD_NONE
    }
};


Event_create_constructor(Event_generator,
                         EVENT_GENERATOR_SET_INT,
                         set_int);


bool Event_generator_set_int_process(Generator* gen,
                                     Channel_state* ch_state,
                                     char* fields)
{
    assert(gen != NULL);
    if (fields == NULL)
    {
        return false;
    }
    Read_state* state = READ_STATE_AUTO;
    fields = read_const_char(fields, '[', state);
    if (state->error)
    {
        return false;
    }
    char* key = Active_names_get(ch_state->parent.active_names,
                                 ACTIVE_CAT_GEN,
                                 ACTIVE_TYPE_INT);
    if (!string_has_suffix(key, ".jsoni"))
    {
        return true;
    }
    return Device_params_modify_value(gen->conf->params, key, fields);
}


