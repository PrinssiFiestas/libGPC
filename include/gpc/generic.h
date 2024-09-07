// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file generic.h
 * Type generic macros.
 */

#ifndef GP_GENERIC_INCLUDED
#define GP_GENERIC_INCLUDED

#include <gpc/array.h>
#include <gpc/string.h>
#include <gpc/unicode.h>
#include <gpc/hashmap.h>
#include <gpc/io.h>

// ----------------------------------------------------------------------------
// Introduction to Generic Macros
//
// Strings, arrays, hash maps, and files have a function API found in their
// corresponding headers. Funcions in the function API are easier to debug,
// explicit, and work better with tooling than macros. However, since C doesn't
// have overloading, optional parameters, or templates, generality is achieved
// with void* and extra parameters. This is not always type safe and remembering
// all the parameters brings friction to quick prototyping.
//     To accommodate this, generic macros are provided. Effort has been made to
// not just have them as syntactic sugar, but actually have some real reason to
// exist. While they are easier to remember and less verbose than their function
// counterparts, the real benefit is added type safety, especially with arrays.
//     Great effort has also been made to make the macros hygienic: there is no
// bugs related to operator precedence, and almost all of them only evaluate
// their arguments once. The very few ones that might evaluate twice, will at
// least give some compiler warning when weird things might happen.
//     In C++, these are implemented with overloads, optional parameters, and
// templates. To see exactly what is provided, you can check below for all the
// overloads. Find-tool of your text editor will be your friend.
//     There is a huge amount of overloads in total. It is not expected for
// anybody to remember all of them. To use generic macros effectively, it is
// only needed to understand conventions and ideas behind them.
//
// Naming
//
// Functions in the function API are named as follows:
/*
    gp_type_func_name()
*/
//     where "gp_" is the namespace for the whole library, and "type_" is some
// abbreviation of the class, and "func_name" is the actual name. Generic macros
// leave "type_" out, so for example, gp_str_trim() becomes gp_trim(), and
// gp_arr_push() becomes gp_push().
//
// Lengths
//
// Functions usually take lengths of arguments for generality. The idea is, that
// you can provide any type of input as an argument, because you provide the
// length explicitly. For example, for strings these all work:
/*
    gp_str_copy(&str, cstr, strlen(cstr)); // char*
    gp_str_copy(&str, str2, gp_str_length(str2)); // GPString
    gp_str_copy(&str, other_str.data, other_str.length); //something else
*/
//     However, string input arguments for generic macros are assumed to be
// null-terminated char* or GPString, and arrays inputs are assumed to be of
// type GPArray(T). Therefore the following works:
/*
    gp_copy(&str, str2); // GPString
    gp_copy(&str, "Null terminated string"); // char*
    gp_copy(&arr, arr2); // GPArrays of same type
*/
//     Note that only (void* in, size_t in_length) pairs are replaced with
// generic input arguments. If you see char* arguments in the function API, like
// gp_str_trim() char_set, they must be null-terminated char* strings even with
// generic macros!
//
// Arrays
//
// All array related functions take the size of an element as their first
// argument. This should always be omitted for type generic macros. Array
// functions also take destination arrays as arguments and return them to be
// assigned back to themselves in case of reallocations. Generic array macros
// are consistent with GPStrings in the sense that they take pointers to
// destination arrays and return void.
//
// Destinations
//
// If destination object argument is going to be completely overwritten, it can
// be replaced with an allocator, which will create and return a new object
// instead of overwriting and returning void. As an example, here are GPString
// analogs for strcpy() and strdup():
/*
    gp_copy(&dest, "This will overwrite dest.");
    GPString copy = gp_copy(gp_heap, "This will be duplicated.");
*/
//     In most other cases, new objects can be created by passing an allocator
// as an extra argument. This should be the first argument in most of the cases.
// Note that what was previously an output variable taken as a pointer, is now
// an input parameter taken by value.
/*
    gp_append(&str1, "This will be appended to str1.");
    GPString str2 = gp_append(gp_heap, str1, "This'll be appended to new str.");
*/
// Optional parameters
//
// In the function API, an optional parameter is a parameter, usually a pointer,
// than can have the value NULL or 0. With generic macros, optional parameters
// can be omitted completely.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


/** Generic hash map.
 * Dictionarys are very specific kind of pointers. Use this macro to
 * differentiate between other pointers.
 */
#define GPDictionary(T) T*

#ifdef __cplusplus

#include <array> // PRIVATE HELPERS, IGNORE THESE -----------------------------
typedef struct gp_str_in { const uint8_t* data; const size_t length; } GPStrIn;
extern "C" {
    GPString gp_replace_new(const GPAllocator*, GPStrIn, GPStrIn, GPStrIn, size_t);
    GPString gp_replace_all_new(const GPAllocator*, GPStrIn, GPStrIn, GPStrIn);
    GPString gp_str_trim_new(const void*, GPStrIn, const char*, int);
    GPString gp_to_upper_new(const GPAllocator*, GPStrIn);
    GPString gp_to_lower_new(const GPAllocator*, GPStrIn);
    GPString gp_to_valid_new(const GPAllocator*, GPStrIn, const char*);
    GPString gp_to_upper_full_new(const GPAllocator*, GPStrIn, const char*);
    GPString gp_to_lower_full_new(const GPAllocator*, GPStrIn, const char*);
    GPString gp_capitalize_new(const GPAllocator*, GPStrIn);
    GPString gp_capitalize_locale_new(const GPAllocator*, GPStrIn, const char*);
    size_t gp_bytes_codepoint_count(const void*, size_t);
    bool gp_bytes_is_valid_utf8(const void*, size_t, size_t*);
    GPString gp_join_new(const GPAllocator*, const GPArray(GPString), const char*);
}
static inline size_t gp_str_length_cpp(const GPString str) { return gp_str_length(str); }
static inline size_t gp_str_length_cpp(const char*const str) { return strlen(str); }
static inline size_t gp_length_cpp(const char*const str) { return strlen(str); }
static inline size_t gp_length_cpp(const void*const arr) { return gp_arr_length(arr); }
static inline void* gp_insert_cpp(const size_t elem_size, void* out, const size_t pos,
    const void*const src1, const size_t src1_length,
    const void*const src2, const size_t src2_length)
{
    memcpy(out, src1, pos * elem_size);
    memcpy((uint8_t*)out + pos * elem_size, src2, src2_length * elem_size);
    memcpy((uint8_t*)out + (pos + src2_length) * elem_size,
        (uint8_t*)src1 + pos * src1_length,
        (src1_length - pos) * elem_size);
    ((GPArrayHeader*)out - 1)->length = src1_length + src2_length;
    return out;
}
// END OF PRIVATE HELPERS -----------------------------------------------------

// C++: Provide overloads for these for your custom allocators.
static inline const GPAllocator* gp_alc_cpp(const GPAllocator* alc)
{
    return (const GPAllocator*)alc;
}
static inline const GPAllocator* gp_alc_cpp(const GPArena* alc)
{
    return (const GPAllocator*)alc;
}
// C11: #define GP_USER_ALLOCATORS to be a comma separated list of your
// custom allocator types.
// C99: just uses void*. No casts or defining macros required but no type safety
// for you either!

// These overloads are also avaiable in C as macros.
//
// T_STRING shall be GPString or a const char*. However, if not C++ and C11
// _Generic() selection is not available in your compiler, const char* arguments
// MUST be string literals.

// ----------------------------------------------------------------------------
// Builder functions

// Create a new array of type GPArray(T)
#define/*GPArray(T)*/gp_arr(/*allocator, T, init_elements*/...) \
    GP_ARR_NEW_CPP(__VA_ARGS__)

// Read-only array. Static if GNUC, on stack otherwise. Not available in C++.
#ifndef __cplusplus
#define/*const GPArray(T)*/gp_arr_ro(T,/*elements*/...)
#endif

template <typename T_ALLOCATOR>
static inline GPString gp_str(T_ALLOCATOR* allocator, const char* init = "")
{
    return gp_str_new(gp_alc_cpp(allocator), 16, init);
}
template <typename T_ALLOCATOR>
static inline GPString gp_str(
    T_ALLOCATOR* allocator,
    const size_t capacity,
    const char*  init)
{
    return gp_str_new(gp_alc_cpp(allocator), capacity, init);
}

template <typename T_ALLOCATOR>
static inline GPHashMap* gp_hmap(
    T_ALLOCATOR* allocator,
    const size_t element_size = 0)
{
    if (element_size == 0)
        return gp_hash_map_new(gp_alc_cpp(allocator), NULL);

    GPMapInitializer init = {
        .element_size = element_size,
        .capacity     = 0,
        .destructor   = NULL
    };
    return gp_hash_map_new(gp_alc_cpp(allocator), &init);
}

template <typename T_ALLOCATOR, typename T_ARG>
static inline GPHashMap* gp_hmap(
    T_ALLOCATOR* allocator,
    const size_t element_size,
    void(*const  destructor)(T_ARG*),
    const size_t capacity            = 0)
{
    if (element_size == 0)
        return gp_hash_map_new(gp_alc_cpp(allocator), NULL);

    GPMapInitializer init = {
        .element_size = element_size,
        .capacity     = capacity,
        .destructor   = (void(*)(void*))destructor
    };
    return gp_hash_map_new(gp_alc_cpp(allocator), &init);
}

// Create a new object of type GPDictionary(T)
#define gp_dict(/*allocator, type, destructor = NULL, capacity = 0*/...) \
    GP_DICT_CPP(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Bytes and strings

// ---------------------------
// gp_equal()

template <typename T_STRING>
static inline bool gp_equal(const GPString a, T_STRING b)
{
    return gp_str_equal(a, b, gp_str_length_cpp(b));
}
static inline bool gp_equal(
    const GPString a, const void*const b, const size_t b_length)
{
    return gp_str_equal(a, b, b_length);
}
static inline bool gp_equal(const void*const a, const size_t a_length,
    const void*const b, const size_t b_length)
{
    return gp_bytes_equal(a, a_length, b, b_length);
}

// ---------------------------
// gp_count()

template <typename T_STRING1, typename T_STRING2>
static inline size_t gp_count(T_STRING1 haystack, T_STRING2 needle)
{
    return gp_bytes_count(
        haystack, gp_str_length_cpp(haystack),
        needle,   gp_str_length_cpp(needle));
}
template <typename T_STRING>
static inline size_t gp_count(
    T_STRING haystack, const void*const needle, const size_t needle_length)
{
    return gp_bytes_count(haystack, gp_str_length_cpp(haystack), needle, needle_length);
}
static inline size_t gp_count(
    const void*const haystack,
    const size_t     haystack_length,
    const void*const needle,
    const size_t     needle_length)
{
    return gp_bytes_count(haystack, haystack_length, needle, needle_length);
}

// ---------------------------
// gp_codepoint_length()

static inline size_t gp_codepoint_length(const void*const ptr)
{
    return gp_utf8_codepoint_length(ptr, 0);
}
static inline size_t gp_codepoint_length(const void*const str, const size_t i)
{
    return gp_utf8_codepoint_length(str, i);
}

// ----------------------------------------------------------------------------
// Strings

// ---------------------------
// gp_repeat()

template <typename T_STRING>
static inline void gp_repeat(GPString* dest, const size_t count, T_STRING src)
{
    gp_str_repeat(dest, count, src, gp_str_length_cpp(src));
}
static inline void gp_repeat(
    GPString* dest, const size_t count,
    const void*const src, const size_t src_length)
{
    gp_str_repeat(dest, count, src, src_length);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_repeat(
    T_ALLOCATOR* allocator, const size_t count, T_STRING src)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), count * gp_str_length_cpp(src), "");
    ((GPStringHeader*)out - 1)->length = gp_bytes_repeat(out, count, src, gp_str_length_cpp(src));
    return out;
}
template <typename T_ALLOCATOR>
static inline GPString gp_repeat(
    T_ALLOCATOR* allocator, const size_t count, const void*const src, const size_t src_length)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), count * src_length, "");
    ((GPStringHeader*)out - 1)->length = gp_bytes_repeat(out, count, src, src_length);
    return out;
}

// ---------------------------
// gp_replace()

template <typename T_STRING1, typename T_STRING2>
static inline size_t gp_replace(
    GPString*    haystack,
    T_STRING1    needle,
    T_STRING2    replacement,
    const size_t start = 0)
{
    return gp_str_replace(
        haystack,
        needle,
        gp_str_length_cpp(needle),
        replacement,
        gp_str_length_cpp(replacement),
        start);
}
template
<typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2>
static inline GPString gp_replace(
    T_ALLOCATOR*   allocator,
    T_STRING1      haystack,
    T_STRING2      needle,
    const GPString replacement,
    const size_t   start = 0)
{
    GPStrIn hay { (uint8_t*)haystack,    gp_str_length_cpp(haystack   ) };
    GPStrIn ndl { (uint8_t*)needle,      gp_str_length_cpp(needle     ) };
    GPStrIn rep { (uint8_t*)replacement, gp_str_length_cpp(replacement) };
    return gp_replace_new(gp_alc_cpp(allocator), hay, ndl, rep, start);
}
template
<typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2>
static inline GPString gp_replace(
    T_ALLOCATOR*     allocator,
    T_STRING1        haystack,
    T_STRING2        needle,
    const char*const replacement,
    const size_t     start = 0)
{
    GPStrIn hay { (uint8_t*)haystack,    gp_str_length_cpp(haystack   ) };
    GPStrIn ndl { (uint8_t*)needle,      gp_str_length_cpp(needle     ) };
    GPStrIn rep { (uint8_t*)replacement, gp_str_length_cpp(replacement) };
    return gp_replace_new(gp_alc_cpp(allocator), hay, ndl, rep, start);
}

// ---------------------------
// gp_replace_all()

template <typename T_STRING1, typename T_STRING2>
static inline size_t gp_replace_all(
    GPString* haystack,
    T_STRING1 needle,
    T_STRING2 replacement)
{
    return gp_str_replace_all(
        haystack,
        needle,
        gp_str_length_cpp(needle),
        replacement,
        gp_str_length_cpp(replacement));
}
template
<typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2, typename T_STRING3>
static inline GPString gp_replace_all(
    T_ALLOCATOR* allocator,
    T_STRING1    haystack,
    T_STRING2    needle,
    T_STRING3    replacement)
{
    GPStrIn hay { (uint8_t*)haystack,    gp_str_length_cpp(haystack   ) };
    GPStrIn ndl { (uint8_t*)needle,      gp_str_length_cpp(needle     ) };
    GPStrIn rep { (uint8_t*)replacement, gp_str_length_cpp(replacement) };
    return gp_replace_all_new(gp_alc_cpp(allocator), hay, ndl, rep);
}

