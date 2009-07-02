

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


#ifndef K_CONTEXT_H
#define K_CONTEXT_H


#include <stdint.h>

#include <Playdata.h>
#include <Song.h>
#include <Voice_pool.h>


#define MAX_VOICES (1024)


typedef struct kqt_Context
{
    Song* song;
    Playdata* play;
    Voice_pool* voices;
} kqt_Context;


/**
 * Creates a new Kunquat Context.
 *
 * \param buf_count          The number of buffers used for mixing. Currently,
 *                           this can be 1 (mono) or 2 (stereo).
 * \param buf_size           The size of the mixing buffer.
 * \param voice_count        The number of Voices used for mixing.
 * \param event_queue_size   The size of the Event queue for each Column.
 *
 * \return   The new Kunquat Context if successful, or \c NULL if memory
 *           allocation failed.
 */
kqt_Context* kqt_new_Context(int buf_count,
                             uint32_t buf_size,
                             uint16_t voice_count,
                             uint8_t event_queue_size);


/**
 * Creates a new Kunquat Context from a file or a directory.
 *
 * \param path               The path to the Kunquat composition file or directory.
 * \param buf_size           The size of the mixing buffer.
 * \param voice_count        The number of Voices used for mixing.
 * \param event_queue_size   The size of the Event queue for each Column.
 *
 * \return   The new Kunquat Context if successful, otherwise \c NULL.
 */
kqt_Context* kqt_new_Context_from_path(char* path,
                                       uint32_t buf_size,
                                       uint16_t voice_count,
                                       uint8_t event_queue_size);


/**
 * Gets the Song of the Kunquat Context.
 *
 * \param context   The Context -- must not be \c NULL.
 *
 * \return   The Song.
 */
Song* kqt_Context_get_song(kqt_Context* context);


/**
 * Gets the Playdata of the Kunquat Context.
 *
 * \param context   The Context -- must not be \c NULL.
 *
 * \return   The Playdata.
 */
Playdata* kqt_Context_get_playdata(kqt_Context* context);


/**
 * Does mixing according to the state of the Kunquat Context.
 *
 * \param context   The Context -- must not be \c NULL.
 * \param nframes   The number of frames to be mixed.
 *
 * \return   The number of frames actually mixed. This is always
 *           <= \a nframes.
 */
uint32_t kqt_Context_mix(kqt_Context* context, uint32_t nframes);


/**
 * Plays one Event. The caller should have set the Event in the desired
 * Channel beforehand.
 *
 * \param context   The kqt_Context -- must not be \c NULL.
 */
void kqt_Context_play_event(kqt_Context* context);


/**
 * Plays one Pattern.
 *
 * \param context   The Context -- must not be \c NULL.
 * \param num       The number of the Pattern -- must be >= \c 0 and
 *                  < \c PATTERNS_MAX.
 * \param tempo     The tempo -- must be > \c 0.
 */
void kqt_Context_play_pattern(kqt_Context* context, int16_t num, double tempo);


/**
 * Plays a subsong.
 *
 * \param context   The Context -- must not be \c NULL.
 * \param num       The number of the subsong -- must be < \c SUBSONGS_MAX.
 */
void kqt_Context_play_subsong(kqt_Context* context, uint16_t subsong);


/**
 * Plays the default subsong of the Song.
 *
 * \param context   The Context -- must not be \c NULL.
 */
void kqt_Context_play_song(kqt_Context* context);


/**
 * Stops playback.
 *
 * \param context   The Context -- must not be \c NULL.
 */
void kqt_Context_stop(kqt_Context* context);


/**
 * Sets a new mixing frequency.
 * 
 * \param context   The Context -- must not be \c NULL.
 * \param freq      The mixing frequency -- must be > \c 0.
 */
void kqt_Context_set_mix_freq(kqt_Context* context, uint32_t freq);


/**
 * Destroys an existing Kunquat Context.
 *
 * The Song inside the context is also destroyed.
 *
 * \param context   The Context -- must not be \c NULL.
 */
void kqt_del_Context(kqt_Context* context);


#endif // K_CONTEXT_H


