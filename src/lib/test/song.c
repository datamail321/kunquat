

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


#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <math.h>
#include <wchar.h>

#include <check.h>

#include <Real.h>
#include <Scale.h>
#include <Reltime.h>
#include <Event.h>
#include <Event_voice_note_on.h>
#include <Event_voice_note_off.h>
#include <Generator_debug.h>
#include <Instrument.h>
#include <Voice.h>
#include <Voice_pool.h>
#include <Column.h>
#include <Channel.h>
#include <Pattern.h>
#include <Song.h>

#include <xmemory.h>


Suite* Song_suite(void);

Playdata* init_play(Song* song);


#if 0
Playdata* init_play(Song* song)
{
    if (song == NULL)
    {
        fprintf(stderr, "Test used init_play() incorrectly\n");
        return NULL;
    }
    Voice_pool* voice_pool = new_Voice_pool(16, 16);
    if (voice_pool == NULL)
    {
        fprintf(stderr, "new_Voice_pool() returned NULL -- out of memory?\n");
        return NULL;
    }
    static kqt_frame buf_l[] = { 0 };
    static kqt_frame* bufs[] = { buf_l, buf_l };
    Playdata* play = new_Playdata(1, voice_pool, Song_get_insts(song), 2, bufs);
    if (play == NULL)
    {
        fprintf(stderr, "xalloc() returned NULL -- out of memory?\n");
        return NULL;
    }
    play->mode = STOP;
    play->freq = 0;
    Reltime_init(&play->play_time);
    play->tempo = 0;
    Reltime_init(&play->pos);
    play->subsongs = song->subsongs;
    play->subsong = 0;
    play->section = 0;
    play->pattern = -1;
    return play;
}
#endif


START_TEST (new)
{
    Song* song = new_Song(2, 128, 64);
    if (song == NULL)
    {
        fprintf(stderr, "new_Song() returned NULL -- out of memory?\n");
        abort();
    }
    double mix_vol = Song_get_mix_vol(song);
    fail_unless(isfinite(mix_vol),
            "new_Song() created a Song without a sane initial mixing volume (%lf).", mix_vol);
    int buf_count = Song_get_buf_count(song);
    fail_unless(buf_count == 2,
            "new_Song() created a Song with a wrong amount of buffers (%d).", buf_count);
    kqt_frame** bufs = Song_get_bufs(song);
    fail_if(bufs == NULL,
            "new_Song() created a Song without buffers.");
    fail_if(bufs[0] == NULL,
            "new_Song() created a Song without buffers.");
    fail_if(bufs[1] == NULL,
            "new_Song() created a Song without a second buffer.");
    Subsong_table* subsongs = Song_get_subsongs(song);
    fail_if(subsongs == NULL,
            "new_Song() created a Song without a Subsong table.");
    Pat_table* pats = Song_get_pats(song);
    fail_if(pats == NULL,
            "new_Song() created a Song without a Pattern table.");
    Ins_table* insts = Song_get_insts(song);
    fail_if(insts == NULL,
            "new_Song() created a Song without a Instrument table.");
    Scale* notes = Song_get_scale(song, 0);
    fail_if(notes == NULL,
            "new_Song() created a Song without a Scale.");
    Event_queue* events = Song_get_events(song);
    fail_if(events == NULL,
            "new_Song() created a Song without Event queue.");
    del_Song(song);
}
END_TEST

#ifndef NDEBUG
START_TEST (new_break_buf_count_inv1)
{
    new_Song(0, 1, 1);
}
END_TEST

START_TEST (new_break_buf_count_inv2)
{
    new_Song(KQT_BUFFERS_MAX + 1, 1, 1);
}
END_TEST

START_TEST (new_break_buf_size_inv)
{
    new_Song(1, 0, 1);
}
END_TEST

START_TEST (new_break_events_inv)
{
    new_Song(1, 1, 0);
}
END_TEST
#endif


