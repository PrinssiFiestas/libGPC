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
#include <gpc/hashmap.h>

#define GPDictionary(T) T*

#ifdef __cplusplus

#include <array>
typedef struct gp_str_in { const uint8_t* data; const size_t length; } GPStrIn;
extern "C" { // Private helpers
    GPString gp_replace_new(const GPAllocator*, GPStrIn, GPStrIn, GPStrIn, size_t);
    GPString gp_replace_all_new(const GPAllocator*, GPStrIn, GPStrIn, GPStrIn);
    GPString gp_str_trim_new(const void*, GPStrIn, const char*, int);
    GPString gp_to_upper_new(const GPAllocator*, GPString);
    GPString gp_to_lower_new(const GPAllocator*, GPString);
    GPString gp_to_valid_new(const GPAllocator*, GPString, const char*);
    size_t gp_bytes_codepoint_count(const void*, size_t);
    bool gp_bytes_is_valid_utf8(const void*, size_t, size_t*);
}
static inline size_t gp_str_length_cpp(const GPString str) { return gp_str_length(str); }
static inline size_t gp_str_length_cpp(const char*const str) { return strlen(str); }
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

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Provide overloads for your custom allocators instead of casting. C++ only.
static inline const GPAllocator* gp_alc_cpp(const GPAllocator* alc)
{
    return (const GPAllocator*)alc;
}
static inline const GPAllocator* gp_alc_cpp(const GPArena* alc)
{
    return (const GPAllocator*)alc;
}

// These overloads are also avaiable in C as macros.
//
// T_STRING shall be GPString or a const char*. However, if C11 _Generic()
// selection is not available in your compiler, const char* arguments MUST be
// string literals.

// ----------------------------------------------------------------------------
// Constructors

#define/*GPArray(T)*/gp_arr(/*allocator, T, init_elements*/...) \
    GP_ARR_NEW_CPP(__VA_ARGS__)

// Read-only array. Static if GNUC, on stack otherwise. Not available in C++.
#define/*const GPArray(T)*/gp_arr_ro(T,/*elements*/...)

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

template <typename T_ALLOCATOR, typename T_ARG>
static inline GPHashMap* gp_hmap(
    T_ALLOCATOR* allocator,
    const size_t element_size       = 0,
    void(*const  destructor)(T_ARG*) = NULL,
    const size_t capacity           = 0)
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

#define gp_dict(allocator, type,/*destructor = NULL, capacity = 0*/...) \
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
    return gp_str_codepoint_length((GPString)ptr, 0);
}
static inline size_t gp_codepoint_length(const void*const str, const size_t i)
{
    return gp_str_codepoint_length((GPString)str, i);
}

// ---------------------------
// gp_codepoint_classify()

static inline bool gp_codepoint_classify(
    const void*const ptr, int(*const classifier)(wint_t))
{
    return gp_str_codepoint_classify((GPString)ptr, 0, classifier);
}
static inline bool gp_codepoint_classify(
    const void*const str, size_t i, int(*const classifier)(wint_t))
{
    return gp_str_codepoint_classify((GPString)str, i, classifier);
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
        gp_str_length(replacement),
        start);
}
template
<typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2, typename T_STRING3>
static inline GPString gp_replace(
    T_ALLOCATOR* allocator,
    T_STRING1    haystack,
    T_STRING2    needle,
    T_STRING3    replacement,
    const size_t start = 0)
{
    GPStrIn hay { (uint8_t*)haystack,    gp_cpp_length(haystack   ) };
    GPStrIn ndl { (uint8_t*)needle,      gp_cpp_length(needle     ) };
    GPStrIn rep { (uint8_t*)replacement, gp_cpp_length(replacement) };
    return gp_replace_new(gp_alc_cpp(allocator), hay, ndl, rep, start);
}

// ---------------------------
// gp_replace_all()

