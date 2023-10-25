// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/assert.c"

int main(void)
{
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
            gpc_assert(arr1[i], ==, arr2[i])

        // Custom comparison and pretty printing
        // TODO write a struct example instead of array example
        #define ARR_MIN_SIZE(ARR1, ARR2) \
            ((sizeof(ARR1) < sizeof(ARR2) ? sizeof(ARR1) : sizeof(ARR2))/sizeof(*(ARR1)))
        bool array_comparator(int* array1, int* array2, size_t array_size);
        gpc_assert_custom(
            "Additional details go here. ", // NULL is also ok
            array_comparator,
            arr1, arr2, ARRAY_MIN_SIZE(arr1, arr2)); // array_comparator args
    }

    // Optional explicit end of all testing and report results. If this
    // function is not called explicitly, it will be called when main() returns.
    gpc_end_testing();
    // However,
    puts("now we can print custom reports here or do whatever we like.");
}

bool array_comparator(int* array1, int* array2, size_t array_size)
{
    for (size_t i = 0; i < array_size; i++)
        if (array1[i] != array2[i])
        {
            // TODO better example implementation preferably using gpc_String
            size_t bufsizes = 256;
            char** bufs = gpc_get_assert_fail_bufs(256);
            snprintf(bufs[1], bufsizes, "{ %i, %i, %i }", array1[0], array1[1], array1[2]);
            snprintf(bufs[2], bufsizes, "{ %i, %i, %i }", array2[0], array2[1], array2[2]);

            return false;
        }
    return true;
}