START_TEST (set_get_mix_vol)
{
    Song* song = new_Song(1, 1, 1);
    if (song == NULL)
    {
        fprintf(stderr, "new_Song() returned NULL -- out of memory?\n");
        abort();
    }
    Song_set_mix_vol(song, -INFINITY);
    double ret = Song_get_mix_vol(song);
    fail_unless(ret == -INFINITY,
            "Song_get_mix_vol() returned %lf instead of %lf.", ret, -INFINITY);
    Song_set_mix_vol(song, 0);
    ret = Song_get_mix_vol(song);
    fail_unless(ret == 0,
            "Song_get_mix_vol() returned %lf instead of %lf.", ret, 0);
    del_Song(song);
}
END_TEST

#ifndef NDEBUG
START_TEST (set_mix_vol_break_song_null)
{
    Song_set_mix_vol(NULL, 0);
}
END_TEST

START_TEST (set_mix_vol_break_mix_vol_inv1)
{
    Song* song = new_Song(1, 1, 1);
    if (song == NULL)
    {
        fprintf(stderr, "new_Song() returned NULL -- out of memory?\n");
        return;
    }
    Song_set_mix_vol(song, INFINITY);
    del_Song(song);
}
END_TEST

START_TEST (set_mix_vol_break_mix_vol_inv2)
{
    Song* song = new_Song(1, 1, 1);
    if (song == NULL)
    {
        fprintf(stderr, "new_Song() returned NULL -- out of memory?\n");
        return;
    }
    Song_set_mix_vol(song, NAN);
    del_Song(song);
}
END_TEST

START_TEST (get_mix_vol_break_song_null)
{
    Song_get_mix_vol(NULL);
}
END_TEST
#endif


