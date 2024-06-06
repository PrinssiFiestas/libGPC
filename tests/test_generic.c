// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// WORK IN PROGRESS

#include "../src/generic.c"
#include <gpc/io.h>
#include <gpc/assert.h>
#include <errno.h>
#include <wchar.h>
#include <locale.h>

#if _WIN32
// quick hack to disable locale dependent tests for now
// TODO operating system agnostic way to set locale to utf-8
#define setlocale(...) NULL
#endif

#define arr_assert_eq(ARR, CARR, CARR_LENGTH) do { \
    typeof(ARR  )  _gp_arr1 = (ARR);  \
    typeof(*CARR)* _gp_arr2 = (CARR); \
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
    GPArena arena = gp_arena_new(0);
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

        gp_test("Codepoint classify");
        { // 1-4 bytes read. No bytes are read past string if string is valid
          // UTF-8.
            if (setlocale(LC_ALL, "C.utf8") != NULL)
            {
                const char* cstr = "x ";
                GPString str = gp_str(&arena, cstr);

                // Using index.
                gp_expect( ! gp_codepoint_classify(cstr,  0, iswspace));
                gp_expect(   gp_codepoint_classify(cstr,  1, iswspace));
                gp_expect( ! gp_codepoint_classify(str,   0, iswspace));
                gp_expect(   gp_codepoint_classify(str,   1, iswspace));

                // Using pointers. Useful with iterators.
                gp_expect( ! gp_codepoint_classify(cstr,     iswspace));
                gp_expect(   gp_codepoint_classify(cstr + 1, iswspace));
                gp_expect( ! gp_codepoint_classify(str,      iswspace));
                gp_expect(   gp_codepoint_classify(str  + 1, iswspace));
            }
        }
    }

    gp_suite("Strings");
    {
        GPString str1 = gp_str(&arena);
        GPString str2 = gp_str(&arena);
        GPString str3 = gp_str(&arena, ""); // same as the ones above
        char cstr[64] = "blah";
        gp_test("Repeat");
        {
            gp_repeat(&str1, 2, cstr, strlen(cstr)); // ok, length given
            gp_repeat(&str2, 3, "BLAH"); // ok, literal string
            gp_repeat(&str3, 4, str1);   // ok, GPString
            //gp_repeat(&str3, 5, cstr); // not ok, non-literal without length!

            // Same as above but by passing an allocator, a copy is made instead
            // of writing to an output string.
            GPString copy1 = gp_repeat(&arena, 2, "blah", strlen("blah"));
            GPString copy2 = gp_repeat(&arena, 3, "BLAH");
            GPString copy3 = gp_repeat(&arena, 4, str1);

            gp_expect(gp_equal(str1, "blahblah", strlen("blahblah")));
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
            GPString str2   = gp_replace(&arena, haystack, "blah", "shloiben", 1);
            GPString result = gp_replace(&arena, str2, "blah", "😂");
            gp_expect(gp_equal(result, "😂 YYyyy shloiben"));
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
            gp_trim(&str3, str); // for completeness
            gp_expect(gp_equal(str3, ""));
        }

        gp_test("To upper, lower, and valid");
        {
            GPString str0 = gp_str(&arena, "blah");
            GPString str1 = gp_to_upper(&arena, str0);
            gp_to_upper(&str0);
            gp_expect(gp_equal(str0, str1) && gp_equal(str0, "BLAH"));
            gp_to_lower(&str1);
            GPString str2 = gp_to_lower(&arena, str1);
            gp_expect(gp_equal(str1, str2) && gp_equal(str1, "blah"));

            gp_append(&str2, "\xff\xff\xff");
            GPString str3 = gp_to_valid(&arena, str2, GP_REPLACEMENT_CHARACTER);
            gp_to_valid(&str2, GP_REPLACEMENT_CHARACTER);
            gp_expect(gp_equal(str2, str3) && gp_equal(str2, "blah�"), str2);
        }
        // #define gp_find_first(...)
        // size_t gp_str_find_first(
        //     GPString    haystack,
        //     const void* needle,
        //     size_t      needle_size,
        //     size_t      start);
        // #define gp_find_last(...)
        // size_t gp_str_find_last(
        //     GPString    haystack,
        //     const void* needle,
        //     size_t      needle_size);
        // #define gp_find_first_of(...)
        // size_t gp_str_find_first_of(
        //     GPString    haystack,
        //     const char* utf8_char_set,
        //     size_t      start);
        // #define gp_find_first_not_of(...)
        // size_t gp_str_find_first_not_of(
        //     GPString    haystack,
        //     const char* utf8_char_set,
        //     size_t      start);
        // #define gp_equal_case(...)
        // bool gp_str_equal_case(
        //     GPString    s1,
        //     const void* s2,
        //     size_t      s2_size);
        // #define gp_codepoint_count(...)
        // size_t gp_str_codepoint_count(
        //     GPString str);
        // #define gp_is_valid(...)
        // bool gp_str_is_valid(
        //     GPString str,
        //     size_t*  optional_invalid_position);
        // #define gp_classify(...)
        // bool gp_char_classify(
        //     const void* str,
        //     int (*classifier)(wint_t c));
    }

    gp_suite("File");
    {
        const char* test_path = "gptestfile.txt";
        GPString str1 = gp_str(&arena, "contents");
        GPString str2 = gp_str(&arena);
        GPString str3 = gp_str(&arena);

        gp_file(str1,  test_path, "write");
        gp_file(&str2, test_path, "read");

        // + not necessary here, but demonstrates that it can be passed. Same
        // would be true for "binary" or just 'b'.
        FILE* f = gp_file(test_path, "read+");
        gp_file_read_line(&str3, f);
        gp_file_close(f);

        GPString str4 = gp_file(&arena, test_path, "read binary");

        gp_expect(gp_equal(str1, str2));
        gp_expect(gp_equal(str1, str3));
        gp_expect(gp_equal(str1, str4));

        remove(test_path);
    }
    gp_arena_delete(&arena);
}

void increment(void* out, const void* in) { *(int*)out = *(int*)in + 1; }
void* sum(void* y, const void* x) { *(int*)y += *(int*)x; return y; }
void* append(void* result, const void*_element)
{
    const char* element = *(const char**)_element;
    const size_t length = result && strlen(result);
    result = gp_mem_realloc(
        gp_last_scope(NULL), result, length, length + strlen(element) + sizeof" ");
    if (length == 0)
        ((char*)result)[0] = '\0';
    return strcat(strcat(result, element), " ");
}
bool even(const void* element) { return !(*(int*)element % 2); }
bool more_than_5(const void* element) { return *(int*)element > 5; }