template <typename T_STRING1, typename T_STRING2>
static inline size_t gp_replace_all(
    GPString*    haystack,
    T_STRING1    needle,
    T_STRING2    replacement)
{
    return gp_str_replace_all(
        haystack,
        needle,
        gp_str_length_cpp(needle),
        replacement,
        gp_str_length(replacement));
}
template
<typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2, typename T_STRING3>
static inline GPString gp_replace_all(
    T_ALLOCATOR* allocator,
    T_STRING1    haystack,
    T_STRING2    needle,
    T_STRING3    replacement)
{
    GPStrIn hay { (uint8_t*)haystack,    gp_cpp_length(haystack   ) };
    GPStrIn ndl { (uint8_t*)needle,      gp_cpp_length(needle     ) };
    GPStrIn rep { (uint8_t*)replacement, gp_cpp_length(replacement) };
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
template <typename T_ALLOCATOR>
static inline GPString gp_to_upper(T_ALLOCATOR* allocator, GPString str)
{
    return gp_to_upper_new(gp_alc_cpp(allocator), str);
}

// ---------------------------
// gp_to_lower()

static inline void gp_to_lower(GPString* str)
{
    gp_str_to_lower(str);
}
template <typename T_ALLOCATOR>
static inline GPString gp_to_lower(T_ALLOCATOR* allocator, GPString str)
{
    return gp_to_lower_new(gp_alc_cpp(allocator), str);
}

// ---------------------------
// gp_to_valid()

static inline void gp_to_valid(GPString* str, const char*const replacement)
{
    gp_str_to_valid(str, replacement);
}
template <typename T_ALLOCATOR>
static inline GPString gp_to_valid(
    T_ALLOCATOR* allocator, GPString str, const char*const replacement)
{
    return gp_to_valid_new(gp_alc_cpp(allocator), str, replacement);
}

// ---------------------------
// gp_find_first()

template <typename T_STRING>
size_t gp_find_first(
    const GPString haystack, T_STRING needle, const size_t start = 0)
{
    return gp_str_find_first(haystack, needle, gp_str_length_cpp(needle), start);
}

// ---------------------------
// gp_find_last()

template <typename T_STRING>
size_t gp_find_last(const GPString haystack, T_STRING needle)
{
    return gp_str_find_last(haystack, needle, gp_str_length_cpp(needle));
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
static inline bool gp_is_valid(const char*const str)
{
    return gp_bytes_is_valid_utf8(str, gp_str_length_cpp(str), NULL);
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

// ----------------------------------------------------------------------------
// Strings and arrays

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
    *parr = gp_arr_reserve(sizeof(*parr)[0], *parr, capacity);
}

// ---------------------------
// gp_copy()

template <typename T_STRING>
static inline void gp_copy(GPString* dest, T_STRING src)
{
    gp_str_copy(dest, src, gp_str_length(src));
}
template <typename T>
static inline void gp_copy(GPArray(T)* dest, GPArray(T) src)
{
    *dest = gp_arr_copy(sizeof(*dest)[0], *dest, src, gp_arr_length(src));
}
template <typename T_ALLOCATOR, typename T_STRING>
static inline GPString gp_copy(T_ALLOCATOR* allocator, T_STRING src)
{
    const size_t length = gp_str_length_cpp(src);
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    return (GPString)memcpy(out, src, length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_copy(T_ALLOCATOR* allocator, GPArray(T) src)
{
    const size_t length = gp_arr_length(src);
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], length);
    ((GPArrayHeader*)out - 1)->length = length;
    return (GPArray(T))memcpy(out, src, length * sizeof out[0]);
}
template <typename T_ALLOCATOR>
static inline GPString gp_copy(
    T_ALLOCATOR* allocator, const GPString src, const size_t src_length)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), src_length, "");
    ((GPStringHeader*)out - 1)->length = src_length;
    return (GPString)memcpy(out, src, src_length);
}
template <typename T_ALLOCATOR>
static inline GPString gp_copy(
    T_ALLOCATOR* allocator, const char*const src, const size_t src_length)
{
    GPString out = gp_str_new(gp_alc_cpp(allocator), src_length, "");
    ((GPStringHeader*)out - 1)->length = src_length;
    return (GPString)memcpy(out, src, src_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPString gp_copy(
    T_ALLOCATOR* allocator, const T*const src, const size_t src_length)
{
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], src_length);
    ((GPArrayHeader*)out - 1)->length = src_length;
    return (GPArray(T))memcpy(out, src, src_length * sizeof out[0]);
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
    *dest = gp_str_slice(sizeof(*dest)[0], *dest, NULL, start, end);
}
static inline void gp_slice(
    GPString* dest, const void*const src, const size_t start, const size_t end)
{
    gp_str_slice(dest, src, start, end);
}
template <typename T>
static inline void gp_slice(
    GPArray(T)* dest, const T*const src, const size_t start, const size_t end)
{
    *dest = gp_arr_slice(sizeof(*dest)[0], *dest, src, start, end);
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
    T_ALLOCATOR* allocator, GPArray(T) src, const size_t start, const size_t end)
{
    const size_t length = end - start;
    GPArray(T) out = (GPArray(T))gp_arr_new(gp_alc_cpp(allocator), sizeof out[0], length);
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
    *dest = gp_arr_append(sizeof(*dest)[0], *dest, src, gp_arr_length(src));
}
template <typename T>
static inline void gp_append(
    GPArray(T)* dest, const T*const src, const size_t src_length)
{
    *dest = gp_arr_append(sizeof(*dest)[0], *dest, src, src_length);
}
template <typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2>
static inline GPString gp_append(
    T_ALLOCATOR* allocator, T_STRING1 str1, T_STRING2 str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    const size_t length = str1_length + str2_length;
    GPString out = gp_str_new(gp_alc_cpp(allocator), length, "");
    ((GPStringHeader*)out - 1)->length = length;
    memcpy(out + str1_length, str2, str2_length);
    return (GPString)memcpy(out, str1, str1_length);
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
    T_ALLOCATOR* allocator, GPArray(T) arr1, GPArray(T) arr2)
{
    const size_t arr1_length = gp_arr_length(arr1);
    const size_t arr2_length = gp_arr_length(arr2);
    const size_t length = arr1_length + arr2_length;
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0], length);
    ((GPArrayHeader*)out - 1)->length = length;
    memcpy(out + arr1_length, arr2, arr2_length * sizeof arr1[0]);
    return (GPArray(T))memcpy(out, arr1, arr1_length * sizeof arr1[0]);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_append(
    T_ALLOCATOR*  allocator,
    GPArray(T)    arr1,
    const T*const arr2,
    const size_t  arr2_length)
{
    const size_t arr1_length = gp_arr_length_cpp(arr1);
    const size_t length = arr1_length + arr2_length;
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), length, "");
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
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), length, "");
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
    *dest = gp_arr_insert(sizeof(*dest)[0], index, *dest, src, gp_arr_length(src));
}
template <typename T>
static inline void gp_insert(
    GPArray(T)* dest, const size_t index, const T*const src, const size_t src_length)
{
    *dest = gp_arr_insert(sizeof(*dest)[0], index, *dest, src, src_length);
}
template <typename T_ALLOCATOR, typename T_STRING1, typename T_STRING2>
static inline GPString gp_insert(
    T_ALLOCATOR* allocator, const size_t index, T_STRING1 str1, T_STRING2 str2)
{
    const size_t str1_length = gp_str_length_cpp(str1);
    const size_t str2_length = gp_str_length_cpp(str2);
    GPString out = gp_str_new(gp_alc_cpp(allocator), str1_length + str2_length, "");
    return (GPString)gp_insert_cpp(sizeof out[0], out, index, str1, str1_length, str2, str2_length);
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
    const size_t length = str1_length + str2_length;
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
    T_ALLOCATOR* allocator, GPArray(T) arr1, GPArray(T) arr2)
{
    const size_t arr1_length = gp_arr_length(arr1);
    const size_t arr2_length = gp_arr_length(arr2);
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0], arr1_length + arr2_length);
    return (GPArray(T))gp_insert_cpp(sizeof out[0], out, index, arr1, arr1_length, arr2, arr2_length);
}
template <typename T_ALLOCATOR, typename T>
static inline GPArray(T) gp_insert(
    T_ALLOCATOR*  allocator,
    const size_t  index,
    GPArray(T)    arr1,
    const T*const arr2,
    const size_t  arr2_length)
{
    const size_t arr1_length = gp_arr_length_cpp(arr1);
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0],  arr1_length + arr2_length);
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
    GPArray(T) out = gp_arr_new(gp_alc_cpp(allocator), sizeof arr1[0], arr1_length + arr2_length);
    return (GPArray(T))gp_insert_cpp(sizeof out[0], out, index, arr1, arr1_length, arr2, arr2_length);
}

