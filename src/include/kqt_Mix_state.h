

/*
 * Copyright 2009 Tomi Jylhä-Ollila
 *
 * This file is part of Kunquat.
 *
 * Kunquat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Kunquat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Kunquat.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef KQT_MIX_STATE_H
#define KQT_MIX_STATE_H


#include <stdbool.h>
#include <stdint.h>

#include <kqt_Reltime.h>


typedef struct kqt_Mix_state
{
    bool playing;          ///< Whether anything is playing.
    uint64_t frames;       ///< Number of frames mixed.
    uint16_t subsong;      ///< Number of the current Subsong.
    uint16_t order;        ///< The current Order index.
    uint16_t pattern;      ///< The current Pattern index.
    kqt_Reltime pos;       ///< The current position inside a Pattern.
    double tempo;          ///< The current tempo (BPM).
    uint16_t voices;       ///< The maximum number of simultaneous Voices since the last update.
    double min_amps[2];    ///< Minimum amplitude values since the last update.
    double max_amps[2];    ///< Maximum amplitude values since the last update.
    uint64_t clipped[2];   ///< Number of clipped frames encountered.
} kqt_Mix_state;


/**
 * A new instance of an uninitialised Mix state object with automatic storage
 * allocation.
 * Useful for passing as a parameter to an initialiser.
 */
#define KQT_MIX_STATE_AUTO (&(kqt_Mix_state){ .playing = false })


/**
 * Initialises the Mix state.
 *
 * \param state   The Mix state. If \a state == \c NULL, nothing happens.
 *
 * \return   The parameter \a state.
 */
kqt_Mix_state* kqt_Mix_state_init(kqt_Mix_state* state);


/**
 * Copies a Mix state into another.
 *
 * \param dest   The destination Mix state. If \a dest == \c NULL, nothing
 *               happens.
 * \param src    The source Mix state. If \ə src == \c NULL, nothing happens.
 *
 * \return   The parameter \ə dest.
 */
kqt_Mix_state* kqt_Mix_state_copy(kqt_Mix_state* dest, kqt_Mix_state* src);


#endif // KQT_MIX_STATE_H


