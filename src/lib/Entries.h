

/*
 * Copyright 2010 Tomi Jylhä-Ollila
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


#ifndef K_ENTRIES_H
#define K_ENTRIES_H


#include <stdbool.h>


/**
 * Entries is a structure that contains all the raw data of a .kqt file
 * in a read-only Kunquat Handle.
 */
typedef struct Entries Entries;


/**
 * Creates a new container for Entries.
 *
 * \return   The new Entries if successful, or \c NULL if memory allocation
 *           failed.
 */
Entries* new_Entries(void);


/**
 * Adds an entry into the Entries.
 *
 * \param entries   The Entries -- must not be \c NULL.
 * \param key       The key of the entry -- must not be \c NULL.
 * \param data      The data of the entry -- must not be \c NULL and must not
 *                  be freed if this function succeeds.
 * \param length    The length of the data -- must be > \c 0.
 *
 * \return   \c true if successful, or \c false if memory allocation failed.
 */
bool Entries_set(Entries* entries,
                 const char* key,
                 void* data,
                 int32_t length);


/**
 * Gets a value of a key in the Entries.
 *
 * \param entries   The Entries -- must not be \c NULL.
 * \param key       The key of the entry -- must not be \c NULL.
 * 
 * \return   The data if \a key exists, otherwise \c NULL.
 */
void* Entries_get_data(Entries* entries, const char* key);


/**
 * Gets length of a value of a key in the Entries.
 *
 * \param entries   The Entries -- must not be \c NULL.
 * \param key       The key of the entry -- must not be \c NULL.
 * 
 * \return   The length if \a key exists, otherwise \c 0.
 */
int32_t Entries_get_length(Entries* entries, const char* key);


/**
 * Destroys Entries.
 *
 * \param entries   The Entries -- must not be \c NULL.
 */
void del_Entries(Entries* entries);


#endif // K_ENTRIES_H


