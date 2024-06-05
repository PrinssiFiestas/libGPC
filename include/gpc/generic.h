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

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// TODO implement what's not implemented

// Note: macros may take variadic arguments even when not necessary for better
// error messages. Also in some occasions, but not always, allows using
// compound literals as macros arguments.

#define GPHashMap(T) T*

// Constructors
#define gp_arr(...)          GP_ARR_NEW(__VA_ARGS__)
#define gp_str(...)          GP_STR_NEW(__VA_ARGS__)
#define gp_hmap(...)

// Bytes and strings
#define gp_equal(...)             GP_EQUAL(__VA_ARGS__)
#define gp_count(...)             GP_COUNT(__VA_ARGS__)
#define gp_equal_case(...)
#define gp_codepoint_count(...)
#define gp_is_valid(...)
#define gp_codepoint_length(...)  gp_char_codepoint_length(__VA_ARGS__)
#define gp_classify(...)
#define gp_find_first(...)
#define gp_find_last(...)
#define gp_find_first_of(...)
#define gp_find_first_not_of(...)

// Strings
#define gp_repeat(...)       GP_REPEAT(__VA_ARGS__)
#define gp_replace(...)
#define gp_replace_all(...)
#define gp_trim(...)
#define gp_to_upper(...)     gp_str_to_upper(__VA_ARGS__)
#define gp_to_lower(...)     gp_str_to_lower(__VA_ARGS__)
#define gp_to_valid(...)

// Strings and arrays
#define gp_length(...)       gp_arr_length(__VA_ARGS__)
#define gp_capacity(...)     gp_arr_capacity(__VA_ARGS__)
#define gp_allocation(...)   gp_arr_allocation(__VA_ARGS__)
#define gp_allocator(...)    gp_arr_allocator(__VA_ARGS__)
#define gp_reserve(...)      GP_RESERVE(__VA_ARGS__)
#define gp_copy(...)         GP_COPY(__VA_ARGS__)
#define gp_slice(...)        GP_SLICE(__VA_ARGS__)
#define gp_append(...)       GP_APPEND(__VA_ARGS__)
#define gp_insert(...)       GP_INSERT(__VA_ARGS__)

// Arrays
#define gp_map(...)
#define gp_fold(...)
#define gp_foldr(...)
#define gp_filter(...)

// Arrays and hash maps
#define gp_at(...)
#define gp_push(...)
#define gp_pop(...)
#define gp_remove(...)

// Memory
#define gp_alloc(...)        GP_ALLOC(__VA_ARGS__)
#define gp_alloc_type(...)   GP_ALLOC_TYPE(__VA_ARGS__)
#define gp_alloc_zeroes(...) GP_ALLOC_ZEROES(__VA_ARGS__)
#define gp_dealloc(...)      GP_DEALLOC(__VA_ARGS__)
#define gp_realloc(...)      GP_REALLOC(__VA_ARGS__)

// File
#define gp_file(...)         GP_FILE(__VA_ARGS__)


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


// Currently C99 compliant, but later on C11 _Generic() selection should be used
// so any char* could be passed as string inputs insead of just literals. This
// would have better type safety too.

#ifdef __GNUC__
// Suppress suspicious usage of sizeof warning.
#define GP_SIZEOF_TYPEOF(X) sizeof(typeof(X))
#else
#define GP_SIZEOF_TYPEOF(X) sizeof(X)
#endif

// ----------------------------------------------------------------------------
// Constructors

#define GP_ARR_NEW(ALLOCATOR, TYPE, ...) \
    (TYPE*)gp_arr_copy(sizeof(TYPE), \
        gp_arr_new((GPAllocator*)(ALLOCATOR), 4, sizeof(TYPE)), \
        (TYPE[]){__VA_ARGS__}, \
        sizeof((TYPE[]){__VA_ARGS__}) / sizeof(TYPE))

struct gp_str_maker { const GPAllocator* allocator; const char* init; };
GPString gp_str_make(struct gp_str_maker maker);
#define GP_STR_NEW(ALLOCATOR, ...) \
    gp_str_make((struct gp_str_maker){(GPAllocator*)(ALLOCATOR), __VA_ARGS__})

