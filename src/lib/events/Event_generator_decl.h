

/*
 * Author: Tomi Jylhä-Ollila, Finland 2013
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#ifndef K_EVENT_GENERATOR_DECL_H
#define K_EVENT_GENERATOR_DECL_H


#include <stdbool.h>

#include <Generator.h>
#include <player/Channel.h>
#include <Value.h>


// Process function declarations

#define EVENT_TYPE_DEF(type)               \
    bool Event_generator_##type##_process( \
            Generator* gen,                \
            Channel* ch,                   \
            Value* value);
#include <events/Event_generator_types.h>


#endif // K_EVENT_GENERATOR_DECL_H