// TODO rest of these

// ----------------------------------------------------------------------------
// Arrays
// ---------------------------
// gp_push()                //GP_PUSH(__VA_ARGS__)
// ---------------------------
// gp_pop()                 //GP_POP(__VA_ARGS__)
// ---------------------------
// gp_erase()               //GP_ERASE(__VA_ARGS__)
// ---------------------------
// gp_map()                 //GP_MAP11(__VA_ARGS__)
// ---------------------------
// gp_fold()                //GP_FOLD(__VA_ARGS__)
// ---------------------------
// gp_foldr()               //GP_FOLDR(__VA_ARGS__)
// ---------------------------
// gp_filter()              //GP_FILTER11(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Dictionarys
// ---------------------------
// gp_get()                 //GP_GET(__VA_ARGS__)
// ---------------------------
// gp_put()                 //GP_PUT(__VA_ARGS__)
// ---------------------------
// gp_remove()              //GP_REMOVE(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Memory
// ---------------------------
// gp_alloc()               //GP_ALLOC(__VA_ARGS__)
// ---------------------------
// gp_alloc_type()          //GP_ALLOC_TYPE(__VA_ARGS__)
// ---------------------------
// gp_alloc_zeroes()        //GP_ALLOC_ZEROES(__VA_ARGS__)
// ---------------------------
// gp_dealloc()             //GP_DEALLOC(__VA_ARGS__)
// ---------------------------
// gp_realloc()             //GP_REALLOC(__VA_ARGS__)

