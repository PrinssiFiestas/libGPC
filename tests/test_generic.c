// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef __cplusplus
#include "../src/generic.c"
#else
// Build the library first using `make` then build and run this file with
// `
// g++ -Wall -Wextra -Iinclude -ggdb3 tests/test_generic.c -fsanitize=address -fsanitize=undefined -lm -lpthread -lasan build/libgpcd.so && ./a.out
// `
#include <gpc/generic.h>
#define gp_arr_ro(...) gp_arr(&arena, __VA_ARGS__)
#endif

#include <gpc/io.h>
#include <gpc/assert.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>

#define arr_assert_eq(ARR, CARR, CARR_LENGTH) do { \
    GP_TYPEOF(ARR  )  _gp_arr1 = (ARR);  \
    GP_TYPEOF(*CARR)* _gp_arr2 = (CARR); \
    const size_t _gp_arr2_length = CARR_LENGTH; \
    gp_expect(gp_arr_length(_gp_arr1) == _gp_arr2_length, \
        gp_arr_length(_gp_arr1), _gp_arr2_length); \
    for (size_t _gp_i = 0; _gp_i < _gp_arr2_length; _gp_i++) { \
        if ( ! gp_expect(_gp_arr1[_gp_i] == _gp_arr2[_gp_i], \
            _gp_arr1[_gp_i], _gp_arr2[_gp_i], _gp_i)) { \
            gp_print("arr1 = { "); \
            for (size_t _gp_j = 0; _gp_j < _gp_arr2_length; _gp_j++) \
                gp_print(_gp_arr1[_gp_j], ", "); \
            gp_print("}\narr2 = { "); \
            for (size_t _gp_j = 0; _gp_j < _gp_arr2_length; _gp_j++) \
                gp_print(_gp_arr2[_gp_j], ", "); \
            gp_println("}"); \
            break;\
        } \
    } \
} while(0)

#define CARR_LEN(CARR) (sizeof(CARR) / sizeof*(CARR))

