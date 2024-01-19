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
            GPString str = gpstr_on_stack([], "Some_string_to slice");
            gpstr_slice(&str, 5, 11); // not including 11!
            gp_expect(gpstr_eq(str, gpstr("string")));
        }

        gp_test("substr");
        {
            GPString src = gpstr("Some_string_to slice");
            GPString dest = gpstr_on_stack([128], "");
            gpstr_substr(&dest, src, 5, 11); // not including 11!
            gp_expect(gpstr_eq(dest, gpstr("string")),
            ("%s", gpcstr(dest)));
        }
    }

    gp_suite("insert");
    {
        GPString str = gpstr_on_stack([128], "test");
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
        GPString haystack = gpstr("bbbaabaaabaa");
        GPString needle = gpstr("aa");
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
            GPString str = gpstr_on_stack([128], "aaabbbcccaaa");
            size_t needlepos = gpstr_replace(&str, gpstr("bbb"), gpstr("X"), 0);
            gp_expect(gpstr_eq(str, gpstr("aaaXcccaaa")));
            gp_expect(needlepos == 3);

            gpstr_replace(&str, gpstr("aaa"), gpstr("XXXXX"), 3);
            gp_expect(gpstr_eq(str, gpstr("aaaXcccXXXXX")));

        }

        gp_test("replace_all");
        {
            GPString str = gpstr_on_stack([128], "aaxxbbxxxccxx");
            unsigned replacement_count =
                gpstr_replace_all(&str, gpstr("xx"), gpstr("XXX"));
            gp_expect(gpstr_eq(str, gpstr("aaXXXbbXXXxccXXX")));
            gp_expect(replacement_count == 3);
        }
    }

    // ------------------------------------------------------------------------
    // Test internals

    gp_test("memchr_r");
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
