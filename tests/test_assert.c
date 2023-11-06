// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/assert.c"

int main(void)
{
    gpc_assert(0 == 0);
    gpc_assert(0 < 1, (0 + 1));
    gpc_assert(1 < 2, ("%i", 0x1));
    gpc_assert(1 < 2 && 2 < 3, (0 + 1), ("%i", 0x2), (1 * 3));

    gpc_expect(0 != 0);
    gpc_expect(0 > 1, (1));
    gpc_expect(1 > 0x2, ("%i", 0x2));
    gpc_expect(1 > 1 + 1 && 0x2 > 1 * 3, (1 + 1), ("%i", 0x2), (1 * 3));
    const char blah[] = "blah";
    gpc_assert(strcmp(blah, "blah"), (blah));

    printf("Done...\n");

#if 0
    gpc_suite("First suite");
    { // Scoping not required but adds readability and structure.
        gpc_test("First test");
        {
            // TODO example code
        }

        // Starting a new test ends the last one
        gpc_test("Second test");
        {
            // TODO example code
        }
    }

    // Assertions can be placed anywhere in code. This one is not part of any
    // test or suite.
    // TODO example code

    // Starting a new suite endst the last one
    gpc_suite("Second suite");
    {
        // Tests are optional.
        // TODO example code
    }
    // Tests and suites can be explicitly ended with NULL which also prints
    // result.
    gpc_suite(NULL);

    // Suites are optional.
    gpc_test("Array test without suite");
    {
        int arr1[] = { 1, 2, 3 };
        int arr2[] = { 1, 2, 3 };

        // Testing pointers. Use the method below for arrays.
        // TODO example code

        // Array comparison
        for (size_t i = 0; i < ARR_MIN_SIZE(arr1, arr2); i++)
            // TODO example code
    }

    // Optional explicit end of all testing and report results. If this
    // function is not called explicitly, it will be called when main() returns.
    gpc_end_testing();
    // However,
    puts("now we can print custom reports here or do whatever we like.");
#endif
}