// ----------------------------------------------------------------------------
// File
// ---------------------------
// gp_file()                GP_FILE11(__VA_ARGS__)

// ---------------------------
#define gp_read_line(...)           gp_file_read_line(...)
#define gp_read_until(...)          gp_file_read_until(...)
#define gp_read_strip(...)          gp_file_read_strip(...)


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
#define GP_DICT_CPP_34(ALC, TYPE, ...) (TYPE*)gp_hmap(ALC, sizeof(TYPE), __VA_ARGS__)
#define GP_DICT_CPP(ALC,...) GP_OVERLOAD3(__VA_ARGS__, \
    GP_DICT_CPP34, GP_DICT_CPP34, GP_DICT_CPP_2)(ALC,__VA_ARGS__)

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
#define gp_codepoint_classify(...)  GP_CODEPOINT_CLASSIFY(__VA_ARGS__)

// Strings
#define gp_repeat(...)              GP_REPEAT11(__VA_ARGS__)
#define gp_replace(...)             GP_REPLACE11(__VA_ARGS__)
#define gp_replace_all(...)         GP_REPLACE_ALL11(__VA_ARGS__)
#define gp_trim(...)                GP_TRIM11(__VA_ARGS__)
#define gp_to_upper(...)            GP_TO_UPPER(__VA_ARGS__)
#define gp_to_lower(...)            GP_TO_LOWER(__VA_ARGS__)
#define gp_to_valid(...)            GP_TO_VALID(__VA_ARGS__)
#define gp_find_first(...)          GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)           GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)       GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)   GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)          GP_EQUAL_CASE(__VA_ARGS__)
#define gp_codepoint_count(...)     GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)            GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
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
#define gp_read_line(...)           gp_file_read_line(...)
#define gp_read_until(...)          gp_file_read_until(...)
#define gp_read_strip(...)          gp_file_read_strip(...)

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
#define gp_codepoint_classify(...)  GP_CODEPOINT_CLASSIFY(__VA_ARGS__)