START_TEST (mix)
{
    Song* song = new_Song(2, 256, 64);
    if (song == NULL)
    {
        fprintf(stderr, "new_Song() returned NULL -- out of memory?\n");
        abort();
    }
    Song_set_mix_vol(song, 0);
    Playdata* play = song->play_state;
    if (play == NULL) abort();
    Pattern* pat1 = new_Pattern();
    if (pat1 == NULL)
    {
        fprintf(stderr, "new_Pattern() returned NULL -- out of memory?\n");
        abort();
    }
    Pattern* pat2 = new_Pattern();
    if (pat2 == NULL)
    {
        fprintf(stderr, "new_Pattern() returned NULL -- out of memory?\n");
        abort();
    }
    Pat_table* pats = Song_get_pats(song);
    if (!Pat_table_set(pats, 0, pat1))
    {
        fprintf(stderr, "Pat_table_set() returned NULL -- out of memory?\n");
        abort();
    }
    if (!Pat_table_set(pats, 1, pat2))
    {
        fprintf(stderr, "Pat_table_set() returned NULL -- out of memory?\n");
        abort();
    }
    Subsong_table* subsongs = Song_get_subsongs(song);
    Subsong* ss = new_Subsong();
    if (ss == NULL)
    {
        fprintf(stderr, "new_Subsong() returned NULL -- out of memory?\n");
        abort();
    }
    if (Subsong_table_set(subsongs, 0, ss) < 0)
    {
        fprintf(stderr, "Subsong_table_set() returned negative -- out of memory?\n");
        abort();
    }
    if (!Subsong_set(ss, 0, 0))
    {
        fprintf(stderr, "Subsong_set() returned NULL -- out of memory?\n");
        abort();
    }
    if (!Subsong_set(ss, 1, 1))
    {
        fprintf(stderr, "Subsong_set() returned NULL -- out of memory?\n");
        abort();
    }
    Scale* notes = Song_get_scale(song, 0);
    Scale_set_ref_pitch(notes, 2);
    kqt_frame** bufs = Song_get_bufs(song);
    kqt_frame** vbufs = Song_get_voice_bufs(song);
    kqt_frame** vbufs2 = Song_get_voice_bufs2(song);
    Instrument* ins = new_Instrument(bufs, vbufs, vbufs2, 2, 256, Song_get_scales(song),
                                     Song_get_active_scale(song), 16);
    if (ins == NULL)
    {
        fprintf(stderr, "new_Instrument() returned NULL -- out of memory?\n");
        abort();
    }
    Generator_debug* gen_debug = new_Generator_debug(Instrument_get_params(ins));
    if (gen_debug == NULL)
    {
        fprintf(stderr, "new_Generator_debug() returned NULL -- out of memory?\n");
        abort();
    }
    Instrument_set_gen(ins, 0, (Generator*)gen_debug);
    Instrument_set_scale(ins, 0);
    Ins_table* insts = Song_get_insts(song);
    if (!Ins_table_set(insts, 1, ins))
    {
        fprintf(stderr, "Ins_table_set() returned false -- out of memory?\n");
        abort();
    }
    Event* ev1_on = new_Event_voice_note_on(Reltime_init(RELTIME_AUTO));
    if (ev1_on == NULL)
    {
        fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
        abort();
    }
    Event* ev1_off = new_Event_voice_note_off(Reltime_init(RELTIME_AUTO));
    if (ev1_off == NULL)
    {
        fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
        abort();
    }
    Event* ev2_on = new_Event_voice_note_on(Reltime_init(RELTIME_AUTO));
    if (ev2_on == NULL)
    {
        fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
        abort();
    }
    Event* ev2_off = new_Event_voice_note_off(Reltime_init(RELTIME_AUTO));
    if (ev2_off == NULL)
    {
        fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
        abort();
    }
    
    // Testing scenario 1:
    //
    // Mixing frequency is 8 Hz.
    // Tempo is 120 BPM.
    // Playing notes of a debug instrument.
    // Note #1 frequency is 1 Hz (0.5 cycles/beat).
    // Note #1 starts at pattern 0, position 0:0 and is released at pattern 1, position 0:0.
    // Note #2 frequency is 2 Hz (1 cycle/beat).
    // Note #2 starts at pattern 1, position 0:0 and plays until the end
    // Both notes are located at column 0.
    // Result should be as described in the code below.
    play->mode = PLAY_SONG;
    play->freq = 8;
    play->tempo = 120;
    Reltime_init(&play->pos);
    int64_t note = 0;
    int64_t mod = -1;
    int64_t octave = KQT_SCALE_MIDDLE_OCTAVE - 1;
    int64_t instrument = 1;
    Event_set_field(ev1_on, 0, &note);
    Event_set_field(ev1_on, 1, &mod);
    Event_set_field(ev1_on, 2, &octave);
    play->channels[0]->cur_inst = instrument;
    octave = KQT_SCALE_MIDDLE_OCTAVE;
    Event_set_field(ev2_on, 0, &note);
    Event_set_field(ev2_on, 1, &mod);
    Event_set_field(ev2_on, 2, &octave);
    Column* col = Pattern_get_col(pat1, 0);
    if (!Column_ins(col, ev1_on))
    {
        fprintf(stderr, "Column_ins() returned false -- out of memory?\n");
        abort();
    }
    col = Pattern_get_col(pat2, 0);
    if (!Column_ins(col, ev2_on))
    {
        fprintf(stderr, "Column_ins() returned false -- out of memory?\n");
        abort();
    }
    uint32_t ret = Song_mix(song, 256, play);
    fail_unless(ret == 128,
            "Song_mix() mixed %lu frames instead of 128.", (unsigned long)ret);
    for (int i = 0; i < 64; ++i)
    {
        if (i % 8 == 0)
        {
            fail_unless(bufs[0][i] > 0.99 && bufs[0][i] < 1.01,
                    "Buffer contains %f at index %d (expected 1).", bufs[0][i], i);
        }
        else
        {
            fail_unless(bufs[0][i] > 0.49 && bufs[0][i] < 0.51,
                    "Buffer contains %f at index %d (expected 0.5).", bufs[0][i], i);
        }
    }
    for (int i = 64; i < 80; ++i)
    {
        if (i % 8 == 4)
        {
            fail_unless(bufs[0][i] > 0.49 && bufs[0][i] < 0.51,
                    "Buffer contains %f at index %d (expected 0.5).", bufs[0][i], i);
        }
        else
        {
            fail_unless(fabs(bufs[0][i]) < 0.01,
                    "Buffer contains %f at index %d (expected 0).", bufs[0][i], i);
        }
    }
    for (int i = 80; i < 104; ++i)
    {
        if (i % 4 == 0)
        {
            fail_unless(bufs[0][i] > 0.99 && bufs[0][i] < 1.01,
                    "Buffer contains %f at index %d (expected 1).", bufs[0][i], i);
        }
        else
        {
            fail_unless(bufs[0][i] > 0.49 && bufs[0][i] < 0.51,
                    "Buffer contains %f at index %d (expected 0.5).", bufs[0][i], i);
        }
    }
    for (int i = 104; i < 256; ++i)
    {
        fail_unless(fabs(bufs[0][i]) < 0.01,
                "Buffer contains %f at index %d (expected 0).", bufs[0][i], i);
    }

    del_Song(song);
}
END_TEST

