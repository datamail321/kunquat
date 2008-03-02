

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
#include <string.h>
#include <math.h>

#include <Reltime.h>
#include <Event.h>
#include <Instrument.h>
#include <Ins_table.h>
#include <Pattern.h>
#include <Pat_table.h>
#include <Song.h>

#include "Listener.h"
#include "Listener_demo.h"


typedef struct Ndesc
{
	Event_type type;
	double pos;
	int note;
	int octave;
} Ndesc;

static Ndesc demo_p1ch1[] =
{
	{ EVENT_TYPE_NOTE_ON,     0, 0, 8 },
	{ EVENT_TYPE_NOTE_OFF, 0.25, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,   0.5, 0, 8 },
	{ EVENT_TYPE_NOTE_OFF,  0.7, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     1, 0, 8 },
	{ EVENT_TYPE_NOTE_ON,   1.5, 4, 8 },
	{ EVENT_TYPE_NOTE_ON,     2, 2, 8 },
	{ EVENT_TYPE_NOTE_OFF,  2.2, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,   2.5, 2, 8 },
	{ EVENT_TYPE_NOTE_OFF,  2.6, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     3, 2, 8 },
	{ EVENT_TYPE_NOTE_ON,   3.5, 5, 8 },
	{ EVENT_TYPE_NOTE_ON,     4, 4, 8 },
	{ EVENT_TYPE_NOTE_OFF,  4.3, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,   4.5, 0, 8 },
	{ EVENT_TYPE_NOTE_ON,     5, 2, 8 },
	{ EVENT_TYPE_NOTE_OFF, 5.25, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,   5.5, 2, 8 },
	{ EVENT_TYPE_NOTE_OFF, 5.65, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     6, 0, 8 },
	{ EVENT_TYPE_NOTE_OFF,    7, 0, 0 },
	{ EVENT_TYPE_NONE, 0, 0, 0 }
};

static Ndesc demo_p2ch1[] =
{
	{ EVENT_TYPE_NOTE_ON,     0, 4, 8 },
	{ EVENT_TYPE_NOTE_OFF,  0.2, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,   0.5, 4, 8 },
	{ EVENT_TYPE_NOTE_OFF, 0.65, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     1, 4, 8 },
	{ EVENT_TYPE_NOTE_ON,   1.5, 7, 8 },
	{ EVENT_TYPE_NOTE_ON,     2, 5, 8 },
	{ EVENT_TYPE_NOTE_OFF,  2.6, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     3, 5, 8 },
	{ EVENT_TYPE_NOTE_OFF,  3.5, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     4, 2, 8 },
	{ EVENT_TYPE_NOTE_OFF,  4.2, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,   4.5, 2, 8 },
	{ EVENT_TYPE_NOTE_OFF, 4.65, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     5, 2, 8 },
	{ EVENT_TYPE_NOTE_ON,   5.5, 5, 8 },
	{ EVENT_TYPE_NOTE_ON,     6, 4, 8 },
	{ EVENT_TYPE_NOTE_OFF,  6.6, 0, 0 },
	{ EVENT_TYPE_NOTE_ON,     7, 4, 8 },
	{ EVENT_TYPE_NOTE_OFF,  7.5, 0, 0 },
	{ EVENT_TYPE_NONE, 0, 0, 0 }
};