// ---------------------------
// gp_trim()

static inline void gp_trim(
    GPString* str, const char*const char_set = NULL, const int flags = 'l'|'r')
{
    gp_str_trim(str, char_set, flags);
}
template <typename T_ALLOCATOR>
static inline GPString gp_trim(T_ALLOCATOR* allocator,
    GPString str, const char*const char_set = NULL, const int flags = 'l'|'r')
{
    GPStrIn in = { (uint8_t*)str, gp_str_length(str) };
    return gp_str_trim_new(gp_alc_cpp(allocator), in, char_set, flags);
}
template <typename T_ALLOCATOR>
static inline GPString gp_trim(T_ALLOCATOR* allocator,
    const char*const str, const char*const char_set=NULL, const int flags='l'|'r')
{
    GPStrIn in = { (uint8_t*)str, strlen(str) };
    return gp_str_trim_new(gp_alc_cpp(allocator), in, char_set, flags);
}

// ---------------------------
// gp_to_upper()

static inline void gp_to_upper(GPString* str)
{
    gp_str_to_upper(str);
}
static inline void gp_to_upper(GPString* str, const char* locale)
{
    gp_str_to_upper_full(str, locale);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_to_upper(
    T_ALLOCATOR* allocator, T_STRING str)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_to_upper_new(gp_alc_cpp(allocator), s);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_to_upper(
    T_ALLOCATOR* allocator, T_STRING str, const char* locale)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_to_upper_full_new(gp_alc_cpp(allocator), s, locale);
}

// ---------------------------
// gp_to_lower()

static inline void gp_to_lower(GPString* str)
{
    gp_str_to_lower(str);
}
static inline void gp_to_lower(GPString* str, const char* locale)
{
    gp_str_to_lower_full(str, locale);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_to_lower(T_ALLOCATOR* allocator, T_STRING str)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_to_lower_new(gp_alc_cpp(allocator), s);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_to_lower(T_ALLOCATOR* allocator, T_STRING str, const char* locale)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_to_lower_full_new(gp_alc_cpp(allocator), s, locale);
}

// ---------------------------
// gp_to_valid()

static inline void gp_to_valid(GPString* str, const char*const replacement)
{
    gp_str_to_valid(str, replacement);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_to_valid(
    T_ALLOCATOR* allocator, T_STRING str, const char*const replacement)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_to_valid_new(gp_alc_cpp(allocator), s, replacement);
}

// ---------------------------
// gp_capitalize()

static inline void gp_capitalize(GPString* str)
{
    gp_str_capitalize(str, "");
}
static inline void gp_capitalize(GPString* str, const char* locale)
{
    gp_str_capitalize(str, locale);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_capitalize(T_ALLOCATOR* allocator, T_STRING str)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_capitalize_new(gp_alc_cpp(allocator), s);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_capitalize(T_ALLOCATOR* allocator, T_STRING str, const char* locale)
{
    GPStrIn s = { (uint8_t*)str, gp_str_length_cpp(str) };
    return gp_capitalize_locale_new(gp_alc_cpp(allocator), s, locale);
}

// ---------------------------
// gp_find_first()

template <typename T_STRING>
static inline size_t gp_find_first(
    const GPString haystack, T_STRING needle, const size_t start = 0)
{
    return gp_str_find_first(haystack, needle, gp_str_length_cpp(needle), start);
}
// ---------------------------
// gp_find_last()

template <typename T_STRING>
static inline size_t gp_find_last(const GPString haystack, T_STRING needle)
{
    return gp_str_find_last(haystack, needle, gp_str_length_cpp(needle));
}
static inline size_t gp_find_last(
    const GPString haystack, const void*const needle, const size_t needle_length)
{
    return gp_str_find_last(haystack, needle, needle_length);
}

// ---------------------------
// gp_find_first_of()

static inline size_t gp_find_first_of(
    const GPString haystack, const char*const char_set, const size_t start = 0)
{
    return gp_str_find_first_of(haystack, char_set, start);
}

// ---------------------------
// gp_find_first_not_of()

static inline size_t gp_find_first_not_of(
    const GPString haystack, const char*const char_set, const size_t start = 0)
{
    return gp_str_find_first_not_of(haystack, char_set, start);
}

// ---------------------------
// gp_equal_case()

template <typename T_STRING>
static inline bool gp_equal_case(const GPString a, T_STRING b)
{
    return gp_str_equal_case(a, b, gp_str_length_cpp(b));
}
static inline bool gp_equal_case(
    const GPString a, const void*const b, const size_t b_length)
{
    return gp_str_equal_case(a, b, b_length);
}

// ---------------------------
// gp_codepoint_count()

template <typename T_STRING>
static inline size_t gp_codepoint_count(T_STRING str)
{
    return gp_bytes_codepoint_count(str, gp_str_length_cpp(str));
}
static inline size_t gp_codepoint_count(const void*const str, const size_t str_length)
{
    return gp_bytes_codepoint_count(str, str_length);
}

// ---------------------------
// gp_is_valid()

template <typename T_STRING>
static inline bool gp_is_valid(T_STRING str, size_t* invalid_index = NULL)
{
    return gp_bytes_is_valid_utf8(str, gp_str_length_cpp(str), invalid_index);
}
static inline bool gp_is_valid(const void*const str, const size_t length)
{
    return gp_bytes_is_valid_utf8(str, length, NULL);
}
static inline bool gp_is_valid(
    const void*const str, const size_t length, size_t*out_invalid_index)
{
    return gp_bytes_is_valid_utf8(str, length, out_invalid_index);
}

// ---------------------------
// gp_compare()

template <typename T_STRING>
static inline int gp_compare(GPString s1, T_STRING s2, int flags = 0, const char* locale = "")
{
    return gp_str_compare(s1, s2, gp_str_length_cpp(s2), flags, locale);
}

// ----------------------------------------------------------------------------
// Strings and arrays

// ---------------------------
// gp_split() and gp_str_join()

template <typename T_STRING, typename T_ALLOCATOR>
static inline GPArray(GPString) gp_split(
    T_ALLOCATOR allocator, T_STRING str, const char* separator_char_set = GP_WHITESPACE)
{
    return gp_str_split(gp_alc_cpp(allocator), str, gp_str_length_cpp(str), separator_char_set);
}

static inline void gp_join(GPString* dest, const GPArray(GPString) srcs, const char* separator = "")
{
    gp_str_join(dest, srcs, separator);
}
template <typename T_ALLOCATOR>
static inline GPString gp_join(T_ALLOCATOR allocator, const GPArray(GPString) srcs, const char* separator = "")
{
    return gp_join_new(gp_alc_cpp(allocator), srcs, separator);
}

// ---------------------------
// gp_sort()

static inline void gp_sort(GPArray(GPString)* strs, int flags = 0, const char* locale = "")
{
    gp_str_sort(strs, flags, locale);
}

// ---------------------------
// Getters

#define gp_length(GPARRAY_OR_GPSTRING)     gp_arr_length(GPARRAY_OR_GPSTRING)
#define gp_capacity(GPARRAY_OR_GPSTRING)   gp_arr_capacity(GPARRAY_OR_GPSTRING)
#define gp_allocation(GPARRAY_OR_GPSTRING) gp_arr_allocation(GPARRAY_OR_GPSTRING)
#define gp_allocator(GPARRAY_OR_GPSTRING)  gp_arr_allocator(GPARRAY_OR_GPSTRING)

// ---------------------------
// gp_reserve()

static inline void gp_reserve(GPString* str, const size_t capacity)
{
    gp_str_reserve(str, capacity);
}
template <typename T>
static inline void gp_reserve(GPArray(T)* parr, const size_t capacity)
{
    *parr = (GPArray(T))gp_arr_reserve(sizeof(*parr)[0], *parr, capacity);
}

// ---------------------------
// gp_copy()

static inline void gp_copy(GPString* dest, const char*const src)
{
    gp_str_copy(dest, src, strlen(src));
}
template <typename T_STRING_OR_ARRAY>
static inline void gp_copy(T_STRING_OR_ARRAY* dest, T_STRING_OR_ARRAY src)
{
    gp_reserve(dest, gp_arr_length(src));
    ((GPArrayHeader*)*dest - 1)->length = gp_arr_length(src);
    memcpy(*dest, src, gp_arr_length(src) * sizeof(*dest)[0]);
}
template <typename T_ALLOCATOR>
static inline GPString gp_copy(
    T_ALLOCATOR* allocator, const char*const src)
{
    return gp_str_new(gp_alc_cpp(allocator), strlen(src), src);
}
template <typename T_ALLOCATOR, typename T_STRING_OR_ARRAY>
static inline T_STRING_OR_ARRAY gp_copy(
    T_ALLOCATOR* allocator, T_STRING_OR_ARRAY src)
{
    const size_t length = gp_length_cpp(src);
    void* out = gp_arr_new(gp_alc_cpp(allocator), sizeof src[0], length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = length;
    return (T_STRING_OR_ARRAY)memcpy(out, src, length * sizeof src[0]);
}
static inline void gp_copy(
    GPString* dest, const char*const src, const size_t src_length)
{
    gp_str_copy(dest, src, src_length);
}
template <typename T_STRING_OR_ARRAY>
static inline void gp_copy(
    T_STRING_OR_ARRAY* dest, const T_STRING_OR_ARRAY src, const size_t src_length)
{
    gp_reserve(dest, src_length);
    ((GPArrayHeader*)*dest - 1)->length = src_length;
    memcpy(*dest, src, src_length * sizeof(*dest)[0]);
}
template <typename T_ALLOCATOR>
static inline GPString gp_copy(
    T_ALLOCATOR* allocator, const char*const src, const size_t src_length)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), src_length, "");
    ((GPStringHeader*)out - 1)->length = src_length;
    return (GPString)memcpy(out, src, src_length);
}
template <typename T_ALLOCATOR, typename T_STRING_OR_ARRAY>
static inline T_STRING_OR_ARRAY gp_copy(
    T_ALLOCATOR* allocator, T_STRING_OR_ARRAY src, const size_t src_length)
{
    T_STRING_OR_ARRAY out = (T_STRING_OR_ARRAY)gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], src_length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = src_length;
    return (T_STRING_OR_ARRAY)memcpy(out, src, src_length * sizeof out[0]);
}

// ---------------------------
// gp_slice()

