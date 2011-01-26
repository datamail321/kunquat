

/*
 * Author: Tomi Jylhä-Ollila, Finland 2011
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <math_common.h>
#include <xassert.h>


bool is_p2(int64_t x)
{
    assert(x > 0);
    return (x & (x - 1)) == 0;
}


int64_t next_p2(int64_t x)
{
    assert(x > 0);
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}


int64_t ceil_p2(int64_t x)
{
    assert(x > 0);
    if (is_p2(x))
    {
        return x;
    }
    return next_p2(x);
}


