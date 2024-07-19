// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/unicode.c"
#include <gpc/assert.h>
#include <gpc/io.h>

// char16_t and char32_t only needed for tests. libGPC does not depend on them.
#if __STDC_VERSION__ >= 201112L
#include <uchar.h>
#endif

int main(void)
{
    // Tiny arena to put address sanitizer to work
    GPArena _arena = gp_arena_new(1);
    _arena.growth_coefficient = 0.0;
    GPAllocator* arena = (GPAllocator*)&_arena;

    gp_suite("Conversions");
    {
        #if __STDC_VERSION__ >= 201112L
        GPString utf8 = gp_str_on_stack(NULL, 32, "zß水🍌");
        GPArray(uint16_t) utf16 = gp_arr_new(arena, sizeof utf16[0], 32);

        gp_test("UTF-8 to UTF-16");
        {
            const char16_t wcs[] = u"zß水🍌";
            utf16 = gp_arr_copy(
                sizeof utf16[0], utf16, wcs, sizeof wcs / sizeof*wcs - sizeof"");
            gp_utf8_to_utf16(&utf16, utf8, gp_str_length(utf8));
            gp_expect(gp_arr_length(utf16) == sizeof wcs / sizeof*wcs - sizeof"",
                gp_arr_length(utf16), sizeof wcs / sizeof*wcs - sizeof"");
            if ( ! gp_expect(gp_bytes_equal(
                utf16, gp_arr_length(utf16) * sizeof utf16[0],
                wcs, sizeof wcs - sizeof u"")))
                for (size_t i = 0; i < gp_arr_length(utf16); i++)
                    gp_file_println(stderr, utf16[i], wcs[i]);
        }

        gp_test("UTF-16 to UTF-8");
        {
            GPString decoding = gp_str_on_stack(NULL, 32, "");
            gp_utf16_to_utf8(&decoding, utf16, gp_arr_length(utf16));
            gp_expect(gp_str_equal(utf8, decoding, gp_str_length(decoding)), utf8, decoding);
        }
        #endif

        // All other conversions work pretty much the same. They are tested
        // elsewhere.
    }

    gp_suite("String extensions");
    {
        const char* turkish    = "tr_TR";
        const char* lithuanian = "lt_LT";

        gp_test("To upper full Unicode mapping");
        {
            // \u0345 is iota subscript which gets capitalized to Ι. The
            // \u0307's are combining dots which are supposed to be combined
            // with omega.
            GPString str = gp_str_new(arena, 32, "ω\u0345\u0307\u0307");
            const char* result = "Ω\u0307\u0307Ι";
            gp_str_to_upper_full(&str, "");
            gp_expect(gp_str_equal(str, result, strlen(result)));

            const char* src = "i maße ﬃ ᾘ";
                     result = "I MASSE FFI ἨΙ";
            gp_str_copy(&str, src, strlen(src));
            gp_str_to_upper_full(&str, "");
            gp_expect(gp_str_equal(str, result, strlen(result)));

            // remove dot above after 'i'
            gp_str_copy(&str, "i\u0307blah", strlen("i\u0307blah"));
            gp_str_to_upper_full(&str, lithuanian);
            gp_expect(gp_str_equal(str, "IBLAH", strlen("IBLAH")), str);

            gp_str_copy(&str, "i", 1);
            gp_str_to_upper_full(&str, turkish);
            gp_expect(gp_str_equal(str, "İ", strlen("İ")));
        }

        gp_test("To lower full Unicode mapping");
        {
            GPString str = gp_str_new(arena, 128, "ὈΔΥΣΣΕΎΣ ὈΔΥΣΣΕΎΣ. ὈΔΥΣΣΕΎΣ3 ΣΣ\tΣ ΣΣ\u0301Σ\u0301");
            gp_str_to_lower_full(&str, "");
            const char* result = "ὀδυσσεύς ὀδυσσεύς. ὀδυσσεύς3 σς\tσ σσ\u0301ς\u0301";
            gp_expect(gp_str_equal(str, result, strlen(result)), str);

            // remove dot above after 'i'
            gp_str_copy(&str, "II\u0300Ì", strlen("II\u0300Ì"));
            gp_str_to_lower_full(&str, lithuanian);
            result = "ii\u0307\u0300i\u0307\u0300";
            gp_expect(gp_str_equal(str, result, strlen(result)), str);

            gp_str_copy(&str, "I", 1);
            gp_str_to_lower_full(&str, turkish);
            gp_expect(gp_str_equal(str, "ı", strlen("ı")));
        }

        gp_test("Capitalize");
        {
            GPString str = gp_str_new(arena, 64, "blah blah blah");
            gp_str_capitalize(&str, "");
            gp_expect(gp_str_equal(str, "Blah blah blah", strlen("Blah blah blah")));

            gp_str_copy(&str, "\u0345\u0307\u0307asdf", strlen("\u0345\u0307\u0307asdf"));
            const char* result = "\u0307\u0307Ιasdf";
            gp_str_capitalize(&str, "");
            gp_expect(gp_str_equal(str, result, strlen(result)));

            gp_str_copy(&str, "ǳ asdf", strlen("ǳ asdf"));
            gp_str_capitalize(&str, "");
            gp_expect(gp_str_equal(str, "ǲ asdf", strlen("ǲ asdf")));

            // remove dot above after 'i'
            gp_str_copy(&str, "i\u0307blah", strlen("i\u0307blah"));
            gp_str_capitalize(&str, lithuanian);
            gp_expect(gp_str_equal(str, "Iblah", strlen("Iblah")), str);

            gp_str_copy(&str, "iasdf", strlen("iasdf"));
            gp_str_capitalize(&str, turkish);
            gp_expect(gp_str_equal(str, "İasdf", strlen("İasdf")));
        }

        gp_test("Split and join");
        {
            GPString str = gp_str_new(arena, 64, "\t\tHello, I'm  the Prince!\r\n");
            GPArray(GPString) substrs = gp_str_split(gp_heap, str, gp_str_length(str), GP_WHITESPACE);
            gp_expect(gp_arr_length(substrs) == 4);
            gp_expect(gp_str_equal(substrs[0], "Hello,",  strlen("Hello,")));
            gp_expect(gp_str_equal(substrs[1], "I'm",     strlen("I'm")));
            gp_expect(gp_str_equal(substrs[2], "the",     strlen("the")));
            gp_expect(gp_str_equal(substrs[3], "Prince!", strlen("Prince!")));

            const char* trimmed = "Hello, I'm the Prince!";
            gp_str_join(&str, substrs, " ");
            gp_expect(gp_str_equal(str, trimmed, strlen(trimmed)));

            // Heap used in this example to show how to free if needed.
            for (size_t i = 0; i < gp_arr_length(substrs); ++i)
                gp_str_delete(substrs[i]);
            gp_arr_delete(substrs);

            // Test edge cases of not having leading or trailing whitespace.
            substrs = gp_str_split(arena, str, gp_str_length(str), GP_WHITESPACE);
            gp_expect(gp_arr_length(substrs) == 4);
            gp_expect(gp_str_equal(substrs[0], "Hello,",  strlen("Hello,")));
            gp_expect(gp_str_equal(substrs[1], "I'm",     strlen("I'm")));
            gp_expect(gp_str_equal(substrs[2], "the",     strlen("the")));
            gp_expect(gp_str_equal(substrs[3], "Prince!", strlen("Prince!")));
        }

        gp_test("Case insensitive but locale sensitive comparison");
        {
            GPString str1 = gp_str_on_stack(NULL, 64, "hrnec");
            GPString str2 = gp_str_on_stack(NULL, 64, "chrt");

            gp_test("Default locale");
            {
                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    GP_COLLATE,
                    "") > 0);

                // Lexicographic comparison of codepoints
                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    0,
                    "") > 0);
            }

            const char* czech = "cs_CZ";
            if (gp_locale(czech) != (GPLocale)0)
            { gp_test("Czech locale");
                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    GP_CASE_FOLD | GP_COLLATE,
                    czech) < 0);

                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    GP_COLLATE,
                    czech) < 0);

                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    0, // comparing codepoints, Czech locale does nothing here
                    czech) > 0);
            }

            gp_str_copy(&str1, "år",    strlen("år"));
            gp_str_copy(&str1, "Ängel", strlen("Ängel"));
            const char* american = "en_US";
            if (gp_locale(american) != (GPLocale)0)
            { gp_test("American locale å");
                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    GP_CASE_FOLD | GP_COLLATE,
                    american) < 0);
            }

            const char* swedish = "sv_SE";
            if (gp_locale(swedish) != (GPLocale)0)
            { gp_test("Swedish locale å");
                gp_expect(gp_str_compare(
                    str1, str2,
                    gp_str_length(str2),
                    GP_CASE_FOLD | GP_COLLATE,
                    swedish) > 0);
            }
        }

        gp_test("Sorting");
        {
            GPArray(GPString) strs = gp_arr_on_stack(NULL, 16, GPString,
                gp_str_on_stack(NULL, 16, "sbloink"),
                gp_str_on_stack(NULL, 16, "i"),
                gp_str_on_stack(NULL, 16, "asdf"),
                gp_str_on_stack(NULL, 16, "İ"),
                gp_str_on_stack(NULL, 16, "Sbloink"));

            // Lexicographic sorting
            gp_str_sort(&strs, 0, "");
            gp_expect(gp_str_equal(strs[0], "Sbloink", strlen("Sbloink")));
            gp_expect(gp_str_equal(strs[1], "asdf",    strlen("asdf")));
            gp_expect(gp_str_equal(strs[2], "i",       strlen("i")));
            gp_expect(gp_str_equal(strs[3], "sbloink", strlen("sbloink")));
            gp_expect(gp_str_equal(strs[4], "İ",       strlen("İ")));

            gp_str_sort(&strs, GP_REVERSE, "");
            gp_expect(gp_str_equal(strs[0], "İ",       strlen("İ")));
            gp_expect(gp_str_equal(strs[1], "sbloink", strlen("sbloink")));
            gp_expect(gp_str_equal(strs[2], "i",       strlen("i")));
            gp_expect(gp_str_equal(strs[3], "asdf",    strlen("asdf")));
            gp_expect(gp_str_equal(strs[4], "Sbloink", strlen("Sbloink")));

            if (gp_locale(turkish) == (GPLocale)0)
                goto skip_turkish_sort;

            // Exact ordering of equivalent strings depend on qsort() implementation.
            gp_str_sort(&strs, GP_CASE_FOLD | GP_COLLATE, turkish);
            gp_expect(gp_str_equal(strs[0], "asdf", strlen("asdf")));
            gp_expect(gp_str_equal(strs[1], "i",    strlen("i")) || gp_str_equal(strs[1], "İ", strlen("İ")));
            gp_expect(gp_str_equal(strs[1], "i",    strlen("i")) || gp_str_equal(strs[1], "İ", strlen("İ")));
            gp_expect(gp_str_equal_case(strs[3], "Sbloink", strlen("Sbloink")));
            gp_expect(gp_str_equal_case(strs[4], "sbloink", strlen("sbloink")));
            skip_turkish_sort: (void)0;
        }
    }

    gp_arena_delete(&_arena);
}