static inline void gp_slice(GPString* dest, const size_t start, const size_t end)
{
    gp_str_slice(dest, NULL, start, end);
}
template <typename T>
static inline void gp_slice(GPArray(T)* dest, const size_t start, const size_t end)
{
    *dest = (GPArray(T))gp_arr_slice(sizeof(*dest)[0], *dest, NULL, start, end);
}
static inline void gp_slice(
    GPString* dest, const char*const src, const size_t start, const size_t end)
{
    gp_str_slice(dest, src, start, end);
}
static inline void gp_slice(
    GPString* dest, const GPString src, const size_t start, const size_t end)
{
    gp_str_slice(dest, src, start, end);
}
template <typename T>
static inline void gp_slice(
    GPArray(T)* dest, const T*const src, const size_t start, const size_t end)
{
    *dest =(GPArray(T))gp_arr_slice(sizeof(*dest)[0], *dest, src, start, end);
}
template <typename T_ALLOCATOR>
static inline GPString gp_slice(
    T_ALLOCATOR* allocator, const GPString src, const size_t start, const size_t end)
{
    const size_t length = end - start;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    return (GPString)memcpy(out, src + start, length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_slice(
    T_ALLOCATOR* allocator, const char*const src, const size_t start, const size_t end)
{
    const size_t length = end - start;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    return (GPString)memcpy(out, src + start, length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_slice(
    T_ALLOCATOR* allocator, const T* src, const size_t start, const size_t end)
{
    const size_t length = end - start;
    GPArray(T) out = (GPArray(T))gp_arr_new((const GPAllocator*)allocator, sizeof out[0], length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = length;
    return (GPArray(T))memcpy(out, src + start, length * sizeof out[0]);
}

// ---------------------------
// gp_append()

template <typename T_STRING>
static inline void gp_append(GPString* dest, T_STRING src)
{
    gp_str_append(dest, src, gp_str_length_cpp(src));
}
static inline void gp_append(
    GPString* dest, const void*const src, const size_t src_length)
{
    gp_str_append(dest, src, src_length);
}
template <typename T>
static inline void gp_append(GPArray(T)* dest, const GPArray(T) src)
{
    *dest = (GPArray(T))gp_arr_append(sizeof(*dest)[0], *dest, src, gp_arr_length(src));
}
static inline void gp_append(GPString* dest, const GPString src, const size_t src_length)
{
    gp_str_append(dest, src, src_length);
}
static inline void gp_append(GPString* dest, const char*const src, const size_t src_length)
{
    gp_str_append(dest, src, src_length);
}
template <typename T>
static inline void gp_append(
    GPArray(T)* dest, const T*const src, const size_t src_length)
{
    *dest = (GPArray(T))gp_arr_append(sizeof(*dest)[0], *dest, src, src_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_append(
    T_ALLOCATOR* allocator, const GPString str1, const GPString str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_append(
    T_ALLOCATOR* allocator, const GPString str1, const char*const str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_append(
    T_ALLOCATOR* allocator, const char*const str1, const GPString str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_append(
    T_ALLOCATOR* allocator, const char*const str1, const char*const str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_append(
    T_ALLOCATOR* allocator, GPArray(T) arr1, GPArray(T) arr2)
{
    const size_t arr1_length = gp_arr_length(arr1);
    const size_t arr2_length = gp_arr_length(arr2);
    const size_t length = arr1_length + arr2_length;
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0], length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = length;
    memcpy(out + arr1_length, arr2, arr2_length * sizeof arr1[0]);
    return (GPArray(T))memcpy(out, arr1, arr1_length * sizeof arr1[0]);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_append(
    T_ALLOCATOR*     allocator,
    T_STRING         str1,
    const void*const str2,
    const size_t     str2_length)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_append(
    T_ALLOCATOR*     allocator,
    const GPString   str1,
    const size_t     str1_length,
    const void*const str2,
    const size_t     str2_length)
{
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_append(
    T_ALLOCATOR*     allocator,
    const char*const str1,
    const size_t     str1_length,
    const void*const str2,
    const size_t     str2_length)
{
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_append(
    T_ALLOCATOR*  allocator,
    GPArray(T)    arr1,
    const T*const arr2,
    const size_t  arr2_length)
{
    const size_t arr1_length = gp_arr_length(arr1);
    const size_t length = arr1_length + arr2_length;
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = length;
    memcpy(out + arr1_length, arr2, arr2_length * sizeof arr1[0]);
    return (GPArray(T))memcpy(out, arr1, arr1_length * sizeof arr1[0]);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_append(
    T_ALLOCATOR*  allocator,
    const T*const arr1,
    const size_t  arr1_length,
    const T*const arr2,
    const size_t  arr2_length)
{
    const size_t length = arr1_length + arr2_length;
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = length;
    memcpy(out + arr1_length, arr2, arr2_length * sizeof arr1[0]);
    return (GPArray(T))memcpy(out, arr1, arr1_length * sizeof arr1[0]);
}

// ---------------------------
// gp_insert()

template <typename T_STRING>
static inline void gp_insert(GPString* dest, const size_t index, T_STRING src)
{
    gp_str_insert(dest, index, src, gp_str_length_cpp(src));
}
static inline void gp_insert(
    GPString* dest, const size_t index,
    const void*const src, const size_t src_length)
{
    gp_str_insert(dest, index, src, src_length);
}
template <typename T>
static inline void gp_insert(
    GPArray(T)* dest, const size_t index, const GPArray(T) src)
{
    *dest = (GPArray(T))gp_arr_insert(sizeof(*dest)[0], *dest, index, src, gp_arr_length(src));
}
template <typename T>
static inline void gp_insert(
    GPArray(T)* dest, const size_t index, const T*const src, const size_t src_length)
{
    *dest = (GPArray(T))gp_arr_insert(sizeof(*dest)[0], *dest, index, src, src_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_insert(
    T_ALLOCATOR* allocator, const size_t index, const GPString str1, const GPString str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_insert(
    T_ALLOCATOR* allocator, const size_t index, const GPString str1, const char*const str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_insert(
    T_ALLOCATOR* allocator, const size_t index, const char*const str1, const GPString str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_insert(
    T_ALLOCATOR* allocator, const size_t index, const char*const str1, const char*const str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_insert(
    T_ALLOCATOR* allocator, const size_t index, const GPArray(T) arr1, const GPArray(T) arr2)
{
    const size_t arr1_length = gp_arr_length(arr1);
    const size_t arr2_length = gp_arr_length(arr2);
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0], arr1_length + arr2_length + sizeof"");
    return (GPArray(T))gp_insert_cpp(sizeof out[0], out, index, arr1, arr1_length, arr2, arr2_length);
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_insert(
    T_ALLOCATOR*     allocator,
    const size_t     index,
    T_STRING         str1,
    const void*const str2,
    const size_t     str2_length)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_insert(
    T_ALLOCATOR*     allocator,
    const size_t     index,
    const GPString   str1,
    const size_t     str1_length,
    const void*const str2,
    const size_t     str2_length)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_insert(
    T_ALLOCATOR*     allocator,
    const size_t     index,
    const char*const str1,
    const size_t     str1_length,
    const void*const str2,
    const size_t     str2_length)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_insert(
    T_ALLOCATOR*  allocator,
    const size_t  index,
    GPArray(T)    arr1,
    const T*const arr2,
    const size_t  arr2_length)
{
    const size_t arr1_length = gp_arr_length(arr1);
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0],  arr1_length + arr2_length + sizeof"");
    return (GPArray(T))gp_insert_cpp(sizeof out[0], out, index, arr1, arr1_length, arr2, arr2_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_insert(
    T_ALLOCATOR*  allocator,
    const size_t  index,
    const T*const arr1,
    const size_t  arr1_length,
    const T*const arr2,
    const size_t  arr2_length)
{
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0], arr1_length + arr2_length + sizeof"");
    return (GPArray(T))gp_insert_cpp(sizeof out[0], out, index, arr1, arr1_length, arr2, arr2_length);
}

// ----------------------------------------------------------------------------
// Arrays

// ---------------------------
// gp_push() and gp_pop()

template <typename T>
static inline void gp_push(GPArray(T)* parr, T element)
{
    *parr = (GPArray(T))gp_arr_reserve(sizeof(*parr)[0], *parr, gp_arr_length(*parr) + 1);
    (*parr)[((GPArrayHeader*)*parr - 1)->length++] = element;
}

template <typename T>
static inline T gp_pop(GPArray(T)* parr)
{
    return (*parr)[((GPArrayHeader*)*parr - 1)->length -= 1];
}

// ---------------------------
// gp_erase()

template <typename T>
static inline void gp_erase(
    GPArray(T)* parr, const size_t index, const size_t count = 1)
{
    *parr = (GPArray(T))gp_arr_erase(sizeof(*parr)[0], *parr, index, count);
}

// ---------------------------
// gp_map()

template <typename T>
static inline void gp_map(GPArray(T)* parr, void(*const f)(T* out, const T* in))
{
    *parr = (GPArray(T))gp_arr_map(sizeof(*parr)[0], *parr, NULL, 0, (void(*)(void*, const void*))f);
}
template <typename T>
static inline void gp_map(
    GPArray(T)* pout, const GPArray(T) in, void(*const f)(T* out, const T* in))
{
    *pout = (GPArray(T))gp_arr_map(sizeof(*pout)[0], *pout, in, gp_arr_length(in), (void(*)(void*, const void*))f);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_map(
    T_ALLOCATOR* allocator, const GPArray(T) in, void(*const f)(T* out, const T* in))
{
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], gp_arr_length(in));
    return out = (GPArray(T))gp_arr_map(sizeof out[0], out, in, gp_arr_length(in), (void(*)(void*, const void*))f);
}
template <typename T>
static inline void gp_map(
    GPArray(T)* pout, const T*const in, const size_t in_length,
    void(*const f)(T* out, const T* in))
{
    *pout = (GPArray(T))gp_arr_map(sizeof(*pout)[0], *pout, in, in_length, (void(*)(void*, const void*))f);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_map(
    T_ALLOCATOR* allocator, const T*const in, const size_t in_length,
    void(*f)(T* out, const T* in))
{
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], in_length);
    return out = (GPArray(T))gp_arr_map(sizeof out[0], out, in, in_length, (void(*)(void*, const void*))f);
}

// ---------------------------
// gp_fold() and gp_foldr()

template <typename T, typename T_ACCUMULATOR>
static inline T_ACCUMULATOR* gp_fold(const GPArray(T) arr, T_ACCUMULATOR* acc,
    T_ACCUMULATOR*(*const f)(T_ACCUMULATOR*, T*))
{
    return (T_ACCUMULATOR*)gp_arr_fold(sizeof arr[0], arr, acc, (void*(*)(void*,const void*))f);
}
template <typename T>
static inline intptr_t gp_fold(const GPArray(T) arr, intptr_t acc,
    intptr_t(*const f)(intptr_t, const T*))
{
    return (intptr_t)gp_arr_fold(sizeof arr[0], arr, (void*)acc, (void*(*)(void*,const void*))(void*)f);
}
template <typename T>
static inline uintptr_t gp_fold(const GPArray(T) arr, uintptr_t acc,
    uintptr_t(*const f)(uintptr_t, const T*))
{
    return (uintptr_t)gp_arr_fold(sizeof arr[0], arr, (void*)acc, (void*(*)(void*,const void*))(void*)f);
}

template <typename T, typename T_ACCUMULATOR>
static inline T_ACCUMULATOR* gp_foldr(const GPArray(T) arr, T_ACCUMULATOR* acc,
    T_ACCUMULATOR*(*const f)(T_ACCUMULATOR*, T*))
{
    return (T_ACCUMULATOR*)gp_arr_foldr(sizeof arr[0], arr, acc, (void*(*)(void*,const void*))f);
}
template <typename T>
static inline intptr_t gp_foldr(const GPArray(T) arr, intptr_t acc,
    intptr_t(*const f)(intptr_t, const T*))
{
    return (intptr_t)gp_arr_foldr(sizeof arr[0], arr, (void*)acc, (void*(*)(void*,const void*))(void*)f);
}
template <typename T>
static inline uintptr_t gp_foldr(const GPArray(T) arr, uintptr_t acc,
    uintptr_t(*const f)(uintptr_t, const T*))
{
    return (uintptr_t)gp_arr_foldr(sizeof *arr[0], arr, (void*)acc, (void*(*)(void*,const void*))(void*)f);
}

// ---------------------------
// gp_filter()

template <typename T>
static inline void gp_filter(GPArray(T)* parr, bool(*const f)(const T* in))
{
    *parr = (GPArray(T))gp_arr_filter(sizeof(*parr)[0], *parr, NULL, 0, (bool(*)(const void*))f);
}
template <typename T>
static inline void gp_filter(
    GPArray(T)* pout, const GPArray(T) in, bool(*const f)(const T* in))
{
    *pout = (GPArray(T))gp_arr_filter(sizeof(*pout)[0], *pout, in, gp_arr_length(in), (bool(*)(const void*))f);
}
template <typename T, typename T_ALLOCATOR>
static inline GPArray(T) gp_filter(
    T_ALLOCATOR* allocator, const GPArray(T) in, bool(*const f)(const T* in))
{
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], gp_arr_length(in));
    return out = (GPArray(T))gp_arr_filter(sizeof out[0], out, in, gp_arr_length(in), (bool(*)(const void*))f);
}
template <typename T>
static inline void gp_filter(
    GPArray(T)* pout, const T*const in, const size_t in_length,
    bool(*const f)(const T* in))
{
    *pout = (GPArray(T))gp_arr_filter(sizeof(*pout)[0], *pout, in, in_length, (bool(*)(const void*))f);
}
template <typename T, typename T_ALLOCATOR>
static inline GPArray(T) gp_filter(
    T_ALLOCATOR* allocator, const T*const in, const size_t in_length,
    bool(*f)(const T* in))
{
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], in_length);
    return out = (GPArray(T))gp_arr_filter(sizeof out[0], out, in, in_length, (bool(*)(const void*))f);
}

// ----------------------------------------------------------------------------
// Dictionarys

// ---------------------------
// gp_get()

template <typename T, typename T_STRING>
static inline T* gp_get(GPDictionary(T) dict, T_STRING key)
{
    return (T*)gp_hash_map_get((GPHashMap*)dict, key, gp_str_length_cpp(key));
}
template <typename T>
static inline T* gp_get(
    GPDictionary(T) dict, const void*const key, const size_t key_length)
{
    return (T*)gp_hash_map_get((GPHashMap*)dict, key, key_length);
}

// ---------------------------
// gp_put()

template <typename T, typename T_STRING>
static inline void gp_put(GPDictionary(T)* dict, T_STRING key, T element)
{
    gp_hash_map_put((GPHashMap*)*dict, key, gp_str_length_cpp(key), &element);
}
template <typename T>
static inline void gp_put(
    GPDictionary(T)* dict, const void*const key, const size_t key_length, T element)
{
    gp_hash_map_put((GPHashMap*)*dict, key, key_length, &element);
}

// ---------------------------
// gp_remove()

template <typename T, typename T_STRING>
static inline bool gp_remove(GPDictionary(T)* dict, T_STRING key)
{
    return gp_hash_map_remove((GPHashMap*)*dict, key, gp_str_length_cpp(key));
}
template <typename T>
static inline bool gp_remove(
    GPDictionary(T)* dict, const void* const key, const size_t key_length)
{
    return gp_hash_map_remove((GPHashMap*)*dict, key, key_length);
}

// ----------------------------------------------------------------------------
// Memory

// ---------------------------
// gp_alloc() and gp_alloc_zeroes()

template <typename T_ALLOCATOR>
static inline void* gp_alloc(T_ALLOCATOR* allocator, const size_t size)
{
    return gp_mem_alloc(gp_alc_cpp(allocator), size);
}

template <typename T_ALLOCATOR>
static inline void* gp_alloc_zeroes(T_ALLOCATOR* allocator, const size_t size)
{
    return gp_mem_alloc_zeroes(gp_alc_cpp(allocator), size);
}

// ---------------------------
// gp_alloc_type()

#define gp_alloc_type(/*allocator, type, count = 1*/...) \
    GP_ALLOC_TYPE_CPP(__VA_ARGS__)

// ---------------------------
// gp_dealloc() and gp_realloc()

template <typename T_ALLOCATOR>
static inline void gp_dealloc(T_ALLOCATOR* allocator, void* block)
{
    gp_mem_dealloc(gp_alc_cpp(allocator), block);
}

template <typename T, typename T_ALLOCATOR>
static inline T* gp_realloc(
    T_ALLOCATOR* allocator, T* old_block, const size_t old_size, const size_t new_size)
{
    return (T*)gp_mem_realloc(gp_alc_cpp(allocator), old_block, old_size, new_size);
}

// ----------------------------------------------------------------------------
// File

// ---------------------------
// gp_file()

static inline FILE* gp_file(const char*const path, const char*const mode)
{
    return gp_file_open(path, mode);
}

// Use this for writing to file
static inline GPString gp_file(
    GPString src, const char*const path, const char*const mode)
{
    if (gp_str_file(&src, path, mode) == 0)
        return src;
    return NULL;
}
// Use this for reading from file
static inline GPString gp_file(
    GPString* dest, const char*const path, const char*const mode)
{
    if (gp_str_file(dest, path, mode) == 0)
        return *dest;
    return NULL;
}
template <typename T_ALLOCATOR>
static inline GPString gp_file(
    T_ALLOCATOR* allocator, const char*const path, const char*const mode)
{
    GPString str = gp_str_new(gp_alc_cpp(allocator), 128, "");
    if (gp_str_file(&str, path, mode) == 0)
        return str;
    gp_str_delete(str);
    return NULL;
}

// ---------------------------
// File operations

#define gp_read_line(...)           gp_file_read_line(__VA_ARGS__)
#define gp_read_until(...)          gp_file_read_until(__VA_ARGS__)
#define gp_read_strip(...)          gp_file_read_strip(__VA_ARGS__)


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


template <typename T_alc, typename T, size_t N>
static inline T* gp_arr_new_cpp(const T_alc*const alc, const std::array<T,N>& init)
{
    GPArray(void) out = gp_arr_new(gp_alc_cpp(alc), sizeof(T), N > 4 ? N : 4);
    ((GPArrayHeader*)out - 1)->length = N;
    return (T*)memcpy(out, init.data(), sizeof(T) * N);
}
#define GP_ARR_NEW_CPP(ALC,...) gp_arr_new_cpp(ALC, \
    std::array<GP_1ST_ARG(__VA_ARGS__), GP_COUNT_ARGS(__VA_ARGS__) - 1>{GP_ALL_BUT_1ST_ARG(__VA_ARGS__)})

#define GP_DICT_CPP_2(ALC, TYPE) (TYPE*)gp_hmap(ALC, sizeof(TYPE))
#define GP_DICT_CPP_3(ALC, TYPE, DCTOR) (TYPE*)gp_hmap(ALC, sizeof(TYPE), DCTOR)
#define GP_DICT_CPP_4(ALC, TYPE, DCTOR, CAP) (TYPE*)gp_hmap(ALC, sizeof(TYPE), DCTOR, CAP)
#define GP_DICT_CPP(ALC,...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_DICT_CPP_4, GP_DICT_CPP_3, GP_DICT_CPP_2)(ALC,__VA_ARGS__)

#define GP_ALLOC_TYPE_WITH_COUNT(ALLOCATOR, TYPE, COUNT) \
    gp_alloc(ALLOCATOR, (COUNT) * sizeof(TYPE))
#define GP_ALLOC_TYPE_WOUT_COUNT(ALLOCATOR, TYPE) \
    gp_alloc(ALLOCATOR, sizeof(TYPE))
#define GP_ALLOC_TYPE_CPP(ALC, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_ALLOC_TYPE_WITH_COUNT,GP_ALLOC_TYPE_WOUT_COUNT)(ALC, __VA_ARGS__)

#else // __cplusplus, C below -------------------------------------------------

#ifdef GP_GENERIC_AVAILABLE // C11 or CompCert --------------------------------

// Constructors
#define gp_arr(...)                 GP_ARR_NEW(__VA_ARGS__)
#define gp_arr_ro(T,...)            GP_ARR_READ_ONLY(T,__VA_ARGS__)
#define gp_str(...)                 GP_STR_NEW(__VA_ARGS__)
#define gp_hmap(...)                GP_HMAP_NEW(__VA_ARGS__)
#define gp_dict(...)                GP_DICT_NEW(__VA_ARGS__)

// Bytes and strings
#define gp_equal(...)               GP_EQUAL(__VA_ARGS__)
#define gp_count(...)               GP_COUNT(__VA_ARGS__)
#define gp_codepoint_length(...)    GP_CODEPOINT_LENGTH(__VA_ARGS__)

// Strings
#define gp_repeat(...)              GP_REPEAT11(__VA_ARGS__)
#define gp_replace(...)             GP_REPLACE11(__VA_ARGS__)
#define gp_replace_all(...)         GP_REPLACE_ALL11(__VA_ARGS__)
#define gp_trim(...)                GP_TRIM11(__VA_ARGS__)
#define gp_to_upper(...)            GP_TO_UPPER11(__VA_ARGS__)
#define gp_to_lower(...)            GP_TO_LOWER11(__VA_ARGS__)
#define gp_to_valid(...)            GP_TO_VALID(__VA_ARGS__)
#define gp_capitalize(...)          GP_CAPITALIZE11(__VA_ARGS__)
#define gp_find_first(...)          GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)           GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)       GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)   GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)          GP_EQUAL_CASE(__VA_ARGS__)
#define gp_compare(...)             GP_COMPARE(__VA_ARGS__)
#define gp_codepoint_count(...)     GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)            GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
#define gp_split(...)               GP_SPLIT(__VA_ARGS__)
#define gp_join(...)                GP_JOIN11(__VA_ARGS__)
#define gp_sort(...)                GP_SORT(__VA_ARGS__)
#define gp_length(...)              gp_arr_length(__VA_ARGS__)
#define gp_capacity(...)            gp_arr_capacity(__VA_ARGS__)
#define gp_allocation(...)          gp_arr_allocation(__VA_ARGS__)
#define gp_allocator(...)           gp_arr_allocator(__VA_ARGS__)
#define gp_reserve(...)             GP_RESERVE11(__VA_ARGS__)
#define gp_copy(...)                GP_COPY11(__VA_ARGS__)
#define gp_slice(...)               GP_SLICE11(__VA_ARGS__)
#define gp_append(...)              GP_APPEND11(__VA_ARGS__)
#define gp_insert(...)              GP_INSERT11(__VA_ARGS__)

// Arrays
#define gp_push(...)                GP_PUSH(__VA_ARGS__)
#define gp_pop(...)                 GP_POP(__VA_ARGS__)
#define gp_erase(...)               GP_ERASE(__VA_ARGS__)
#define gp_map(...)                 GP_MAP11(__VA_ARGS__)
#define gp_fold(...)                GP_FOLD(__VA_ARGS__)
#define gp_foldr(...)               GP_FOLDR(__VA_ARGS__)
#define gp_filter(...)              GP_FILTER11(__VA_ARGS__)

// Dictionarys
#define gp_get(...)                 GP_GET(__VA_ARGS__)
#define gp_put(...)                 GP_PUT(__VA_ARGS__)
#define gp_remove(...)              GP_REMOVE(__VA_ARGS__)

// Memory
#define gp_alloc(...)               GP_ALLOC(__VA_ARGS__)
#define gp_alloc_type(...)          GP_ALLOC_TYPE(__VA_ARGS__)
#define gp_alloc_zeroes(...)        GP_ALLOC_ZEROES(__VA_ARGS__)
#define gp_dealloc(...)             GP_DEALLOC(__VA_ARGS__)
#define gp_realloc(...)             GP_REALLOC(__VA_ARGS__)

// File
#define gp_file(...)                GP_FILE11(__VA_ARGS__)
#define gp_read_line(...)           gp_file_read_line(__VA_ARGS__)
#define gp_read_until(...)          gp_file_read_until(__VA_ARGS__)
#define gp_read_strip(...)          gp_file_read_strip(__VA_ARGS__)

#else // C99 ------------------------------------------------------------------

// Constructors
#define gp_arr(...)                 GP_ARR_NEW(__VA_ARGS__)
#define gp_arr_ro(T,...)            GP_ARR_READ_ONLY(T,__VA_ARGS__)
#define gp_str(...)                 GP_STR_NEW(__VA_ARGS__)
#define gp_hmap(...)                GP_HMAP_NEW(__VA_ARGS__)
#define gp_dict(...)                GP_DICT_NEW(__VA_ARGS__)

// Bytes and strings
#define gp_equal(...)               GP_EQUAL(__VA_ARGS__)
#define gp_count(...)               GP_COUNT(__VA_ARGS__)
#define gp_codepoint_length(...)    GP_CODEPOINT_LENGTH(__VA_ARGS__)

// Strings
#define gp_repeat(...)              GP_REPEAT99(__VA_ARGS__)
#define gp_replace(...)             GP_REPLACE99(__VA_ARGS__)
#define gp_replace_all(...)         GP_REPLACE_ALL99(__VA_ARGS__)
#define gp_trim(...)                GP_TRIM99(__VA_ARGS__)
#define gp_to_upper(...)            GP_TO_UPPER99(__VA_ARGS__)
#define gp_to_lower(...)            GP_TO_LOWER99(__VA_ARGS__)
#define gp_to_valid(...)            GP_TO_VALID(__VA_ARGS__)
#define gp_capitalize(...)          GP_CAPITALIZE99(__VA_ARGS__)
#define gp_find_first(...)          GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)           GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)       GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)   GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)          GP_EQUAL_CASE(__VA_ARGS__)
#define gp_compare(...)             GP_COMPARE(__VA_ARGS__)
#define gp_codepoint_count(...)     GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)            GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
#define gp_split(...)               GP_SPLIT(__VA_ARGS__)
#define gp_join(...)                GP_JOIN99(__VA_ARGS__)
#define gp_sort(...)                GP_SORT(__VA_ARGS__)
#define gp_length(...)              gp_arr_length(__VA_ARGS__)
#define gp_capacity(...)            gp_arr_capacity(__VA_ARGS__)
#define gp_allocation(...)          gp_arr_allocation(__VA_ARGS__)
#define gp_allocator(...)           gp_arr_allocator(__VA_ARGS__)
#define gp_reserve(...)             GP_RESERVE99(__VA_ARGS__)
#define gp_copy(...)                GP_COPY99(__VA_ARGS__)
#define gp_slice(...)               GP_SLICE99(__VA_ARGS__)
#define gp_append(...)              GP_APPEND99(__VA_ARGS__)
#define gp_insert(...)              GP_INSERT99(__VA_ARGS__)

// Arrays
#define gp_push(...)                GP_PUSH(__VA_ARGS__)
#define gp_pop(...)                 GP_POP(__VA_ARGS__)
#define gp_erase(...)               GP_ERASE(__VA_ARGS__)
#define gp_map(...)                 GP_MAP99(__VA_ARGS__)
#define gp_fold(...)                GP_FOLD(__VA_ARGS__)
#define gp_foldr(...)               GP_FOLDR(__VA_ARGS__)
#define gp_filter(...)              GP_FILTER99(__VA_ARGS__)

// Dictionarys
#define gp_get(...)                 GP_GET(__VA_ARGS__)
#define gp_put(...)                 GP_PUT(__VA_ARGS__)
#define gp_remove(...)              GP_REMOVE(__VA_ARGS__)

// Memory
#define gp_alloc(...)               GP_ALLOC(__VA_ARGS__)
#define gp_alloc_type(...)          GP_ALLOC_TYPE(__VA_ARGS__)
#define gp_alloc_zeroes(...)        GP_ALLOC_ZEROES(__VA_ARGS__)
#define gp_dealloc(...)             GP_DEALLOC(__VA_ARGS__)
#define gp_realloc(...)             GP_REALLOC(__VA_ARGS__)

// File
#define gp_file(...)                GP_FILE99(__VA_ARGS__)
#define gp_read_line(...)           gp_file_read_line(__VA_ARGS__)
#define gp_read_until(...)          gp_file_read_until(__VA_ARGS__)
#define gp_read_strip(...)          gp_file_read_strip(__VA_ARGS__)

#endif // C macros

typedef struct gp_str_in { const uint8_t* data; const size_t length; } GPStrIn;
#if GP_GENERIC_AVAILABLE
#define GP_STR_IN(...) GP_STR_IN11(__VA_ARGS__)
#define GP_ALC(...) GP_ALC11(__VA_ARGS__)
#else
#define GP_STR_IN(...) GP_STR_IN99(__VA_ARGS__)
#define GP_ALC(...) GP_ALC99(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
//
//          C11 IMPLEMENTATIONS
//
//

#ifndef GP_USER_ALLOCATORS
typedef struct { GPAllocator alc; } GPDummyAlc; // for comma issues in GP_ALC_TYPE
#define GP_USER_ALLOCATORS GPDummyAlc
#endif

#define GP_ALC_TYPES GP_USER_ALLOCATORS, GPAllocator, GPArena
#define GP_ALC_SELECTION(T) const T*: 0, T*: 0
#define GP_ALC11(A) ((int){0} = _Generic(A, \
    GP_PROCESS_ALL_ARGS(GP_ALC_SELECTION, GP_COMMA, GP_ALC_TYPES)), \
    (const GPAllocator*)(A))

// ----------------------------------------------------------------------------
// Bytes and strings

#define GP_STR_T(S) _Generic(S, GPString: GP_STRING, char*: GP_CHAR_PTR, const char*: GP_CHAR_PTR)
GP_NONNULL_ARGS()
static inline GPStrIn gp_str_in11(const GPType T, const void*const data, const size_t length)
{
    switch (T)
    {
        case GP_STRING:   return (GPStrIn){ data, gp_arr_length(data) };
        case GP_CHAR_PTR: return (GPStrIn){ data, strlen(data) };
        default: break;
    }
    return (GPStrIn){ data, length };
}
#define GP_STR_IN11_1(S)    gp_str_in11(GP_STR_T(S), S, 0)
#define GP_STR_IN11_2(S, L) gp_str_in11(GP_PTR,      S, L)
#define GP_STR_IN11(...) GP_OVERLOAD2(__VA_ARGS__, GP_STR_IN11_2, GP_STR_IN11_1)(__VA_ARGS__)

static inline size_t gp_length_in11(const GPType T_unused, const size_t length, const size_t unused)
{
    (void)T_unused; (void)unused;
    return length;
}
 #define GP_STR_OR_LEN1(A) _Generic(A, \
     GPString: gp_str_in11, char*: gp_str_in11, const char*: gp_str_in11, default: gp_length_in11) \
     (_Generic(A, GPString: GP_STRING, char*: GP_CHAR_PTR, const char*: GP_CHAR_PTR, default: -1), A, 0)

#define GP_STR_OR_LEN(...) GP_OVERLOAD2(__VA_ARGS__, GP_STR_IN11_2, GP_STR_OR_LEN1)(__VA_ARGS__)

GP_NONNULL_ARGS_AND_RETURN
static inline GPString gp_str_repeat_new(const void* alc, const size_t count, GPStrIn in)
{
    GPString out = gp_str_new(alc, count * in.length, "");
    ((GPStringHeader*)out - 1)->length = gp_bytes_repeat(out, count, in.data, in.length);
    return out;
}
GP_NONNULL_ARGS()
static inline void gp_str_repeat_str(GPString* dest, const size_t count, GPStrIn in)
{
    gp_str_repeat(dest, count, in.data, in.length);
}
#define GP_REPEAT_SELECTION(T) const T*: gp_str_repeat_new, T*: gp_str_repeat_new
#define GP_REPEAT11(A, COUNT, ...) _Generic(A, GPString*: gp_str_repeat_str, \
    GP_PROCESS_ALL_ARGS(GP_REPEAT_SELECTION, GP_COMMA, GP_ALC_TYPES))(A, COUNT, GP_STR_IN(__VA_ARGS__))

GP_NONNULL_ARGS()
static inline void gp_replace11(GPString* hay, GPStrIn ndl, GPStrIn repl, const size_t start)
{
    gp_str_replace(hay, ndl.data, ndl.length, repl.data, repl.length, start);
}
GP_NONNULL_ARGS_AND_RETURN
GPString gp_replace_new(const GPAllocator* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl, size_t start);
static inline GPString gp_replace_new4(const void* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl)
{
    return gp_replace_new(alc, hay, ndl, repl, 0);
}
#define GP_REPLACE11_3(HAY, NDL, REPL) gp_replace11(HAY, GP_STR_IN11(NDL), GP_STR_IN11(REPL), 0)
#define GP_REPLACE_SELECTION(T) const T*: gp_replace_new4, T*: gp_replace_new4
#define GP_REPLACE11_4(A, B, C, D) _Generic(A, GPString*: gp_replace11, \
    GP_PROCESS_ALL_ARGS(GP_REPLACE_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A, GP_STR_IN11(B), GP_STR_IN(C), GP_STR_OR_LEN(D))
#define GP_REPLACE11_5(ALC, HAY, NDL, REPL, START) gp_replace_new( \
    GP_ALC(ALC), GP_STR_IN11(HAY), GP_STR_IN11(NDL), GP_STR_IN11(REPL), START)
#define GP_REPLACE11(A,B,...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_REPLACE11_5, GP_REPLACE11_4, GP_REPLACE11_3)(A,B,__VA_ARGS__)

GP_NONNULL_ARGS()
static inline size_t gp_replace_all11(GPString* hay, GPStrIn ndl, GPStrIn repl)
{
    return gp_str_replace_all(hay, ndl.data, ndl.length, repl.data, repl.length);
}
GP_NONNULL_ARGS_AND_RETURN
GPString gp_reaplce_all_new(const void* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl);
#define GP_REPLACE_ALL11_3(HAY, NDL, REPL) gp_replace_all11(HAY, GP_STR_IN11(NDL), GP_STR_IN11(REPL))
#define GP_REPLACE_ALL11_4(ALC, HAY, NDL, REPL) gp_replace_all_new( \
    GP_ALC(ALC), GP_STR_IN11(HAY), GP_STR_IN11(NDL), GP_STR_IN11(REPL))
#define GP_REPLACE_ALL11(A,B,...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_REPLACE_ALL11_4, GP_REPLACE_ALL11_3)(A,B,__VA_ARGS__)

GP_NONNULL_ARGS(1) GP_NONNULL_RETURN
GPString gp_str_trim_new(const void* alc, GPStrIn str, const char* char_set, int flags);
GP_NONNULL_ARGS()
static inline void gp_str_trim2(GPString* str, const char*const char_set)
{
    gp_str_trim(str, char_set, 'l' | 'r');
}
GP_NONNULL_ARGS_AND_RETURN
static inline GPString gp_str_trim_new2(const void*const alc, GPStrIn str)
{
    return gp_str_trim_new(alc, str, NULL, 'l' | 'r');
}
GP_NONNULL_ARGS_AND_RETURN
static inline GPString gp_str_trim_new3(const void*const alc, GPStrIn str, const char*const char_set)
{
    return gp_str_trim_new(alc, str, char_set, 'l' | 'r');
}
#define GP_TRIM2_SELECTION(T) T*: gp_str_trim_new2, const T*: gp_str_trim_new2
#define GP_TRIM3_SELECTION(T) T*: gp_str_trim_new3, const T*: gp_str_trim_new3
#define GP_TRIM1(STR) gp_str_trim(STR, NULL, 'l' | 'r')
#define GP_TRIM11_2(A, B) _Generic(A, GPString*: gp_str_trim2, \
    GP_PROCESS_ALL_ARGS(GP_TRIM2_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A, _Generic(A, GPString*: B, default: GP_STR_IN(B)))
#define GP_TRIM11_3(A, B, C) _Generic(A, GPString*: gp_str_trim, \
    GP_PROCESS_ALL_ARGS(GP_TRIM3_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A, _Generic(A, GPString*: B, default: GP_STR_IN(B)), C)
#define GP_TRIM4(ALC, STR, CHARS, FLAGS) gp_str_trim_new(GP_ALC(ALC), GP_STR_IN(STR), CHARS, FLAGS)
#define GP_TRIM11(...) GP_OVERLOAD4(__VA_ARGS__, GP_TRIM4, GP_TRIM11_3, GP_TRIM11_2, GP_TRIM1)(__VA_ARGS__)

#define GP_STR_OR_LOCALE(A, B) _Generic(A, GPString*: B, default: GP_STR_IN(B))

#define GP_TO_UPPER_SELECTION(T) T*: gp_to_upper_new, const T*: gp_to_upper_new
#define GP_TO_UPPER11_2(A, B) _Generic(A, GPString*: gp_str_to_upper_full, \
    GP_PROCESS_ALL_ARGS(GP_TO_UPPER_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    ((void*)(A), GP_STR_OR_LOCALE(A, B))
#define GP_TO_UPPER11(...) GP_OVERLOAD3(__VA_ARGS__, GP_TO_UPPER3, GP_TO_UPPER11_2, GP_TO_UPPER1)(__VA_ARGS__)

#define GP_TO_LOWER_SELECTION(T) T*: gp_to_lower_new, const T*: gp_to_lower_new
#define GP_TO_LOWER11_2(A, B) _Generic(A, GPString*: gp_str_to_lower_full, \
    GP_PROCESS_ALL_ARGS(GP_TO_LOWER_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    ((void*)(A), GP_STR_OR_LOCALE(A, B))
#define GP_TO_LOWER11(...) GP_OVERLOAD3(__VA_ARGS__, GP_TO_LOWER3, GP_TO_LOWER11_2, GP_TO_LOWER1)(__VA_ARGS__)

#define GP_CAPITALIZE_SELECTION(T) T*: gp_capitalize_new, const T*: gp_capitalize_new
#define GP_CAPITALIZE11_2(A, B) _Generic(A, GPString*: gp_str_capitalize, \
    GP_PROCESS_ALL_ARGS(GP_CAPITALIZE_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    ((void*)(A), GP_STR_OR_LOCALE(A, B))
#define GP_CAPITALIZE11(...) GP_OVERLOAD3(__VA_ARGS__, GP_CAPITALIZE3, GP_CAPITALIZE11_2, GP_CAPITALIZE1)(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Strings and arrays

GPString gp_join_new(const GPAllocator* allocator, const GPArray(GPString) srcs, const char* separator);
#define GP_JOIN_SELECTION(T) T*: gp_join_new, const T*: gp_join_new
#define GP_JOIN11_2(A, STRS) _Generic(A, GPString*: gp_str_join, \
    GP_PROCESS_ALL_ARGS(GP_JOIN_SELECTION, GP_COMMA, GP_ALC_TYPES))((void*)(A), STRS, "")
#define GP_JOIN11_3(A, STRS, SEP) _Generic(A, GPString*: gp_str_join, \
    GP_PROCESS_ALL_ARGS(GP_JOIN_SELECTION, GP_COMMA, GP_ALC_TYPES))((void*)(A), STRS, SEP)
#define GP_JOIN11(A,...) GP_OVERLOAD2(__VA_ARGS__, GP_JOIN11_3, GP_JOIN11_2)(A,__VA_ARGS__)

GP_NONNULL_ARGS()
static inline void gp_str_reserve11(const size_t unused, GPString* str, const size_t size)
{
    (void)unused;
    gp_str_reserve(str, size);
}
#define GP_RESERVE11(A, SIZE) _Generic(A, \
    GPString: gp_str_reserve11, default: gp_arr_reserve)(sizeof**(A), A, SIZE)

typedef GPStrIn GPArrIn;
#define GP_ARR_T(A) _Generic(A, \
    GPString: GP_STRING, char*: GP_CHAR_PTR, const char*: GP_CHAR_PTR, default: GP_PTR)
GP_NONNULL_ARGS()
static inline GPStrIn gp_arr_in11(const GPType T, const void*const data, const size_t length)
{
    if (length != SIZE_MAX)
        return (GPArrIn){ data, length };
    else if (T == GP_CHAR_PTR)
        return (GPArrIn){ data, strlen(data) };
    return (GPArrIn){ data, gp_arr_length(data) };
}
#define GP_ARR_IN11_1(A)    gp_arr_in11(GP_ARR_T(A), A, SIZE_MAX)
#define GP_ARR_IN11_2(A, L) gp_arr_in11(GP_PTR,      A, L)
#define GP_ARR_IN11(...) GP_OVERLOAD2(__VA_ARGS__, GP_ARR_IN11_2, GP_ARR_IN11_1)(__VA_ARGS__)

typedef struct { int dummy; } GPDummyType;
#define GP_TYPE_CHECK_SELECTION(T) T*: &(GPDummyType){0}, const T*: &(GPDummyType){0}
#define GP_TYPE_CHECK(PA, B) *_Generic(PA, \
    GP_PROCESS_ALL_ARGS(GP_TYPE_CHECK_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: *(PA)) = *_Generic(PA, \
        GP_PROCESS_ALL_ARGS(GP_TYPE_CHECK_SELECTION, GP_COMMA, GP_ALC_TYPES), \
        GPString*: _Generic(B, char*: (GPString)(B), const char*: (GPString)(B), default: B), \
        default: B)

GP_NONNULL_ARGS()
static inline void gp_str_copy11(const size_t unused, GPString* dest, GPStrIn src)
{
    (void)unused;
    gp_str_copy(dest, src.data, src.length);
}
GP_NONNULL_ARGS()
static inline void gp_arr_copy11(const size_t elem_size, void* _dest, GPArrIn src)
{
    GPArray(void)* dest = _dest;
    *dest = gp_arr_copy(elem_size, *dest, src.data, src.length);
}
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_copy_new11(const size_t elem_size, const void* alc, GPArrIn src)
{
    void* out = gp_arr_new(alc, elem_size, src.length + sizeof"");
    ((GPArrayHeader*)out - 1)->length = src.length;
    return memcpy(out, src.data, src.length * elem_size);
}
#define GP_COPY_SELECTION(T) T*: gp_arr_copy_new11, const T*: gp_arr_copy_new11
#define GP_COPY11(A,...) _Generic((GP_TYPE_CHECK(A, GP_1ST_ARG(__VA_ARGS__)), A), \
    GPString*: gp_str_copy11, \
    GP_PROCESS_ALL_ARGS(GP_COPY_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_arr_copy11) \
    (GP_SIZEOF_TYPEOF(*(GP_1ST_ARG(__VA_ARGS__))), A, GP_ARR_IN11(__VA_ARGS__))

GP_NONNULL_ARGS()
static inline void gp_str_slice11(
    const size_t unused, GPString* pdest, const void* src, const size_t start, const size_t end)
{
    (void)unused;
    gp_str_slice(pdest, src, start, end);
}
GP_NONNULL_ARGS()
static inline void gp_arr_slice11(
    const size_t elem_size, void*_pdest, const void* src, const size_t start, const size_t end)
{
    GPArray(void)* pdest = _pdest;
    *pdest = gp_arr_slice(elem_size, *pdest, src, start, end);
}
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_slice_new(
    const size_t elem_size, const void* alc, const void* src, const size_t start, const size_t end)
{
    void* out = gp_arr_new(alc, elem_size, end - start + sizeof"");
    ((GPArrayHeader*)out - 1)->length = end - start;
    return memcpy(out, (uint8_t*)src + start * elem_size, (end - start) * elem_size);
}
#define GP_SLICE_SELECTION(T) T*: gp_arr_slice_new, const T*: gp_arr_slice_new
#define GP_SLICE_INPUT11(A, SRC, START, END) _Generic((GP_TYPE_CHECK(A, SRC), A), \
    GPString*: gp_str_slice11, \
    GP_PROCESS_ALL_ARGS(GP_SLICE_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_arr_slice11) \
    (GP_SIZEOF_TYPEOF(*(SRC)), A, SRC, START, END)
#define GP_SLICE11(A, B,...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_SLICE_INPUT11, GP_SLICE_WOUT_INPUT99)(A, B,__VA_ARGS__)

GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_src_in11(const GPType T_unused, const void*const data, const size_t unused)
{
    (void)T_unused; (void)unused;
    return (void*)data;
}
#define GP_SRC_IN_SELECTION(T) T*: gp_arr_in11, const T*: gp_arr_in11
#define GP_SRC_IN11(A, SRC) _Generic(A, \
    GP_PROCESS_ALL_ARGS(GP_SRC_IN_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_src_in11)(GP_ARR_T(SRC), SRC, SIZE_MAX)

static inline size_t gp_len_in11(const GPType T_unused, const size_t length, const size_t unused)
{
    (void)T_unused; (void)unused;
    return length;
}
#define GP_LEN_IN_SELECTION(T) T*: gp_arr_in11, const T*: gp_arr_in11
#define GP_SRC_OR_LEN11(A, SRC_OR_LEN) _Generic(A, \
    GP_PROCESS_ALL_ARGS(GP_LEN_IN_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_len_in11)(GP_ARR_T(SRC_OR_LEN), SRC_OR_LEN, SIZE_MAX)

GP_NONNULL_ARGS()
static inline void gp_str_append11(const size_t unused, GPString* pstr, GPStrIn src)
{
    (void)unused;
    gp_str_append(pstr, src.data, src.length);
}
GP_NONNULL_ARGS()
static inline void gp_arr_append11(const size_t elem_size, void*_pdest, GPArrIn src)
{
    GPArray(void)* pdest = _pdest;
    *pdest = gp_arr_append(elem_size, *pdest, src.data, src.length);
}
GP_NONNULL_ARGS()
static inline void gp_str_append11_4(const size_t unused, GPString* dest, void* src, const size_t src_len)
{
    (void)unused;
    gp_str_append(dest, src, src_len);
}
GP_NONNULL_ARGS()
static inline void gp_arr_append11_4(const size_t elem_size, void*_pdest, void* src, const size_t src_len)
{
    GPArray(void)* pdest = _pdest;
    *pdest = gp_arr_append(elem_size, *pdest, src, src_len);
}
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_append_new11(
    const size_t elem_size, const void* alc, GPArrIn src1, GPArrIn src2)
{
    void* out = gp_arr_new(alc, elem_size, src1.length + src2.length + sizeof"");
    memcpy(out, src1.data, src1.length * elem_size);
    memcpy((uint8_t*)out + src1.length * elem_size, src2.data, src2.length * elem_size);
    ((GPArrayHeader*)out - 1)->length = src1.length + src2.length;
    return out;
}
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_append_new11_5(const size_t elem_size,
    const void* alc, GPArrIn src1, const void*const src2, const size_t src2_len)
{
    return gp_arr_append_new11(elem_size, alc, src1, (GPArrIn){ src2, src2_len });
}
#define GP_APPEND11_2(DEST, SRC) _Generic((GP_TYPE_CHECK(DEST, SRC), DEST), \
    GPString*: gp_str_append11, default: gp_arr_append11) \
    (sizeof(**(DEST)), DEST, GP_ARR_IN11(SRC))
#define GP_APPEND_SELECTION(T) T*: gp_arr_append_new11, const T*: gp_arr_append_new11
#define GP_APPEND11_3(A, SRC, B) _Generic(A, GPString*: gp_str_append11_4, \
    GP_PROCESS_ALL_ARGS(GP_APPEND_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_arr_append11_4) \
    (sizeof*(SRC), A, GP_SRC_IN11(A, SRC), GP_SRC_OR_LEN11(A, B))
#define GP_APPEND11_4(ALC, SRC1, SRC2, SRC2_LEN) \
    gp_arr_append_new11_5(sizeof*(SRC2), GP_ALC(ALC), GP_ARR_IN11(SRC1), SRC2, SRC2_LEN)
#define GP_APPEND11_5(ALC, SRC1, SRC1_LEN, SRC2, SRC2_LEN) \
    gp_arr_append_new11(sizeof(*(SRC1)), GP_ALC(ALC), \
        GP_ARR_IN11(SRC1, SRC1_LEN), GP_ARR_IN11(SRC2, SRC2_LEN))
#define GP_APPEND11(A,...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_APPEND11_5, GP_APPEND11_4, GP_APPEND11_3, GP_APPEND11_2)(A,__VA_ARGS__)

GP_NONNULL_ARGS()
static inline void gp_str_insert11(const size_t unused, GPString* pstr, const size_t pos, GPStrIn src)
{
    (void)unused;
    gp_str_insert(pstr, pos, src.data, src.length);
}
GP_NONNULL_ARGS()
static inline void gp_arr_insert11(const size_t elem_size, void*_pdest, const size_t pos, GPArrIn src)
{
    GPArray(void)* pdest = _pdest;
    *pdest = gp_arr_insert(elem_size, *pdest, pos, src.data, src.length);
}
GP_NONNULL_ARGS()
static inline void gp_str_insert11_4(
    const size_t unused, GPString* dest, const size_t pos, void* src, const size_t src_len)
{
    (void)unused;
    gp_str_insert(dest, pos, src, src_len);
}
GP_NONNULL_ARGS()
static inline void gp_arr_insert11_4(
    const size_t elem_size, void*_pdest, const size_t pos, void* src, const size_t src_len)
{
    GPArray(void)* pdest = _pdest;
    *pdest = gp_arr_insert(elem_size, *pdest, pos, src, src_len);
}
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_insert_new11(
    const size_t elem_size, const void* alc, const size_t pos, GPArrIn src1, GPArrIn src2)
{
    void* out = gp_arr_new(alc, elem_size, src1.length + src2.length);
    memcpy(out, src1.data, pos * elem_size);
    memcpy((uint8_t*)out + pos * elem_size, src2.data, src2.length * elem_size);
    memcpy((uint8_t*)out + (pos + src2.length) * elem_size,
        (uint8_t*)src1.data + pos * src1.length,
        (src1.length - pos) * elem_size);
    ((GPArrayHeader*)out - 1)->length = src1.length + src2.length;
    return out;
}
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_insert_new11_5(const size_t elem_size,
    const void* alc, const size_t pos, GPArrIn src1, const void*const src2, const size_t src2_len)
{
    return gp_arr_insert_new11(elem_size, alc, pos, src1, (GPArrIn){ src2, src2_len });
}
#define GP_INSERT11_3(DEST, POS, SRC) _Generic((GP_TYPE_CHECK(DEST, SRC), DEST), \
    GPString*: gp_str_insert11, default: gp_arr_insert11) \
    (sizeof(**(DEST)), DEST, POS, GP_ARR_IN11(SRC))
#define GP_INSERT_SELECTION(T) T*: gp_arr_insert_new11, const T*: gp_arr_insert_new11
#define GP_INSERT11_4(A, POS, SRC, B) _Generic(A, GPString*: gp_str_insert11_4, \
    GP_PROCESS_ALL_ARGS(GP_INSERT_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: gp_arr_insert11_4) \
    (sizeof*(SRC), A, POS, GP_SRC_IN11(A, SRC), GP_SRC_OR_LEN11(A, B))
#define GP_INSERT11_5(ALC, POS, SRC1, SRC2, SRC2_LEN) \
    gp_arr_insert_new11_5(sizeof*(SRC2), GP_ALC(ALC), POS, GP_ARR_IN11(SRC1), SRC2, SRC2_LEN)
#define GP_INSERT11_6(ALC, POS, SRC1, SRC1_LEN, SRC2, SRC2_LEN) \
    gp_arr_insert_new11(sizeof(*(SRC1)), GP_ALC(ALC), POS, \
        GP_ARR_IN11(SRC1, SRC1_LEN), GP_ARR_IN11(SRC2, SRC2_LEN))
#define GP_INSERT11(A,POS,...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_INSERT11_6, GP_INSERT11_5, GP_INSERT11_4, GP_INSERT11_3)(A,POS,__VA_ARGS__)

// ----------------------------------------------------------------------------
// Arrays

#define GP_ARR_TYPE_CHECK(PA, B) *_Generic(PA, \
    GP_PROCESS_ALL_ARGS(GP_TYPE_CHECK_SELECTION, GP_COMMA, GP_ALC_TYPES), \
    default: *(PA)) = *_Generic(PA, \
        GP_PROCESS_ALL_ARGS(GP_TYPE_CHECK_SELECTION, GP_COMMA, GP_ALC_TYPES), \
        default: B)