static Ndesc demo_p2ch2[] =
{
	{ EVENT_TYPE_NOTE_ON,     2,  9, 8 },
	{ EVENT_TYPE_NOTE_ON,   2.5,  2, 9 },
	{ EVENT_TYPE_NOTE_ON,  2.75,  0, 9 },
	{ EVENT_TYPE_NOTE_ON,     3, 11, 8 },
	{ EVENT_TYPE_NOTE_OFF, 3.15,  0, 0 },
	{ EVENT_TYPE_NOTE_ON,   3.5,  9, 8 },
	{ EVENT_TYPE_NOTE_OFF,  3.6,  0, 0 },
	{ EVENT_TYPE_NOTE_ON,     4,  7, 8 },
	{ EVENT_TYPE_NOTE_OFF, 5.25,  0, 0 },
	{ EVENT_TYPE_NOTE_ON,   5.5,  9, 8 },
	{ EVENT_TYPE_NOTE_ON,  5.66, 10, 8 },
	{ EVENT_TYPE_NOTE_ON,  5.83, 11, 8 },
	{ EVENT_TYPE_NOTE_ON,     6,  0, 9 },
	{ EVENT_TYPE_NOTE_OFF,  6.2,  0, 0 },
	{ EVENT_TYPE_NOTE_ON,   6.5,  9, 8 },
	{ EVENT_TYPE_NOTE_OFF,  6.7,  0, 0 },
	{ EVENT_TYPE_NOTE_ON,     7,  7, 8 },
	{ EVENT_TYPE_NOTE_OFF,  7.4,  0, 0 },
	{ EVENT_TYPE_NONE, 0, 0, 0 }
};


