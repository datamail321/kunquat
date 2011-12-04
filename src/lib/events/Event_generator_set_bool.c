

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
#include <stdbool.h>

#include <Active_names.h>
#include <Event_common.h>
#include <Event_generator_set_bool.h>
#include <File_base.h>
#include <Generator.h>
#include <string_common.h>
#include <xassert.h>
#include <xmemory.h>


static Event_field_desc set_bool_desc[] =
{
    {
        .type = EVENT_FIELD_BOOL
    },
    {
        .type = EVENT_FIELD_NONE
    }
};


Event_create_constructor(Event_generator,
                         EVENT_GENERATOR_SET_BOOL,
                         set_bool);


bool Event_generator_set_bool_process(Generator* gen,
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
                                 ACTIVE_TYPE_BOOL);
    if (!string_has_suffix(key, ".jsonb"))
    {
        return true;
    }
    return Device_params_modify_value(gen->conf->params, key, fields);
}