GP_NONNULL_ARGS()
static inline GPArrIn gp_arr_in11_1(const GPArray(void) arr)
{
    return (GPArrIn){ arr, gp_arr_length(arr) };
}

GP_NONNULL_ARGS()
static inline void gp_arr_map11(
    const size_t elem_size, void*_parr, GPArrIn src, void(*const f)(void*,const void*))
{
    GPArray(void)* parr = _parr;
    parr = gp_arr_map(elem_size, *parr, src.data, src.length, f);
}
GP_NONNULL_ARGS_AND_RETURN
static inline GPArray(void) gp_arr_map_new11(
    const size_t elem_size, const GPAllocator*const alc, GPArrIn src, void(*const f)(void*, const void*))
{
    GPArray(void) out = gp_arr_new(alc, elem_size, src.length);
    return out = gp_arr_map(elem_size, out, src.data, src.length, f);
}

#define GP_MAP_SELECTION(T) T*: gp_arr_map_new11, const T*: gp_arr_map_new11
#define GP_MAP11_3(A, SRC, F) _Generic((GP_ARR_TYPE_CHECK(A,SRC), A), \
    GP_PROCESS_ALL_ARGS(GP_MAP_SELECTION, GP_COMMA, GP_ALC_TYPES), default: gp_arr_map11) \
    (GP_SIZEOF_TYPEOF(((F)((void*)(SRC),SRC), *(SRC))), A, gp_arr_in11_1(SRC), (void(*)(void*,const void*))(F))
