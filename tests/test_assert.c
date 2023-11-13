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
            gpc_expect(0 == 0);
        }

        // Starting a new test ends the last one.
        gpc_test("Second test");
        {
            int var = 0;
            // Parenthesized format string and variable for better fail message.
            gpc_assert(var == 0, ("%i Additional note", var));
        }
    }

    void* p;
    // Assertions can be placed anywhere in code. This one is not part of any
    // test or suite.
    gpc_assert(p = malloc(1));
    free(p);

    // Starting a new suite endst the last one.
    gpc_suite("Second suite");
    {
        // Tests are optional.

        long l1 = 0;
        long l2 = 0;
        double f1 = 0.707;
        double f2 = 3.141;

        #if __STDC_VERSION__ >= 201112L
        // Format string is optional.
        gpc_assert(l1 == l2 && f1 < f2, (l1), (l2), (f1), (f2), ("My note"));
        #else
        // Literals do not require a format string.
        gpc_assert(l1 == l2 && f1 < f2,
            ("%l", l1), ("%l", l2), ("%g", f1), ("%g", f2), ("My note"));
        #endif
    }
    // Tests and suites can be explicitly ended with NULL which also prints
    // result.
    gpc_suite(NULL);

    // Suites are optional.
    gpc_test("Array test without suite");
    {
        unsigned arr1[] = { 1, 2, 3, 4 };
        unsigned arr2[] = { 1, 2, 3, 4 };

        // Array assertion using the return value of gpc_expect()
        for (size_t i = 0; i < sizeof(arr1)/sizeof(arr1[0]); i++)
            if ( ! gpc_expect(arr1[i] == arr2[i], ("%u", arr1[i]), ("%u", arr2[i])))
                exit(EXIT_FAILURE);
    }

    // Optional explicit end of all testing and report results. If this
    // function is not called explicitly, it will be called when main() returns.
    gpc_end_testing();

    // Define this to see failing messages
    //#define GPC_NON_PASSING_TESTS
    #ifdef GPC_NON_PASSING_TESTS

    // char dest[100];
    // char* p = dest;
    // p = gpc_str_push(p, "this", -1);
    // p = gpc_str_push(p, "_is_a_sentence", -1);
    // printf("%s\n", dest);

    // gpc_assert(0 <= 0);
    // gpc_assert(0 < 1, (0 + 1));
    // gpc_assert(1 < 2, ("%i", 0x1));
    // gpc_assert(1 < 2 && 2 < 3, (0 + 1), ("%i", 0x2), (1 * 3));

    // gpc_expect(0 != 0);
    // gpc_expect(0 > 1, (1));
    // gpc_expect(1 > 0x2, ("%i", 0x2));
    // gpc_expect(1 > 1 + 1 && 0x2 > 1 * 3, (1 + 1), ("%i", 0x2), (1 * 3));
    // const char blah[] = "blah";
    // gpc_expect(strcmp(blah, "blah"), (blah));
    // gpc_expect(strcmp(blah, "blah"), ("b%%l%%a%%h %.99s IS \"blah\"", blah), ("My additional notes."));

    #endif // GPC_NON_PASSING_TESTS
}
