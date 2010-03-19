

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


#ifndef AUDIO_H
#define AUDIO_H


#include <stdbool.h>
#include <stdint.h>

#include <pthread.h>

#include <kunquat/Handle.h>

#include <Mix_state.h>


#define AUDIO_ERROR_LENGTH (128)


typedef struct Audio
{
    char* name;
    const char* full_name;
    bool active;
    char error[AUDIO_ERROR_LENGTH];
    bool pause;
    uint32_t nframes;
    uint32_t freq;
    kqt_Handle* handle;
    bool (*open)(struct Audio*);
    bool (*close)(struct Audio*);
    bool (*set_buffer_size)(struct Audio*, uint32_t nframes);
    bool (*set_freq)(struct Audio*, uint32_t freq);
    bool (*set_frame_format)(struct Audio*, char* format);
    bool (*set_file)(struct Audio*, char* path);
    void (*destroy)(struct Audio*);
    pthread_cond_t state_cond;
    pthread_mutex_t state_mutex;
    Mix_state state;
} Audio;


/**
 * Creates a new Audio.
 *
 * \param name   The name of the driver -- must not be \c NULL.
 *
 * \return   The new driver if successful, or \c NULL if memory allocation
 *           failed.
 */
Audio* new_Audio(char* name);


/**
 * Gets the name of the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   The name.
 */
char* Audio_get_name(Audio* audio);


/**
 * Gets the full name of the Audio (suitable for printing).
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   The full name.
 */
const char* Audio_get_full_name(Audio* audio);


/**
 * Gets an error message from the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   The error message. This is never \c NULL. Empty string indicates
 *           that the previous operation succeeded.
 */
char* Audio_get_error(Audio* audio);


/**
 * Initialises the Audio.
 *
 * \param audio     The Audio -- must not be \c NULL.
 * \param name      The name of the Audio -- must not be \c NULL.
 * \param open      The opening function -- must not be \c NULL.
 * \param close     The closing function -- must not be \c NULL.
 * \param destroy   The destructor of the Audio subclass -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_init(Audio* audio,
                char* name,
                bool (*open)(Audio*),
                bool (*close)(Audio*),
                void (*destroy)(Audio*));


/**
 * Sets the buffer size in the Audio.
 *
 * \param audio     The Audio -- must not be \c NULL.
 * \param nframes   The new size in frames -- must be > \c 0.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_set_buffer_size(Audio* audio, uint32_t nframes);


/**
 * Sets the mixing frequency in the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 * \param freq    The mixing frequency -- must be > \c 0.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_set_freq(Audio* audio, uint32_t freq);


/**
 * Sets the frame format in the Audio.
 *
 * \param audio    The Audio -- must not be \c NULL.
 * \param format   The format description -- must not be \c NULL.
 *                 The description consists of number type ('i' for integer,
 *                 'f' for float) and width. E.g. "i16" is 16-bit integer,
 *                 "f32" is 32-bit float. 8-bit integers are unsigned, others
 *                 are signed.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_set_frame_format(Audio* audio, char* format);


/**
 * Sets the output file in the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 * \param path    The path of the output file -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_set_file(Audio* audio, char* path);


/**
 * Opens the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_open(Audio* audio);


/**
 * Closes the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false.
 */
bool Audio_close(Audio* audio);


/**
 * Sets the Kunquat Handle for the Audio.
 *
 * \param audio    The Audio -- must not be \c NULL.
 * \param handle   The Handle.
 */
void Audio_set_handle(Audio* audio, kqt_Handle* handle);


/**
 * Gets the mixing frequency of the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   The mixing frequency.
 */
uint32_t Audio_get_freq(Audio* audio);


/**
 * Gets the buffer size of the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   The buffer size in frames.
 */
uint32_t Audio_get_buffer_size(Audio* audio);


/**
 * Pauses/resumes audio processing in the Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 * \param pause   \c true to pause, \c false to resume.
 */
void Audio_pause(Audio* audio, bool pause);


/**
 * Gets an updated state from the Audio.
 *
 * This call may block the calling thread. It may only be called between
 * \a Audio_open and \a Audio_close calls.
 *
 * \param audio   The Audio -- must not be \c NULL and must be open.
 * \param state   The Mix state structure -- must not be \c NULL.
 *
 * \return   \c true if Mix state could be retrieved, otherwise \c false.
 */
bool Audio_get_state(Audio* audio, Mix_state* state);


/**
 * Sends a notification of state information change.
 *
 * This function does not set the Audio error message.
 *
 * \param audio   The Audio -- must not be \c NULL.
 *
 * \return   \c 0 if successful, otherwise an error code from POSIX thread
 *           functions.
 */
int Audio_notify(Audio* audio);


/**
 * Sets an error message in the Audio.
 *
 * \param audio     The Audio -- must not be \c NULL.
 * \param message   The error message format -- must not be \c NULL. This and
 *                  subsequent arguments follow the printf family conventions.
 */
void Audio_set_error(Audio* audio, char* message, ...);


/**
 * Destroys an existing Audio.
 *
 * \param audio   The Audio -- must not be \c NULL.
 */
void del_Audio(Audio* audio);


#endif // AUDIO_H