#define GP_MAP11_4(A, SRC, SRC_LEN, F) _Generic((GP_ARR_TYPE_CHECK(A,SRC), A), \
    GP_PROCESS_ALL_ARGS(GP_MAP_SELECTION, GP_COMMA, GP_ALC_TYPES), default: gp_arr_map11) \
    (GP_SIZEOF_TYPEOF(((F)((void*)(SRC),SRC), *(SRC))), A, (GPArrIn){(uint8_t*)(SRC),SRC_LEN}, (void(*)(void*,const void*))(F))
#define GP_MAP11(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_MAP11_4, GP_MAP11_3, GP_MAP99_2)(A,__VA_ARGS__)

GP_NONNULL_ARGS()
static inline void gp_arr_filter11(
    const size_t elem_size, void*_parr, GPArrIn src, bool(*const f)(const void*))
{
    GPArray(void)* parr = _parr;
    parr = gp_arr_filter(elem_size, *parr, src.data, src.length, f);
}
GP_NONNULL_ARGS_AND_RETURN
static inline GPArray(void) gp_arr_filter_new11(
    const size_t elem_size, const GPAllocator*const alc, GPArrIn src, bool(*const f)(const void*))
{
    GPArray(void) out = gp_arr_new(alc, elem_size, src.length);
    return out = gp_arr_filter(elem_size, out, src.data, src.length, f);
}
#define GP_FILTER_SELECTION(T) T*: gp_arr_filter_new11, const T*: gp_arr_filter_new11
#define GP_FILTER11_3(A, SRC, F) _Generic((GP_ARR_TYPE_CHECK(A,SRC), A), \
    GP_PROCESS_ALL_ARGS(GP_FILTER_SELECTION, GP_COMMA, GP_ALC_TYPES), default: gp_arr_filter11) \
    (GP_SIZEOF_TYPEOF(((bool){0} = (F)(SRC), *(SRC))), A, gp_arr_in11_1(SRC), (bool(*)(const void*))(F))