int main(void)
{
    gp_assert(gp_default_locale() != NULL);

    // Tiny arena to put address sanitizer to work
    GPArena arena = gp_arena_new(1);
    arena.growth_coefficient = 0.0;

    gp_suite("Bytes and strings");
    {
        gp_test("Count");
        {
            GPString haystack = gp_str(&arena, "1 and 2 and 3");
            GPString needle   = gp_str(&arena, "and");
            gp_expect(gp_count(haystack, needle) == 2);
            gp_expect(gp_count(haystack, "and") == 2);
            gp_expect(gp_count(haystack, needle, gp_length(needle)) == 2);
            gp_expect(gp_count(haystack, gp_length(haystack), needle, gp_length(needle)) == 2);
        }
        gp_test("Codepoint length");
        { // aka how many bytes does a UTF-8 codepoint take. Only 1 byte read.
            const char* cstr = "x😂";
            GPString str = gp_str(&arena, cstr);

            // Using index.
            gp_expect(gp_codepoint_length(cstr,  0)  == 1);
            gp_expect(gp_codepoint_length(cstr,  1)  == 4);
            gp_expect(gp_codepoint_length(str,   0)  == 1);
            gp_expect(gp_codepoint_length(str,   1)  == 4);

            // Using pointers. Useful with iterators.
            gp_expect(gp_codepoint_length(cstr)      == 1);
            gp_expect(gp_codepoint_length(cstr + 1)  == 4);
            gp_expect(gp_codepoint_length(str)       == 1);
            gp_expect(gp_codepoint_length(str  + 1)  == 4);
        }
    }

    gp_suite("Strings");
    {
        GPString str1 = gp_str(&arena);
        GPString str2 = gp_str(&arena);
        GPString str3 = gp_str(&arena, ""); // same as the ones above
        gp_test("Repeat");
        {
            gp_repeat(&str1, 2, "blah"); // ok, literla string
            gp_repeat(&str2, 3, "BLAH"); // ok, literal string
            gp_repeat(&str3, 4, str1);   // ok, GPString
            //gp_repeat(&str3, 5, cstr); // not ok in C99, non-literal

            // Same as above but by passing an allocator, a copy is made instead
            // of writing to an output string.
            GPString copy1 = gp_repeat(&arena, 2, "blah");
            GPString copy2 = gp_repeat(&arena, 3, "BLAH");
            GPString copy3 = gp_repeat(&arena, 4, str1);

            gp_expect(gp_equal(str1, "blahblah"));
            gp_expect(gp_equal(str2, "BLAHBLAHBLAH"));
            gp_expect(gp_equal(copy1, str1));
            gp_expect(gp_equal(copy2, str2));
            gp_expect(gp_equal(copy3, str3));
        }

        gp_test("Replace");
        {
            // This one does not take lengths. Only GPString and literals are
            // allowed.
            GPString haystack = gp_str(&arena, "blah skiibel blah");
            GPString needle   = gp_str(&arena, "BLAHH");
            gp_replace(&haystack, "skiibel", "BLAHH");
            gp_replace(&haystack, needle, "XX");
            gp_replace(&haystack, "XX", gp_str(&arena, "YYYY"));
            gp_replace(&haystack, "YY", "yyy", 7);
            GPString str2    = gp_replace(&arena, haystack, "blah", "shloiben", 1);
            GPString result  = gp_replace(&arena, str2, "blah", "😂");
            GPString result2 = gp_replace(&arena, "BLAHH", needle, "blah");
            gp_expect(gp_equal(result,  "😂 YYyyy shloiben"));
            gp_expect(gp_equal(result2, "blah"));
        }

        gp_test("Replace all");
        {
            // This one does not take lengths. Only GPString and literals are
            // allowed.
            GPString haystack = gp_str(&arena, "blah skiibel skiibel blah");
            GPString needle   = gp_str(&arena, "BLAHH");
            gp_replace_all(&haystack, "skiibel", "BLAHH");
            gp_expect(gp_equal(haystack, "blah BLAHH BLAHH blah"));
            gp_replace_all(&haystack, needle, "XX");
            gp_expect(gp_equal(haystack, "blah XX XX blah"));
            gp_replace_all(&haystack, "XX", gp_str(&arena, "YYYY"));
            gp_expect(gp_equal(haystack, "blah YYYY YYYY blah"));
            gp_replace_all(&haystack, "YY", "yyy");
            gp_expect(gp_equal(haystack, "blah yyyyyy yyyyyy blah"));
            GPString result = gp_replace_all(&arena, haystack, "blah", "😂");
            gp_expect(gp_equal(result, "😂 yyyyyy yyyyyy 😂"), result);
        }

        gp_test("Trim");
        {
            GPString str = gp_str(&arena, "\t XYX  asdfg\r  YYX  \n");
            gp_trim(&str);
            gp_expect(gp_equal(str, "XYX  asdfg\r  YYX"));
            gp_trim(&str, "XY");
            gp_expect(gp_equal(str, "  asdfg\r  "));
            GPString str1 = gp_trim(&arena, str);
            gp_expect(gp_equal(str1, "asdfg"));
            gp_trim(&str1, "ag", 'l');
            gp_expect(gp_equal(str1, "sdfg"));
            gp_trim(&str1, "ag", 'r');
            gp_expect(gp_equal(str1, "sdf"));
            GPString str2 = gp_trim(&arena, str1, "f");
            gp_expect(gp_equal(str2, "sd"));
            GPString str3 = gp_trim(&arena, str2, "s", 'l');
            gp_expect(gp_equal(str3, "d"));
            GPString str4 = gp_trim(&arena, "asdf", gp_cstr(str)); // for completeness
            gp_expect(gp_equal(str4, ""));
        }

        GPLocale* turkish = gp_locale_new("tr_TR");

        gp_test("To upper, lower, and valid");
        {
            GPString str0 = gp_str(&arena, "blah");
            GPString str1 = gp_to_upper(&arena, str0);
            gp_to_upper(&str0);
            gp_expect(gp_equal(str0, str1) && gp_equal(str0, "BLAH"));
            gp_to_lower(&str1);
            GPString str2 = gp_to_lower(&arena, str1);
            gp_expect(gp_equal(str1, str2) && gp_equal(str1, "blah"));

            // Pass locale for full language sensitive case mapping.
            gp_copy(&str0, "ﬁre!🔥");
            gp_to_upper(&str0, gp_default_locale());
            gp_expect(gp_equal(str0, "FIRE!🔥"));
            if (turkish != NULL)
            {
                gp_copy(&str0, "iıİI");
                GPString str3 = gp_to_upper(&arena, str0, turkish);
                gp_expect(gp_equal(str3, "İIİI"), str3);
                gp_to_lower(&str3, turkish);
                gp_expect(gp_equal(str3, "iıiı"));
                GPString str4 = gp_to_lower(&arena, str0, turkish);
                gp_expect(gp_equal(str4, "iıiı"));
            }

            gp_append(&str2, "\xff\xff\xff");
            GPString str5 = gp_to_valid(&arena, str2, GP_REPLACEMENT_CHARACTER);
            gp_to_valid(&str2, GP_REPLACEMENT_CHARACTER);
            gp_expect(gp_equal(str2, str5) && gp_equal(str2, "blah���"), str2);
        }

        gp_test("Capitalize");
        {
            GPString str1 = gp_str(&arena, "ﬁre!🔥");
            GPString str2 = gp_str(&arena, "iasdf");
            gp_capitalize(&str1);
            gp_capitalize(&str2, turkish);
            gp_expect(gp_equal(str1, "Fire!🔥"));
            gp_expect(gp_equal(str2, "İasdf"));
        }

        gp_locale_delete(turkish);

        gp_test("Find first");
        {
            GPString haystack = gp_str(&arena, "yeah blah nope blah yeah");
            gp_expect(gp_find_first(haystack, "blah")    ==  5);
            gp_expect(gp_find_first(haystack, "blah", 6) == 15);
        }

        gp_test("Find last");
        {
            GPString haystack = gp_str(&arena, "yeah blah nope blah yeah");
            gp_expect(gp_find_last(haystack, "blah")                 == 15);
        }

        gp_test("Find first of");
        {
            GPString haystack = gp_str(&arena, "yeah blah nope blah yeah");
            gp_expect(gp_find_first_of(haystack, "blah")    == 2);
            gp_expect(gp_find_first_of(haystack, "blah", 6) == 6);
        }

        gp_test("Find first not of");
        {
            GPString haystack = gp_str(&arena, "yeah blah nope blah yeah");
            gp_expect(gp_find_first_not_of(haystack, "haey")    == 4);
            gp_expect(gp_find_first_not_of(haystack, "hlab", 6) == 9);
        }

        gp_test("Equal case");
        {
            GPString a = gp_str(&arena, "😂aAaAäÄä😂");
            GPString b = gp_copy(&arena, a);
            gp_expect(gp_equal_case(a, b));
            gp_expect(gp_equal_case(a, "😂aAaAäÄä😂"));
        }

        gp_test("Compare");
        {
            GPString str = gp_str(&arena, "chrt");
            GPLocale* czech = gp_locale_new("cs_CZ");

            gp_expect(gp_compare(str, "hrnec") < 0);
            gp_expect(gp_compare(str, "HRNEC") > 0);
            gp_expect(gp_compare(str, "HRNEC", GP_CASE_FOLD) < 0);
            if (czech != NULL)
                gp_expect(gp_compare(str, "hrnec", GP_COLLATE, czech) > 0);

            gp_expect(gp_compare(str, gp_str(&arena, "hrnec")) < 0);
            gp_expect(gp_compare(str, gp_str(&arena, "HRNEC")) > 0);
            gp_expect(gp_compare(str, gp_str(&arena, "HRNEC"), GP_CASE_FOLD) < 0);
            if (czech != NULL)
                gp_expect(gp_compare(str, gp_str(&arena, "hrnec"), GP_COLLATE, czech) > 0);

            gp_locale_delete(czech);
        }

        gp_test("Codepoint count");
        {
            GPString str = gp_str(&arena, "😂aÄ😂");
            gp_expect(gp_codepoint_count(str) == 4);
            gp_expect(gp_codepoint_count("😂aÄ😂") == 4);
        }

        gp_test("Is valid");
        {
            GPString str = gp_str(&arena, "😂aÄ😂");
            size_t invalid_index;
            gp_expect(   gp_is_valid(str));
            gp_expect(   gp_is_valid("😂aÄ😂"));
            gp_copy(&str, "😂a\xffÄ😂");
            gp_expect( ! gp_is_valid(str));
            gp_expect( ! gp_is_valid("😂a\xffÄ😂"));
            gp_expect( ! gp_is_valid(str, &invalid_index));
            gp_expect(invalid_index == 5);
        }
    }

    gp_suite("Arrays and strings");
    {
        gp_test("Split and join");
        {
            GPString str1 = gp_str(&arena, "blah blah blah");
            GPArray(GPString) arr1 = gp_split(&arena, str1, " ");
            GPArray(GPString) arr2 = gp_split(&arena, "BLAH BLAH BLAH", " ");
            gp_join(&str1, arr1, "_");
            gp_expect(gp_equal(str1, "blah_blah_blah"));
            GPString str2 = gp_join(&arena, arr2, "|");
            gp_expect(gp_equal(str2, "BLAH|BLAH|BLAH"));
        }

        gp_test("Sort");
        {
            // No separator for gp_split() defaults to GP_WHITESPACE
            GPArray(GPString) arr = gp_split(&arena, "asdf ÄLÄSDEE BLOINK öö");
            gp_sort(&arr);
            gp_expect(gp_equal(gp_join(&arena, arr), "BLOINKasdfÄLÄSDEEöö"));
            gp_sort(&arr, GP_CASE_FOLD);
            gp_expect(gp_equal(gp_join(&arena, arr), "asdfBLOINKÄLÄSDEEöö"));
            GPLocale* finnish = gp_locale_new("fi_FI");
            if (finnish != NULL) {
                gp_sort(&arr, GP_COLLATE | GP_CASE_FOLD, finnish);
                gp_expect(gp_equal(gp_join(&arena, arr), "asdfBLOINKÄLÄSDEEöö"), gp_join(&arena, arr));
                gp_locale_delete(finnish);
            }
        }

        gp_test("Copy");
        {
            GPString str1 = gp_str(&arena);
            gp_copy(&str1, "blah");
            gp_expect(gp_equal(str1, "blah"));
            GPString str2 = gp_copy(&arena, "BLAH");
            gp_expect(gp_equal(str2, "BLAH"));
            gp_copy(&str1, str2);
            gp_expect(gp_equal(str1, "BLAH"));
            GPString str3 = gp_copy(&arena, str1);
            gp_expect(gp_equal(str3, "BLAH"));
            gp_copy(&str3, "XXX", 3);
            gp_expect(gp_equal(str3, "XXX"));
            GPString str4 = gp_copy(&arena, str3, gp_length(str3));
            gp_expect(gp_equal(str4, "XXX"));

            GPArray(int) arr1 = gp_arr(&arena, int);
            gp_copy(&arr1, gp_arr_ro(int, 1, 2, 3, 4));
            arr_assert_eq(arr1, gp_arr_ro(int, 1, 2, 3, 4), 4);
            GPArray(int) arr2 = gp_copy(&arena, gp_arr_ro(int, 3, 2, 1));
            arr_assert_eq(arr2, gp_arr_ro(int, 3, 2, 1), 3);
            gp_copy(&arr1, arr2);
            arr_assert_eq(arr1, gp_arr_ro(int, 3, 2, 1), 3);
            GPArray(int) arr3 = gp_copy(&arena, arr1);
            arr_assert_eq(arr3, gp_arr_ro(int, 3, 2, 1), 3);
            int carr[] = { 9, 8, 7, 6, 5 };
            gp_copy(&arr3, carr, 5);
            arr_assert_eq(arr3, carr, 5);
            GPArray(int) arr4 = gp_copy(&arena, arr3, gp_length(arr3));
            arr_assert_eq(arr4, carr, 5);

            #if TYPE_CHECK
            gp_copy(&str1, arr1); // not okay, wrong type
            gp_copy(str1, "still not okay, taking str1 by value!");
            gp_copy(&arr1, str1); // not okay, wrong type
            gp_copy(arr1, arr2); // not okay, arr1 by value!
            #endif
        }

        gp_test("Slice");
        {
            GPString str1 = gp_str(&arena);
            gp_slice(&str1, "XXblahYY", 1, 7);
            gp_expect(gp_equal(str1, "XblahY"));
            gp_slice(&str1, 1, 5);
            gp_expect(gp_equal(str1, "blah"));
            GPString str2 = gp_slice(&arena, str1, 1, 3);
            gp_expect(gp_equal(str2, "la"), str2);

            GPArray(int) arr1 = gp_arr(&arena, int);
            gp_slice(&arr1, gp_arr_ro(int, 1, 2, 3, 4, 5, 6, 7, 8), 1, 7);
            arr_assert_eq(arr1, gp_arr_ro(int, 2, 3, 4, 5, 6, 7), 6);
            gp_slice(&arr1, 1, 5);
            arr_assert_eq(arr1, gp_arr_ro(int, 3, 4, 5, 6), 4);
            GPArray(int) arr2 = gp_slice(&arena, arr1, 1, 3);
            arr_assert_eq(arr2, gp_arr_ro(int, 4, 5), 2);

            #if TYPE_CHECK
            gp_slice(&str1, arr1, 1, 2); // not okay, wrong type
            gp_slice(str1, "still not okay, taking str1 by value!", 1, 2);
            gp_slice(&arr1, str1, 1, 2); // not okay, wrong type
            gp_slice(arr1, arr2, 1, 2); // not okay, arr1 by value!
            #endif
        }

        gp_test("Append");
        {
            GPString str1 = gp_str(&arena);
            gp_append(&str1, "ab");
            gp_expect(gp_equal(str1, "ab"));
            gp_append(&str1, "cd");
            gp_expect(gp_equal(str1, "abcd"));
            gp_append(&str1, "efg", 3);
            gp_expect(gp_equal(str1, "abcdefg"), str1);
            GPString str2 = gp_append(&arena, str1, "h");
            gp_expect(gp_equal(str2, "abcdefgh"));
            GPString str3 = gp_append(&arena, str1, str2);
            gp_expect(gp_equal(str3, "abcdefgabcdefgh"));
            GPString str4 = gp_append(&arena, str1, "h", 1);
            gp_expect(gp_equal(str4, str2));

            GPArray(int) arr1 = gp_arr(&arena, int);
            gp_append(&arr1, gp_arr_ro(int, 1));
            arr_assert_eq(arr1, gp_arr_ro(int, 1), 1);
            gp_append(&arr1, gp_arr_ro(int, 2));
            arr_assert_eq(arr1, gp_arr_ro(int, 1, 2), 2);
            gp_append(&arr1, gp_arr_ro(int, 3, 4, 5 ), 3);
            arr_assert_eq(arr1, gp_arr_ro(int, 1, 2, 3, 4, 5), 5);
            GPArray(int) arr2 = gp_append(&arena, arr1, gp_arr_ro(int, 6));
            arr_assert_eq(arr2, gp_arr_ro(int, 1, 2, 3, 4, 5, 6 ), 6);
            GPArray(int) arr3 = gp_append(&arena, arr1, arr2);
            arr_assert_eq(arr3, gp_arr_ro(int, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6), 11);
            GPArray(int) arr4 = gp_append(&arena, arr1, gp_arr_ro(int, 6), 1);
            arr_assert_eq(arr4, arr2, gp_length(arr2));
            GPArray(int) arr5 = gp_append(&arena, arr1, gp_length(arr1), arr2, gp_length(arr2));
            arr_assert_eq(arr5, gp_arr_ro(int, 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6), 11);
        }

        gp_test("Insert");
        {
            GPString str1 = gp_str(&arena);
            gp_insert(&str1, 0, "ab");
            gp_expect(gp_equal(str1, "ab"));
            gp_insert(&str1, 0, "cd");
            gp_expect(gp_equal(str1, "cdab"));
            gp_insert(&str1, 0, "efg", 3);
            gp_expect(gp_equal(str1, "efgcdab"), str1);
            GPString str2 = gp_insert(&arena, 0, str1, "h");
            gp_expect(gp_equal(str2, "hefgcdab"));
            GPString str3 = gp_insert(&arena, 0, str1, str2);
            gp_expect(gp_equal(str3, "hefgcdabefgcdab"));
            GPString str4 = gp_insert(&arena, 0, str1, "h", 1);
            gp_expect(gp_equal(str4, str2));

            GPArray(int) arr1 = gp_arr(&arena, int);
            gp_insert(&arr1, 0, gp_arr_ro(int, 1));
            arr_assert_eq(arr1, gp_arr_ro(int, 1), 1);
            gp_insert(&arr1, 0, gp_arr_ro(int, 2));
            arr_assert_eq(arr1, gp_arr_ro(int, 2, 1 ), 2);
            gp_insert(&arr1, 0, gp_arr_ro(int, 3, 4, 5 ), 3);
            arr_assert_eq(arr1, gp_arr_ro(int, 3, 4, 5, 2, 1 ), 5);
            GPArray(int) arr2 = gp_insert(&arena, 0, arr1, gp_arr_ro(int, 6));
            arr_assert_eq(arr2, gp_arr_ro(int, 6, 3, 4, 5, 2, 1), 6);
            GPArray(int) arr3 = gp_insert(&arena, 0, arr1, arr2);
            arr_assert_eq(arr3, gp_arr_ro(int, 6, 3, 4, 5, 2, 1, 3, 4, 5, 2, 1), 11);
            GPArray(int) arr4 = gp_insert(&arena, 0, arr1, gp_arr_ro(int, 6), 1);
            arr_assert_eq(arr4, arr2, gp_length(arr2));
        }
    }

    gp_suite("Array");
    { // Definitions for helper functions are below main()
        GPAllocator* scope = gp_begin(0);
        gp_test("Push and pop");
        {
            GPArray(int) arr = gp_arr(scope, int, 1, 2, 3);
            gp_push(&arr, 4);
            arr_assert_eq(arr, gp_arr_ro(int, 1, 2, 3, 4), 4);
            int i = gp_pop(&arr);
            arr_assert_eq(arr, gp_arr_ro(int, 1, 2, 3), 3);
            gp_expect(i == 4, i);

            gp_erase(&arr, 1);
            arr_assert_eq(arr, gp_arr_ro(int, 1, 3), 2);
            gp_erase(&arr, 0, 2);
            gp_expect(gp_length(arr) == 0);
        }

        gp_test("Map");
        {
            // Arguments do not need to be of type void* as long as they are
            // pointers and in is const. Return must be void.
            void increment(int* out, const int* in);
            GPArray(int) arr1 = gp_arr(scope, int, 1, 2, 3, 4);
            gp_map(&arr1, increment);
            arr_assert_eq(arr1, gp_arr_ro(int, 2, 3, 4, 5), 4);
            GPArray(int) arr2 = gp_map(scope, arr1, increment);
            arr_assert_eq(arr2, gp_arr_ro(int, 3, 4, 5, 6), 4);
            gp_map(&arr1, arr2, increment);
            arr_assert_eq(arr1, gp_arr_ro(int, 4, 5, 6, 7), 4);
            GPArray(int) arr3 = gp_map(scope, gp_arr_ro(int,  1, 1, 1), increment);
            arr_assert_eq(arr3, gp_arr_ro(int, 2, 2, 2), 3);
            int carr[] = { 9, 9, 9, 9, 9 };
            GPArray(int) arr4 = gp_map(scope, carr, sizeof carr / sizeof*carr, increment);
            arr_assert_eq(arr4, gp_arr_ro(int, 10, 10, 10, 10, 10), 5);

            #if TYPE_CHECK
            GPArray(char*) chars = gp_arr(scope, char*, "asdf", "fdsa");
            gp_map(&chars, arr1, increment); // wrong type
            gp_map(arr1, arr2, increment);   // wrong, arr1 passed by value
            gp_map(&chars, increment);       // incompatible function pointer
            #endif
        }

        gp_test("Fold");
        {
            // The return value and accumulator y MUST be a pointer or a pointer
            // sized integer and in must be a const pointer.
            intptr_t sum(intptr_t accumulator, const int* in);
            gp_expect(gp_fold(gp_arr_ro(int, 1, 2, 3, 4, 5), 0, sum) == 15);

            GPArray(const char*) cstrs = gp_arr(scope, const char*, "one", "two", "three");
            char* append(char* result, const char**_element);
            #ifndef __cplusplus
            char* result = gp_foldr(cstrs, NULL, append);
            #else // cast required
            char* result = gp_foldr(cstrs, (char*)nullptr, append);
            #endif
            gp_expect(gp_bytes_equal(result, strlen(result), "three two one ", strlen("three two one ")));

            #if TYPE_CHECK // incompatible pointers
            gp_fold(cstrs, 0, sum);
            gp_fold(gp_arr_ro(int, 1, 2), NULL, append);
            #endif
        }

        gp_test("Filter");
        {
            // Argument must be a const pointer, return value must be bool.
            bool not_divisible_by_3(const int* element);
            bool even(const int* element);
            bool not_4(const int* element);
            bool more_than_3(const int* element);
            bool less_than_7(const int* element);
            GPArray(int) arr1 = gp_arr(scope,
                int, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
            gp_filter(&arr1, not_divisible_by_3);
            arr_assert_eq(arr1, gp_arr_ro(int, 1, 2, 4, 5, 7, 8, 10, 11, 13, 14, 16), 11);
            GPArray(int) arr2 = gp_filter(scope, arr1, even);
            arr_assert_eq(arr2, gp_arr_ro(int, 2, 4, 8, 10, 14, 16), 6);
            gp_filter(&arr1, arr2, not_4);
            arr_assert_eq(arr1, gp_arr_ro(int, 2, 8, 10, 14, 16), 5);
            GPArray(int) arr3 = gp_filter(scope, gp_arr_ro(int,  2, 3, 4 ,5), more_than_3);
            arr_assert_eq(arr3, gp_arr_ro(int, 4, 5), 2);
            int carr[] = { 5, 6, 7, 8, 9 };
            GPArray(int) arr4 = gp_filter(scope, carr, sizeof carr / sizeof*carr, less_than_7);
            arr_assert_eq(arr4, gp_arr_ro(int, 5, 6), 2);

            #if TYPE_CHECK
            GPArray(const char*const) cstrs = gp_arr_ro(const char*, "blah", "blah");
            gp_filter(&cstrs, even); // wrong, passing const char** to const int*
            gp_filter(arr1, even);   // wrong, passing by value
            gp_filter(&cstrs, arr1, even); // wrong destination type
            #endif
        }
        gp_end(scope);
    }

    gp_suite("Dictionarys");
    {
        gp_test("Functionality");
        {
            GPDictionary(int) dict = gp_dict(&arena, int);
            GPString key1 = gp_str(&arena, "key1");
            gp_put(&dict, key1,   1);
            gp_put(&dict, "key2", 3);
            #if defined(GP_TYPEOF) || __cplusplus
            gp_expect(*gp_get(dict, key1)   == 1);
            gp_expect(*gp_get(dict, "key2") == 3);
            #else // cast required for dereferencing the return value
            gp_expect(*(int*)gp_get(dict, key1)   == 1);
            gp_expect(*(int*)gp_get(dict, "key2") == 3);
            #endif
            gp_expect(gp_remove(&dict, key1));
            gp_expect(gp_remove(&dict, "key2"));
            gp_expect(gp_get(dict, key1)   == NULL);
            gp_expect(gp_get(dict, "key2") == NULL);

            #if TYPE_CHECK
            gp_put(&dict, key1, "blah"); // assign char[] to int
            #endif
        }

        gp_test("Constructors");
        {
            void int_destructor(int*_);

            // This is incredibly wasteful! We just test if compiles though.
            GPHashMap* hmap = gp_hmap(&arena);
            hmap = gp_hmap(&arena, sizeof(int));
            hmap = gp_hmap(&arena, sizeof(int), int_destructor);
            hmap = gp_hmap(&arena, sizeof(int), int_destructor, 128);
            (void)hmap;

            // Same here, don't write code like this!
            GPDictionary(int) dict = gp_dict(&arena, int);
            dict = gp_dict(&arena, int, int_destructor);
            dict = gp_dict(&arena, int, int_destructor, 128);
            (void)dict;

            #if TYPE_CHECK
            // Incompatible destructor argument type.
            GPDictionary(GPString) strs = gp_dict(&arena, GPString, int_destructor);
            // Still wrong, destructors take pointers to objects.
            strs = gp_dict(&arena, GPString, gp_str_delete);
            strs = gp_dict(&arena, GPString, gp_str_ptr_delete); // ok
            #endif
        }
    }

    gp_suite("Allocators");
    {
        gp_test("Basics");
        {
            void* pheap   = gp_alloc(gp_heap, 1);
            void* parena  = gp_alloc(&arena, 1);
            void* pzeroes = gp_alloc_zeroes(&arena, 1);
            pzeroes = gp_realloc(&arena, pzeroes, 1, 2);
            gp_dealloc(gp_heap, pheap);
            gp_dealloc(&arena,  parena);
            gp_dealloc(&arena,  pzeroes);
        }

        gp_test("Types");
        {
            #ifndef __cplusplus
            int* pint     = gp_alloc_type(&arena, int);
            int* parr     = gp_alloc_type(&arena, int[4]);
            int* pcnt     = gp_alloc_type(&arena, int, 4);
            int* pname    = gp_alloc_type(&arena, pname);
            int* pnamearr = gp_alloc_type(&arena, pnamearr, 4);
            #else // casts required
            int* pint     = (int*)gp_alloc_type(&arena, int);
            int* parr     = (int*)gp_alloc_type(&arena, int[4]);
            int* pcnt     = (int*)gp_alloc_type(&arena, int, 4);
            int* pname    = (int*)gp_alloc_type(&arena, pname);
            int* pnamearr = (int*)gp_alloc_type(&arena, pnamearr, 4);
            #endif

            gp_dealloc(&arena, pint    );
            gp_dealloc(&arena, parr    );
            gp_dealloc(&arena, pcnt    );
            gp_dealloc(&arena, pname   );
            gp_dealloc(&arena, pnamearr);
        }
    }

    gp_suite("File");
    {
        const char* test_path = "gptestfile.txt";
        GPString str1 = gp_str(&arena, "contents");
        GPString str2 = gp_str(&arena);
        GPString str3 = gp_str(&arena);

        // Default mode is binary mode. Add "text" or 'x' if you want Windows to
        // do unnecessary text processing.
        gp_file(str1,  test_path, "write");
        gp_file(&str2, test_path, "read");

        // + not necessary here, but demonstrates that it can be passed. Same
        // would be true for "text" or just 't'.
        FILE* f = gp_file(test_path, "read+");
        gp_file_read_line(&str3, f);
        gp_file_close(f);

        GPString str4 = gp_file(&arena, test_path, "read");

        gp_expect(gp_equal(str1, str2));
        gp_expect(gp_equal(str1, str3));
        gp_expect(gp_equal(str1, str4));

        remove(test_path);
    }

    gp_arena_delete(&arena);
}

void int_destructor(int*_) { (void)_; }

void increment(int* out, const int* in) { *out = *in + 1; }
intptr_t sum(intptr_t y, const int* x)  { return y + *x; }
char* append(char* result, const char**_element)
{
    const char* element = *_element;
    const size_t length = result && strlen(result);
    result = (char*)gp_mem_realloc(
        gp_last_scope(NULL), result, length, length + strlen(element) + sizeof" ");
    if (length == 0)
        ((char*)result)[0] = '\0';
    return strcat(strcat(result, element), " ");
}
bool even(const int* element)               { return !(*element  % 2); }
bool more_than_3(const int* element)        { return   *element  > 3 ; }
bool less_than_7(const int* element)        { return   *element  < 7 ; }
bool not_4(const int* element)              { return   *element != 4 ; }
bool not_divisible_by_3(const int* element) { return   *element  % 3 ; }

