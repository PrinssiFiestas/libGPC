// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/assert.c"

int main(void)
{
    // Note the semicolon. These are not macro magic but just regular functions
    // executed instead.
    gp_suite("First suite");
    { // Scoping not required, but adds readability and structure. It also gives
      // familiarity to other frameworks, where tests are top level functions.
        gp_test("First test");
        {
            gp_expect(0 == 0);
        }

        // Starting a new test ends the last one.
        gp_test("Second test");
        {
            int var = 0;
            // Pass format string and variable for better fail message.
            gp_assert(var == 0, "%i Additional note", var);
        }
    }

    // Starting a new suite ends the last one.
    gp_suite("Second suite");
    {
        // Tests are optional. This suite has only assertions without dedicated
        // tests.

        long l1 = 0;
        long l2 = 0;
        double f1 = 0.707;
        double f2 = 3.141;

        #if __STDC_VERSION__ >= 201112L // C11
        // Format string is optional.
        gp_assert(l1 == l2 && f1 < f2, l1, l2, f1, f2, "My note");
        #else // C99
        // Literals do not require a format string.
        gp_assert(l1 == l2 && f1 < f2,
            "%l", l1, "%l", l2, "%g", f1, "%g", f2, "My note");
        #endif
    }
    // Tests and suites can be explicitly ended with NULL which also prints
    // result.
    gp_suite(NULL);

    void* p;
    // Assertions can be placed anywhere in code. This one is not part of any
    // test or suite.
    gp_assert(p = malloc(1));
    free(p);

    // Suites are optional. This test would be part of the last suite, but
    // there's none running, so this one is not part of any suite.
    gp_test("Array test without suite");
    {
        unsigned arr1[] = { 1, 2, 3, 4 };
        unsigned arr2[] = { 1, 2, 3, 4 };

        // Array assertion using the return value of gp_expect()
        for (size_t i = 0; i < sizeof(arr1)/sizeof(arr1[0]); i++)
            if ( ! gp_expect(arr1[i] == arr2[i], arr1[i], arr2[i]))
                exit(EXIT_FAILURE);
    }

    // Optional explicit end of all testing and report results. If this
    // function is not called explicitly, it will be called when main() returns.
    gp_end_testing();

    // Define this to see failing messages
    #ifdef GPC_NON_PASSING_TESTS
    gp_suite("Non passing suite");
    {
        gp_test("Non passing test");
        {
            int my_int = 3;
            gp_expect(1 + 1 == my_int,
                "Non-format literals will be printed without formatting.\n"
                "They can be used as additional comments.\n"
                "Here we demo automatic formatting for variables.",// no \n here
                1 + 1, my_int);

            char my_c = 'X';
            gp_expect(0,
                "Format string can be passed too.",
                "\'%c\' my note for my_c", my_c);

            struct {
                short i;
                char* s;
                float f;
            } my_s = { -1, "blah", 3.f };
            gp_expect(false,
                "Printing collection of data",
                "%i, \"%s\", %g", my_s.i, my_s.s, my_s.f);

            gp_expect(NULL,
                "Surround with curlies",
                "{ %i, \"%s\", %g }", my_s.i + 1, "bloink", my_s.f/0.,
                "{%i, \"%s\", %g} without spaces", my_s.i + 1, "bloink", -my_s.f/0.,
                "<%i, \"%s\", %g> these are fine too", my_s.i + 1, NULL, -my_s.f/0.);

            const char* my_string = NULL;
            gp_expect(my_string, my_string);
        }
    }
    #endif // GPC_NON_PASSING_TESTS
}
