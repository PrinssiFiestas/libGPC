// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    gp_suite("Substrings");
    {
        gp_test("slice");
        {
            struct GPString str = gpstr_on_stack([], "Some_string_to slice");
            gpstr_slice(&str, 5, 11); // not including 11!
            gp_expect(gpstr_eq(str, gpstr("string")));
        }

        gp_test("substr");
        {
            struct GPString src = gpstr("Some_string_to slice");
            struct GPString dest = gpstr_on_stack([128], "");
            gpstr_substr(&dest, src, 5, 11); // not including 11!
            gp_expect(gpstr_eq(dest, gpstr("string")),
            ("%s", gpcstr(dest)));
        }
    }

    gp_suite("insert");
    {
        struct GPString str = gpstr_on_stack([128], "test");
        gp_test("Appending");
        {
            gpstr_insert(&str, str.length, gpstr(" tail"));
            gp_expect(gpstr_eq(str, gpstr("test tail")),
            (gpcstr(str)));
        }
        gp_test("Prepending");
        {
            gpstr_insert(&str, 0, gpstr("head "));
            gp_expect(gpstr_eq(str, gpstr("head test tail")),
            (gpcstr(str)));
        }
        gp_test("Insertion");
        {
            gpstr_insert(&str, 5, gpstr("insertion "));
            gp_expect(gpstr_eq(str, gpstr("head insertion test tail")),
            (gpcstr(str)));
        }
    }

    gp_suite("finding");
    {
        struct GPString haystack = gpstr("bbbaabaaabaa");
        struct GPString needle = gpstr("aa");
        size_t pos = 0;

        gp_test("find");
        {
            pos = gpstr_find(haystack, needle, 0);
            gp_expect(pos == 3);
            pos = gpstr_find(haystack, needle, 4);
            gp_expect(pos == 6);
            pos = gpstr_find(haystack, gpstr("not in haystack string"), 0);
            gp_expect(pos == GP_NOT_FOUND);
        }
        gp_test("find_last");
        {
            pos = gpstr_find_last(haystack, needle);
            gp_expect(pos == 10, (pos));
            pos = gpstr_find_last(haystack, gpstr("not in haystack string"));
            gp_expect(pos == GP_NOT_FOUND);
        }
        gp_test("count");
        {
            size_t count = gpstr_count(haystack, needle);
            gp_expect(count == 4);
        }
    }

    gp_suite("replacing substrings");
    {
        gp_test("replace");
        {
            struct GPString str = gpstr_on_stack([128], "aaabbbcccaaa");
            size_t needlepos = gpstr_replace(&str, gpstr("bbb"), gpstr("X"), 0);
            gp_expect(gpstr_eq(str, gpstr("aaaXcccaaa")));
            gp_expect(needlepos == 3);

            gpstr_replace(&str, gpstr("aaa"), gpstr("XXXXX"), 3);
            gp_expect(gpstr_eq(str, gpstr("aaaXcccXXXXX")));

        }

        gp_test("replace_all");
        {
            struct GPString str = gpstr_on_stack([128], "aaxxbbxxxccxx");
            unsigned replacement_count =
                gpstr_replace_all(&str, gpstr("xx"), gpstr("XXX"));
            gp_expect(gpstr_eq(str, gpstr("aaXXXbbXXXxccXXX")));
            gp_expect(replacement_count == 3);
        }
    }

    gp_suite("trim");
    {
        gp_test("left");
        {
            struct GPString str = gpstr_on_stack([128], "  \t\f \nLeft");
            gpstr_trim(&str, GPSTR_WHITESPACE, 'l');
            gp_expect(gpstr_eq(str, gpstr("Left")), (gpcstr(str)));
        }

        gp_test("right");
        {
            struct GPString str = gpstr_on_stack([128], "Right   \t\v\n\r");
            gpstr_trim(&str, GPSTR_WHITESPACE, 'r');
            gp_expect(gpstr_eq(str, gpstr("Right")), (gpcstr(str)));
        }

        gp_test("left and right");
        {
            struct GPString str = gpstr_on_stack([128], "   __Left and Right__   ");
            gpstr_trim(&str, GPSTR_WHITESPACE "_", 'l' + 'r');
            gp_expect(gpstr_eq(str, gpstr("Left and Right")), (gpcstr(str)));
        }
    }

    gp_suite("print");
    {
        #if __STDC_VERSION__ >= 201112L
        gp_test("Numbers");
        {
            struct GPString str = gpstr_on_stack([128], "");
            gpstr_print(&str, 1, " divided by ", 3, " is ", 1./3.);
            char buf[128];
            sprintf(buf, "%i divided by %i is %g", 1, 3, 1./3.);
            gp_expect(gpstr_eq(str, gpstr(buf)), (gpcstr(str)));
        }

        gp_test("Strings");
        {
            struct GPString str  = gpstr_on_stack([128], "");
            struct GPString str1 = gpstr_on_stack([128], "strings");
            gpstr_print(&str, "Copying ", str1, (char)'.');
            gp_expect(gpstr_eq(str, gpstr("Copying strings.")), (gpcstr(str)));
        }
        #endif
    }

    // ------------------------------------------------------------------------
    // C string tests

    gp_suite("C equal");
    {
        gp_expect(gp_cstr_equal("blah", "blah"));
        gp_expect( ! gp_cstr_equal("blah", "BLOH"));
        gp_expect( ! gp_cstr_equal("blah", "blahhhhhhhh"));
    }

    gp_suite("C Substrings");
    {
        gp_test("C slice");
        {
            char str[64]; // non-initialized buffers test null termination
            strcpy(str, "Some_string_to slice");
            gp_cstr_slice(str, 5, 11); // not including 11!
            gp_expect(gp_cstr_equal(str, "string"));
        }
        gp_test("C slice big string");
        {
            // Not really big here, but doesn't matter for testing.
            char  str_buf[64];
            strcpy(str_buf, "Some_string_to slice");
            char* str = str_buf;
            gp_big_cstr_slice(&str, 5, 11);
            gp_expect(gp_cstr_equal(str, "string"));
            gp_expect(str != str_buf, ("Pointer should've gotten mutated."));
        }

        gp_test("C substr");
        {
            const char* src = "Some_string_to slice";
            char dest[128];
            strcpy(dest, "");
            gp_cstr_substr(dest, src, 5, 11); // not including 11!
            gp_expect(gp_cstr_equal(dest, "string"),
                ("%s", dest));
        }
    }

    gp_suite("C insert and append");
    {
        gp_test("C Appending");
        {
            char str[128];
            strcpy(str, "test");
            gp_cstr_append(str, " tail");
            gp_expect(gp_cstr_equal(str, "test tail"));
            gp_cstr_append_n(str, ".BLAHBLAHBLAH", 1);
            gp_expect(gp_cstr_equal(str, "test tail."));
        }
        char str[128];
        strcpy(str, "test");
        gp_test("C Appending with insert");
        {
            gp_cstr_insert(str, strlen(str), " tail");
            gp_expect(gp_cstr_equal(str, "test tail"));
        }
        gp_test("C Prepending");
        {
            gp_cstr_insert_n(str, 0, "head XXXXXX", 5);
            gp_expect(gp_cstr_equal(str, "head test tail"));
        }
        gp_test("C Insertion");
        {
            gp_cstr_insert(str, 5, "insertion ");
            gp_expect(gp_cstr_equal(str, "head insertion test tail"));
        }
    }

    gp_suite("C finding");
    {
        const char* haystack = "bbbaabaaabaa";
        const char* needle = "aa";
        size_t pos = 0;

        gp_test("C find");
        {
            pos = gp_cstr_find(haystack, needle, 0);
            gp_expect(pos == 3);
            pos = gp_cstr_find(haystack, needle, 4);
            gp_expect(pos == 6);
            pos = gp_cstr_find(haystack, "not in haystack string", 0);
            gp_expect(pos == GP_NOT_FOUND);
        }
        gp_test("C find_last");
        {
            pos = gp_cstr_find_last(haystack, needle);
            gp_expect(pos == 10, (pos));
            pos = gp_cstr_find_last(haystack, "not in haystack string");
            gp_expect(pos == GP_NOT_FOUND);
        }
        gp_test("C count");
        {
            size_t count = gp_cstr_count(haystack, needle);
            gp_expect(count == 4);
        }
    }

    gp_suite("C replacing substrings");
    {
        gp_test("C replace");
        {
            char str[128];
            strcpy(str, "aaabbbcccaaa");
            size_t needlepos = 0;
            gp_cstr_replace(str, "bbb", "X", &needlepos);
            gp_expect(gp_cstr_equal(str,"aaaXcccaaa"), (str));
            gp_expect(needlepos == 3, (needlepos));

            size_t start = 3;
            gp_cstr_replace(str, "aaa", "XXXXX", &start);
            gp_expect(gp_cstr_equal(str, "aaaXcccXXXXX"), (str));
        }

        gp_test("C replace_all");
        {
            char str[128];
            strcpy(str, "aaxxbbxxxccxx");
            size_t replacement_count;
            gp_cstr_replace_all(str, "xx", "XXX", &replacement_count);

            gp_expect(gp_cstr_equal(str, "aaXXXbbXXXxccXXX"));
            gp_expect(replacement_count == 3);
        }
    }

    gp_suite("C trim"); // TODO
    {
        // gp_test("C left");
        // {
        //     struct GPString str = gpstr_on_stack([128], "  \t\f \nLeft");
        //     gpstr_trim(&str, GPSTR_WHITESPACE, 'l');
        //     gp_expect(gpstr_eq(str, gpstr("Left")), (gpcstr(str)));
        // }

        // gp_test("C right");
        // {
        //     struct GPString str = gpstr_on_stack([128], "Right   \t\v\n\r");
        //     gpstr_trim(&str, GPSTR_WHITESPACE, 'r');
        //     gp_expect(gpstr_eq(str, gpstr("Right")), (gpcstr(str)));
        // }

        // gp_test("C left and right");
        // {
        //     struct GPString str = gpstr_on_stack([128], "   __Left and Right__   ");
        //     gpstr_trim(&str, GPSTR_WHITESPACE "_", 'l' + 'r');
        //     gp_expect(gpstr_eq(str, gpstr("Left and Right")), (gpcstr(str)));
        // }
    }

    gp_suite("C print");
    {
        #if __STDC_VERSION__ >= 201112L
        gp_test("C Numbers");
        {
            char str[128];
            gp_cstr_print(str, 1, " divided by ", 3, " is ", 1./3.);
            char buf[128];
            sprintf(buf, "%i divided by %i is %g", 1, 3, 1./3.);
            gp_expect(gp_cstr_equal(str, buf), (str), (buf));
        }

        gp_test("C Strings");
        {
            char str[128];
            char str1[128];
            strcpy(str1, "strings");
            gp_cstr_print(str, "Copying ", str1, (char)'.');
            gp_expect(gp_cstr_equal(str, "Copying strings."));
        }
        #endif
    }

    // ------------------------------------------------------------------------
    // Test internals

    gp_suite("memchr_r");
    {
        const char* haystack = "dcba";
        const char* haystack_end = haystack + strlen(haystack);
        size_t pos = GP_NOT_FOUND;

        pos = (size_t)(memchr_r(haystack_end, 'a', strlen(haystack)) - haystack);
        gp_expect(pos == 3, (pos));

        pos = (size_t)(memchr_r(haystack_end, 'd', strlen(haystack)) - haystack);
        gp_expect(pos == 0, (pos));

        const char* _pos = memchr_r(haystack_end, 'x', strlen(haystack));
        gp_expect(_pos == NULL);
    }
}