#define GP_FILTER11_4(A, SRC, SRC_LEN, F) _Generic((GP_ARR_TYPE_CHECK(A,SRC), A), \
    GP_PROCESS_ALL_ARGS(GP_FILTER_SELECTION, GP_COMMA, GP_ALC_TYPES), default: gp_arr_filter11) \
    (GP_SIZEOF_TYPEOF(((bool){0} = (F)(SRC), *(SRC))), A, (GPArrIn){(uint8_t*)(SRC),SRC_LEN}, (bool(*)(const void*))(F))
#define GP_FILTER11(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_FILTER11_4, GP_FILTER11_3, GP_FILTER99_2)(A,__VA_ARGS__)
#define GP_FILTER11(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_FILTER11_4, GP_FILTER11_3, GP_FILTER99_2)(A,__VA_ARGS__)

GP_NONNULL_ARGS()
static inline GPString gp_file_from_str11(GPString src, const char*const path, const char*const mode)
{
    if (gp_str_file(&src, path, mode) == 0)
        return src;
    return NULL;
}
GP_NONNULL_ARGS()
static inline GPString gp_file_to_str11(GPString* dest, const char*const path, const char*const mode)
{
    if (gp_str_file(dest, path, mode) == 0)
        return *dest;
    return NULL;
}
GP_NONNULL_ARGS()
static inline GPString gp_file_to_new_str11(const void* alc, const char*const path, const char*const mode)
{
    GPString str = gp_str_new(alc, 128, "");
    if (gp_str_file(&str, path, mode) == 0)
        return str;
    gp_str_delete(str);
    return NULL;
}
#define GP_FILE_SELECTION(T) T*: gp_file_to_new_str11, const T*: gp_file_to_new_str11
#define GP_FILE11_3(A, ...) _Generic(A, \
    GPString: gp_file_from_str11, GPString*: gp_file_to_str11, \
    GP_PROCESS_ALL_ARGS(GP_FILE_SELECTION, GP_COMMA, GP_ALC_TYPES)) \
    (A,__VA_ARGS__)
#define GP_FILE11(A,...) GP_OVERLOAD2(__VA_ARGS__, GP_FILE11_3, GP_FILE99_2)(A,__VA_ARGS__)

// ----------------------------------------------------------------------------
//
//          C99 IMPLEMENTATIONS
//
//

#if __GNUC__ && !defined(GP_PEDANTIC)
// Suppress suspicious usage of sizeof warning.
#define GP_SIZEOF_TYPEOF(...) sizeof(typeof(__VA_ARGS__))
#else
#define GP_SIZEOF_TYPEOF(...) sizeof(__VA_ARGS__)
#endif

#define GP_ALC99(A) ((const GPAllocator*)(A))

// ----------------------------------------------------------------------------
// Constructors

inline GPArray(void) gp_arr99(const GPAllocator* alc,
    const size_t elem_size, const void*const init, const size_t init_length)
{
    GPArray(void) out = gp_arr_new(alc, elem_size, init_length > 4 ? init_length : 4);
    ((GPArrayHeader*)out - 1)->length = init_length;
    return memcpy(out, init, elem_size * init_length);
}
#define GP_ARR_NEW(ALC, TYPE, ...) (TYPE*)gp_arr99( \
    GP_ALC(ALC), \
    sizeof(TYPE), \
    (TYPE[]){(TYPE){0},__VA_ARGS__} + 1, \
    sizeof((TYPE[]){(TYPE){0},__VA_ARGS__}) / sizeof(TYPE) - 1)

#if __GNUC__ && !defined(GP_PEDANTIC)
#define GP_ARR_READ_ONLY(T, ...) (T const *)({ \
    static const struct GP_C99_UNIQUE_STRUCT(__LINE__) { \
        GPArrayHeader header; T data[GP_COUNT_ARGS(__VA_ARGS__)]; \
    }_gp_arr_ro = {.header = { \
        .length = GP_COUNT_ARGS(__VA_ARGS__), .capacity = GP_COUNT_ARGS(__VA_ARGS__), \
        .allocator = NULL, .allocation = NULL \
    }, .data = {__VA_ARGS__}}; \
    _gp_arr_ro.data; \
})
#else
#define GP_ARR_READ_ONLY(T, ...) \
    (T const *)(gp_arr_on_stack(NULL, GP_COUNT_ARGS(__VA_ARGS__), T, __VA_ARGS__))
#endif

#define GP_STR_NEW1(ALC)            gp_str_new(GP_ALC(ALC), 16, "")
#define GP_STR_NEW2(ALC, INIT)      gp_str_new(GP_ALC(ALC), 16, INIT)
#define GP_STR_NEW3(ALC, CAP, INIT) gp_str_new(GP_ALC(ALC), CAP, INIT)
#define GP_STR_NEW(...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_STR_NEW3, GP_STR_NEW2, GP_STR_NEW1)(__VA_ARGS__)

#define GP_HMAP1(ALC) gp_hash_map_new(GP_ALC(ALC), NULL)
#define GP_HMAP2(ALC, ELEM_SIZE) \
    gp_hash_map_new(GP_ALC(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = 0, .destructor = NULL})
#define GP_HMAP3(ALC, ELEM_SIZE, DCTOR) \
    gp_hash_map_new(GP_ALC(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = 0, .destructor = (void(*)(void*))(DCTOR)})
#define GP_HMAP4(ALC, ELEM_SIZE, DCTOR, CAP) \
    gp_hash_map_new(GP_ALC(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = CAP, .destructor = (void(*)(void*))(DCTOR)})
