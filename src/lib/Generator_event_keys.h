

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


#ifndef K_GENERATOR_EVENT_KEYS_H
#define K_GENERATOR_EVENT_KEYS_H


#include <Generator_params.h>
#include <Event_handler.h>
#include <File_base.h>


/**
 * Parses the Generator Event list.
 *
 * \param params   The Generator parameters -- must not be \c NULL.
 * \param eh       The Event handler -- must not be \c NULL.
 * \param str      The textual description.
 * \param state    The Read state -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false. \a state will _not_ be
 *           modified if memory allocation failed.
 */
bool Generator_params_parse_events(Generator_params* params,
                                   Event_handler* eh,
                                   char* str,
                                   Read_state* state);


#endif // K_GENERATOR_EVENT_KEYS