int Listener_demo(const char* path,
		const char* types,
		lo_arg** argv,
		int argc,
		lo_message msg,
		void* user_data)
{
	(void)path;
	(void)types;
	(void)argv;
	(void)argc;
	(void)msg;
	assert(user_data != NULL);
	Listener* lr = user_data;
	if (lr->host == NULL)
	{
		return 0;
	}
	if (lr->driver_id == -1)
	{
		strcpy(lr->method_path + lr->host_path_len, "error");
		lo_send(lr->host, lr->method_path, "s", "No audio driver active");
		return 0;
	}
	Player* player = NULL;
	Song* song = new_Song(2, 128, 16); // TODO: get params from relevant parts of the Listener
	if (song == NULL)
	{
		goto cleanup;
	}
	Note_table* notes = Song_get_notes(song);
	Note_table_set_ref_pitch(notes, 528);
	int nums[] = { 16, 9, 6, 5, 4, 45, 3, 8, 5, 9, 15 };
	int dens[] = { 15, 8, 5, 4, 3, 32, 2, 5, 3, 5, 8 };
	for (int i = 1; i <= 11; ++i)
	{
		Note_table_set_note(notes, i,
				Note_table_get_note_name(notes, i),
				Real_init_as_frac(REAL_AUTO, nums[i - 1], dens[i - 1]));
	}
	Note_table_set_name(notes, L"5-limit JI (C major)");
	Song_set_tempo(song, 110);
	Song_set_mix_vol(song, 0);
	Song_set_global_vol(song, 0);
	player = new_Player(lr->freq, lr->voice_count, song);
	if (player == NULL)
	{
		goto cleanup;
	}
	assert(song != NULL);
	frame_t** bufs = Song_get_bufs(song);
	Instrument* ins = new_Instrument(INS_TYPE_SINE, bufs, 128, 16);
	if (ins == NULL)
	{
		goto cleanup;
	}
	Instrument_set_name(ins, L"sine");
	Instrument_set_note_table(ins, Song_get_notes(song));
	Ins_table* insts = Song_get_insts(song);
	if (!Ins_table_set(insts, 1, ins))
	{
		del_Instrument(ins);
		goto cleanup;
	}
	Pattern* pat = new_Pattern();
	if (pat == NULL)
	{
		goto cleanup;
	}
	Pat_table* pats = Song_get_pats(song);
	if (!Pat_table_set(pats, 0, pat))
	{
		del_Pattern(pat);
		goto cleanup;
	}
	Pattern_set_length(pat, Reltime_set(RELTIME_AUTO, 8, 0));
	for (int i = 0; demo_p1ch1[i].type != EVENT_TYPE_NONE; ++i)
	{
		Event* ev = new_Event(Reltime_init(RELTIME_AUTO), demo_p1ch1[i].type);
		if (ev == NULL)
		{
			goto cleanup;
		}
		Event_set_pos(ev, Reltime_set(RELTIME_AUTO,
				(int)floor(demo_p1ch1[i].pos),
				(int)((demo_p1ch1[i].pos - floor(demo_p1ch1[i].pos))
						* RELTIME_FULL_PART)));
		if (demo_p1ch1[i].type == EVENT_TYPE_NOTE_ON)
		{
			Event_set_int(ev, 0, demo_p1ch1[i].note);
			Event_set_int(ev, 1, -1);
			Event_set_int(ev, 2, demo_p1ch1[i].octave);
			Event_set_int(ev, 3, 1);
		}
		Column* col = Pattern_col(pat, 0);
		if (!Column_ins(col, ev))
		{
			del_Event(ev);
			goto cleanup;
		}
	}
	pat = new_Pattern();
	if (pat == NULL)
	{
		goto cleanup;
	}
	if (!Pat_table_set(pats, 1, pat))
	{
		del_Pattern(pat);
		goto cleanup;
	}
	Pattern_set_length(pat, Reltime_set(RELTIME_AUTO, 8, 0));
	for (int i = 0; demo_p2ch1[i].type != EVENT_TYPE_NONE; ++i)
	{
		Event* ev = new_Event(Reltime_init(RELTIME_AUTO), demo_p2ch1[i].type);
		if (ev == NULL)
		{
			goto cleanup;
		}
		Event_set_pos(ev, Reltime_set(RELTIME_AUTO,
				(int)floor(demo_p2ch1[i].pos),
				(int)((demo_p2ch1[i].pos - floor(demo_p2ch1[i].pos))
						* RELTIME_FULL_PART)));
		if (demo_p2ch1[i].type == EVENT_TYPE_NOTE_ON)
		{
			Event_set_int(ev, 0, demo_p2ch1[i].note);
			Event_set_int(ev, 1, -1);
			Event_set_int(ev, 2, demo_p2ch1[i].octave);
			Event_set_int(ev, 3, 1);
		}
		Column* col = Pattern_col(pat, 0);
		if (!Column_ins(col, ev))
		{
			del_Event(ev);
			goto cleanup;
		}
	}
	for (int i = 0; demo_p2ch2[i].type != EVENT_TYPE_NONE; ++i)
	{
		Event* ev = new_Event(Reltime_init(RELTIME_AUTO), demo_p2ch2[i].type);
		if (ev == NULL)
		{
			goto cleanup;
		}
		Event_set_pos(ev, Reltime_set(RELTIME_AUTO,
				(int)floor(demo_p2ch2[i].pos),
				(int)((demo_p2ch2[i].pos - floor(demo_p2ch2[i].pos))
						* RELTIME_FULL_PART)));
		if (demo_p2ch2[i].type == EVENT_TYPE_NOTE_ON)
		{
			Event_set_int(ev, 0, demo_p2ch2[i].note);
			Event_set_int(ev, 1, -1);
			Event_set_int(ev, 2, demo_p2ch2[i].octave);
			Event_set_int(ev, 3, 1);
		}
		Column* col = Pattern_col(pat, 1);
		if (!Column_ins(col, ev))
		{
			del_Event(ev);
			goto cleanup;
		}
	}
	Order* order = Song_get_order(song);
	if (!Order_set(order, 0, 0, 0))
	{
		goto cleanup;
	}
	if (!Order_set(order, 0, 1, 1))
	{
		goto cleanup;
	}
	if (!Order_set(order, 0, 2, 0))
	{
		goto cleanup;
	}
	Playlist_ins(lr->playlist, player);
	Player_set_state(player, PLAY_SONG);
	strcpy(lr->method_path + lr->host_path_len, "new_song");
	int ret = lo_send(lr->host, lr->method_path, "i", player->id);
	if (ret == -1)
	{
		fprintf(stderr, "Failed to send the response message\n");
		return 0;
	}
	return 0;

cleanup:
	
	if (player != NULL)
	{
		del_Player(player);
	}
	else if (song != NULL)
	{
		del_Song(song);
	}
	strcpy(lr->method_path + lr->host_path_len, "error");
	lo_send(lr->host, lr->method_path, "s", "Couldn't allocate memory");
	return 0;
}

