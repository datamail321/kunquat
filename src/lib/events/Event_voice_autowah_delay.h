

/*
 * Author: Tomi Jylhä-Ollila, Finland 2010
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat. This work
 * is published from various territories.
 */


#ifndef K_EVENT_VOICE_AUTOWAH_DELAY_H
#define K_EVENT_VOICE_AUTOWAH_DELAY_H


#include <Event_voice.h>
#include <Reltime.h>


typedef struct Event_voice_autowah_delay
{
    Event_voice parent;
    Reltime delay;
} Event_voice_autowah_delay;


Event* new_Event_voice_autowah_delay(Reltime* pos);


#endif // K_EVENT_VOICE_AUTOWAH_DELAY_H


