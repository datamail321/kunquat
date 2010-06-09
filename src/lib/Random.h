

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


#ifndef K_RANDOM_H
#define K_RANDOM_H


#include <stdlib.h>
#include <stdint.h>


/**
 * This is a portable pseudo-random generator.
 */
typedef struct Random Random;


/**
 * The maximum number generated by the pseudo-random generator.
 */
#define KQT_RANDOM_MAX UINT32_MAX


/**
 * Creates a new Random generator.
 *
 * \return   The new Random if successful, or \c NULL if memory allocation
 *           failed.
 */
Random* new_Random(void);


/**
 * Sets the random seed in the Random.
 *
 * \param random   The Random generator -- must not be \c NULL.
 * \param seed     The random seed.
 */
void Random_set_seed(Random* random, uint32_t seed);


/**
 * Restarts the random sequence in the Random.
 *
 * \param random   The Random generator -- must not be \c NULL.
 */
void Random_reset(Random* random);


/**
 * Gets a pseudo-random number from the Random generator
 *
 * \param random   The Random generator -- must not be \c NULL.
 *
 * \return   A pseudo-random integer in the range [0, KQT_RANDOM_MAX].
 */
uint32_t Random_get(Random* random);


/**
 * Destroys an existing Random generator.
 *
 * \param random   The Random generator -- must not be \c NULL.
 */
void del_Random(Random* random);


#endif // K_RANDOM_H


