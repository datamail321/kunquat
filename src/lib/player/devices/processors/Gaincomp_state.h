

/*
 * Author: Tomi Jylhä-Ollila, Finland 2015-2017
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#ifndef KQT_GAINCOMP_STATE_H
#define KQT_GAINCOMP_STATE_H


#include <player/devices/Device_state.h>
#include <player/devices/Voice_state.h>


Device_state_create_func new_Gaincomp_pstate;

Voice_state_get_size_func Gaincomp_vstate_get_size;
Voice_state_render_voice_func Gaincomp_vstate_render_voice;


#endif // KQT_GAINCOMP_STATE_H


