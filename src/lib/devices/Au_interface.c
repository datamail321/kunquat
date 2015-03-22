

/*
 * Author: Tomi Jylhä-Ollila, Finland 2011-2015
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <stdlib.h>

#include <debug/assert.h>
#include <devices/Au_interface.h>
#include <devices/Device.h>
#include <memory.h>


Au_interface* new_Au_interface(void)
{
    Au_interface* iface = memory_alloc_item(Au_interface);
    if (iface == NULL)
        return NULL;

    if (!Device_init(&iface->parent, false))
    {
        del_Au_interface(iface);
        return NULL;
    }

    Device_set_existent(&iface->parent, true);

    return iface;
}


void del_Au_interface(Au_interface* iface)
{
    if (iface == NULL)
        return;

    Device_deinit(&iface->parent);
    memory_free(iface);

    return;
}


