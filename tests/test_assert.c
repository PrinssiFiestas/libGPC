// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/assert.c"

int main(void)
{
    gpc_assert(1, <, 2, "blah");
    gpc_assert(1, >=, 1 + 1, "blah");

#if 0
    gpc_suite("First suite");
    { // Scoping not required but adds readability and structure.
        gpc_test("First test");
        {
            // Single argument assertion
            gpc_assert(true);

            // Logical operator can be given as arg for better fail message.
            gpc_assert(1.1, <, 2.2);
            gpc_assert(1 + 1, ==, 2, "Here's optional failure details");
        }

        // Starting a new test ends the last one
        gpc_test("Second test");
        {
            char str[] = "some string";
            gpc_assert_str(str, ==, "some string");

            // Lexicographic comparison
            gpc_assert_str("alpha", <, "beta");
        }
    }

    // Assertions can be placed anywhere in code. This one is not part of any
    // test or suite.
    gpc_assert( ! 0);

    // Starting a new suite endst the last one
    gpc_suite("Second suite");
    {
        // Tests are optional.
        gpc_assert( ! false, "optional message for single arg assertion");
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
        gpc_assert(arr1, !=, arr2);

        // Array comparison
        for (size_t i = 0; i < ARR_MIN_SIZE(arr1, arr2); i++)
            gpc_assert(arr1[i], ==, arr2[i]);
    }

    // Optional explicit end of all testing and report results. If this
    // function is not called explicitly, it will be called when main() returns.
    gpc_end_testing();
    // However,
    puts("now we can print custom reports here or do whatever we like.");
#endif
}
