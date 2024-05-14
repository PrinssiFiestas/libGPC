// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file string.h
 * String data type.
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED

#include <gpc/memory.h>
#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#ifdef __GNUC__
#include <alloca.h>
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

//
typedef struct gp_char
{
    uint8_t c;
} GPChar;

typedef          GPChar*         GPString;
typedef const    GPChar*restrict GPStringIn;
typedef restrict GPString        GPStringOut;

// init : C string literal or GPString if n not given, any char array otherwise.
#define gp_str_new(allocator_ptr, size_t_capacity,/*init, size_t n=0*/...) \
GP_OVERLOAD2(__VA_ARGS__, \
    gp_str_new_init_n, \
    gp_str_new_init)(allocator, size_t_capacity, __VA_ARGS__)

#define GP_NO_ALLOC &gp_crash_on_alloc
#define gp_str_on_stack(optional_allocator, const_capacity, cstr_literal_init) \
    gp_str_on_stack_init(optional_allocator, const_capacity, cstr_literal_init)

GPString gp_str_delete(GPString optional_string);

// TODO get rid of this. That string is not helpful. Also should return nothing
// for defer().
#ifndef gp_clear
#define gp_clear(GPString_me) ( \
    gp_str_delete((void*)(GPString_me)), \
    (void*)"Deallocated at "__FILE__" line "GP_MEM_STRFY(__LINE__) \
)
#endif

const char* gp_cstr(GPString) GP_NONNULL_ARGS_AND_RETURN;

size_t             gp_length    (const void*) GP_NONNULL_ARGS();
size_t             gp_capacity  (const void*) GP_NONNULL_ARGS();
void*              gp_allocation(const void*) GP_NONNULL_ARGS();
const GPAllocator* gp_allocator (const void*) GP_NONNULL_ARGS();

void gp_str_reserve(
    GPStringOut* str,
    size_t       capacity) GP_NONNULL_ARGS();