// ----------------------------------------------------------------------------
// Bytes and strings

typedef struct gp_str_in { const void* data; const size_t length; } GPStrIn;
static inline GPStrIn gp_str_in99(const void* data, const size_t length)
{
    return (GPStrIn) {
        .data   = data,
        .length = length != SIZE_MAX ? length : gp_arr_length(data)
    };
}
#define GP_STR_IN1(A) gp_str_in99(A, #A[0] == '"' ? GP_SIZEOF_TYPEOF(A) - sizeof "" : SIZE_MAX)
#define GP_STR_IN(...) GP_OVERLOAD2(__VA_ARGS__, gp_str_in99, GP_STR_IN1)(__VA_ARGS__)

static inline bool gp_equal99(
    const size_t elem_size, const void* a, const void* b, size_t b_length)
{
    if (b_length == SIZE_MAX)
        b_length = gp_arr_length(b);
    if (gp_arr_length(a) != b_length)
        return false;
    return memcmp(a, b, b_length * elem_size) == 0;
}
#define GP_EQUAL2(A, B) \
    gp_equal99(sizeof*(A), A, B, #B[0] == '"' ? GP_SIZEOF_TYPEOF(B) - sizeof"" : SIZE_MAX)
#define GP_EQUAL3(A, B, B_LENGTH) \
    gp_equal99(sizeof*(A), A, B, B_LENGTH)
#define GP_EQUAL4(A, A_LEN, B, B_LEN) \
    gp_bytes_equal(A, sizeof*(A) * (A_LEN), B, sizeof*(B) * (B_LEN))
#define GP_EQUAL(A, ...) \
    GP_OVERLOAD3(__VA_ARGS__, GP_EQUAL4, GP_EQUAL3, GP_EQUAL2)(A, __VA_ARGS__)

static inline size_t gp_count99(GPStrIn haystack, GPStrIn needle) {
    return gp_bytes_count(haystack.data, haystack.length, needle.data, needle.length);
}
#define GP_COUNT2(A, B)       gp_count99(GP_STR_IN(A), GP_STR_IN(B))
#define GP_COUNT3(A, B, C)    gp_count99(GP_STR_IN(A), GP_STR_IN(B, C))
#define GP_COUNT4(A, B, C, D) gp_count99(GP_STR_IN(A, B), GP_STR_IN(C, D))
#define GP_COUNT(A, ...) GP_OVERLOAD3(__VA_ARGS__, GP_COUNT4, GP_COUNT3, GP_COUNT2)(A, __VA_ARGS__)

// ----------------------------------------------------------------------------
// String

GPString gp_repeat99(
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
#define GP_REPEAT(A, COUNT, ...) gp_repeat99(GP_SIZEOF_TYPEOF(*(A)), A, COUNT, GP_STR_IN(__VA_ARGS__))

// ----------------------------------------------------------------------------
// Srting and array shared

void gp_reserve99(size_t elem_size, void* px, const size_t capacity);
#define GP_RESERVE(A, CAPACITY) gp_reserve99(sizeof**(A), A, CAPACITY)

void* gp_copy99(size_t y_size, void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size);

#define GP_COPY2(A, B)    gp_copy99(sizeof*(A), A, B, #B, sizeof(B) - sizeof"", sizeof*(B))
#define GP_COPY3(A, B, C) gp_copy99(sizeof*(A), A, B, NULL, C, sizeof*(B))
#define GP_COPY(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_COPY3, GP_COPY2)(A,__VA_ARGS__)

void* gp_slice99(
    const size_t y_size, const void* y,
    const size_t x_size, const void* x,
    const size_t start, const size_t end);

#define GP_SLICE_WITH_INPUT(Y, X, START, END) \
    gp_slice99(sizeof*(Y), Y, sizeof*(X), X, START, END)
#define GP_SLICE_WOUT_INPUT(Y, START, END) \
    ((void*){0} = gp_arr_slice(sizeof**(Y), *(void**)(Y), NULL, START, END))
#define GP_SLICE(A, START, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_SLICE_WITH_INPUT, GP_SLICE_WOUT_INPUT)(A, START, __VA_ARGS__)

void* gp_append99(
    const size_t a_size, void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);

#define GP_IS_ALC(A) (sizeof*(A) == sizeof(GPAllocator))

#define GP_APPEND2(A, B) \
    gp_append99(sizeof*(A), A, B, #B, sizeof(B), sizeof*(B), NULL, NULL, 0)

#define GP_APPEND3(A, B, C) \
    gp_append99(sizeof*(A), A, \
        B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? sizeof(B) : (uintptr_t)(C), sizeof*(B), \
        GP_IS_ALC(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))

#define GP_APPEND4(A, B, C, D) \
    gp_append99(sizeof*(A), A, \
        B, #B, sizeof(B) : sizeof*(B), \
        C, NULL, D)

#define GP_APPEND5(A, B, C, D, E) \
    gp_append99(sizeof*(A), A, B, NULL, C, sizeof*(B), D, NULL, E)
#define GP_APPEND(A, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_APPEND5, GP_APPEND4, GP_APPEND3, GP_APPEND2)(A, __VA_ARGS__)

void* gp_insert99(
    const size_t a_size, void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);

#define GP_INSERT3(A, POS, B) \
    gp_insert99(sizeof*(A), A, POS, B, #B, sizeof(B), sizeof*(B), NULL, NULL, 0)

#define GP_INSERT4(A, POS, B, C) \
    gp_insert99(sizeof*(A), A, POS, \
        B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? sizeof(B) : (uintptr_t)(C), sizeof*(B), \
        GP_IS_ALC(A) ? (void*)(C) : NULL, #C, GP_SIZEOF_TYPEOF(C))

#define GP_INSERT5(A, POS, B, C, D) \
    gp_insert99(sizeof*(A), A, POS, \
        B, #B, sizeof(B): sizeof*(B), \
        C, NULL, D)

#define GP_INSERT6(A, POS, B, C, D, E) \
    gp_insert99(sizeof*(A), A, POS, B, NULL, C, sizeof*(B), D, NULL, E)

#define GP_INSERT(A, POS, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_INSERT6, GP_INSERT5, GP_INSERT4, GP_INSERT3)(A, POS, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Allocators

#define GP_ALLOC(ALLOCATOR, SIZE) gp_mem_alloc((GPAllocator*)(ALLOCATOR), SIZE)

#define GP_ALLOC_TYPE_WITH_COUNT(ALLOCATOR, TYPE, COUNT) \
    gp_mem_alloc((GPAllocator*)(ALLOCATOR), (COUNT) * sizeof(TYPE))
#define GP_ALLOC_TYPE_WOUT_COUNT(ALLOCATOR, TYPE) \
    gp_mem_alloc((GPAllocator*)(ALLOCATOR), sizeof(TYPE))
#define GP_ALLOC_TYPE(ALC, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_ALLOC_TYPE_WITH_COUNT,GP_ALLOC_TYPE_WOUT_COUNT)(ALC, __VA_ARGS__)

#define GP_ALLOC_ZEROES(ALLOCATOR, SIZE, COUNT) \
    gp_mem_alloc_zeroes((GPAllocator*)(ALLOCATOR), (SIZE) * sizeof(TYPE))

#define GP_DEALLOC(ALLOCATOR, BLOCK) \
    gp_mem_dealloc((GPAllocator*)(ALLOCATOR), (BLOCK))

#define GP_REALLOC(ALLOCATOR, ...) \
    gp_mem_realloc((GPAllocator*)(ALLOCATOR),__VA_ARGS__)

// ----------------------------------------------------------------------------
// File

static inline GPString gp_file99(size_t a_size, void* a, const char* path, const char* mode)
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
#define GP_FILE3(A, ...) gp_file99(GP_SIZEOF_TYPEOF(*(A)), A, __VA_ARGS__)

#define GP_FILE2(PATH, ...) gp_file_open(PATH, __VA_ARGS__)
#define GP_FILE(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_FILE3, GP_FILE2)(A,__VA_ARGS__)

#endif // GP_GENERIC_INCLUDED
