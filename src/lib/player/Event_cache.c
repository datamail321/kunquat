

/*
 * Author: Tomi Jylhä-Ollila, Finland 2012-2016
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <player/Event_cache.h>

#include <containers/AAtree.h>
#include <debug/assert.h>
#include <memory.h>
#include <player/Event_names.h>
#include <player/Event_type.h>
#include <Value.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


struct Event_cache
{
    AAtree* cache;
};


typedef struct Event_state
{
    char event_name[EVENT_NAME_MAX + 1];
    Value value;
} Event_state;


static Event_state* new_Event_state(const char* event_name);


static void Event_state_reset(Event_state* es);


static void del_Event_state(Event_state* es);


Event_cache* new_Event_cache(void)
{
    Event_cache* cache = memory_alloc_item(Event_cache);
    if (cache == NULL)
        return NULL;

    cache->cache = new_AAtree(
            (AAtree_item_cmp*)strcmp, (AAtree_item_destroy*)del_Event_state);
    if (cache->cache == NULL)
    {
        del_Event_cache(cache);
        return NULL;
    }

    return cache;
}


bool Event_cache_add_event(Event_cache* cache, char* event_name)
{
    rassert(cache != NULL);
    rassert(event_name != NULL);

    if (AAtree_get_exact(cache->cache, event_name) != NULL)
        return true;

    Event_state* es = new_Event_state(event_name);
    if (es == NULL || !AAtree_ins(cache->cache, es))
    {
        del_Event_state(es);
        return false;
    }

    return true;
}


void Event_cache_update(Event_cache* cache, const char* event_name, const Value* value)
{
    rassert(cache != NULL);
    rassert(event_name != NULL);
    rassert(value != NULL);

    Event_state* state = AAtree_get_exact(cache->cache, event_name);
    if (state == NULL)
        return;

    Value_copy(&state->value, value);
    return;
}


const Value* Event_cache_get_value(const Event_cache* cache, const char* event_name)
{
    rassert(cache != NULL);
    rassert(event_name != NULL);

    Event_state* state = AAtree_get_exact(cache->cache, event_name);
    rassert(state != NULL);

    return &state->value;
}


void Event_cache_reset(Event_cache* cache)
{
    rassert(cache != NULL);

    AAiter* iter = AAiter_init(AAITER_AUTO, cache->cache);
    Event_state* es = AAiter_get_at_least(iter, "");
    while (es != NULL)
    {
        Event_state_reset(es);
        es = AAiter_get_next(iter);
    }

    return;
}


void del_Event_cache(Event_cache* cache)
{
    if (cache == NULL)
        return;

    del_AAtree(cache->cache);
    memory_free(cache);
    return;
}


static Event_state* new_Event_state(const char* event_name)
{
    rassert(event_name != NULL);
    rassert(strlen(event_name) <= EVENT_NAME_MAX);

    Event_state* es = memory_alloc_item(Event_state);
    if (es == NULL)
        return NULL;

    strcpy(es->event_name, event_name);
    es->value.type = VALUE_TYPE_NONE;
    return es;
}


static void Event_state_reset(Event_state* es)
{
    rassert(es != NULL);
    es->value.type = VALUE_TYPE_NONE;
    return;
}


static void del_Event_state(Event_state* es)
{
    if (es == NULL)
        return;

    memory_free(es);
    return;
}