void gp_str_copy(
    GPStringOut*        dest,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

void gp_str_repeat(
    GPStringOut*        dest,
    size_t              count,
    const void*restrict src,
    size_t              src_length) GP_NONNULL_ARGS();

void gp_str_slice(
    GPStringOut* str,
    size_t       start,
    size_t       end) GP_NONNULL_ARGS();

void gp_str_substr(
    GPStringOut*        dest,
    const void*restrict src,
    size_t              src_start,
    size_t              src_end) GP_NONNULL_ARGS();

void gp_str_append(
    GPStringOut*        dest,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

void gp_str_insert(
    GPStringOut*        dest,
    size_t              pos,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

// Returns index to the first occurrence of needle in haystack.
size_t gp_str_replace(
    GPStringOut* haystack,
    const void*restrict needle,
    size_t needle_length,
    const void*restrict replacement,
    size_t replacement_length,
    size_t start) GP_NONNULL_ARGS();

// Returns number of replacements made.
size_t gp_str_replace_all(
    GPStringOut* haystack,
    const void*restrict needle,
    size_t needle_length,
    const void*restrict replacement,
    size_t replacement_length) GP_NONNULL_ARGS();

// The required buffer size is not calculated precicely to increase preformance.
// This means that sometimes may allocate needlessly.
#define/* size_t */gp_str_print(str_ptr_out, ...) \
    GP_STR_PRINT(false, false, str_ptr_out, (size_t)-1, __VA_ARGS__)

// Does not allocate but limits the amount of printed bytes to n. If n is 0
// str_ptr_out may be NULL.
#define/* size_t */gp_str_n_print(str_ptr_out, n, ...) \
    GP_STR_PRINT(false, true, str_ptr_out, n, __VA_ARGS__)

#define/* size_t */gp_str_println(str_ptr_out, ...) \
    GP_STR_PRINT(true, false, str_ptr_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_str_n_println(str_ptr_out, n, ...) \
    GP_STR_PRINT(true, true, str_ptr_out, n, __VA_ARGS__)

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"
#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

// Flags: 'l' left, 'r' right, 'a' ASCII char set only. Separate flags with |.
// Trims whitespace if char_set is NULL.
void gp_str_trim(
    GPStringOut* str,
    const char*  optional_char_set,
    int          flags) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
void gp_str_to_upper(
    GPStringOut* str) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
void gp_str_to_lower(
    GPStringOut* str) GP_NONNULL_ARGS();

void gp_str_to_valid(
    GPStringOut* str,
    const char*  replacement) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// String examination

#define GP_NOT_FOUND ((size_t)-1)

size_t gp_str_find(
    GPStringIn  haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start) GP_NONNULL_ARGS();

size_t gp_str_find_last(
    GPStringIn  haystack,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

size_t gp_str_count(
    GPStringIn  haystack,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

bool gp_str_equal(
    GPStringIn  s1,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

// Locale dependent!
bool gp_str_equal_case(
    GPStringIn  s1,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

size_t gp_str_codepoint_count(
    GPStringIn str) GP_NONNULL_ARGS();

bool gp_str_is_valid(
    GPStringIn str) GP_NONNULL_ARGS();

size_t gp_str_codepoint_length(
    GPStringIn str) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// Low level access
// Accessing internals is useful for writing optimized custom functionality.
// Any other use is highly discouraged.

#ifndef GP_ARRAY_INCLUDED
typedef struct gp_array_header
{
    uintptr_t length;
    uintptr_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPArrayHeader;
#endif

GPArrayHeader* gp_str_header(const void* arr);
inline GPArrayHeader* gp_str_set(GPStringOut* me) {
    return gp_str_header(*me);
}

// Only use this for newly created strings!
inline GPArrayHeader* gp_str_set_new(GPStringOut* me) {
    return (GPArrayHeader*)*me - 1;
}


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


GPString gp_str_new_init_n(
    const void* allocator,
    size_t capacity,
    const void* init,
    size_t n) GP_NONNULL_ARGS_AND_RETURN;

GPString gp_str_build(
    void* buf,
    const void* allocator,
    size_t capacity,
    const char* init) GP_NONNULL_ARGS(1, 4) GP_NONNULL_RETURN;

// gp_str_new_init()
#if __STDC_VERSION__ >= 201112L
static inline GPString gp_str_new_init_str (const void* a, size_t c, GPStringIn s) {
    return gp_str_new_init_n(a, c, s, gp_length(s));
}
static inline GPString gp_str_new_init_cstr(const void* a, size_t c, const char* s) {
    size_t strlen(const char*);
    return gp_str_new_init_n(a, c, s, strlen(s));
}
#define gp_str_new_init(ALLOCATOR, CAPACITY, INIT) _Generic(INIT, \
    const char*:   gp_str_new_init_cstr, \
    char*:         gp_str_new_init_cstr, \
    const GPChar*: gp_str_new_init_str,  \
    GPString:      gp_str_new_init_str)(ALLOCATOR, CAPACITY, INIT)

#else // less type safety and only accepts GPString or C string literal
#define gp_str_new_init(ALLOCATOR, CAPACITY, INIT) \
    gp_str_new_init_n( \
        ALLOCATOR, \
        CAPACITY, \
        INIT, \
        #INIT[0] == '\"' ? sizeof(INIT) - 1 : gp_str_length((void*)INIT))
#endif

// gp_str_on_stak_init()
#ifdef __GNUC__ // use alloca() which doesn't zero-initialize memory which is
                // faster than using a compound literal. A compound literal in
                // sizeof() gives bounds checking and forces using a literal
                // anyway so alloca() can't be abused.
#define gp_str_on_stack_init(ALLOCATOR, CAPACITY, INIT) \
    ({ \
        void* _gp_str_buf = alloca( \
            sizeof(GPArrayHeader) + (CAPACITY) + 1);\
        gp_str_build( \
            _gp_str_buf, \
            ALLOCATOR, \
            sizeof((char[CAPACITY]){ INIT }), \
            INIT); \
    })
#else // use compound literal to allocate buffer on stack
#define gp_str_on_stack_init(ALLOCATOR, CAPACITY, INIT) \
    gp_str_build( \
        (GPArrayHeader[2 + (CAPACITY)/sizeof(GPArrayHeader)]){0}, \
        ALLOCATOR, \
        sizeof((char[CAPACITY]){ INIT }), \
        INIT)
#endif // gp_str_on_stack_init()

// ----------------------------------------------------------------------------
// Printing

#ifndef GP_ASSERT_INCLUDED
//
struct GPPrintable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum gp_type type;

    // Actual data is in gp_str_print_internal() variadic args.
};
#if __STDC_VERSION__ >= 201112L
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#else
#define GP_PRINTABLE(X) { #X, -1 }
#endif

#endif

size_t gp_str_print_internal(
    bool is_println,
    bool is_n,
    GPStringOut* out,
    size_t n,
    size_t arg_count,
    const struct GPPrintable* objs,
    ...);

#define GP_STR_PRINT(IS_PRINTLN, IS_N, OUT, N, ...) \
    gp_str_print_internal( \
        IS_PRINTLN, \
        IS_N, \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (struct GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_STRING_INCLUDED
