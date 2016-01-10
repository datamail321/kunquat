

/*
 * Author: Tomi Jylhä-Ollila, Finland 2016
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#ifndef K_PITCH_STATE_H
#define K_PITCH_STATE_H


#include <decl.h>
#include <player/devices/Proc_state.h>
#include <player/devices/Voice_state.h>


Voice_state_get_size_func Pitch_vstate_get_size;

void Pitch_vstate_init(Voice_state* vstate, const Proc_state* proc_state);


#endif // K_PITCH_STATE_H


