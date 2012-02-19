

/*
 * Author: Tomi Jylhä-Ollila, Finland 2011-2012
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <stdlib.h>

#include <Event_common.h>
#include <Event_general_call_bool.h>
#include <General_state.h>
#include <Value.h>
#include <xassert.h>
#include <xmemory.h>


bool Event_general_call_bool_process(General_state* gstate, Value* value)
{
    assert(gstate != NULL);
    assert(value != NULL);
    return value->type == VALUE_TYPE_BOOL;
}


