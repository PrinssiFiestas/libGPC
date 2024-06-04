// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/generic.c"
#include <gpc/io.h>
#include <gpc/assert.h>

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
    gp_suite("Strings");
    {
        // GPString str1 = gp_str(&arena);
        // GPString str2 = gp_str(&arena);
        // GPString str3 = gp_str(&arena, ""); // same as the ones above
        // char cstr[64] = "whatever";
        gp_test("Repeat");
        {
            // gp_repeat(&str1, 2, "blah", strlen("blah")); // ok, length given
            // gp_repeat(&str2, 3, "BLAH"); // ok, literal string
            // gp_repeat(&str3, 4, str1);   // ok, GPString
            // //gp_repeat(&str3, 5, cstr); // not ok, non-literal without length!

            // // Same as above but by passing an allocator, a copy is made instead
            // // of writing to an output string.
            // GPString copy1 = gp_repeat(&arena, 2, "blah", strlen("blah"));
            // GPString copy2 = gp_repeat(&arena, 3, "BLAH");
            // GPString copy3 = gp_repeat(&arena, 4, str1);
            // //GPString copy4 = gp_repeat(&arena, 5, cstr);

            // gp_expect(gp_equal(str1, "blahblah", strlen("blahblah")));
            // gp_expect(gp_equal(str2, "BLAHBLAHBLAH"));
            // gp_expect(gp_equal(copy1, str1));
            // gp_expect(gp_equal(copy2, str2));
            // gp_expect(gp_equal(copy3, str3));
        }
        // #define gp_replace(...)
        // size_t gp_str_replace(
        //     GPString*           haystack,
        //     const void*restrict needle,
        //     size_t              needle_length,
        //     const void*restrict replacement,
        //     size_t              replacement_length,
        //     size_t              start);
        // #define gp_replace_all(...)
        // size_t gp_str_replace_all(
        //     GPString*           haystack,
        //     const void*restrict needle,
        //     size_t              needle_length,
        //     const void*restrict replacement,
        //     size_t              replacement_length);
        // #define gp_trim(...)
        // void gp_str_trim(
        //     GPString*,
        //     const char* optional_char_set,
        //     int         flags);
        // #define gp_to_upper(...)     gp_str_to_upper(__VA_ARGS__)
        // void gp_str_to_upper(GPString*);
        // #define gp_to_lower(...)     gp_str_to_lower(__VA_ARGS__)
        // void gp_str_to_lower(GPString*);
        // #define gp_to_valid(...)
        // void gp_str_to_valid(
        //     GPString*   str,
        //     const char* replacement);
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
        // #define gp_count(...)
        // size_t gp_str_count(
        //     GPString    haystack,
        //     const void* needle,
        //     size_t      needle_size);
        // #define gp_equal(...)
        // bool gp_str_equal(
        //     GPString    s1,
        //     const void* s2,
        //     size_t      s2_size);
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
        // #define gp_codepoint_length(...) gp_char_codepoint_length(__VA_ARGS__)
        // size_t gp_char_codepoint_length(
        //     const void* str);
        // #define gp_classify(...)
        // bool gp_char_classify(
        //     const void* str,
        //     int (*classifier)(wint_t c));
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
