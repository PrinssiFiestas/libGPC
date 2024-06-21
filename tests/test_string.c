// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"
#include <locale.h>
#include <errno.h>
#include <signal.h>
#include <inttypes.h>

#if _WIN32
// quick hack to disable locale dependent tests for now
#define setlocale(...) NULL
#endif

int main(void)
{
    gp_suite("Creating strings");
    {
        gp_test("on stack");
        {
            #if GP_INITIALIZER_STRING_IS_TOO_LONG_WARNING
            GPString str = gp_str_on_stack(NULL, 1, "too long!");
            #elif GP_NON_LITERAL_INITIALIZER_WARNING
            const char* non_literal = "only literals allowed!";
            GPString str = gp_str_on_stack(NULL, 32, non_literal);
            #else
            GPString str = gp_str_on_stack(NULL, 8, "ok");
            #endif
            gp_expect( ! gp_arr_allocation(str),
                "No allocator given so no allocation either.");
            gp_str_copy(&str, "1234567", 7);
            gp_expect(
                strcmp(gp_cstr(str), "1234567") == 0 &&
                str[7].c == '\0' &&
                "\"%s\" extra byte not counted here", gp_arr_capacity(str) == 8,
                "Extra byte should be reserved so gp_cstr() can null-terminate.",
                gp_cstr(str), gp_arr_capacity(str));

            gp_str_delete(str); // safe but pointless

            str = gp_str_on_stack(gp_heap, 1);
            const char* cstr = "Allocator provided, extending is safe!";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_expect(gp_arr_allocation(str),
                "Now in heap, must free with gp_str_delete()!");
            gp_str_delete(str);
        }

        gp_test("Reserve");
        {
            size_t old_capacity;
            GPString str = gp_str_on_stack(gp_heap, 1);
            old_capacity = gp_arr_capacity(str);

            gp_str_reserve(&str, 12);
            gp_expect(gp_arr_capacity(str) > old_capacity);

            gp_str_delete(str);
        }

        gp_test("somewhere else than stack");
        {
            GPString str = gp_str_new(gp_heap, 1, "");
            gp_expect(gp_arr_allocation(str) != NULL);

            gp_str_repeat(&str, gp_arr_capacity(str), "X", strlen("X"));
            (void)gp_cstr(str); // again, extra reserved byte makes this safe!

            // Must free object on heap!
            gp_str_delete(str);
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
            pos = gp_str_find_first(haystack, needle, strlen(needle), 0);
            gp_expect(pos == 3);
            pos = gp_str_find_first(haystack, needle, strlen(needle), 4);
            gp_expect(pos == 6);
            pos = gp_str_find_first(haystack, needle2, strlen(needle2), 0);
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
            gp_expect(gp_str_equal(blah, blah, gp_arr_length(blah)));
            gp_expect(gp_str_equal(blaah, blaah, gp_arr_length(blaah)));
            gp_expect( ! gp_str_equal(blah, "BLOH", strlen("BLOH")));
            gp_expect( ! gp_str_equal(blah, "blahhhh", 7));
        }

        gp_test("Case insensitive");
        {
            if (setlocale(LC_ALL, "C.utf8") != NULL)
            {
                const GPString AaAaOo = gp_str_on_stack(NULL, 24, "AaÄäÖö");
                gp_expect(   gp_str_equal_case(AaAaOo, "aaÄÄöÖ", strlen("aaÄÄöÖ")));
                gp_expect( ! gp_str_equal_case(AaAaOo, "aaxÄöÖ", strlen("aaxÄöÖ")));
                gp_expect( ! gp_str_equal_case(AaAaOo, "aaÄÄöÖuu", strlen("aaÄÄöÖuu")));
            }
        }
    }

    gp_suite("UTF-8 examination");
    {
        gp_test("Character classification");
        {
            if (setlocale(LC_ALL, "C.utf8") != NULL)
            { // Note that GP_WHITESPACE includes characters defined by unicode
              // while iswspace() includes characters defined by locale which
              // may differ slightly.
                const GPString str = gp_str_on_stack(NULL, 16, " \t\n\u2008XÄ");
                gp_expect(   gp_str_codepoint_classify(str, 0, iswspace));
                gp_expect(   gp_str_codepoint_classify(str, 1, iswspace));
                gp_expect(   gp_str_codepoint_classify(str, 2, iswspace));
                gp_expect(   gp_str_codepoint_classify(str, 3, iswspace));
                gp_expect( ! gp_str_codepoint_classify(str, 5, iswspace));
                gp_expect( ! gp_str_codepoint_classify(str, 6, iswspace));
            }
        }

        gp_test("Find first of");
        {
            const GPString str = gp_str_on_stack(NULL, 16, "blörö");
            gp_expect(gp_str_find_first_of(str, "yö", 0) == 2);
            gp_expect(gp_str_find_first_of(str, "aä", 0) == GP_NOT_FOUND);
        }

        gp_test("Find first not of");
        {
            const GPString str = gp_str_on_stack(NULL, 16, "blörö");
            gp_expect(gp_str_find_first_not_of(str, "blö", 0)   == strlen("blö"));
            gp_expect(gp_str_find_first_not_of(str, "blörö", 0) == GP_NOT_FOUND);
        }
    }

    gp_suite("UTF-8 indices");
    {
        gp_test("Valid index");
        {
            GPString str = gp_str_on_stack(NULL, 8, "\u1153");
            gp_expect(   gp_str_codepoint_length(str, 0));
            gp_str_slice(&str, NULL, 1, gp_arr_length(str));
            gp_expect( ! gp_str_codepoint_length(str, 0));
        }

        gp_test("Codepoint size");
        {
            GPString str = gp_str_on_stack(NULL, 8, "\u1153");
            gp_expect(gp_str_codepoint_length(str, 0) == strlen("\u1153"));
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
            gp_str_slice(&str, NULL, 5, 11);
            gp_expect(gp_str_equal(str, "string", strlen("string")));
        }

        gp_test("Substr");
        {
            const char* src = "Some_string_to slice";
            GPString dest = gp_str_on_stack(NULL, 64);
            gp_str_slice(&dest, src, 5, 11); // not including 11!
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
            gp_str_insert(&str, gp_arr_length(str), " tail", strlen(" tail"));
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

    #ifndef _WIN32 // gp_print() conversions match glibc
    gp_suite("String print");
    {
        gp_test("Numbers");
        {
            GPString str = gp_str_on_stack(gp_heap, 1);
            gp_str_print(&str, 1, " divided by ", 3, " is ", 1./3.);
            char buf[128];
            sprintf(buf, "%i divided by %i is %g", 1, 3, 1./3.);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);
            gp_str_delete(str);
        }

        gp_test("Strings");
        {
            GPString str = gp_str_on_stack(NULL, 16);
            char str1[128];
            strcpy(str1, "strings");
            gp_str_print(&str, "Copying ", str1, (char)'.');
            gp_expect(gp_str_equal(str, "Copying strings.", strlen("Copying strings.")));
        }

        gp_test("Formatting");
        {
            GPString str = gp_str_on_stack(NULL, 128);
            gp_str_print(&str,
                "%%No zeroes in this %g", 1.0, " float. %% %%");
            char buf[128];
            sprintf(buf, "%%No zeroes in this %g float. %% %%", 1.0);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, 2, " formats here: %x%g", 0xbeef, 0., (char)'.');
            sprintf(buf, "2 formats here: %x%g.", 0xbeef, 0.);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            GPString str2 = gp_str_on_stack(NULL, 128, "Capital S");
            gp_str_print(&str, "%S for GPString", str2);
            sprintf(buf, "%s for GPString", gp_cstr(str2));
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            GPString str3 = gp_str_on_stack(NULL, 128);
            gp_str_copy(&str,  "a", 1);
            gp_str_copy(&str2, "ä", strlen("ä"));
            gp_str_print(&str3, "|%4S|%4S|%4s|", str, str2, "ö");
            strcpy(buf, "|   a|   ä|  ö|");
            gp_expect(gp_str_equal(str3, buf, strlen(buf)),
                "GPString should calculate field width based on UTF-8 "
                "codepoints. This is not true for char* though.", str3);
            gp_str_print(&str3, "|%4.1S|", str2);
            gp_expect(gp_str_equal(str3, "|    |", strlen("|    |")),
                "Precision is calculated in bytes. Here 'ä' didn't fit. Instead "
                "of truncating the codepoint making the stirng invalid UTF-8, "
                "'ä' got removed completely.");
        }

        gp_test("Fixed width length modifiers for format strings");
        { // Can be used for any integer formats. Here we stick with %u for
          // simplicity.
            GPString str = gp_str_on_stack(NULL, 128);
            char buf[128];

            gp_str_print(&str, "Byte %Bu", (uint8_t)-1);
            sprintf(buf, "Byte %"PRIu8, (uint8_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, "Word %Wu", (uint16_t)-1);
            sprintf(buf, "Word %"PRIu16, (uint16_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, "Double word %Du", (uint32_t)-1);
            sprintf(buf, "Double word %"PRIu32, (uint32_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, "Quad word %Qu", (uint64_t)-1);
            sprintf(buf, "Quad word %"PRIu64, (uint64_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

        }

        gp_test("%% only");
        {
            GPString str = gp_str_on_stack(NULL, 128);
            gp_str_print(&str, "%%blah%%");
            char buf[128];
            sprintf(buf, "%%blah%%");
            gp_expect(gp_str_equal(str, buf, strlen(buf)));
        }

        gp_test("Pointers");
        {
            GPString str = gp_str_on_stack(NULL, 128);
            char buf[128];
            uintptr_t _buf = (uintptr_t)buf; // shut up -Wrestrict
            gp_str_print(&str, (void*)buf);
            sprintf(buf, "%p", (void*)_buf);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            gp_str_print(&str, NULL);
            sprintf(buf, "(nil)");
            gp_expect(gp_str_equal(str, buf, strlen(buf)));
        }

        gp_test("Print n");
        {
            GPString str = gp_str_on_stack(NULL, 128);
            gp_str_n_print(&str, 7, "blah", 12345);
            gp_expect(gp_str_equal(str, "blah123", strlen("blah123")), str);
        }

        gp_test("Println");
        {
            GPString str = gp_str_on_stack(NULL, 128);
            gp_str_println(&str, "Spaces", 3, "inserted.");
            const char* cstr = "Spaces 3 inserted.\n";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));

            cstr = "With 20 fmt specs.\n";
            gp_str_println(&str, "With %g%i", 2., 0, "fmt specs.");
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);
        }
    }
    #endif

    gp_suite("Trim");
    {
        gp_test("ASCII");
        {
            GPString str = gp_str_on_stack(NULL, 128, "  \t\f \nLeft Ascii  ");
            gp_str_trim(&str, NULL, 'l' | 'a');
            gp_expect(gp_str_equal(str, "Left Ascii  ", strlen("Left Ascii  ")));

            const char* cstr = " AA RightSAICASIACSIACIAS";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_str_trim(&str, "ASCII", 'r' | 'a');
            gp_expect(gp_str_equal(str, " AA Right", strlen(" AA Right")));

            cstr = "  __Left and Right__  ";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_str_trim(&str, GP_ASCII_WHITESPACE "_", 'l' | 'r' | 'a');
            gp_expect(gp_str_equal(str, "Left and Right", strlen( "Left and Right")), str);
        }
        gp_test("UTF-8");
        {
            GPString str = gp_str_on_stack(NULL, 128, "¡¡¡Left!!!");
            gp_str_trim(&str, "¡", 'l');
            gp_expect(gp_str_equal(str, "Left!!!", strlen("Left!!!")), str);

            gp_str_copy(&str, " Right\r\u200A\r\n", strlen(" Right\r\u200A\r\n"));
            gp_str_trim(&str, NULL, 'r');
            gp_expect(gp_str_equal(str, " Right", strlen(" Right")), str);

            gp_str_copy(&str, "\t\u3000 ¡¡Left and Right!! \n", strlen("\t\u3000 ¡¡Left and Right!! \n"));
            gp_str_trim(&str, GP_WHITESPACE "¡!", 'l' | 'r');
            gp_expect(gp_str_equal(str, "Left and Right", strlen("Left and Right")));
        }
    }

    gp_suite("To upper/lower");
    {
        if (setlocale(LC_ALL, "C.utf8") != NULL)
        {
            gp_test("Finnish");
            {
                GPString str = gp_str_on_stack(NULL, 64, "blääf");
                gp_str_to_upper(&str);
                gp_expect(gp_str_equal(str, "BLÄÄF", strlen("BLÄÄF")), str);
                gp_str_to_lower(&str);
                gp_expect(gp_str_equal(str, "blääf", strlen("blääf")));
            }
        }

        if (setlocale(LC_ALL, "tr_TR.utf8") != NULL)
        {
            gp_test("Turkish"); // Note how ı changes to ASCII and back
            {
                GPString str = gp_str_on_stack(NULL, 128, "yaşar bayrı");
                gp_str_to_upper(&str);
                gp_expect(gp_str_equal(str, "YAŞAR BAYRI", strlen("YAŞAR BAYRI")), str);
                gp_str_to_lower(&str);
                gp_expect(gp_str_equal(str, "yaşar bayrı", strlen("yaşar bayrı")));
            }
        } // else Turkish language pack not installed.
    }

    gp_suite("Case insensitive comparison");
    {
        GPString str1 = gp_str_on_stack(NULL, 64, "hrnec");
        GPString str2 = gp_str_on_stack(NULL, 64, "chrt");

        if (setlocale(LC_ALL, "en_US.utf8") != NULL)
        { gp_test("American locale");
            gp_expect(gp_str_case_compare(str1, str2) > 0);
        }

        if (setlocale(LC_COLLATE, "cs_CZ.utf8") != NULL)
        { gp_test("Czech lcoale");
            gp_expect(gp_str_case_compare(str1, str2) < 0);
        }

        gp_str_copy(&str1, "år",    strlen("år"));
        gp_str_copy(&str1, "ängel", strlen("ängel"));
        if (setlocale(LC_COLLATE, "en_US.utf8") != NULL)
        { gp_test("American locale å");
            gp_expect(gp_str_case_compare(str1, str2) < 0);
        }

        if (setlocale(LC_COLLATE, "sv_SE.utf8") != NULL)
        { gp_test("Swedish locale å");
            gp_expect(gp_str_case_compare(str1, str2) > 0);
        }
    }

    gp_suite("Validate");
    {
        GPString str = gp_str_new(gp_heap, 32, "");

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
            {
                gp_str_copy(&str, goodsequences[i], strlen(goodsequences[i]));
                gp_expect(gp_str_is_valid(str, NULL), i);
            }
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
            {
                gp_str_copy(&str, badsequences[i], strlen(badsequences[i]));
                gp_expect( ! gp_str_is_valid(str, NULL), i);
            }
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
            for (size_t i = 0; i < sizeof(badsequences)/sizeof(*badsequences); i++)
            {
                gp_str_copy(&str, badsequences[i], strlen(badsequences[i]));
                gp_str_to_valid(&str, "_");
                gp_expect(gp_str_equal(str, cleanedsequences[i], strlen(cleanedsequences[i])), i);
            }
        }
        gp_str_delete(str);
    }

    gp_suite("Read file");
    {
        gp_test("Reading and writing");
        {
            GPString str = gp_str_on_stack(NULL, 36, "blah blah blah");
            gp_assert(gp_str_file(&str, "gp_test_str_file.txt", "write") == 0);
            gp_str_copy(&str, "XXXX XXXX XXXX", strlen("XXXX XXXX XXXX")); // corrupt memory
            gp_str_copy(&str, "", 0); // empty string
            gp_assert(gp_str_file(&str, "gp_test_str_file.txt", "read") == 0);
            gp_expect(gp_str_equal(str, "blah blah blah", strlen("blah blah blah")));
        }

        gp_test("Non existent");
        {
            GPString str = gp_str_on_stack(gp_heap, 1);
            // Only first char in mode is checked so "r" is fine too
            gp_expect(gp_str_file(&str, "NON_EXISTENT.txt", "r") != 0);
            gp_str_delete(str);
        }

        gp_expect(remove("gp_test_str_file.txt") == 0);
    }
}
