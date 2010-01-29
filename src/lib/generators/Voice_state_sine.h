

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


#ifndef K_VOICE_STATE_SINE_H
#define K_VOICE_STATE_SINE_H


#include <Voice_state.h>


typedef struct Voice_state_sine
{
    Voice_state parent;
    double phase;
} Voice_state_sine;


/**
 * Initialises the Sine Instrument parameters.
 *
 * \param sine   The Sine parameters -- must not be \c NULL.
 */
void Voice_state_sine_init(Voice_state* state);


#endif // K_VOICE_STATE_SINE_H

