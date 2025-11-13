// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../include/gpc/io.h"
#include "../src/string.c"
#include <locale.h>
#include <errno.h>
#include <signal.h>
#include <inttypes.h>

int main(void)
{
    gp_suite("Creating strings");
    {
        gp_test("on stack");
        {
            #if GP_INITIALIZER_STRING_IS_TOO_LONG_WARNING
            GPStringBuffer(0) buf;
            GPString str = gp_str_buffered(NULL, &buf, "too long!");
            #elif GP_NON_LITERAL_INITIALIZER_WARNING
            GPStringBuffer(0) buf;
            const char* non_literal = "only literals allowed!";
            // The issued error may be confusing. Clang: "Expected ')'"
            GPString str = gp_str_buffered(NULL, &buf, non_literal);
            #else
            GPStringBuffer(7) buf;
            GPString str = gp_str_buffered(NULL, &buf, "ok");
            #endif
            gp_expect( ! gp_arr_allocation(str),
                "No allocator given so no allocation either.");
            gp_str_copy(&str, "1234567", 7);
            gp_expect(
                strcmp(gp_cstr(str), "1234567") == 0 &&
                str[7].c == '\0' &&
                gp_str_capacity(str) == 7,
                "Extra byte should be reserved so gp_cstr() can null-terminate.",
                gp_cstr(str), gp_arr_capacity(str));

            gp_str_delete(str); // safe but pointless

            GPStringBuffer(0) small_buf;
            str = gp_str_buffered(gp_global_heap, &small_buf);
            const char* cstr = "Allocator provided, extending is safe!";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_expect(gp_arr_allocation(str),
                "Now in heap, must free with gp_str_delete()!");
            gp_str_delete(str);
        }

        gp_test("Reserve");
        {
            size_t old_capacity;
            GPStringBuffer(0) buf;
            GPString str    = gp_str_buffered(gp_global_heap, &buf);
            old_capacity    = gp_str_capacity(str);
            GPChar* old_ptr = str;

            gp_str_reserve(&str, old_capacity);
            gp_expect(gp_str_capacity(str) == old_capacity);
            gp_expect(str == old_ptr);

            size_t new_requested_capacity = 12;
            gp_str_reserve(&str, new_requested_capacity);
            gp_expect(gp_str_capacity(str) > old_capacity);
            gp_expect(gp_str_capacity(str) > new_requested_capacity,
                "Reserving should round up for exponential growth.");
            gp_expect(gp_str_capacity(str) & 1,
                "One allocated byte should be taken out from the space reserved "
                "for the string (making capacity odd). This byte guarantees "
                "safe null-termination without reallocations.");
            gp_expect(old_ptr != str);

            gp_str_delete(str);
        }

        gp_test("somewhere else than stack");
        {
            GPString str = gp_str_new(gp_global_heap, 1);
            gp_expect(gp_arr_allocation(str) != NULL);

            gp_str_repeat(&str, gp_arr_capacity(str), "X", strlen("X"));
            (void)gp_cstr(str); // again, extra reserved byte makes this safe!

            // Must free object on heap!
            gp_str_delete(str);
        }
    }

    gp_suite("Finding");
    {
        GPStringBuffer(15) buf;
        const GPString haystack = gp_str_buffered(NULL, &buf, "bbbaabaaabaa");
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
            GPStringBuffer(7) blah_buf;
            GPStringBuffer(15) blaah_buf;
            const GPString blah  = gp_str_buffered(NULL, &blah_buf,  "blah");
            const GPString blaah = gp_str_buffered(NULL, &blaah_buf, "blääh");
            gp_expect(gp_str_equal(blah, blah, gp_arr_length(blah)));
            gp_expect(gp_str_equal(blaah, blaah, gp_arr_length(blaah)));
            gp_expect( ! gp_str_equal(blah, "BLOH", strlen("BLOH")));
            gp_expect( ! gp_str_equal(blah, "blahhhh", 7));
        }

        gp_test("Case insensitive");
        {
            GPStringBuffer(23) buf;
            const GPString AaAaOo = gp_str_buffered(NULL, &buf, "AaÄäÖö");
            gp_expect(   gp_str_equal_case(AaAaOo, "aaÄÄöÖ", strlen("aaÄÄöÖ")));
            gp_expect( ! gp_str_equal_case(AaAaOo, "aaxÄöÖ", strlen("aaxÄöÖ")));
            gp_expect( ! gp_str_equal_case(AaAaOo, "aaÄÄöÖuu", strlen("aaÄÄöÖuu")));
        }
    }

    gp_suite("UTF-8 examination");
    {
        gp_test("Find first of");
        {
            GPStringBuffer(15) buf;
            GPString str = gp_str_buffered(NULL, &buf, "blörö");
            gp_expect(gp_str_find_first_of(str, "yö", 0) == 2);
            gp_expect(gp_str_find_first_of(str, "aä", 0) == GP_NOT_FOUND);

            str = gp_str_buffered(NULL, &buf, "em\xFFoji🙊");
            gp_expect(gp_str_find_first_of(str, "X🙊", 0) == strlen("em\xFFoji"));
            gp_str_set(str)->length -= strlen("🙊")-1; // truncate emoji
            gp_expect(gp_str_find_first_of(str, "X🙊", 0) == GP_NOT_FOUND);
        }

        gp_test("Find first not of");
        {
            GPStringBuffer(18) buf;
            GPString str = gp_str_buffered(NULL, &buf, "blörö");
            gp_expect(gp_str_find_first_not_of(str, "blö", 0)   == strlen("blö"));
            gp_expect(gp_str_find_first_not_of(str, "blörö", 0) == GP_NOT_FOUND);

            str = gp_str_buffered(NULL, &buf, "❌X❎bläh🙊");
            gp_expect(gp_str_find_first_not_of(str, "X❌❎", 0) == strlen("❌X❎"));

            gp_str_slice(&str, NULL, strlen("❌X❎"), gp_str_length(str));
            gp_expect(gp_str_find_first_not_of(str, "lXäbYh", 0) == strlen("bläh"));
            gp_str_set(str)->length -= strlen("🙊")-1; // truncate emoji
            // If codepoint is invalid (truncated in this case) then it cannot
            // be in character set.
            gp_expect(gp_str_find_first_not_of(str, "lX🙊äbYh", 0) != GP_NOT_FOUND);
        }
    }

    gp_suite("UTF-8 indices");
    {
        gp_test("Valid index");
        {
            GPStringBuffer(7) buf;
            GPString str = gp_str_buffered(NULL, &buf, "\u1153");
            gp_expect(   gp_utf8_decode_codepoint_length(str, 0));
            gp_str_slice(&str, NULL, 1, gp_arr_length(str));
            gp_expect( ! gp_utf8_decode_codepoint_length(str, 0));
        }

        gp_test("Codepoint size");
        {
            GPStringBuffer(7) buf;
            GPString str = gp_str_buffered(NULL, &buf, "\u1153");
            gp_expect(gp_utf8_decode_codepoint_length(str, 0) == strlen("\u1153"));
        }

        gp_test("Codepoint count");
        {
            size_t count;

            GPStringBuffer(31) buf;
            GPString str = gp_str_buffered(NULL, &buf, "aÄb🍌x");
            gp_expect(gp_str_codepoint_count(str) == 5);
            count = 0;
            for (size_t cp_len, i = 0; i < gp_str_length(str); ++count, i += cp_len)
                cp_len = gp_utf8_decode(
                    &(uint32_t){0}, str, gp_str_length(str), i, &(bool){0});
            gp_expect(count == gp_str_codepoint_count(str), count);

            // String with invalids
            count = 0;
            str = gp_str_buffered(NULL, &buf, "\xf1\x80""ascii\xff\xffää\xc0\x80ȿȿⱥⱥ\xf2");
            for (size_t cp_len, i = 0; i < gp_str_length(str); ++count, i += cp_len)
                cp_len = gp_utf8_decode(
                    &(uint32_t){0}, str, gp_str_length(str), i, &(bool){0});
            gp_expect(count == gp_str_codepoint_count(str));
        }
    }

    gp_suite("Substrings");
    {
        gp_test("Slice");
        {
            GPStringBuffer(20) buf;
            GPString str = gp_str_buffered(NULL, &buf, "Some_string_to slice");
            gp_str_slice(&str, NULL, 5, 11);
            gp_expect(gp_str_equal(str, "string", strlen("string")));
        }

        gp_test("Substr");
        {
            GPStringBuffer(63) buf;
            const char* src = "Some_string_to slice";
            GPString dest = gp_str_buffered(NULL, &buf);
            gp_str_slice(&dest, src, 5, 11); // not including 11!
            gp_expect(gp_str_equal(dest, "string", strlen("string")), dest);
        }
    }

    gp_suite("Insert, Push, and Append");
    {
        gp_test("Appending");
        {
            GPStringBuffer(35) buf;
            GPString str = gp_str_buffered(NULL, &buf, "test");
            gp_str_append(&str, " tail", strlen(" tail"));
            gp_expect(gp_str_equal(str, "test tail", strlen("test tail")));
            gp_str_append(&str, ".BLAHBLAHBLAH", 1);
            gp_expect(gp_str_equal(str, "test tail.", strlen("test tail.")));
        }
        GPStringBuffer(127) buf;
        GPString str = gp_str_buffered(NULL, &buf, "test");
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
            GPStringBuffer(127) buf;
            GPString str = gp_str_buffered(NULL, &buf, "aaabbbcccaaa");
            const char* cstr = "aaaXcccaaa";
            size_t needlepos = gp_str_find_first(str, "bbb", sizeof"bbb"-1, 0);
            gp_str_replace(&str, needlepos, sizeof"bbb"-1, "X", 1);
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);

            needlepos = gp_str_find_first(str, "aaa", sizeof"aaa"-1, 3);
            gp_str_replace(&str, needlepos, sizeof"aaa"-1, "XXXXX", 5);
            cstr = "aaaXcccXXXXX";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);
        }
    }

    #ifdef __GLIBC__ // gp_print() conversions match glibc
    gp_suite("String Print");
    {
        gp_test("Numbers");
        {
            GPStringBuffer(0) buf;
            GPString str = gp_str_buffered(gp_global_heap, &buf);
            gp_str_print(&str, 1, " divided by ", 3, " is ", 1./3.);
            char buf1[128];
            sprintf(buf1, "%i divided by %i is %g", 1, 3, 1./3.);
            gp_expect(gp_str_equal(str, buf1, strlen(buf1)), str, buf1);
            gp_str_delete(str);
        }

        gp_test("Strings");
        {
            GPStringBuffer(15) buf;
            GPString str = gp_str_buffered(NULL, &buf);
            char str1[128];
            strcpy(str1, "string");
            gp_str_print(&str, "Copying ", str1, (char)'.');
            gp_expect(gp_str_equal(str, "Copying string.", strlen("Copying string.")));
        }

        gp_test("Formatting");
        {
            GPStringBuffer(127) buf0;
            GPString str = gp_str_buffered(NULL, &buf0);
            gp_str_print(&str,
                "%%No zeroes in this %g", 1.0, " float. %% %%");
            char buf[128];
            sprintf(buf, "%%No zeroes in this %g float. %% %%", 1.0);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, 2, " formats here: %x%g", 0xbeef, 0., (char)'.');
            sprintf(buf, "2 formats here: %x%g.", 0xbeef, 0.);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            GPStringBuffer(127) buf1;
            GPString str2 = gp_str_buffered(NULL, &buf1, "Capital S");
            gp_str_print(&str, "%S for GPString", str2);
            sprintf(buf, "%s for GPString", gp_cstr(str2));
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            GPStringBuffer(127) buf2;
            GPString str3 = gp_str_buffered(NULL, &buf2);
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
            GPStringBuffer(127) buf0;
            GPString str = gp_str_buffered(NULL, &buf0);
            char buf[128];

            // C23 compatible specifiers

            gp_str_print(&str, "Width 8 %w8u", (uint8_t)-1);
            sprintf(buf, "Width 8 %"PRIu8, (uint8_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, "Width 16 %w16u", (uint16_t)-1);
            sprintf(buf, "Width 16 %"PRIu16, (uint16_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, "Width 32 %w32u", (uint32_t)-1);
            sprintf(buf, "Width 32 %"PRIu32, (uint32_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            gp_str_print(&str, "Width 64 %w64u", (uint64_t)-1);
            sprintf(buf, "Width 64 %"PRIu64, (uint64_t)-1);
            gp_expect(gp_str_equal(str, buf, strlen(buf)));

            // Old ones kept for compatibility, it is recommended to use the
            // ones above.

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
            GPStringBuffer(127) buf0;
            GPString str = gp_str_buffered(NULL, &buf0);
            gp_str_print(&str, "%%blah%%");
            char buf[128];
            sprintf(buf, "%%blah%%");
            gp_expect(gp_str_equal(str, buf, strlen(buf)));
        }

        gp_test("Pointers");
        {
            GPStringBuffer(127) buf0;
            GPString str = gp_str_buffered(NULL, &buf0);
            char buf[128];
            uintptr_t _buf = (uintptr_t)buf; // shut up -Wrestrict
            gp_str_print(&str, (void*)buf);
            sprintf(buf, "%p", (void*)_buf);
            gp_expect(gp_str_equal(str, buf, strlen(buf)), str, buf);

            gp_str_print(&str, NULL);
            sprintf(buf, "(nil)");
            gp_expect(gp_str_equal(str, buf, strlen(buf)));
        }

        gp_test("Println");
        {
            GPStringBuffer(127) buf;
            GPString str = gp_str_buffered(NULL, &buf);
            gp_str_println(&str, "Spaces", 3, "inserted.");
            const char* cstr = "Spaces 3 inserted.\n";
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));

            cstr = "With 20 fmt specs.\n";
            gp_str_println(&str, "With %g%i", 2., 0, "fmt specs.");
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)), str);
        }
    } // gp_suite("String Print");
    #endif

    gp_suite("Trim");
    {
        GPStringBuffer(127) buf;

        gp_test("ASCII");
        {
            GPString str = gp_str_buffered(NULL, &buf, "  \t\f \nLeft Ascii  ");
            gp_str_trim(&str, NULL, 0, GP_ASCII_WHITESPACE, 'l');
            gp_expect(gp_str_equal(str, "Left Ascii  ", strlen("Left Ascii  ")));

            const char* cstr = " AA RightSAICASIACSIACIAS";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_str_trim(&str, NULL, 0, "ASCII", 'r');
            gp_expect(gp_str_equal(str, " AA Right", strlen(" AA Right")));

            cstr = "  __Left and Right__  ";
            gp_str_copy(&str, cstr, strlen(cstr));
            gp_str_trim(&str, NULL, 0, GP_ASCII_WHITESPACE "_", 'l'|'r');
            gp_expect(gp_str_equal(str, "Left and Right", strlen( "Left and Right")), str);

            int flags[] = { 'l', 'r', 'l'|'r' };
            for (size_t i = 0; i < sizeof flags/sizeof flags[0]; ++i) {
                cstr = "xxyyxxyxy";
                gp_str_copy(&str, cstr, strlen(cstr));
                gp_str_trim(&str, NULL, 0, "xy", flags[i]);
                gp_assert(gp_str_length(str) == 0);
            }

            cstr = "   Just space      ";
            gp_str_trim(&str, cstr, strlen(cstr), " ", 'l'|'r');
        }

        gp_test("UTF-8");
        {
            GPString str = gp_str_buffered(NULL, &buf, "¡¡¡Left!!!");
            gp_str_trim(&str, NULL, 0, "¡", 'l');
            gp_expect(gp_str_equal(str, "Left!!!", strlen("Left!!!")), str);

            gp_str_copy(&str, " Right\r\u200A\r\n", strlen(" Right\r\u200A\r\n"));
            gp_str_trim(&str, NULL, 0, GP_WHITESPACE, 'r');
            gp_expect(gp_str_equal(str, " Right", strlen(" Right")), str);

            gp_str_copy(&str, "\t\u3000 ¡¡Left and Right!! \u3000\n", strlen("\t\u3000 ¡¡Left and Right!! \u3000\n"));
            gp_str_trim(&str, NULL, 0, GP_WHITESPACE "¡!", 'l'|'r');
            gp_expect(gp_str_equal(str, "Left and Right", strlen("Left and Right")));

            int flags[] = { 'l', 'r', 'l'|'r' };
            for (size_t i = 0; i < sizeof flags/sizeof flags[0]; ++i) {
                const char* cstr = "xx😫yyxxy😫xy";
                gp_str_copy(&str, cstr, strlen(cstr));
                gp_str_trim(&str, NULL, 0, "xy😫", flags[i]);
                gp_assert(gp_str_length(str) == 0);
            }
        }

        gp_test("Invalid UTF-8");
        {
            // Use heap to put sanitizers to work
            const char* cstr = "\xF2 Too shorts \xF2";
            GPString str = gp_str_new_init(gp_global_heap, 0, cstr);
            gp_str_trim(&str, NULL, 0, "", 'l'|'r');
            // Didn't buffer overflow, but didn't really do anything either.
            gp_expect(gp_str_equal(str, cstr, strlen(cstr)));
            gp_str_delete(str);

            // Only invalids
            // - left:  continuation byte, overlong, and truncated codepoint
            // - right: truncated codepoint, bunch of continuation bytes, overlong, more continuation bytes
            // Detecting codepoints boundaries from right is a bit tricky, which
            // is why we do this in a loop.
            for (char cstr[] = "\x80\xC0\xAF\xF2 😁 \x80\x80\x80\x80\x80\xC0\xAF\x80\x80\x80\x80\x80\xF2";
                strlen(cstr) > strlen("\x80\xC0\xAF\xF2 😁 ");
                cstr[strlen(cstr)-1] = '\0')
            {
                GP_TRY_POISON_MEMORY_REGION(cstr + strlen(cstr) + 1, 8);
                str = gp_str_new_init(gp_global_heap, 0, cstr);
                gp_str_trim(&str, NULL, 0, "", 'l'|'r'|GP_TRIM_INVALID);
                // Note: empty character set instead of NULL to only trim invalid.
                gp_assert(gp_str_equal(str, " 😁 ", strlen(" 😁 ")));
                gp_str_delete(str);
            }

            // Mixed invalids and valids
            // Same ones as above but some random Unicode whitespace mixed in.
            for (char cstr[] = "\x80\u3000\xC0\xAF\n\xF2 😁\x80 \x80\n\x80\x80\x80\x80\xC0\xAF\x80\u3000\x80\x80\x80\x80\xF2";
                strlen(cstr) > strlen("\x80\u3000\xC0\xAF\n\xF2 😁");
                cstr[strlen(cstr)-1] = '\0')
            {
                GP_TRY_POISON_MEMORY_REGION(cstr + strlen(cstr) + 1, 8);
                str = gp_str_new_init(gp_global_heap, 0, cstr);
                gp_str_trim(&str, NULL, 0, GP_WHITESPACE, 'l'|'r' | GP_TRIM_INVALID);
                gp_assert(gp_str_equal(str, "😁", strlen("😁")));
                gp_str_delete(str);
            }

            int flags[] = { 'l', 'r', 'l'|'r' };
            for (size_t i = 0; i < sizeof flags/sizeof flags[0]; ++i) {
                const char* cstr = "x\xFFx😫yyx\x80xy\xC3😫xy\xF3";
                str = gp_str_new_init(gp_global_heap, 0, cstr);
                gp_str_trim(&str, NULL, 0, "xy😫", flags[i] | GP_TRIM_INVALID);
                gp_assert(gp_str_length(str) == 0);
                gp_str_delete(str);
            }
        }
    } // gp_suite("Trim");

    gp_suite("To upper/lower/title case");
    {
        GPStringBuffer(64) buf;
        GPString str = gp_str_buffered(NULL, &buf, "blörö");
        gp_test("Upper and lower");
        {
            gp_str_to_upper(&str);
            gp_expect(gp_str_equal(str, "BLÖRÖ", strlen("BLÖRÖ")), str);
            gp_str_to_lower(&str);
            gp_expect(gp_str_equal(str, "blörö", strlen("blörö")));

            // ASCII, invalid sequence, multi-byte, size changing codepoints, trunced
            const char* cstr_lower = "ascii\xff\xffääȿȿⱥⱥ\xf2";
            const char* cstr_upper = "ASCII\xff\xffÄÄⱾⱾȺȺ\xf2";
            gp_str_copy(&str, cstr_lower, strlen(cstr_lower));
            gp_str_to_upper(&str);
            gp_expect(gp_str_equal(str, cstr_upper, strlen(cstr_upper)));
            gp_str_to_lower(&str);
            gp_expect(gp_str_equal(str, cstr_lower, strlen(cstr_lower)));
        }

        gp_test("Title");
        {
            gp_str_copy(&str, "aäǉǈǇǱ", strlen("aäǉǈǇǱ"));
            gp_str_to_title(&str);
            gp_expect(gp_str_equal(str, "AÄǈǈǈǲ", strlen("AÄǈǈǈǲ")), str);
        }
    }

    gp_suite("Validate");
    {
        GPString str = gp_str_new(gp_global_heap, 32);

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
            const char* cleanedsequences[][3] = {
                { "_\x28",      "\x28",     "�\x28",},
                { "__",         "",         "��",},
                { "_\x28_",     "\x28",     "�\x28�",},
                { "__\x28",     "\x28",     "��\x28",},
                { "_\x28__",    "\x28",     "�\x28��",},
                { "__\x28_",    "\x28",     "��\x28�",},
                { "_\x28_\x28", "\x28\x28", "�\x28�\x28",},
                { "__",         "",         "��",},
                { "____",       "",         "����",},
                { "___",        "",         "���",},
                { "_____",      "",         "�����",},
                { "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35_", "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35", "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35�",},
                { "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35_", "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35", "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35�",},
                { "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35_", "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35", "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35�",},
                { "_\x7f", "\x7f", "�\x7f",},
                { "_", "", "�",},
                { "\xce\xba_", "\xce\xba", "\xce\xba�",},
                { "\xce\xba__", "\xce\xba", "\xce\xba��",},
                { "\xce\xba\xe1\xbd\xb9_", "\xce\xba\xe1\xbd\xb9", "\xce\xba\xe1\xbd\xb9�",},
                { "\xce\xba\xe1\xbd\xb9\xcf\x83_", "\xce\xba\xe1\xbd\xb9\xcf\x83", "\xce\xba\xe1\xbd\xb9\xcf\x83�",},
                { "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc_", "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc", "\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc�",},
                { "_",    "", "�",},
                { "__",   "", "��",},
                { "_",    "", "�",},
                { "____", "", "����",},
                { "\x6c\x2_\x18", "\x6c\x2\x18", "\x6c\x2�\x18",},
                { "\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x5\x29\x2c\x33\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x20\x1\x1\x1\x1\x1\x2\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x23\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x7e\x7e\xa\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1_\x1\x1\x1\x79\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1", "\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x5\x29\x2c\x33\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x20\x1\x1\x1\x1\x1\x2\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x23\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x7e\x7e\xa\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1\x1\x1\x1\x79\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1", "\x25\x5b\x6e\x2c\x32\x2c\x5b\x5b\x33\x2c\x34\x2c\x5\x29\x2c\x33\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x35\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x20\x1\x1\x1\x1\x1\x2\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x23\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x7e\x7e\xa\xa\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5d\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x2c\x37\x2e\x33\x2c\x39\x2e\x33\x2c\x37\x2e\x33\x2c\x39\x2e\x34\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x5d\x1\x1�\x1\x1\x1\x79\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1",},
                { "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b_\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x10\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1", "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x10\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1", "\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b�\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x10\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1",},
                { "\x20\xb\x1\x1\x1\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1_\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1", "\x20\xb\x1\x1\x1\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1", "\x20\xb\x1\x1\x1\x64\x3a\x64\x3a\x64\x3a\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x5b\x30\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1�\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1\x1"}};
            for (size_t i = 0; i < sizeof(badsequences)/sizeof(*badsequences); i++)
            {
                static const char* replacements[] = {"_", "", GP_REPLACEMENT_CHARACTER};
                for (size_t j = 0; j < sizeof replacements / sizeof*replacements; ++j)
                {
                    gp_str_copy(&str, badsequences[i], strlen(badsequences[i]));
                    gp_str_to_valid(&str, NULL, 0, replacements[j]);
                    gp_expect(gp_str_equal(str, cleanedsequences[i][j], strlen(cleanedsequences[i][j])), i, j);
                }
            }
            const char* cstr = "\xf2ö\x80\xf2";
            gp_str_to_valid(&str, cstr, strlen(cstr), "_");
            gp_expect(gp_str_equal(str, "_ö__", strlen("_ö__")));
        }
        gp_str_delete(str);
    }

    gp_suite("Read file");
    {
        gp_test("Reading and writing");
        {
            GPStringBuffer(35) buf;
            GPString str = gp_str_buffered(NULL, &buf, "blah blah blah");
            gp_assert(gp_str_file(&str, "gp_test_str_file.txt", "write") == 0);
            gp_str_copy(&str, "XXXX XXXX XXXX", strlen("XXXX XXXX XXXX")); // write junk to memory
            gp_str_set(str)->length = 0;
            gp_assert(gp_str_file(&str, "gp_test_str_file.txt", "read") == 0);
            gp_expect(gp_str_equal(str, "blah blah blah", strlen("blah blah blah")));
        }

        gp_test("Non existent");
        {
            GPStringBuffer(0) buf;
            GPString str = gp_str_buffered(gp_global_heap, &buf);
            // Only first char in mode is checked so "r" is fine too
            gp_expect(gp_str_file(&str, "NON_EXISTENT.txt", "r") != 0);
            gp_str_delete(str);
        }

        gp_expect(remove("gp_test_str_file.txt") == 0);
    }

    gp_suite("Truncating Strings");
    {
        // To test truncation at all possible points, we loop until no
        // truncation happened at all.
        size_t trunced_sum = -1;

        gp_test("Truncation");
        for (size_t capacity = 0; trunced_sum != 0; ++capacity)
        {
            trunced_sum = 0;
            size_t trunced;
            const char* cstr;
            // Heap allocated string with varying capacity is used for testing
            // purposes (heap to put sanitizers to work). More common way of
            // using truncating strings is a stack allocated GPStringBuffer.
            // gp_str_new() does some rounding, so we'll construct the string
            // manually for precise buffer sizes (not recommended normally).
            // Don't forget to add sizeof"" for implicit null terminator capacity!
            GPStringHeader* header = gp_mem_alloc(
                gp_global_heap, sizeof(GPStringHeader) + capacity + sizeof"");
            header->length     = 0;
            header->capacity   = capacity; // must not include implicit null terminator capacity!
            header->allocation = header;
            // A string withouth allocator cannot reallocate making it a
            // truncating string.
            header->allocator = NULL;
            GPString str = (GPString)(header + 1);

            cstr = "Stuff and words";
            trunced_sum += trunced = gp_str_copy(&str, cstr, strlen(cstr));
            gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)));
            gp_assert(trunced == (size_t)gp_imax(0, strlen(cstr) - capacity));

            const char* cstr1 = " with things";
            size_t old_length = gp_str_length(str);
            trunced_sum += trunced = gp_str_append(&str, cstr1, strlen(cstr1));
            cstr = "Stuff and words with things";
            gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)));
            gp_assert(trunced == (size_t)gp_imax(0, old_length + strlen(cstr1) - capacity));

            cstr1 = "ing";
            old_length = gp_str_length(str);
            trunced_sum += trunced = gp_str_insert(&str, strlen("Stuff"), cstr1, strlen(cstr1));
            cstr = "Stuffing and words with things";
            gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)));
            gp_assert(trunced == (size_t)gp_imax(0, old_length + strlen(cstr1) - capacity));

            const char* lowers = "ɀɀɀ";
            const char* uppers = "ⱿⱿⱿ";
            gp_assert(strlen(lowers) < strlen(uppers), "These codepoints expand when upcasing.");

            size_t trunced_lowers = gp_str_append(&str, lowers, strlen(lowers));
            trunced_sum += trunced = gp_str_to_upper(&str);
            cstr = "STUFFING AND WORDS WITH THINGSⱿⱿⱿ";
            if (capacity <= strlen("STUFFING AND WORDS WITH THINGS") || trunced_lowers % strlen("Ɀ") == 0)
                gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)), str, capacity, trunced);

            const char* needle = "and words with";
            const char* replacement = "with savory herbs and";
            cstr = "Stuffing with savory herbs and thingsⱿⱿⱿ";
            size_t needle_pos = gp_str_find_first(str, needle, strlen(needle), 0);
            if (needle_pos != GP_NOT_FOUND) {
                trunced_sum += trunced = gp_str_replace(
                    &str,
                    needle_pos,
                    strlen(needle),
                    replacement,
                    strlen(replacement));
                gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)));
                gp_assert(
                    trunced ==
                        (strlen(replacement) - strlen(needle)) - (capacity - gp_str_length(str)));
            }

            // IMPORTANT!
            // Truncating string may truncate a multi byte codepoint resulting
            // in invalid UTF-8. We cannot remove the invalid bytes
            // automatically, it could just corrupt the data further. Consider
            // appending two strings, first one starting with an emoji and the
            // second one starting with ASCII '/', to a string with only a
            // single byte left in capacity. If we were to automatically
            // truncate the now invalid first byte of the truncated emoji, then
            // the string would gain capacity for another byte so now the first
            // character of the second string '/' would get appended even when
            // it should've gotten truncated as well. We also cannot decrease
            // capacity either, this could cause the string to slowly lose
            // capacity, which has it's own problems like future bad allocation
            // size calculations. This means that any truncating strings must be
            // trunceated manually after ALL other processing and before using
            // as input to any Unicode sensitive processing.
            cstr = "😂😂😂😂";
            trunced_sum += trunced = gp_str_repeat(&str, 4, "😂", strlen("😂"));
            gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)));
            gp_assert(trunced == (size_t)gp_imax(0, strlen(cstr) - capacity));
            if (trunced & 3) // emoji partially trunced
                gp_assert( ! gp_str_is_valid(str, NULL));
            gp_assert(gp_str_equal(str, cstr, gp_min(strlen(cstr), capacity)));
            gp_assert(trunced == (size_t)gp_imax(0, strlen(cstr) - capacity));
            if (trunced & 3) {
                trunced = gp_str_truncate_invalid_tail(&str);
                gp_assert(trunced == (capacity & 3));
            }
            gp_assert(gp_str_is_valid(str, NULL));

            gp_assert(gp_str_capacity(str) == capacity, "Truncating string cannot grow.");
            gp_assert(gp_str_allocation(str) == header, "Truncating string cannot reallocate.");

            // A heap allocated string must be freed of course. However, the
            // string does not know how to deallocate itself without an
            // allocator. Here is two ways of deallocating a truncating string.
            if (capacity & 1) { // restore allocator and delete
                gp_str_set(str)->allocator = gp_global_heap;
                gp_str_delete(str);
            } else // manual deallocation
                gp_mem_dealloc(gp_global_heap, gp_str_allocation(str));
            // Note that truncating strings are usually stack or arena allocated
            // and wouldn't need deallocating.
        }
    }

    // ------------------------------------------------------------------------
    // Internal tests

    if (sizeof(wchar_t) == sizeof(uint32_t))
    {
        gp_assert(gp_set_utf8_global_locale(LC_ALL, ""));

        gp_suite("Unicode conversions");
        {
            #define BUF_LEN (1024)
            GPStringBuffer(BUF_LEN) buf0;
            GPString str = gp_str_buffered(NULL, &buf0, "");
            GPRandomState rs = gp_random_state((uintptr_t)str);
            for (size_t j = 0; j < BUF_LEN ; j += sizeof(uint32_t)) {
                uint32_t u = gp_random(&rs);
                memcpy(str + j, &u, sizeof u);
            }
            ((GPStringHeader*)str - 1)->length = BUF_LEN - 1;
            gp_str_to_valid(&str, NULL, 0, "");
            for (size_t pos; (pos = gp_str_find_first(str, "\0", 1, pos)) != GP_NOT_FOUND; )
                gp_str_replace(&str, pos, 1, "", 0);

            GPArena* scratch = gp_scratch_arena();
            wchar_t* std_buf = gp_mem_alloc((GPAllocator*)scratch, (gp_str_length(str) + sizeof"") * sizeof*std_buf);
            const char* src = gp_cstr(str);
            size_t std_buf_length = mbsrtowcs(std_buf, &src, gp_str_length(str) + sizeof"", &(mbstate_t){0});

            GPArray(uint32_t) gp_buf = gp_arr_new(sizeof(uint32_t), &scratch->base, gp_str_length(str));
            gp_utf8_to_utf32(&gp_buf, str, gp_str_length(str));

            if ( ! gp_expect(gp_bytes_equal(
                gp_buf,  gp_arr_length(gp_buf) * sizeof gp_buf[0],
                std_buf, std_buf_length * sizeof std_buf[0]),
                    gp_arr_length(gp_buf), std_buf_length, gp_str_length(str)))
            {
                for (size_t i = 0; i < gp_arr_length(gp_buf); i++)
                    if (gp_buf[i] != (uint32_t)std_buf[i])
                        gp_file_println(stderr, i, ",", (uint32_t)std_buf[i], ",", gp_buf[i]);
            }

            char std_chars[BUF_LEN * sizeof(wchar_t)];
            const wchar_t* pbuf = (const wchar_t*)std_buf;
            const size_t std_chars_length = wcsrtombs(std_chars,
                &pbuf, sizeof std_chars, &(mbstate_t){0});

            gp_utf32_to_utf8(&str, gp_buf, gp_arr_length(gp_buf));

            gp_expect(gp_str_equal(str, std_chars, std_chars_length));
        }
    }
}
