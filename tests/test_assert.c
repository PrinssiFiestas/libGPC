// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/assert.c"

// Basic unit test demo can be found in main() after inline_test_demo().

// Blocks inside functions can be tested with GP_TEST_OUT() and GP_TEST_INS().
// Inline tests require GCC compatible compiler.
// GCC Extensions used: compound statements and typeof operator.
void inline_test_demo(void)
{
    #if defined(__GNUC__) && defined(GP_TESTS)

    int output = 0;
    const char* str = "this does not get tested";
    int n = 1;

    // GP_TEST_OUT() should ALWAYS come before GP_TEST_IN(). First argument is
    // the variable V to be tested. Second arg is any expression that tests the
    // variable against V_EXPECTED which has a value from rest of the args.
    GP_TEST_OUT(output, output == output_EXPECTED, 2, 6) // semicol is optional
    // First arg tells what's being tested so it must match the first arg of
    // GP_TEST_OUT(). All subsequent args are parentheses enclosed lists where
    // the first element is the input variable and the rest are testing values.
    // The number of test values must match the number of expected values in
    // GP_TEST_OUT().
    GP_TEST_INS(output, (str, "aa", "bbb"), (n, 1, 2)) // DON'T ADD SEMICOL HERE
    { // Code block to be tested. Could be a single line without curlies too.
      // After test, this block gets executed with original values of str and n.
      // Note that if GP_TESTS is defined, this block gets executed multiple
      // times: one time with the original values after testing and in this case
      // two times during testing so be mindful about side effects. If GP_TESTS
      // is not defined, the block gets executed once with the original values.
        size_t len = strlen(str);
        output = len * n;
    }

    // User defined struct test demo
    struct MyI { int i; } my_i = {0};
    int dummy = 0; // Inputs length have to match outputs length, thus dummy.
    // Expected outputs in brace enclosed initializer lists.
    GP_TEST_OUT(my_i, my_i.i == my_i_EXPECTED.i, {1});
    GP_TEST_INS(my_i, (dummy, 0))
        my_i.i = 1;

    #endif // __GNUC__
}

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
            inline_test_demo();
        }

        // Starting a new test ends the last one.
        gp_test("Second test");
        {
            int var = 0;
            // Pass format string and variable for better fail message.
            gp_assert(var == 0, ("%i Additional note", var));
        }
    }

    void* p;
    // Assertions can be placed anywhere in code. This one is not part of any
    // test or suite.
    gp_assert(p = malloc(1));
    free(p);

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
        gp_assert(l1 == l2 && f1 < f2, (l1), (l2), (f1), (f2), ("My note"));
        #else // C99
        // Literals do not require a format string.
        gp_assert(l1 == l2 && f1 < f2,
            ("%l", l1), ("%l", l2), ("%g", f1), ("%g", f2), ("My note"));
        #endif
    }
    // Tests and suites can be explicitly ended with NULL which also prints
    // result.
    gp_suite(NULL);

    // Suites are optional. This test is not part of any suite.
    gp_test("Array test without suite");
    {
        unsigned arr1[] = { 1, 2, 3, 4 };
        unsigned arr2[] = { 1, 2, 3, 4 };

        // Array assertion using the return value of gp_expect()
        for (size_t i = 0; i < sizeof(arr1)/sizeof(arr1[0]); i++)
            if ( ! gp_expect(arr1[i] == arr2[i], ("%u", arr1[i]), ("%u", arr2[i])))
                exit(EXIT_FAILURE);
    }

    // Optional explicit end of all testing and report results. If this
    // function is not called explicitly, it will be called when main() returns.
    gp_end_testing();

    // Define this to see failing messages
    #ifdef GPC_NON_PASSING_TESTS

    // char dest[100];
    // char* p = dest;
    // p = gp_str_push(p, "this", -1);
    // p = gp_str_push(p, "_is_a_sentence", -1);
    // printf("%s\n", dest);

    // gp_assert(0 <= 0);
    // gp_assert(0 < 1, (0 + 1));
    // gp_assert(1 < 2, ("%i", 0x1));
    // gp_assert(1 < 2 && 2 < 3, (0 + 1), ("%i", 0x2), (1 * 3));

    // gp_expect(0 != 0);
    // gp_expect(0 > 1, (1));
    // gp_expect(1 > 0x2, ("%i", 0x2));
    // gp_expect(1 > 1 + 1 && 0x2 > 1 * 3, (1 + 1), ("%i", 0x2), (1 * 3));
    // const char blah[] = "blah";
    // gp_expect(strcmp(blah, "blah"), (blah));
    // gp_expect(strcmp(blah, "blah"), ("b%%l%%a%%h %.99s IS \"blah\"", blah), ("My additional notes."));

    #endif // GPC_NON_PASSING_TESTS
}
