// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"
#include <locale.h>
#include <errno.h>
#include <signal.h>

int main(void)
{
    gp_suite("Creating strings");
    {
        gp_test("on stack");
        {
            #if GP_INITIALIZER_STRING_IS_TOO_LONG_WARNING
            GPString str = gp_str_on_stack(GP_NO_ALLOC, 1, "too long!");
            #elif GP_NON_LITERAL_INITIALIZER_WARNING
            const char* non_literal = "only literals allowed!";
            GPString str = gp_str_on_stack(GP_NO_ALLOC, 32, non_literal);
            #else
            GPString str = gp_str_on_stack(GP_NO_ALLOC, 7, "ok");
            #endif
            gp_expect( ! gp_allocation(str),
                "No allocator given so no allocation either.");
            gp_str_copy(&str, "1234567", 7);
            gp_expect(
                strcmp(gp_cstr(str), "1234567") == 0 &&
                str[7].c == '\0' &&
                "\"%s\" extra byte not counted here", gp_capacity(str) == 7,
                "Extra byte is reserved so gp_cstr() can safely null-terminate.",
                gp_cstr(str), gp_capacity(str));

            str = gp_str_delete(str); // safe but pointless

            str = gp_str_on_stack(&gp_heap, 1, "");
            const char* cstr = "Allocator provided, extending is safe!";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_expect(gp_allocation(str),
                "Now in heap, must free with gp_str_clear() or gp_clear()!");

            gp_expect((str = gp_str_delete(str)) == NULL,
                "gp_str_clear() always returns NULL and it's a good idea to "
                "assign it back to str for easier memory bug debugging.");
        }

        gp_test("Reserve");
        {
            size_t old_capacity;
            GPString str = gp_str_on_stack(&gp_heap, 1, "");
            old_capacity = gp_capacity(str);

            gp_str_reserve(&str, 12);
            gp_expect(gp_capacity(str) > old_capacity);

            str = gp_str_delete(str);
        }

        gp_test("somewhere else than stack");
        {
            char* non_const_init = "too long but no worries.";
            GPString str = gp_str_new(&gp_heap, 1, non_const_init, strlen(non_const_init));
            gp_expect(gp_allocation(str));
            gp_expect(gp_capacity(str) == gp_next_power_of_2(strlen(non_const_init)),
                gp_capacity(str));

            gp_str_repeat(&str, gp_capacity(str), "X", strlen("X"));
            (void)gp_cstr(str); // again, extra reserved byte makes this safe!

            // Must free object on heap!
            gp_expect((str = gp_clear(str)) != NULL,
                "As opposed to gp_str_clear() function, gp_clear() macro does "
                "not return NULL, but a debug string instead.");
        }
    }

    gp_suite("Finding");
    {
        const GPString haystack = gp_str_on_stack(NULL, 16, "bbbaabaaabaa");
        const char* needle = "aa";
        const char* needle2 = "not in haystack string";
        size_t pos = 0;

        gp_test("Find");
        {
            pos = gp_str_find(haystack, needle, strlen(needle), 0);
            gp_expect(pos == 3);
            pos = gp_str_find(haystack, needle, strlen(needle), 4);
            gp_expect(pos == 6);
            pos = gp_str_find(haystack, needle2, strlen(needle2), 0);
            gp_expect(pos == GP_NOT_FOUND);
        }
        gp_test("Find last");
        {
            pos = gp_str_find_last(haystack, needle, strlen(needle));
            gp_expect(pos == 10, (pos));
            pos = gp_str_find_last(haystack, needle2, strlen(needle2));
            gp_expect(pos == GP_NOT_FOUND);
        }
        gp_test("Count");
        {
            size_t count = gp_str_count(haystack, needle, strlen(needle));
            gp_expect(count == 4);
        }
    }

    gp_suite("Str equal");
    {
        gp_test("Case sensitive");
        {
            const GPString blah  = gp_str_on_stack(NULL, 8,  "blah");
            const GPString blaah = gp_str_on_stack(NULL, 16, "blääh");
            gp_expect(gp_str_equal(blah, blah, gp_length(blah)));
            gp_expect(gp_str_equal(blaah, blaah, gp_length(blaah)));
            gp_expect( ! gp_str_equal(blah, "BLOH", strlen("BLOH")));
            gp_expect( ! gp_str_equal(blah, "blahhhh", 7));
        }

        gp_test("Case insensitive");
        {
            gp_assert(setlocale(LC_ALL, "C.utf8"));
            const GPString AaAaOo = gp_str_on_stack(NULL, 24, "AaÄäÖö");
            gp_expect(   gp_str_equal_case(AaAaOo, "aaÄÄöÖ", strlen("aaÄÄöÖ")));
            gp_expect( ! gp_str_equal_case(AaAaOo, "aaxÄöÖ", strlen("aaxÄöÖ")));
            gp_expect( ! gp_str_equal_case(AaAaOo, "aaÄÄöÖuu", strlen("aaÄÄöÖuu")));
        }
    }

    gp_suite("UTF-8 indices");
    {
        gp_test("Valid index");
        {
            GPString str = gp_str_on_stack(NULL, 8, "\u1153");
            gp_expect(   gp_str_codepoint_length(str));
            gp_str_slice(&str, 1, gp_length(str));
            gp_expect( ! gp_str_codepoint_length(str));
        }

        gp_test("Codepoint size");
        {
            GPString str = gp_str_on_stack(NULL, 8, "\u1153");
            gp_expect(gp_str_codepoint_length(str) == strlen("\u1153"));
        }

        gp_test("Codepoint count");
        {
            GPString str = gp_str_on_stack(NULL, 16, "aÄb🍌x");
            gp_expect(gp_str_codepoint_count(str) == 5);
        }
    }

    gp_suite("Substrings");
    {
        gp_test("Slice");
        {
            GPString str = gp_str_on_stack(NULL, 20, "Some_string_to slice");
            gp_str_slice(&str, 5, 11);
            gp_expect(gp_str_equal(str, "string", strlen("string")));
        }

        gp_test("Memory management on slice");
        {
            GPString str = gp_str_on_stack(NULL, 16, "...Junk prefix.");
            size_t old_capacity = gp_capacity(str);
            GPChar* old_ptr = str;

            gp_str_slice(&str, 3, gp_length(str));
            gp_expect(str > old_ptr && gp_capacity(str) < old_capacity,
                "Memory should've not moved, just the pointer, "
                "but capacity should shrink!");

            void* old_allocation = gp_allocation(str);
            old_capacity = gp_capacity(str);
            gp_str_reserve(&str, 15);
            gp_expect(gp_capacity(str) > old_capacity,
                "Capacity should've grown back,");
            gp_expect(gp_allocation(str) == old_allocation,
                "but no new allocation!");

            // Sanity check
            const char* cstr = "Junk prefix.";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)),
                str, cstr);
        }

        gp_test("Substr");
        {
            const char* src = "Some_string_to slice";
            GPString dest = gp_str_on_stack(NULL, 64, "");
            gp_str_substr(&dest, src, 5, 11); // not including 11!
            gp_expect(gp_str_equal(dest, "string", strlen("string")), dest);
        }
    }

    gp_suite("Insert and append");
    {
        gp_test("Appending");
        {
            GPString str = gp_str_on_stack(NULL, 36, "test");
            gp_str_append(&str, " tail", strlen(" tail"));
            gp_expect(gp_str_equal(str, "test tail", strlen("test tail")));
            gp_str_append(&str, ".BLAHBLAHBLAH", 1);
            gp_expect(gp_str_equal(str, "test tail.", strlen("test tail.")));
        }
        GPString str = gp_str_on_stack(NULL, 128, "test");
        const char* cstr = "test tail";
        gp_test("Appending with insert");
        {
            gp_str_insert(&str, gp_length(str), " tail", strlen(" tail"));
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));
        }
        gp_test("Prepending");
        {
            cstr = "head test tail";
            gp_str_insert(&str, 0, "head XXXXXX", 5);
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));
        }
        gp_test("Insertion");
        {
            cstr = "head insertion test tail";
            gp_str_insert(&str, 5, "insertion ", strlen("insertion "));
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));
        }
    }

    gp_suite("Replacing substrings");
    {
        gp_test("Replace");
        {
            GPString str = gp_str_on_stack(NULL, 128, "aaabbbcccaaa");
            const char* cstr = "aaaXcccaaa";
            size_t needlepos = gp_str_replace(&str, "bbb", 3, "X", 1, 0);
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);
            gp_expect(needlepos == 3, (needlepos));

            size_t start = 3;
            gp_str_replace(&str, "aaa", 3, "XXXXX", 5, start);
            cstr = "aaaXcccXXXXX";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);
        }

        gp_test("Replace_all");
        {
            GPString str = gp_str_on_stack(NULL, 128, "aaxxbbxxxccxx");
            size_t replacement_count = gp_str_replace_all(
                &str, "xx", 2, "XXX", 3);

            const char* cstr = "aaXXXbbXXXxccXXX";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));
            gp_expect(replacement_count == 3);
        }
    }

    gp_suite("String print");
    {
        gp_test("Numbers");
        {
            GPString str = gp_str_on_stack(&gp_heap, 1, "");
            gp_str_print(&str, 1, " divided by ", 3, " is ", 1./3.);
            char buf[128];
            sprintf(buf, "%i divided by %i is %g", 1, 3, 1./3.);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);
            str = gp_str_delete(str);
        }

        gp_test("Strings");
        {
            GPString str = gp_str_on_stack(NULL, 128, "");
            char str1[128];
            strcpy(str1, "strings");
            gp_str_print(&str, "Copying ", str1, (char)'.');
            gp_expect(gp_str_equal(str, "Copying strings.", strlen("Copying strings.")));
        }

        gp_test("Custom formats");
        {
            GPString str = gp_str_on_stack(NULL, 128, "");
            gp_str_print(&str,
                "%%No zeroes in this %g", 1.0, " float. %% %%");
            char buf[128];
            sprintf(buf, "%%No zeroes in this %g float. %% %%", 1.0);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, 2, " formats here: %x%g", 0xbeef, 0., (char)'.');
            sprintf(buf, "2 formats here: %x%g.", 0xbeef, 0.);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);
        }

        gp_test("%% only");
        {
            GPString str = gp_str_on_stack(NULL, 128, "");
            gp_str_print(&str, "%%blah%%");
            char buf[128];
            sprintf(buf, "%%blah%%");
            gp_expect(gp_str_equal(str, buf, strlen(buf)));
        }

        gp_test("Pointers");
        {
            GPString str = gp_str_on_stack(NULL, 128, "");
            char buf[128];
            gp_str_print(&str, (void*)buf);
            sprintf(buf, "%p", buf);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            gp_str_print(&str, NULL);
            sprintf(buf, "(nil)");
            gp_expect(gp_str_equal(str, buf, strlen(buf)));
        }

        gp_test("Print n");
        {
            GPString str = gp_str_on_stack(NULL, 128, "");
            gp_str_n_print(&str, 7, "blah", 12345);
            gp_expect(gp_str_equal(str, "blah123", strlen("blah123")), str);
        }

        gp_test("Println");
        {
            GPString str = gp_str_on_stack(NULL, 128, "");
            gp_str_println(&str, "Spaces", 3, "inserted.");
            const char* cstr = "Spaces 3 inserted.\n";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));

            cstr = "With 20 fmt specs.\n";
            gp_str_println(&str, "With %g%i", 2., 0, "fmt specs.");
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);
        }
    }


    // --------------------------------------
    //
    //
    //

    gp_suite("C validate");
    {
        gp_test("Valids");
        {
            const char *goodsequences[] = {
                "a",
                "\xc3\xb1",
                "\xe2\x82\xa1",
                "\xf0\x90\x8c\xbc",
                "\xc2\x80",
                "\xf0\x90\x80\x80",
                "\xee\x80\x80",
                "\xef\xbb\xbf"};
            for (size_t i = 0; i < sizeof(goodsequences)/sizeof(goodsequences[0]); i++)
                gp_expect(gp_cstr_is_valid(goodsequences[i]), i);
        }
        char *badsequences[] = {
            (char[]){"\xc3\x28"},
            (char[]){"\xa0\xa1"},
            (char[]){"\xe2\x28\xa1"},
            (char[]){"\xe2\x82\x28"},
            (char[]){"\xf0\x28\x8c\xbc"},
            (char[]){"\xf0\x90\x28\xbc"},
            (char[]){"\xf0\x28\x8c\x28"},
            (char[]){"\xc0\x9f"},
            (char[]){"\xf5\xff\xff\xff"},
            (char[]){"\xed\xa0\x81"},
            (char[]){"\xf8\x90\x80\x80\x80"},
            (char[]){"123456789012345\xed"},
            (char[]){"123456789012345\xf1"},
            (char[]){"123456789012345\xc2"},
            (char[]){"\xC2\x7F"},
            (char[]){"\xce"},
            (char[]){"\xce\xba\xe1"},
            (char[]){"\xce\xba\xe1\xbd"},
            (char[]){"\xce\xba\xe1\xbd\xb9\xcf"},
            (char[]){"\xce\xba\xe1\xbd\xb9\xcf\x83\xce"},
            (char[]){"\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce"},
            (char[]){"\xdf"},
            (char[]){"\xef\xbf"},
            (char[]){"\x80"},
            (char[]){"\x91\x85\x95\x9e"},
            (char[]){"\x6c\x02\x8e\x18"},
            (char[]){"\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x05\x29\x2c\x33\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x20\x01\x01\x01\x01\x01\x02\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x23\x0a\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x7e\x7e\x0a\x0a\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x01\x01\x80\x01\x01\x01\x79\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"},
            (char[]){"[[[[[[[[[[[[[[[\x80\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x010\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"},
            (char[]){"\x20\x0b\x01\x01\x01\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x80\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"}};
        gp_test("Invalids");
        {
            for (size_t i = 0; i < sizeof(badsequences)/sizeof(badsequences[0]); i++)
                gp_expect( ! gp_cstr_is_valid(badsequences[i]), i);
        }

        gp_test("Make valid");
        {
            const char* cleanedsequences[] = {
                "_\x28",
                "_",
                "_\x28_",
                "_\x28",
                "_\x28_",
                "_\x28_",
                "_\x28_\x28",
                "_",
                "_",
                "_",
                "_",
                "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35_",
                "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35_",
                "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35_",
                "_\x7f",
                "_",
                "\xce\xba_",
                "\xce\xba_",
                "\xce\xba\xe1\xbd\xb9_",
                "\xce\xba\xe1\xbd\xb9\xcf\x83_",
                "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc_",
                "_",
                "_",
                "_",
                "_",
                "\x6c\x2_\x18",
                "\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x5\x29\x2c\x33\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x20\x1\x1\x1\x1\x1\x2\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x23\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x7e\x7e\xa\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1_\x1\x1\x1\x79\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1",
                "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b_\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x10\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1",
                "\x20\xb\x1\x1\x1\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1_\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1"};
            for (size_t i = 0; i < sizeof(badsequences)/sizeof(badsequences[0]); i++)
                gp_cstr_to_valid(badsequences[i], "_");
            for (size_t i = 0; i < sizeof(badsequences)/sizeof(badsequences[0]); i++)
                gp_expect(gp_cstr_equal(badsequences[i], cleanedsequences[i]), i);
        }
    }

    gp_suite("C trim");
    {
        gp_test("C ASCII");
        {
            char str[128];
            strcpy(str, "  \t\f \nLeft Ascii  ");
            gp_cstr_trim(str, NULL, 'l' | 'a');
            gp_expect(gp_cstr_equal(str,  "Left Ascii  "));

            strcpy(str, " AA RightSAICASIACSIACIAS");
            gp_cstr_trim(str, "ASCII", 'r' | 'a');
            gp_expect(gp_cstr_equal(str, " AA Right"));

            strcpy(str, "  __Left and Right__  ");
            gp_cstr_trim(str, GP_ASCII_WHITESPACE "_", 'l' | 'r' | 'a');
            gp_expect(gp_cstr_equal(str, "Left and Right"), str);
        }
        gp_test("C UTF-8");
        {
            char str[128];
            strcpy(str, "¡¡¡Left!!!");
            gp_cstr_trim(str, "¡", 'l');
            gp_expect(gp_cstr_equal(str, "Left!!!"), str);

            strcpy(str, " Right\r\u200A\r\n");
            gp_cstr_trim(str, NULL, 'r');
            gp_expect(gp_cstr_equal(str, " Right"), str);

            strcpy(str, "\t\u3000 ¡¡Left and Right!! \n");
            gp_cstr_trim(str, GP_WHITESPACE "¡!", 'l' | 'r');
            gp_expect(gp_cstr_equal(str, "Left and Right"));
        }
    }

    gp_suite("C to upper/lower");
    {
        gp_test("Finnish");
        {
            gp_assert(setlocale(LC_ALL, "C.utf8"));

            char str[128];
            strcpy(str, "blääf");
            gp_cstr_to_upper(str);
            gp_expect(gp_cstr_equal(str, "BLÄÄF"), str);
            gp_cstr_to_lower(str);
            gp_expect(gp_cstr_equal(str, "blääf"));
        }

        if (setlocale(LC_ALL, "tr_TR.utf8") != NULL)
        {
            gp_test("Turkish"); // Note how ı changes to ASCII and back
            {
                char str[128];
                strcpy(str, "yaşar bayrı");
                size_t upper_length = gp_cstr_to_upper(str);
                gp_expect(gp_cstr_equal(str, "YAŞAR BAYRI"), (str));
                size_t lower_length = gp_cstr_to_lower(str);
                gp_expect(gp_cstr_equal(str, "yaşar bayrı"));
                gp_expect(upper_length != lower_length,
                    "Lengths may change!");
            }
        } // else Turkish language pack not installed.
    }


    // ------------------------------------------------------------------------
    // Test internals

    gp_suite("memchr_r");
    {
        const char* haystack = "dcba";
        const char* haystack_end = haystack + strlen(haystack);
        size_t pos = GP_NOT_FOUND;

        gp_test("last index");
        pos = (size_t)(memchr_r(haystack_end, 'a', strlen(haystack)) - haystack);
        gp_expect(pos == 3, (pos));

        gp_test("index 0");
        pos = (size_t)(memchr_r(haystack_end, 'd', strlen(haystack)) - haystack);
        gp_expect(pos == 0, (pos));

        gp_test("not found");
        const char* _pos = memchr_r(haystack_end, 'x', strlen(haystack));
        gp_expect(_pos == NULL);
    }

    char fjdskla[1] = "";
    gp_cstr_print_n(
        fjdskla,
        0,
        gp_str_on_stack(NULL, 0, ""));
}
