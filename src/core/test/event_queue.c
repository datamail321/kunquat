

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
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <limits.h>

#include <check.h>

#include <Reltime.h>
#include <Event.h>
#include <Event_queue.h>


Suite* Event_queue_suite(void);


START_TEST (new)
{
	Event* event = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (event == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		abort();
	}
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		abort();
	}
	Event* ev2 = NULL;
	uint32_t pos = 0;
	fail_if(Event_queue_get(q, &ev2, &pos),
			"new_Event_queue() created a non-empty queue.");
	fail_unless(Event_queue_ins(q, event, 0),
			"new_Event_queue() created a queue without space for events.");
	del_Event_queue(q);
	q = new_Event_queue(32);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		abort();
	}
	for (int i = 0; i < 32; ++i)
	{
		fail_unless(Event_queue_ins(q, event, 0),
				"new_Event_queue() created a queue with space for %d events"
				" instead of 32.", i);
	}
	fail_if(Event_queue_ins(q, event, 0),
			"new_Event_queue() created a queue with space for more than 32 events.");
	del_Event_queue(q);
	del_Event(event);
}
END_TEST

#ifndef NDEBUG
START_TEST (new_break_size)
{
	new_Event_queue(0);
}
END_TEST
#endif


START_TEST (ins_get)
{
	Event* ev1 = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (ev1 == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		abort();
	}
	Event* ev2 = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (ev2 == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		abort();
	}
	Event* ev3 = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (ev3 == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		abort();
	}
	Event_queue* q = new_Event_queue(3);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		abort();
	}
	fail_unless(Event_queue_ins(q, ev1, 1),
			"Event_queue_ins() failed prematurely.");
	fail_unless(Event_queue_ins(q, ev2, 1),
			"Event_queue_ins() failed prematurely.");
	fail_unless(Event_queue_ins(q, ev3, 0),
			"Event_queue_ins() failed prematurely.");
	fail_if(Event_queue_ins(q, ev2, 4),
			"Event_queue_ins() inserted an Event into a full Event queue.");
	Event* ret = NULL;
	uint32_t pos = UINT32_MAX;
	
	fail_unless(Event_queue_get(q, &ret, &pos),
			"Event_queue_get() failed at getting an Event.");
	fail_unless(ret == ev3,
			"Event_queue_get() returned wrong Event (%p instead of %p).", ret, ev3);
	fail_unless(pos == 0,
			"Event_queue_get() returned wrong Event position (%lu instead of 0).", (unsigned long)pos);
	
	fail_unless(Event_queue_get(q, &ret, &pos),
			"Event_queue_get() failed at getting an Event.");
	fail_unless(ret == ev1,
			"Event_queue_get() returned wrong Event (%p instead of %p).", ret, ev1);
	fail_unless(pos == 1,
			"Event_queue_get() returned wrong Event position (%lu instead of 1).", (unsigned long)pos);
	
	fail_unless(Event_queue_get(q, &ret, &pos),
			"Event_queue_get() failed at getting an Event.");
	fail_unless(ret == ev2,
			"Event_queue_get() returned wrong Event (%p instead of %p).", ret, ev2);
	fail_unless(pos == 1,
			"Event_queue_get() returned wrong Event position (%lu instead of 1).", (unsigned long)pos);

	fail_if(Event_queue_get(q, &ret, &pos),
			"Event_queue_get() returned an Event from an empty Event queue.");

	del_Event_queue(q);
	del_Event(ev1);
	del_Event(ev2);
	del_Event(ev3);
}
END_TEST

#ifndef NDEBUG
START_TEST (ins_break_null_q)
{
	Event* event = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (event == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		return;
	}
	Event_queue_ins(NULL, event, 0);
	del_Event(event);
}
END_TEST

START_TEST (ins_break_null_event)
{
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		return;
	}
	Event_queue_ins(q, NULL, 0);
	del_Event_queue(q);
}
END_TEST

START_TEST (get_break_null_q)
{
	Event* event = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (event == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		return;
	}
	uint32_t pos = 0;
	Event_queue_get(NULL, &event, &pos);
	del_Event(event);
}
END_TEST

START_TEST (get_break_null_event)
{
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		return;
	}
	uint32_t pos = 0;
	Event_queue_get(q, NULL, &pos);
	del_Event_queue(q);
}
END_TEST

START_TEST (get_break_null_pos)
{
	Event* event = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (event == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		return;
	}
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		return;
	}
	Event_queue_get(q, &event, NULL);
	del_Event_queue(q);
	del_Event(event);
}
END_TEST
#endif


