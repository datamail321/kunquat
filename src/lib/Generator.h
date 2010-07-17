

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


#ifndef K_GENERATOR_H
#define K_GENERATOR_H


#include <stdbool.h>
#include <stdint.h>

#include <Device.h>
#include <Device_params.h>
#include <Gen_conf.h>
#include <Generator_type.h>
#include <Instrument_params.h>
#include <kunquat/limits.h>
#include <pitch_t.h>
#include <Random.h>
#include <Voice_state.h>
#include <File_base.h>


/**
 * Generator is an object used for creating sound based on a specific sound
 * synthesising method.
 */
typedef struct Generator
{
    Device parent;
    Gen_type type;
#if 0
    bool enabled;
    double volume_dB;
    double volume;
    bool pitch_lock_enabled;
    double pitch_lock_cents;
    pitch_t pitch_lock_freq;
#endif
    Random* random;
//    Device_params* type_params;
    Gen_conf* conf;
    void (*init_state)(struct Generator*, Voice_state*);
    void (*destroy)(struct Generator*);
    uint32_t (*mix)(struct Generator*, Voice_state*, uint32_t, uint32_t,
                    uint32_t, double);
    Instrument_params* ins_params;
} Generator;


#define GENERATOR_DEFAULT_ENABLED (false)
#define GENERATOR_DEFAULT_VOLUME (0)
#define GENERATOR_DEFAULT_PITCH_LOCK_ENABLED (false)
#define GENERATOR_DEFAULT_PITCH_LOCK_CENTS (0)


/**
 * Creates a new Generator of the specified type.
 *
 * \param type          The Generator type -- must be a valid and supported
 *                      type.
 * \param ins_params    The Instrument parameters -- must not be \c NULL.
 * \param buffer_size   The mixing buffer size -- must be > \c 0 and
 *                      <= \c KQT_BUFFER_SIZE_MAX.
 * \param mix_rate      The mixing rate -- must be > \c 0.
 * \param random        The Random source -- must not be \c NULL.
 *
 * \return   The new Generator if successful, or \c NULL if memory allocation
 *           failed.
 */
Generator* new_Generator(Gen_type type,
                         Instrument_params* ins_params,
//                         Device_params* gen_params,
                         uint32_t buffer_size,
                         uint32_t mix_rate,
                         Random* random);


/**
 * Initialises the general Generator parameters.
 *
 * \param gen      The Generator -- must not be \c NULL.
 *
 * \return   \c true if successful, or \c false if memory allocation failed.
 */
bool Generator_init(Generator* gen);


/**
 * Uninitialises the general Generator parameters.
 *
 * \param gen   The Generator -- must not be \c NULL.
 */
void Generator_uninit(Generator* gen);


/**
 * Sets the configuration of the Generator.
 *
 * \param gen    The Generator -- must not be \c NULL.
 * \param conf   The Generator configuration -- must not be \c NULL.
 */
void Generator_set_conf(Generator* gen, Gen_conf* conf);


/**
 * Retrieves the Generator parameter tree.
 *
 * \param gen   The Generator -- must not be \c NULL.
 *
 * \return   The Generator parameter tree.
 */
Device_params* Generator_get_params(Generator* gen);


/**
 * Copies the general Generator parameters.
 *
 * \param dest   The destination Generator -- must not be \c NULL.
 * \param src    The source Generator -- must not be \c NULL.
 */
//void Generator_copy_general(Generator* dest, Generator* src);


/**
 * Parses general Generator header (p_generator.json).
 *
 * \param gen     The Generator -- must not be \c NULL.
 * \param str     The textual description.
 * \param state   The Read state -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false. The state error will
 *           _not_ be set in case memory allocation failed.
 */
//bool Generator_parse_general(Generator* gen, char* str, Read_state* state);


/**
 * Parses a Generator parameter.
 *
 * \param gen      The Generator -- must not be \c NULL.
 * \param subkey   The subkey of the parameter -- must begin with either "i/"
 *                 or "c/".
 * \param data     The data -- must not be \c NULL unless \a length is 0.
 * \param length   The length of the data -- must be >= \c 0.
 * \param state    The Read state -- must not be \c NULL.
 *
 * \return   \c true if successful, otherwise \c false. \a state will not be
 *           modified if memory allocation failed.
 */
//bool Generator_parse_param(Generator* gen,
//                           const char* subkey,
//                           void* data,
//                           long length,
//                           Read_state* state);


/**
 * Returns the type of the Generator.
 *
 * \param gen   The Generator -- must not be \c NULL.
 *
 * \return   The type.
 */
Gen_type Generator_get_type(Generator* gen);


/**
 * Handles a given note as appropriate for the Generator.
 *
 * \param gen      The Generator -- must not be \c NULL.
 * \param states   The array of Voice states -- must not be \c NULL.
 * \param cents    The pitch in cents -- must be finite.
 */
void Generator_process_note(Generator* gen,
                            Voice_state* states,
                            double cents);


/**
 * Mixes the Generator.
 *
 * \param gen       The Generator -- must not be \c NULL.
 * \param state     The Voice state -- must not be \c NULL.
 * \param nframes   The number of frames to be mixed -- must not be greater
 *                  than the mixing buffer size.
 * \param offset    The starting frame offset (\a nframes - \a offset are
 *                  actually mixed).
 * \param freq      The mixing frequency -- must be > \c 0.
 * \param tempo     The current tempo -- must be > \c 0.
 */
void Generator_mix(Generator* gen,
                   Voice_state* state,
                   uint32_t nframes,
                   uint32_t offset,
                   uint32_t freq,
                   double tempo);


/**
 * Uninitialises an existing Generator.
 *
 * \param gen   The Generator -- must not be \c NULL.
 */
void del_Generator(Generator* gen);


#endif // K_GENERATOR_H