#define GP_HMAP_NEW(...) GP_OVERLOAD4(__VA_ARGS__, GP_HMAP4, GP_HMAP3, GP_HMAP2, GP_HMAP1)(__VA_ARGS__)

#define GP_DICT2(ALC, TYPE) (TYPE*) \
    gp_hash_map_new(GP_ALC(ALC), &(GPMapInitializer){ \
        .element_size = sizeof(TYPE), .capacity = 0, .destructor = NULL})
#define GP_DICT3(ALC, TYPE, DCTOR) (TYPE*) \
    gp_hash_map_new(GP_ALC(ALC), &(GPMapInitializer){ \
        .element_size = GP_SIZEOF_TYPEOF((DCTOR)((TYPE*)0), (TYPE){0}), .capacity = 0, .destructor = (void(*)(void*))(DCTOR)})
#define GP_DICT4(ALC, TYPE, DCTOR, CAP) (TYPE*) \
    gp_hash_map_new(GP_ALC(ALC), &(GPMapInitializer){ \
        .element_size = GP_SIZEOF_TYPEOF((DCTOR)((TYPE*)0), (TYPE){0}), .capacity = CAP, .destructor = (void(*)(void*))(DCTOR)})
#define GP_DICT_NEW(A,...) GP_OVERLOAD3(__VA_ARGS__, GP_DICT4, GP_DICT3, GP_DICT2)(A, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Bytes and strings

inline GPStrIn gp_str_in99(const void* data, const size_t length)
{
    return (GPStrIn) {
        .data   = data,
        .length = length != SIZE_MAX ? length : gp_arr_length(data)
    };
}
#define GP_STR_IN1(A) gp_str_in99( \
    (void*)(A), #A[0] == '"' ? GP_SIZEOF_TYPEOF(A) - sizeof "" : SIZE_MAX)
#define GP_STR_IN99(...) GP_OVERLOAD2(__VA_ARGS__, gp_str_in99, GP_STR_IN1)(__VA_ARGS__)

inline bool gp_equal99(const GPString a, GPStrIn b) {
    return gp_bytes_equal(a, gp_str_length(a), b.data, b.length);
}
#define GP_EQUAL2(A, B)               gp_equal99(A, GP_STR_IN(B))
#define GP_EQUAL3(A, B, B_LENGTH)     gp_str_equal(A, B, B_LENGTH)
#define GP_EQUAL4(A, A_LEN, B, B_LEN) gp_bytes_equal(A, A_LEN, B, B_LEN)
#define GP_EQUAL(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_EQUAL4, GP_EQUAL3, GP_EQUAL2)(A, __VA_ARGS__)

inline size_t gp_count99(GPStrIn haystack, GPStrIn needle) {
    return gp_bytes_count(haystack.data, haystack.length, needle.data, needle.length);
}
#define GP_COUNT2(A, B)       gp_count99(GP_STR_IN(A), GP_STR_IN(B))
#define GP_COUNT3(A, B, C)    gp_count99(GP_STR_IN(A), GP_STR_IN(B, C))
#define GP_COUNT4(A, B, C, D) gp_count99(GP_STR_IN(A, B), GP_STR_IN(C, D))
#define GP_COUNT(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_COUNT4, GP_COUNT3, GP_COUNT2)(A, __VA_ARGS__)

#define GP_CODEPOINT_LENGTH1(PTR)    gp_utf8_codepoint_length(PTR, 0)
#define GP_CODEPOINT_LENGTH2(STR, I) gp_utf8_codepoint_length(STR, I)
#define GP_CODEPOINT_LENGTH(...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_CODEPOINT_LENGTH2, GP_CODEPOINT_LENGTH1)(__VA_ARGS__)

// ----------------------------------------------------------------------------
// String

inline GPString gp_repeat99(
    const size_t a_size, const void* a, const size_t count, GPStrIn in)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_repeat((GPString*)a, count, in.data, in.length);
        return *(GPString*)a;
    }
    GPString out = gp_str_new(a, count * in.length, "");
    gp_str_repeat(&out, count, in.data, in.length);
    return out;
}
#define GP_REPEAT99(A, COUNT, ...) gp_repeat99(GP_SIZEOF_TYPEOF(*(A)), A, COUNT, GP_STR_IN99(__VA_ARGS__))

GPString gp_replace99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d,
    const size_t start);
#define GP_REPLACE99_3(HAY, NDL, REPL) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(HAY)), HAY, GP_STR_IN99(NDL), GP_STR_IN99(REPL), GP_STR_IN99(NULL, 0), 0)
#define GP_REPLACE99_4(A, B, C, D) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(A)), A, GP_STR_IN99(B), GP_STR_IN99(C), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? \
        GP_STR_IN99(NULL, 0) : GP_STR_IN99(D), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? \
        (uintptr_t)(D) : 0)
#define GP_REPLACE99_5(ALC, HAY, NDL, REPL, START) gp_replace99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN99(HAY), GP_STR_IN99(NDL), GP_STR_IN99(REPL), START)
#define GP_REPLACE99(A, B, ...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_REPLACE99_5, GP_REPLACE99_4, GP_REPLACE99_3)(A, B, __VA_ARGS__)

GPString gp_replace_all99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d);
#define GP_REPLACE_ALL99_3(HAY, NDL, REPL) gp_replace_all99( \
    GP_SIZEOF_TYPEOF(*(HAY)), HAY, GP_STR_IN99(NDL), GP_STR_IN99(REPL), GP_STR_IN99(NULL, 0))
#define GP_REPLACE_ALL99_4(ALC, HAY, NDL, REPL) gp_replace_all99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN99(HAY), GP_STR_IN99(NDL), GP_STR_IN99(REPL))
#define GP_REPLACE_ALL99(A, B, ...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_REPLACE_ALL99_4, GP_REPLACE_ALL99_3)(A, B, __VA_ARGS__)

GPString gp_trim99(
    const size_t a_size, const void* a, GPStrIn b, const char* char_set, int flags);
#define GP_TRIM99_1(STR) gp_str_trim(STR, NULL, 'l' | 'r')
#define GP_TRIM99_2(A, B) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(A)), A, \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? GP_STR_IN99(NULL, 0) : GP_STR_IN99(B), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (char*)(B) : NULL, \
    'l' | 'r')
#define GP_TRIM99_3(A, B, C) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(A)), A, \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? GP_STR_IN99(NULL, 0) : GP_STR_IN99(B), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (char*)(B) : (char*)(C), \
    GP_SIZEOF_TYPEOF(*(A)) < sizeof(GPAllocator) ? (intptr_t)(C) : 'l' | 'r')
#define GP_TRIM99_4(ALC, STR, CHARS, FLAGS) gp_trim99( \
    GP_SIZEOF_TYPEOF(*(ALC)), ALC, GP_STR_IN99(STR), CHARS, FLAGS)
#define GP_TRIM99(...) \
    GP_OVERLOAD4(__VA_ARGS__, GP_TRIM99_4, GP_TRIM99_3, GP_TRIM99_2, GP_TRIM99_1)(__VA_ARGS__)

GPString gp_to_upper_new(const GPAllocator*, GPStrIn);
GPString gp_to_upper_full_new(const GPAllocator*, GPStrIn, const char*);
inline GPString gp_to_upper99(const size_t a_size, const void* a, const void* b, const char* b_id)
{
    if (a_size <= sizeof(GPAllocator)) {
        gp_str_to_upper_full((GPString*)a, b);
        return (GPString)a;
    } // TODO don't copy and process, just write to output!
    const size_t b_length = b_id[0] == '"' ? strlen(b) : gp_str_length((GPString)b);
    GPString out = gp_str_new(a, b_length, "");
    memcpy(out, b, b_length);
    ((GPStringHeader*)out - 1)->length = b_length;
    gp_str_to_upper(&out);
    return out;
}
#define GP_TO_UPPER1(STR)           gp_str_to_upper(STR)
#define GP_TO_UPPER99_2(A, B)       gp_to_upper99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B)
#define GP_TO_UPPER3(ALC, STR, LOC) gp_to_upper_full_new(GP_ALC(ALC), GP_STR_IN(STR), LOC)
#define GP_TO_UPPER99(...) GP_OVERLOAD3(__VA_ARGS__, GP_TO_UPPER3, GP_TO_UPPER99_2, GP_TO_UPPER1)(__VA_ARGS__)

GPString gp_to_lower_new(const GPAllocator*, GPStrIn);
GPString gp_to_lower_full_new(const GPAllocator*, GPStrIn, const char*);
inline GPString gp_to_lower99(const size_t a_size, const void* a, const void* b, const char* b_id)
{
    if (a_size <= sizeof(GPAllocator)) {
        gp_str_to_lower_full((GPString*)a, b);
        return (GPString)a;
    } // TODO don't copy and process, just write to output!
    const size_t b_length = b_id[0] == '"' ? strlen(b) : gp_str_length((GPString)b);
    GPString out = gp_str_new(a, b_length, "");
    memcpy(out, b, b_length);
    ((GPStringHeader*)out - 1)->length = b_length;
    gp_str_to_lower(&out);
    return out;
}
#define GP_TO_LOWER1(STR)           gp_str_to_lower(STR)
#define GP_TO_LOWER99_2(A, B)       gp_to_lower99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B)
#define GP_TO_LOWER3(ALC, STR, LOC) gp_to_lower_full_new(GP_ALC(ALC), GP_STR_IN(STR), LOC)
#define GP_TO_LOWER99(...) GP_OVERLOAD3(__VA_ARGS__, GP_TO_LOWER3, GP_TO_LOWER99_2, GP_TO_LOWER1)(__VA_ARGS__)

GPString gp_to_valid_new(
    const GPAllocator* alc, GPStrIn str, const char*const replacement);
#define GP_TO_VALID2(A, REPL)        gp_str_to_valid(A, REPL)
#define GP_TO_VALID3(ALC, STR, REPL) gp_to_valid_new(GP_ALC(ALC), GP_STR_IN(STR), REPL)
#define GP_TO_VALID(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_VALID3, GP_TO_VALID2)(A,__VA_ARGS__)

GPString gp_capitalize_new(const GPAllocator*, GPStrIn);
GPString gp_capitalize_locale_new(const GPAllocator*, GPStrIn, const char*);
inline GPString gp_capitalize99(const size_t a_size, const void* a, const void* b, const char* b_id)
{
    if (a_size <= sizeof(GPAllocator)) {
        gp_str_capitalize((GPString*)a, b);
        return (GPString)a;
    } // TODO don't copy and process, just write to output!
    const size_t b_length = b_id[0] == '"' ? strlen(b) : gp_str_length((GPString)b);
    GPString out = gp_str_new(a, b_length, "");
    memcpy(out, b, b_length);
    ((GPStringHeader*)out - 1)->length = b_length;
    gp_str_capitalize(&out, "");
    return out;
}
#define GP_CAPITALIZE1(STR)           gp_str_capitalize(STR, "")
#define GP_CAPITALIZE99_2(A, B)       gp_capitalize99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B)
#define GP_CAPITALIZE3(ALC, STR, LOC) gp_capitalize_full_new(GP_ALC(ALC), GP_STR_IN(STR), LOC)
#define GP_CAPITALIZE99(...) GP_OVERLOAD3(__VA_ARGS__, GP_CAPITALIZE3, GP_CAPITALIZE99_2, GP_CAPITALIZE1)(__VA_ARGS__)

inline size_t gp_find_first99(const GPString haystack, GPStrIn needle, const size_t start)
{
    return gp_str_find_first(haystack, needle.data, needle.length, start);
}
#define GP_FIND_FIRST2(HAY, NDL)               gp_find_first99(HAY, GP_STR_IN(NDL), 0)
#define GP_FIND_FIRST3(HAY, NDL, START)        gp_find_first99(HAY, GP_STR_IN(NDL), START)
#define GP_FIND_FIRST(...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_FIND_FIRST3, GP_FIND_FIRST2, UNUSED)(__VA_ARGS__)

inline size_t gp_find_last99(const GPString haystack, GPStrIn needle)
{
    return gp_str_find_last(haystack, needle.data, needle.length);
}
#define GP_FIND_LAST2(HAY, NDL) gp_find_last99(HAY, GP_STR_IN(NDL))
#define GP_FIND_LAST(A, ...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_find_last, GP_FIND_LAST2)(A, __VA_ARGS__)

#define GP_FIND_FIRST_OF2(HAY, CHARS) gp_str_find_first_of(HAY, CHARS, 0)
#define GP_FIND_FIRST_OF(A, ...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_find_first_of, GP_FIND_FIRST_OF2)(A, __VA_ARGS__)

#define GP_FIND_FIRST_NOT_OF2(HAY, CHARS) gp_str_find_first_not_of(HAY, CHARS, 0)
#define GP_FIND_FIRST_NOT_OF(A, ...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_find_first_not_of, GP_FIND_FIRST_NOT_OF2)(A, __VA_ARGS__)

inline bool gp_equal_case99(const GPString a, GPStrIn b)
{
    return gp_str_equal_case(a, b.data, b.length);
}
#define GP_EQUAL_CASE2(A, B) gp_equal_case99(a, GP_STR_IN(B))
#define GP_EQUAL_CASE(A,...) \
    GP_OVERLOAD2(__VA_ARGS__, gp_str_equal_case, GP_EQUAL_CASE2)(A, __VA_ARGS__)

inline int gp_compare99(const GPString str1, GPStrIn str2, int flags, const char* locale)
{
    return gp_str_compare(str1, str2.data, str2.length, flags, locale);
}
#define GP_COMPARE2(STR1, STR2)                gp_compare99(STR1, GP_STR_IN(STR2), 0, "")
#define GP_COMPARE3(STR1, STR2, FLAGS)         gp_compare99(STR1, GP_STR_IN(STR2), FLAGS, "")
#define GP_COMPARE4(STR1, STR2, FLAGS, LOCALE) gp_compare99(STR1, GP_STR_IN(STR2), FLAGS, LOCALE)
#define GP_COMPARE(STR1,...) GP_OVERLOAD3(__VA_ARGS__, GP_COMPARE4, GP_COMPARE3, GP_COMPARE2)(STR1,__VA_ARGS__)

size_t gp_codepoint_count99(GPStrIn s);
#define GP_CODEPOINT_COUNT(...) gp_codepoint_count99(GP_STR_IN(__VA_ARGS__))

