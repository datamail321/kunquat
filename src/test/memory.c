

/*
 * Author: Tomi Jylhä-Ollila, Finland 2013-2016
 *
 * This file is part of Kunquat.
 *
 * CC0 1.0 Universal, http://creativecommons.org/publicdomain/zero/1.0/
 *
 * To the extent possible under law, Kunquat Affirmers have waived all
 * copyright and related or neighboring rights to Kunquat.
 */


#include <handle_utils.h>
#include <test_common.h>

#include <kunquat/Handle.h>
#include <kunquat/testing.h>

#include <string.h>


#ifdef KQT_LONG_TESTS
START_TEST(Out_of_memory_at_handle_creation_fails_cleanly)
{
    assert(handle == 0);

    // Get the number of memory allocations required by a successfull call
    handle = kqt_new_Handle();
    if (handle == 0)
    {
        fail("Normal handle creation failed");
        abort();
    }

    const long alloc_count = kqt_get_memory_alloc_count();
    fail_if(alloc_count <= 0,
            "kqt_new_Handle did not allocate memory");
    fail_if(alloc_count >= 65536,
            "kqt_new_Handle made too many memory allocations (%ld)",
            alloc_count);

    kqt_del_Handle(handle);
    handle = 0;

    // Test errors at every memory allocation point
    for (long i = 0; i < alloc_count; ++i)
    {
        //fprintf(stderr, "%ld\n", i);

        kqt_fake_out_of_memory(i);

        handle = kqt_new_Handle();
        fail_if(handle != 0,
                "kqt_new_Handle returned a handle with fake out of memory");

        const char* error_msg = kqt_Handle_get_error(0);
        fail_if(strlen(error_msg) == 0,
                "Memory allocation failure did not give an error message");
        fail_if(strstr(error_msg, "\"MemoryError\"") == NULL,
                "Error message on memory allocation failure was not a MemoryError");
    }

    // Make sure that we succeed with the error step far enough
    kqt_fake_out_of_memory(alloc_count);

    handle = kqt_new_Handle();
    fail_if(handle == 0,
            "kqt_new_Handle did not succeed with %ld allocations",
            alloc_count);
    kqt_del_Handle(handle);
    handle = 0;
}
END_TEST
#endif // KQT_LONG_TESTS


Suite* Memory_suite(void)
{
    Suite* s = suite_create("Memory");

    const int timeout = DEFAULT_TIMEOUT;

    TCase* tc_create = tcase_create("create");
    suite_add_tcase(s, tc_create);
    tcase_set_timeout(tc_create, timeout);
    //tcase_add_checked_fixture(tc_create, setup_empty, handle_teardown);

#ifdef KQT_LONG_TESTS
    tcase_set_timeout(tc_create, LONG_TIMEOUT);
    tcase_add_test(tc_create, Out_of_memory_at_handle_creation_fails_cleanly);
#endif

    return s;
}


int main(void)
{
    Suite* suite = Memory_suite();
    SRunner* sr = srunner_create(suite);
#ifdef K_MEM_DEBUG
    srunner_set_fork_status(sr, CK_NOFORK);
#endif
    srunner_run_all(sr, CK_NORMAL);
    int fail_count = srunner_ntests_failed(sr);
    srunner_free(sr);
    exit(fail_count > 0);
}