#ifndef NDEBUG
START_TEST (mix_break_song_null)
{
    Playdata* play = xalloc(Playdata);
    if (play == NULL)
    {
        fprintf(stderr, "xalloc() returned NULL -- out of memory?\n");
        return;
    }
    Song_mix(NULL, 1, play);
    xfree(play);
}
END_TEST

START_TEST (mix_break_play_null)
{
    Song* song = new_Song(1, 1, 1);
    if (song == NULL)
    {
        fprintf(stderr, "new_Song() returned NULL -- out of memory?\n");
        return;
    }
    Song_mix(song, 1, NULL);
    del_Song(song);
}
END_TEST
#endif


Suite* Song_suite(void)
{
    Suite* s = suite_create("Song");
    TCase* tc_new = tcase_create("new");
    TCase* tc_set_get_name = tcase_create("set_get_name");
    TCase* tc_set_get_tempo = tcase_create("set_get_tempo");
    TCase* tc_set_get_mix_vol = tcase_create("set_get_mix_vol");
    TCase* tc_set_get_global_vol = tcase_create("set_get_global_vol");
    TCase* tc_mix = tcase_create("mix");
    suite_add_tcase(s, tc_new);
    suite_add_tcase(s, tc_set_get_tempo);
    suite_add_tcase(s, tc_set_get_mix_vol);
    suite_add_tcase(s, tc_set_get_global_vol);
    suite_add_tcase(s, tc_mix);

    int timeout = 10;
    tcase_set_timeout(tc_new, timeout);
    tcase_set_timeout(tc_set_get_name, timeout);
    tcase_set_timeout(tc_set_get_tempo, timeout);
    tcase_set_timeout(tc_set_get_mix_vol, timeout);
    tcase_set_timeout(tc_set_get_global_vol, timeout);
    tcase_set_timeout(tc_mix, timeout);

    tcase_add_test(tc_new, new);
    tcase_add_test(tc_set_get_mix_vol, set_get_mix_vol);
    tcase_add_test(tc_mix, mix);

#ifndef NDEBUG
    tcase_add_test_raise_signal(tc_new, new_break_buf_count_inv1, SIGABRT);
    tcase_add_test_raise_signal(tc_new, new_break_buf_count_inv2, SIGABRT);
    tcase_add_test_raise_signal(tc_new, new_break_buf_size_inv, SIGABRT);
    tcase_add_test_raise_signal(tc_new, new_break_events_inv, SIGABRT);

    tcase_add_test_raise_signal(tc_set_get_mix_vol, set_mix_vol_break_song_null, SIGABRT);
    tcase_add_test_raise_signal(tc_set_get_mix_vol, set_mix_vol_break_mix_vol_inv1, SIGABRT);
    tcase_add_test_raise_signal(tc_set_get_mix_vol, set_mix_vol_break_mix_vol_inv2, SIGABRT);
    tcase_add_test_raise_signal(tc_set_get_mix_vol, get_mix_vol_break_song_null, SIGABRT);

    tcase_add_test_raise_signal(tc_mix, mix_break_song_null, SIGABRT);
    tcase_add_test_raise_signal(tc_mix, mix_break_play_null, SIGABRT);
#endif

    return s;
}


int main(void)
{
    int fail_count = 0;
    Suite* s = Song_suite();
    SRunner* sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    fail_count = srunner_ntests_failed(sr);
    srunner_free(sr);
    if (fail_count > 0)
    {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