START_TEST (clear)
{
	Event* event = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (event == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		abort();
	}
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		abort();
	}
	uint32_t pos = 0;
	Event_queue_clear(q);
	fail_if(Event_queue_get(q, &event, &pos),
			"Event_queue_clear() created a non-empty queue.");
	fail_unless(Event_queue_ins(q, event, 1),
			"Event_queue_ins() failed.");
	Event_queue_clear(q);
	fail_if(Event_queue_get(q, &event, &pos),
			"Event_queue_clear() didn't clear the Event queue.");
	del_Event_queue(q);
	del_Event(event);
}
END_TEST

#ifndef NDEBUG
START_TEST (clear_break_null)
{
	Event_queue_clear(NULL);
}
END_TEST
#endif


START_TEST (resize)
{
	Event* event = new_Event(Reltime_init(RELTIME_AUTO), EVENT_TYPE_NOTE_ON);
	if (event == NULL)
	{
		fprintf(stderr, "new_Event() returned NULL -- out of memory?\n");
		abort();
	}
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		abort();
	}
	fail_unless(Event_queue_ins(q, event, 1),
			"Event_queue_ins() failed.");
	if (!Event_queue_resize(q, 16))
	{
		fprintf(stderr, "Event_queue_resize() returned false -- out of memory?\n");
		abort();
	}
	uint32_t posit = 0;
	fail_if(Event_queue_get(q, &event, &posit),
			"Event_queue_resize() didn't clear the Event queue.");
	for (int i = 0; i < 16; ++i)
	{
		fail_unless(Event_queue_ins(q, event, i),
				"Event_queue_ins() couldn't insert Event # %d.", i);
	}
	for (int i = 0; i < 16; ++i)
	{
		uint32_t pos = UINT32_MAX;
		Event* ret = NULL;
		fail_unless(Event_queue_get(q, &ret, &pos),
				"Event_queue_get() couldn't get Event # %d.", i);
		fail_unless(ret == event,
				"Event_queue_get() couldn't get the correct Event # %d.", i);
		fail_unless(pos == (uint32_t)i,
				"Event_queue_get() couldn't get the correct position # %d"
				" (got %lu instead of %d).", i, (unsigned long)pos, i);
	}
	del_Event_queue(q);
	del_Event(event);
}
END_TEST

#ifndef NDEBUG
START_TEST (resize_break_null)
{
	Event_queue_resize(NULL, 1);
}
END_TEST

START_TEST (resize_break_inv_size1)
{
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		return;
	}
	Event_queue_resize(q, 0);
}
END_TEST

START_TEST (resize_break_inv_size2)
{
	Event_queue* q = new_Event_queue(1);
	if (q == NULL)
	{
		fprintf(stderr, "new_Event_queue() returned NULL -- out of memory?\n");
		return;
	}
	Event_queue_resize(q, INT_MIN);
}
END_TEST
#endif


Suite* Event_queue_suite(void)
{
	Suite* s = suite_create("Event_queue");
	TCase* tc_new = tcase_create("new");
	TCase* tc_ins_get = tcase_create("ins_get");
	TCase* tc_clear = tcase_create("clear");
	TCase* tc_resize = tcase_create("resize");
	suite_add_tcase(s, tc_new);
	suite_add_tcase(s, tc_ins_get);
	suite_add_tcase(s, tc_clear);
	suite_add_tcase(s, tc_resize);

	tcase_add_test(tc_new, new);
	tcase_add_test(tc_ins_get, ins_get);
	tcase_add_test(tc_clear, clear);
	tcase_add_test(tc_resize, resize);

#ifndef NDEBUG
	tcase_add_test_raise_signal(tc_new, new_break_size, SIGABRT);

	tcase_add_test_raise_signal(tc_ins_get, ins_break_null_q, SIGABRT);
	tcase_add_test_raise_signal(tc_ins_get, ins_break_null_event, SIGABRT);
	tcase_add_test_raise_signal(tc_ins_get, get_break_null_q, SIGABRT);
	tcase_add_test_raise_signal(tc_ins_get, get_break_null_event, SIGABRT);
	tcase_add_test_raise_signal(tc_ins_get, get_break_null_pos, SIGABRT);

	tcase_add_test_raise_signal(tc_clear, clear_break_null, SIGABRT);

	tcase_add_test_raise_signal(tc_resize, resize_break_null, SIGABRT);
	tcase_add_test_raise_signal(tc_resize, resize_break_inv_size1, SIGABRT);
	tcase_add_test_raise_signal(tc_resize, resize_break_inv_size2, SIGABRT);
#endif

	return s;
}


int main(void)
{
	int fail_count = 0;
	Suite* s = Event_queue_suite();
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


