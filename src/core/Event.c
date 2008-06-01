

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

#include "Event.h"

#include <Note_table.h>

#include <xmemory.h>


char* Event_type_get_field_types(Event_type type)
{
	assert(EVENT_TYPE_IS_VALID(type));
	switch (type)
	{
		case EVENT_TYPE_NOTE_ON:
			return "iiii";
		case EVENT_TYPE_NOTE_OFF:
			return "";
		default:
			return NULL;
	}
	return NULL;
}


Event* new_Event(Reltime* pos, Event_type type)
{
	assert(pos != NULL);
	assert(EVENT_TYPE_IS_VALID(type));
	Event* event = xalloc(Event);
	if (event == NULL)
	{
		return NULL;
	}
	Reltime_copy(&event->pos, pos);
	Event_reset(event, type);
	return event;
}


void Event_reset(Event* event, Event_type type)
{
	assert(event != NULL);
	assert(EVENT_TYPE_IS_VALID(type));
	event->type = type;
	switch (event->type)
	{
		case EVENT_TYPE_NOTE_ON:
			event->fields[0].i = -1; // note
			event->fields[1].i = -1; // modifier
			event->fields[2].i = INT64_MIN; // octave
			event->fields[3].i = 0; // instrument
			break;
		case EVENT_TYPE_NOTE_OFF:
			break;
		default:
			break;
	}
	return;
}


Reltime* Event_pos(Event* event)
{
	assert(event != NULL);
	return &event->pos;
}


void Event_set_pos(Event* event, Reltime* pos)
{
	assert(event != NULL);
	assert(pos != NULL);
	Reltime_copy(&event->pos, pos);
	return;
}


Event_type Event_get_type(Event* event)
{
	assert(event != NULL);
	return event->type;
}


bool Event_int(Event* event, uint8_t index, int64_t* value)
{
	assert(event != NULL);
	assert(index < EVENT_FIELDS);
	assert(value != NULL);
	switch (event->type)
	{
		case EVENT_TYPE_NOTE_ON:
			*value = event->fields[index].i;
			return true;
		case EVENT_TYPE_NOTE_OFF:
		case EVENT_TYPE_GLOBAL_TEMPO:
			break;
		default:
			break; // FIXME: replace with assert(0) after supporting all types
	}
	return false;
}


bool Event_float(Event* event, uint8_t index, double* value)
{
	assert(event != NULL);
	assert(index < EVENT_FIELDS);
	assert(value != NULL);
	switch (event->type)
	{
		case EVENT_TYPE_GLOBAL_VOLUME:
			if (index == 0)
			{
				*value = event->fields[index].d;
				return true;
			}
			break;
		case EVENT_TYPE_GLOBAL_TEMPO:
			if (index == 0)
			{
				*value = event->fields[index].d;
				return true;
			}
			break;
		case EVENT_TYPE_NOTE_ON:
		case EVENT_TYPE_NOTE_OFF:
			break;
		default:
			break; // FIXME: replace with assert(0) after supporting all types
	}
	return false;
}


bool Event_set_int(Event* event, uint8_t index, int64_t value)
{
	assert(event != NULL);
	assert(index < EVENT_FIELDS);
	switch (event->type)
	{
		case EVENT_TYPE_NOTE_ON:
			if (index == 0 && value >= 0 && value < NOTE_TABLE_NOTES)
			{
				event->fields[index].i = value;
				return true;
			}
			else if (index == 1 && value >= -1 && value < NOTE_TABLE_NOTE_MODS)
			{
				event->fields[index].i = value;
				return true;
			}
			else if (index == 2 && value >= NOTE_TABLE_OCTAVE_FIRST
					&& value <= NOTE_TABLE_OCTAVE_LAST)
			{
				event->fields[index].i = value;
				return true;
			}
			else if (index == 3 && value > 0 && value < 256) // FIXME: max num of inst
			{
				event->fields[index].i = value;
				return true;
			}
			break;
		case EVENT_TYPE_NOTE_OFF:
			break;
		default:
			break;
	}
	return false;
}


bool Event_set_float(Event* event, uint8_t index, double value)
{
	assert(event != NULL);
	assert(index < EVENT_FIELDS);
	(void)value;
	switch (event->type)
	{
		case EVENT_TYPE_GLOBAL_VOLUME:
			if (index == 0)
			{
				event->fields[index].d = value;
				return true;
			}
			break;
		case EVENT_TYPE_GLOBAL_TEMPO:
			if (index == 0 && value > 0)
			{
				event->fields[index].d = value;
				return true;
			}
			break;
		case EVENT_TYPE_NOTE_ON:
		case EVENT_TYPE_NOTE_OFF:
			break;
		default:
			break;
	}
	return false;
}


void del_Event(Event* event)
{
	assert(event != NULL);
	xfree(event);
	return;
}


