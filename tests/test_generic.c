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

        gp_test("Find first");
        {
            GPString haystack = gp_str(&arena, "yeah blah nope blah yeah");
            gp_expect(gp_find_first(haystack, "blah")                    == 5);
            gp_expect(gp_find_first(haystack, "blah", strlen("blah"))    == 5);
            gp_expect(gp_find_first(haystack, "blah", strlen("blah"), 6) == 15);
        }

        gp_test("Find last");
        {
            GPString haystack = gp_str(&arena, "yeah blah nope blah yeah");
            gp_expect(gp_find_last(haystack, "blah")                    == 15);
            gp_expect(gp_find_last(haystack, "blah", strlen("blah"))    == 15);
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
            if (setlocale(LC_ALL, "C.utf8") != NULL)
            {
                GPString a = gp_str(&arena, "😂aAaAäÄä😂");
                GPString b = gp_copy(&arena, a);
                gp_expect(gp_equal_case(a, b));
                gp_expect(gp_equal_case(a, b, gp_length(b)));
                gp_expect(gp_equal_case(a, "😂aAaAäÄä😂"));
            }
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
            gp_expect(   gp_is_valid(str, gp_length(str)));
            gp_expect(   gp_is_valid(str, gp_length(str), &invalid_index));
            gp_copy(&str, "😂a\xffÄ😂");
            gp_expect( ! gp_is_valid(str));
            gp_expect( ! gp_is_valid("😂a\xffÄ😂"));
            gp_expect( ! gp_is_valid(str, gp_length(str)));
            gp_expect( ! gp_is_valid(str, gp_length(str), &invalid_index));
            gp_expect(invalid_index == 5);
        }
    }

    gp_suite("Arrays and strings");
    {
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
            gp_copy(&arr1, ((int[]){1, 2, 3, 4}));
            arr_assert_eq(arr1, ((int[]){1, 2, 3, 4}), 4);
            GPArray(int) arr2 = gp_copy(&arena, ((int[]){3, 2, 1}));
            arr_assert_eq(arr2, ((int[]){3, 2, 1}), 3);
            gp_copy(&arr1, arr2);
            arr_assert_eq(arr1, ((int[]){3, 2, 1}), 3);
            GPArray(int) arr3 = gp_copy(&arena, arr1);
            arr_assert_eq(arr3, ((int[]){3, 2, 1}), 3);
            int carr[] = { 9, 8, 7, 6, 5 };
            gp_copy(&arr3, carr, 5);
            arr_assert_eq(arr3, carr, 5);
            GPArray(int) arr4 = gp_copy(&arena, arr3, gp_length(arr3));
            arr_assert_eq(arr4, carr, 5);
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
            gp_slice(&arr1, ((int[]){ 1, 2, 3, 4, 5, 6, 7, 8 }), 1, 7);
            arr_assert_eq(arr1, ((int[]){ 2, 3, 4, 5, 6, 7 }), 6);
            gp_slice(&arr1, 1, 5);
            arr_assert_eq(arr1, ((int[]){ 3, 4, 5, 6 }), 4);
            GPArray(int) arr2 = gp_slice(&arena, arr1, 1, 3);
            arr_assert_eq(arr2, ((int[]){ 4, 5 }), 2);
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
            gp_append(&arr1, (int[]){1}); // MUST be array literal even if 1 element
            arr_assert_eq(arr1, (int[]){1}, 1);
            gp_append(&arr1, (int[]){2});
            arr_assert_eq(arr1, ((int[]){ 1, 2 }), 2);
            gp_append(&arr1, ((int[]){ 3, 4, 5 }), 3);
            arr_assert_eq(arr1, ((int[]){ 1, 2, 3, 4, 5 }), 5);
            GPArray(int) arr2 = gp_append(&arena, arr1, (int[]){6});
            arr_assert_eq(arr2, ((int[]){ 1, 2, 3, 4, 5, 6 }), 6);
            GPArray(int) arr3 = gp_append(&arena, arr1, arr2);
            arr_assert_eq(arr3, ((int[]){ 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, 6 }), 11);
            GPArray(int) arr4 = gp_append(&arena, arr1, ((int[]){6}), 1);
            arr_assert_eq(arr4, arr2, gp_length(arr2));
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
            gp_insert(&arr1, 0, (int[]){1}); // MUST be array literal even if 1 element
            arr_assert_eq(arr1, (int[]){1}, 1);
            gp_insert(&arr1, 0, (int[]){2});
            arr_assert_eq(arr1, ((int[]){ 2, 1 }), 2);
            gp_insert(&arr1, 0, ((int[]){ 3, 4, 5 }), 3);
            arr_assert_eq(arr1, ((int[]){ 3, 4, 5, 2, 1 }), 5);
            GPArray(int) arr2 = gp_insert(&arena, 0, arr1, (int[]){6});
            arr_assert_eq(arr2, ((int[]){ 6, 3, 4, 5, 2, 1 }), 6);
            GPArray(int) arr3 = gp_insert(&arena, 0, arr1, arr2);
            arr_assert_eq(arr3, ((int[]){ 6, 3, 4, 5, 2, 1, 3, 4, 5, 2, 1 }), 11);
            GPArray(int) arr4 = gp_insert(&arena, 0, arr1, ((int[]){6}), 1);
            arr_assert_eq(arr4, arr2, gp_length(arr2));
        }
    }

    gp_suite("Array");
    {
        GPAllocator* scope = gp_begin(0);
        gp_test("Map");
        {
            // Arguments do not need to be of type void* as long as they are
            // pointers and in is const. Return must be void.
            void increment(int* out, const int* in);
            GPArray(int) arr1 = gp_arr(scope, int, 1, 2, 3, 4);
            gp_map(&arr1, increment);
            arr_assert_eq(arr1, ((int[]){ 2, 3, 4, 5 }), 4);
            GPArray(int) arr2 = gp_map(scope, arr1, increment);
            arr_assert_eq(arr2, ((int[]){ 3, 4, 5, 6 }), 4);
            gp_map(&arr1, arr2, increment);
            arr_assert_eq(arr1, ((int[]){ 4, 5, 6, 7 }), 4);
            GPArray(int) arr3 = gp_map(scope, ((int[]){ 1, 1, 1 }), increment);
            arr_assert_eq(arr3, ((int[]){ 2, 2, 2 }), 3);
            int carr[] = { 9, 9, 9, 9, 9 };
            GPArray(int) arr4 = gp_map(scope, carr, sizeof carr / sizeof*carr, increment);
            arr_assert_eq(arr4, ((int[]){ 10, 10, 10, 10, 10 }), 5);
        }
            int* sum(int* y, const int* x);
            char* append(char* result, const char**_element);
            bool even(const int* element);
            bool more_than_5(const int* element);
        gp_end(scope);
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

void increment(int* out, const int* in) { *out = *in + 1; }
int* sum(int* y, const int* x) { *y += *x; return y; }
char* append(char* result, const char**_element)
{
    const char* element = *_element;
    const size_t length = result && strlen(result);
    result = gp_mem_realloc(
        gp_last_scope(NULL), result, length, length + strlen(element) + sizeof" ");
    if (length == 0)
        ((char*)result)[0] = '\0';
    return strcat(strcat(result, element), " ");
}
bool even(const int* element) { return !(*element % 2); }
bool more_than_5(const int* element) { return *element > 5; }