// Strings
#define gp_repeat(...)              GP_REPEAT99(__VA_ARGS__)
#define gp_replace(...)             GP_REPLACE99(__VA_ARGS__)
#define gp_replace_all(...)         GP_REPLACE_ALL99(__VA_ARGS__)
#define gp_trim(...)                GP_TRIM99(__VA_ARGS__)
#define gp_to_upper(...)            GP_TO_UPPER(__VA_ARGS__)
#define gp_to_lower(...)            GP_TO_LOWER(__VA_ARGS__)
#define gp_to_valid(...)            GP_TO_VALID(__VA_ARGS__)
#define gp_find_first(...)          GP_FIND_FIRST(__VA_ARGS__)
#define gp_find_last(...)           GP_FIND_LAST(__VA_ARGS__)
#define gp_find_first_of(...)       GP_FIND_FIRST_OF(__VA_ARGS__)
#define gp_find_first_not_of(...)   GP_FIND_FIRST_NOT_OF(__VA_ARGS__)
#define gp_equal_case(...)          GP_EQUAL_CASE(__VA_ARGS__)
#define gp_codepoint_count(...)     GP_CODEPOINT_COUNT(__VA_ARGS__)
#define gp_is_valid(...)            GP_IS_VALID(__VA_ARGS__)

// Strings and arrays
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
#define gp_read_line(...)           gp_file_read_line(...)
#define gp_read_until(...)          gp_file_read_until(...)
#define gp_read_strip(...)          gp_file_read_strip(...)

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

// ----------------------------------------------------------------------------
// Strings and arrays

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

#ifdef __GNUC__
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
    (GPAllocator*)(ALC), \
    sizeof(TYPE), \
    (TYPE[]){(TYPE){0},__VA_ARGS__} + 1, \
    sizeof((TYPE[]){(TYPE){0},__VA_ARGS__}) / sizeof(TYPE) - 1)

#if __GNUC__
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

#define GP_STR_NEW1(ALC)            gp_str_new((GPAllocator*)(ALC), 16, "")
#define GP_STR_NEW2(ALC, INIT)      gp_str_new((GPAllocator*)(ALC), 16, INIT)
#define GP_STR_NEW3(ALC, CAP, INIT) gp_str_new((GPAllocator*)(ALC), CAP, INIT)
#define GP_STR_NEW(...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_STR_NEW3, GP_STR_NEW2, GP_STR_NEW1)(__VA_ARGS__)

#define GP_HMAP1(ALC) gp_hash_map_new((GPAllocator*)(ALC), NULL)
#define GP_HMAP2(ALC, ELEM_SIZE) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = 0, .destructor = NULL})
#define GP_HMAP3(ALC, ELEM_SIZE, DCTOR) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = 0, .destructor = (void(*)(void*))(DCTOR)})
#define GP_HMAP4(ALC, ELEM_SIZE, DCTOR, CAP) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = ELEM_SIZE, .capacity = CAP, .destructor = (void(*)(void*))(DCTOR)})
#define GP_HMAP_NEW(...) GP_OVERLOAD4(__VA_ARGS__, GP_HMAP4, GP_HMAP3, GP_HMAP2, GP_HMAP1)(__VA_ARGS__)

#define GP_DICT2(ALC, TYPE) (TYPE*) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = sizeof(TYPE), .capacity = 0, .destructor = NULL})
#define GP_DICT3(ALC, TYPE, DCTOR) (TYPE*) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
        .element_size = GP_SIZEOF_TYPEOF((DCTOR)((TYPE*)0), (TYPE){0}), .capacity = 0, .destructor = (void(*)(void*))(DCTOR)})
