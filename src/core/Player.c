

/*
 * Copyright 2008 Tomi Jylhä-Ollila
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


#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "Player.h"

#include <xmemory.h>


Player* new_Player(uint32_t freq, uint16_t voices, Song* song)
{
	static int32_t id = 0;
	assert(freq > 0);
//	assert(pool != NULL);
	assert(voices > 0);
	assert(voices < MAX_VOICES);
	assert(song != NULL);
	Player* player = xalloc(Player);
	if (player == NULL)
	{
		return NULL;
	}
	player->voices = new_Voice_pool(voices, 32); // TODO: event count
	if (player->voices == NULL)
	{
		xfree(player);
		return NULL;
	}
	player->prev = player->next = NULL;
	player->song = song;
	player->play = new_Playdata(freq, player->voices, Song_get_insts(song));
	if (player->play == NULL)
	{
		del_Voice_pool(player->voices);
		xfree(player);
		return NULL;
	}
	player->play->order = Song_get_order(song);
	player->play->events = Song_get_events(song);
	player->id = id++;
	return player;
}


uint32_t Player_mix(Player* player, uint32_t nframes)
{
	assert(player != NULL);
	if (!player->play || player->song == NULL)
	{
		return 0;
	}
/*	int buf_count = Song_get_buf_count(player->song);
	frame_t** bufs = Song_get_bufs(player->song);
	for (int i = 0; i < buf_count; ++i)
	{
		assert(bufs[i] != NULL);
		for (uint32_t k = 0; k < nframes; ++k)
		{
			bufs[i][k] = 0;
		}
	} */
	return Song_mix(player->song, nframes, player->play);
}


void Player_play_pattern(Player* player, int16_t num, double tempo)
{
	assert(player != NULL);
	assert(num >= 0);
	assert(num < PATTERNS_MAX);
	assert(isfinite(tempo));
	assert(tempo > 0);
	Player_stop(player);
	player->play->pattern = num;
	player->play->tempo = tempo;
	player->play->mode = PLAY_PATTERN;
	return;
}


void Player_play_subsong(Player* player, uint16_t subsong)
{
	assert(player != NULL);
	assert(subsong < SUBSONGS_MAX);
	Player_stop(player);
	player->play->subsong = subsong;
	player->play->tempo = Song_get_tempo(player->song, player->play->subsong);
	player->play->mode = PLAY_SONG;
	return;
}


void Player_play_song(Player* player)
{
	assert(player != NULL);
	Player_stop(player);
	player->play->subsong = Song_get_subsong(player->song);
	player->play->mode = PLAY_SONG;
	return;
}


void Player_play_event(Player* player)
{
	assert(player != NULL);
	if (player->play->mode >= PLAY_EVENT)
	{
		return;
	}
	Player_stop(player);
	player->play->mode = PLAY_EVENT;
	return;
}


void Player_stop(Player* player)
{
	assert(player != NULL);
	player->play->mode = STOP;
	Voice_pool_reset(player->voices);
	for (int i = 0; i < COLUMNS_MAX; ++i)
	{
		Channel_reset(player->play->channels[i]);
	}
	Reltime_init(&player->play->play_time);
	player->play->play_frames = 0;
	player->play->subsong = Song_get_subsong(player->song);
	player->play->tempo = Song_get_tempo(player->song, player->play->subsong);
	player->play->order_index = 0;
	player->play->pattern = 0;
	Reltime_init(&player->play->pos);
	return;
}


void Player_set_mix_freq(Player* player, uint32_t freq)
{
	assert(player != NULL);
	assert(freq > 0);
	Playdata_set_mix_freq(player->play, freq);
	return;
}


void del_Player(Player* player)
{
	assert(player != NULL);
	del_Playdata(player->play);
	del_Voice_pool(player->voices);
	if (player->song != NULL)
	{
		del_Song(player->song);
	}
	xfree(player);
}