bool gp_is_valid99(GPStrIn s, size_t*i);
#define GP_IS_VALID1(S)    gp_is_valid99(GP_STR_IN(S), NULL)
#define GP_IS_VALID2(S, I) gp_is_valid99(GP_STR_IN(S), I)
#define GP_IS_VALID(...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_IS_VALID2, GP_IS_VALID1)(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Strings and arrays

inline GPArray(GPString) gp_split99(const GPAllocator* alc, GPStrIn str, const char* separators)
{
    return gp_str_split(alc, str.data, str.length, separators);
}
#define GP_SPLIT2(ALC, STR)      gp_split99(GP_ALC(ALC), GP_STR_IN(STR), GP_WHITESPACE)
#define GP_SPLIT3(ALC, STR, SEP) gp_split99(GP_ALC(ALC), GP_STR_IN(STR), SEP)
#define GP_SPLIT(ALC,...) GP_OVERLOAD2(__VA_ARGS__, GP_SPLIT3, GP_SPLIT2)(ALC,__VA_ARGS__)

GPString gp_join_new(const GPAllocator* allocator, const GPArray(GPString) srcs, const char* separator);
inline GPString gp_join99(
    size_t a_size, const void* a, const GPArray(GPString) srcs, const char* separator)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_join((GPString*)a, srcs, separator);
        return *(GPString*)a;
    }
    return gp_join_new(a, srcs, separator);
}

#define GP_JOIN99_2(A, STRS)      gp_join99(GP_SIZEOF_TYPEOF(*(A)), A, STRS, "")
#define GP_JOIN99_3(A, STRS, SEP) gp_join99(GP_SIZEOF_TYPEOF(*(A)), A, STRS, SEP)
#define GP_JOIN99(A,...) GP_OVERLOAD2(__VA_ARGS__, GP_JOIN99_3, GP_JOIN99_2)(A,__VA_ARGS__)

#define GP_SORT1(STRS)        gp_str_sort(STRS, 0,     "")
#define GP_SORT2(STRS, FLAGS) gp_str_sort(STRS, FLAGS, "")
#define GP_SORT(...) GP_OVERLOAD3(__VA_ARGS__, gp_str_sort, GP_SORT2, GP_SORT1)(__VA_ARGS__)

#define GP_IS_ALC99(A) (GP_SIZEOF_TYPEOF(*(A)) >= sizeof(GPAllocator))

void gp_reserve99(size_t elem_size, void* px, const size_t capacity);
#define GP_RESERVE99(A, CAPACITY) gp_reserve99(sizeof**(A), A, CAPACITY)

void* gp_copy99(size_t y_size, const void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size);
#define GP_COPY99_2(A, B) \
    gp_copy99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)))
#define GP_COPY99_3(A, B, C) gp_copy99(GP_SIZEOF_TYPEOF(*(A)), A, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)))
#define GP_COPY99(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_COPY99_3, GP_COPY99_2)(A,__VA_ARGS__)

void* gp_slice99(
    const size_t y_size, const void* y,
    const size_t x_size, const void* x,
    const size_t start, const size_t end);
#define GP_SLICE_WITH_INPUT99(Y, X, START, END) \
    gp_slice99(GP_SIZEOF_TYPEOF(*(Y)), Y, GP_SIZEOF_TYPEOF(*(X)), X, START, END)
#define GP_SLICE_WOUT_INPUT99(Y, START, END) \
    ((void*){0} = gp_arr_slice(sizeof**(Y), *(void**)(Y), NULL, START, END))
#define GP_SLICE99(A, START, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_SLICE_WITH_INPUT99, GP_SLICE_WOUT_INPUT99)(A, START, __VA_ARGS__)

void* gp_append99(
    const size_t a_size, const void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);
#define GP_APPEND99_2(A, B) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), NULL, NULL, 0)
#define GP_APPEND99_3(A, B, C) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, \
        B, GP_IS_ALC99(A) ? #B : NULL, GP_IS_ALC99(A) ? GP_SIZEOF_TYPEOF(B) : (uintptr_t)(C), GP_SIZEOF_TYPEOF(*(B)), \
        GP_IS_ALC99(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))
#define GP_APPEND99_4(A, B, C, D) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, \
        B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), \
        C, NULL, D)
#define GP_APPEND99_5(A, B, C, D, E) \
    gp_append99(GP_SIZEOF_TYPEOF(*(A)), A, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)), D, NULL, E)
#define GP_APPEND99(A, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_APPEND99_5, GP_APPEND99_4, GP_APPEND99_3, GP_APPEND99_2)(A, __VA_ARGS__)

void* gp_insert99(
    const size_t a_size, const void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);
#define GP_INSERT99_3(A, POS, B) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), NULL, NULL, 0)
#define GP_INSERT99_4(A, POS, B, C) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, \
        B, GP_IS_ALC99(A) ? #B : NULL, GP_IS_ALC99(A) ? GP_SIZEOF_TYPEOF(B) : (uintptr_t)(C), GP_SIZEOF_TYPEOF(*(B)), \
        GP_IS_ALC99(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))
#define GP_INSERT99_5(A, POS, B, C, D) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, \
        B, #B, GP_SIZEOF_TYPEOF(B), GP_SIZEOF_TYPEOF(*(B)), \
        C, NULL, D)
#define GP_INSERT99_6(A, POS, B, C, D, E) \
    gp_insert99(GP_SIZEOF_TYPEOF(*(A)), A, POS, B, NULL, C, GP_SIZEOF_TYPEOF(*(B)), D, NULL, E)
#define GP_INSERT99(A, POS, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_INSERT99_6, GP_INSERT99_5, GP_INSERT99_4, GP_INSERT99_3)(A, POS, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Arrays

inline void* gp_push99(const size_t elem_size, void*_parr)
{
    uint8_t** parr = _parr;
    *parr = gp_arr_reserve(elem_size, *parr, gp_arr_length(*parr) + 1);
    return *parr + elem_size * ((GPArrayHeader*)*parr - 1)->length++;
}
#ifdef GP_TYPEOF
#define GP_PUSH(ARR, ELEM) \
    (*(GP_TYPEOF(*(ARR)))gp_push99(sizeof(**(ARR) = (ELEM)), (ARR)) = (ELEM))
#else
#define GP_PUSH(ARR, ELEM) \
    (gp_push99(sizeof**(ARR), (ARR)), (*(ARR))[gp_length(*(ARR)) - 1] = (ELEM))
#endif

#ifdef GP_TYPEOF
#define GP_POP(ARR) (*(GP_TYPEOF(*(ARR)))gp_arr_pop(GP_SIZEOF_TYPEOF(**(ARR)), *(ARR)))
#else
#define GP_POP(ARR) (gp_arr_pop(sizeof(**(ARR)), *(ARR)), (*(ARR))[gp_arr_length(*(ARR))])
#endif

#define GP_ERASE2(ARR, POS)        ((void*){0} = gp_arr_erase(sizeof**(ARR), *(ARR), POS, 1))
#define GP_ERASE3(ARR, POS, COUNT) ((void*){0} = gp_arr_erase(sizeof**(ARR), *(ARR), POS, COUNT))
#define GP_ERASE(A,...) GP_OVERLOAD2(__VA_ARGS__, GP_ERASE3, GP_ERASE2)(A,__VA_ARGS__)

GPArray(void) gp_map99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    void(*f)(void*,const void*));
#define GP_MAP99_2(ARR, F) \
    gp_arr_map(sizeof**((F)(*(ARR),*(ARR)),(ARR)), *(ARR), NULL, 0, (void(*)(void*,const void*))(F))
#define GP_MAP99_3(A, SRC, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, #SRC, GP_SIZEOF_TYPEOF(((F)((void*)(SRC),SRC), SRC)), GP_SIZEOF_TYPEOF(*(SRC)), (void(*)(void*,const void*))(F))
#define GP_MAP99_4(A, SRC, SRC_LENGTH, F) gp_map99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, NULL, SRC_LENGTH, GP_SIZEOF_TYPEOF(((F)((void*)(SRC),SRC), *(SRC))), (void(*)(void*,const void*))(F))
#define GP_MAP99(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_MAP99_4, GP_MAP99_3, GP_MAP99_2)(A,__VA_ARGS__)

#ifdef GP_TYPEOF // better type safety and allow using integer accumulator
#define GP_FOLD(ARR, ACC, F) \
    (GP_TYPEOF(ACC))(uintptr_t)gp_arr_fold (sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#define GP_FOLDR(ARR, ACC, F) \
    (GP_TYPEOF(ACC))(uintptr_t)gp_arr_foldr(sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#elif !defined(GP_PEDANTIC)
#define GP_FOLD(ARR, ACC, F)  gp_arr_fold (sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#define GP_FOLDR(ARR, ACC, F) gp_arr_foldr(sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#else
#define GP_FOLD(ARR, ACC, F)  gp_arr_fold (sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*(*)(void*,const void*))(F))
#define GP_FOLDR(ARR, ACC, F) gp_arr_foldr(sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*(*)(void*,const void*))(F))
#endif

GPArray(void) gp_filter99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    bool(*f)(const void* element));
#define GP_FILTER99_2(ARR, F) ((void*){0} = \
    gp_arr_filter(sizeof**((bool){0} = (F)(*(ARR)), (ARR)), *(ARR), NULL, 0, (bool(*)(const void*))(F)))
#define GP_FILTER99_3(A, SRC, F) gp_filter99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, #SRC, GP_SIZEOF_TYPEOF(((bool){0} = (F)(SRC), SRC)), GP_SIZEOF_TYPEOF(*(SRC)), (bool(*)(const void*))(F))
#define GP_FILTER99_4(A, SRC, SRC_LENGTH, F) gp_filter99(GP_SIZEOF_TYPEOF(*(A)), A, \
    SRC, NULL, SRC_LENGTH, GP_SIZEOF_TYPEOF(*((bool){0} = (F)(SRC), SRC)), (bool(*)(const void*))(F))
#define GP_FILTER99(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_FILTER99_4, GP_FILTER99_3, GP_FILTER99_2)(A,__VA_ARGS__)

// ----------------------------------------------------------------------------
// Dictionarys

inline void* gp_put99(GPHashMap* dict, GPStrIn key)
{
    return gp_hash_map_put(dict, key.data, key.length, NULL);
}
#ifdef GP_TYPEOF
#define GP_PUT_ELEM(DICT, ELEM, ...) ( \
    *(GP_TYPEOF(*(DICT)))(gp_put99((GPHashMap*)*(DICT), GP_STR_IN99(__VA_ARGS__))) = (ELEM))
#else
#define GP_PUT_ELEM(DICT, ELEM, ...) do \
{ \
    void* _gp_dict = *(DICT); \
    GPStrIn _gp_key = GP_STR_IN99(__VA_ARGS__); \
     *(DICT) = gp_hash_map_put(_gp_dict, _gp_key.data, _gp_key.length, NULL); \
    **(DICT) = (ELEM); \
     *(DICT) = _gp_dict; \
} while(0)
#endif
#define GP_PUT3(DICT, KEY, ELEM)             GP_PUT_ELEM(DICT, ELEM, KEY)
#define GP_PUT4(DICT, KEY, KEY_LENGTH, ELEM) GP_PUT_ELEM(DICT, ELEM, KEY, KEY_LENGTH)
#define GP_PUT(A, B, ...) GP_OVERLOAD2(__VA_ARGS__, GP_PUT4, GP_PUT3)(A, B,__VA_ARGS__)

GP_NONNULL_ARGS(1)
inline void* gp_get99(void* map, GPStrIn key)
{
    return gp_hash_map_get(map, key.data, key.length);
}

#ifdef GP_TYPEOF
#define GP_GET(DICT, ...) ((GP_TYPEOF(DICT))gp_get99(DICT, GP_STR_IN99(__VA_ARGS__)))
#else
#define GP_GET(DICT, ...) gp_get99(DICT, GP_STR_IN99(__VA_ARGS__))
#endif

inline bool gp_remove99(GPHashMap* dict, GPStrIn key)
{
    return gp_hash_map_remove(dict, key.data, key.length);
}
#define GP_REMOVE(DICT, ...) gp_remove99((GPHashMap*)*(DICT), GP_STR_IN99(__VA_ARGS__))

// ----------------------------------------------------------------------------
// Allocators

#define GP_ALLOC(ALLOCATOR, SIZE) gp_mem_alloc(GP_ALC(ALLOCATOR), SIZE)

#define GP_ALLOC_TYPE_WITH_COUNT(ALLOCATOR, TYPE, COUNT) \
    gp_mem_alloc(GP_ALC(ALLOCATOR), (COUNT) * sizeof(TYPE))
#define GP_ALLOC_TYPE_WOUT_COUNT(ALLOCATOR, TYPE) \
    gp_mem_alloc(GP_ALC(ALLOCATOR), sizeof(TYPE))
#define GP_ALLOC_TYPE(ALC, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_ALLOC_TYPE_WITH_COUNT,GP_ALLOC_TYPE_WOUT_COUNT)(ALC, __VA_ARGS__)

#define GP_ALLOC_ZEROES(ALLOCATOR, SIZE) \
    gp_mem_alloc_zeroes(GP_ALC(ALLOCATOR), SIZE)

#define GP_DEALLOC(ALLOCATOR, BLOCK) \
    gp_mem_dealloc(GP_ALC(ALLOCATOR), (BLOCK))

#define GP_REALLOC(ALLOCATOR, ...) \
    gp_mem_realloc(GP_ALC(ALLOCATOR),__VA_ARGS__)

// ----------------------------------------------------------------------------
// File

inline GPString gp_file99(size_t a_size, void* a, const char* path, const char* mode)
{
    switch (a_size)
    {
        case sizeof(GPChar): // read a, write to path
            if (gp_str_file((GPString*)&a, path, mode) == 0)
                return a;
            break;

        case sizeof(GPString): // read from path, write to a
            if (gp_str_file(a, path, mode) == 0)
                return *(GPString*)a;
            break;

        GPString str;
        default: // read from path to a new string
            str = gp_str_new(a, 128, "");
            if (gp_str_file(&str, path, mode) == 0)
                return str;
            gp_str_delete(str);
            break;
    }
    return NULL;
}
#define GP_FILE99_3(A, ...) gp_file99(GP_SIZEOF_TYPEOF(*(A)), A, __VA_ARGS__)
#define GP_FILE99_2(PATH, ...) gp_file_open(PATH, __VA_ARGS__)
#define GP_FILE99(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_FILE99_3, GP_FILE99_2)(A,__VA_ARGS__)

#endif // __cplusplus

#endif // GP_GENERIC_INCLUDED