#define GP_DICT4(ALC, TYPE, DCTOR, CAP) (TYPE*) \
    gp_hash_map_new((GPAllocator*)(ALC), &(GPMapInitializer){ \
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

#define GP_CODEPOINT_LENGTH1(PTR)    gp_str_codepoint_length((GPString)(PTR), 0)
#define GP_CODEPOINT_LENGTH2(STR, I) gp_str_codepoint_length((GPString)(STR), I)
#define GP_CODEPOINT_LENGTH(...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_CODEPOINT_LENGTH2, GP_CODEPOINT_LENGTH1)(__VA_ARGS__)

#define GP_CODEPOINT_CLASSIFY2(PTR, F)    gp_str_codepoint_classify((GPString)(PTR), 0, F)
#define GP_CODEPOINT_CLASSIFY3(PTR, I, F) gp_str_codepoint_classify((GPString)(PTR), I, F)
#define GP_CODEPOINT_CLASSIFY(P, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_CODEPOINT_CLASSIFY3, GP_CODEPOINT_CLASSIFY2)(P, __VA_ARGS__)

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

GPString gp_to_upper_new(const GPAllocator* alc, const GPString str);
#define GP_TO_UPPER1(A)        gp_str_to_upper(A)
#define GP_TO_UPPER2(ALC, STR) gp_to_upper_new((GPAllocator*)(ALC), STR)
#define GP_TO_UPPER(...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_UPPER2, GP_TO_UPPER1)(__VA_ARGS__)

GPString gp_to_lower_new(const GPAllocator* alc, const GPString str);
#define GP_TO_LOWER1(A)        gp_str_to_lower(A)
#define GP_TO_LOWER2(ALC, STR) gp_to_lower_new((GPAllocator*)(ALC), STR)
#define GP_TO_LOWER(...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_LOWER2, GP_TO_LOWER1)(__VA_ARGS__)

GPString gp_to_valid_new(
    const GPAllocator* alc, const GPString str, const char*const replacement);
#define GP_TO_VALID2(A, REPL)        gp_str_to_valid(A, REPL)
#define GP_TO_VALID3(ALC, STR, REPL) gp_to_valid_new((GPAllocator*)(ALC), STR, REPL)
#define GP_TO_VALID(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_TO_VALID3, GP_TO_VALID2)(A,__VA_ARGS__)

inline size_t gp_find_first99(const GPString haystack, GPStrIn needle)
{
    return gp_str_find_first(haystack, needle.data, needle.length, 0);
}
#define GP_FIND_FIRST2(HAY, NDL)                gp_find_first99(HAY, GP_STR_IN(NDL))
#define GP_FIND_FIRST3(HAY, NDL, NDLLEN)        gp_str_find_first(HAY, NDL, NDLLEN, 0)
#define GP_FIND_FIRST(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, gp_str_find_first, GP_FIND_FIRST3, GP_FIND_FIRST2)(A, __VA_ARGS__)

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

size_t gp_codepoint_count99(GPStrIn s);
#define GP_CODEPOINT_COUNT(...) gp_codepoint_count99(GP_STR_IN(__VA_ARGS__))

bool gp_is_valid99(GPStrIn s, size_t*i);
#define GP_IS_VALID1(S)       gp_is_valid99(GP_STR_IN(S),    NULL)
#define GP_IS_VALID2(S, L)    gp_is_valid99(GP_STR_IN(S, L), NULL)
#define GP_IS_VALID3(S, L, I) gp_is_valid99(GP_STR_IN(S, L), I)
#define GP_IS_VALID(...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_IS_VALID3, GP_IS_VALID2, GP_IS_VALID1)(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Strings and arrays

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

#ifdef GP_TYPEOF
inline void* gp_push99(const size_t elem_size, void*_parr)
{
    uint8_t** parr = _parr;
    *parr = gp_arr_reserve(elem_size, *parr, gp_arr_length(*parr) + 1);
    return *parr + elem_size * ((GPArrayHeader*)*parr - 1)->length++;
}
#define GP_PUSH(ARR, ELEM) \
    (*(GP_TYPEOF(*(ARR)))gp_push99(sizeof(**(ARR) = (ELEM)), (ARR)) = (ELEM))
#else
#define GP_PUSH(ARR, ELEM) ( \
    ((GPArrayHeader*)*(ARR) - 1)->length++, (*(ARR))[gp_length(*(ARR)) - 1] = (ELEM))
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
#else
#define GP_FOLD(ARR, ACC, F)  gp_arr_fold (sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
#define GP_FOLDR(ARR, ACC, F) gp_arr_foldr(sizeof*((F)(ACC,ARR),(ARR)),ARR,(void*)(ACC),(void*)(F))
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

#ifdef GP_TYPEOF
inline void* gp_put99(GPHashMap* dict, GPStrIn key)
{
    return gp_hash_map_put(dict, key.data, key.length, NULL);
}
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
            break;
    }
    return NULL;
}
#define GP_FILE99_3(A, ...) gp_file99(GP_SIZEOF_TYPEOF(*(A)), A, __VA_ARGS__)
#define GP_FILE99_2(PATH, ...) gp_file_open(PATH, __VA_ARGS__)
#define GP_FILE99(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_FILE99_3, GP_FILE99_2)(A,__VA_ARGS__)

#endif // __cplusplus

#endif // GP_GENERIC_INCLUDED
