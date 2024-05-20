// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file string.h
 * String data type.
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED

#include <gpc/memory.h>
#include <gpc/bytes.h>
#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#if __GNUC__ && ! _WIN32
#include <alloca.h>
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

//
typedef struct gp_char { uint8_t c; } GPChar;
typedef struct gp_string_header
{
    size_t length;
    size_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPStringHeader;

typedef GPChar* GPString;

GPString gp_str_new(
    const GPAllocator*,
    size_t capacity) GP_NONNULL_ARGS_AND_RETURN;

#define/* GPString */gp_str_on_stack( \
    optional_allocator_ptr, \
    size_t_capacity, \
    const_char_ptr_init) (GPString) \
(struct GP_C99_UNIQUE_STRUCT(__LINE__) \
{ GPStringHeader header; char data[ (size_t_capacity) + sizeof"" ]; }) { \
{ \
    .length     = sizeof(const_char_ptr_init) - 1, \
    .capacity   = size_t_capacity, \
    .allocator  = optional_allocator_ptr, \
    .allocation = NULL \
}, { const_char_ptr_init } }.data

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, arrays can be created on stack manually. Capacity should be
// initialized to be actual capacity -1 for null termination.
/*
    struct optional_name{ GPStringHeader header; GPChar data[2048]; }my_str_mem;
    my_str_mem.header = (GPStringHeader) {.capacity = 2048 - sizeof"" };
    GPString my_string = my_str_mem.data;
*/

// Passing strings on stack is safe too.
void gp_str_delete(GPString optional_string);

const char* gp_cstr(GPString) GP_NONNULL_ARGS_AND_RETURN;

size_t             gp_str_length    (GPString) GP_NONNULL_ARGS();
size_t             gp_str_capacity  (GPString) GP_NONNULL_ARGS();
void*              gp_str_allocation(GPString) GP_NONNULL_ARGS();
const GPAllocator* gp_str_allocator (GPString) GP_NONNULL_ARGS();

void gp_str_reserve(
    GPString* str,
    size_t       capacity) GP_NONNULL_ARGS();

void gp_str_copy(
    GPString*           dest,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

void gp_str_repeat(
    GPString*           dest,
    size_t              count,
    const void*restrict src,
    size_t              src_length) GP_NONNULL_ARGS();

void gp_str_slice(
    GPString*           dest,
    const void*restrict optional_src, // mutates dest if NULL
    size_t              src_start,
    size_t              src_end) GP_NONNULL_ARGS(1);

void gp_str_append(
    GPString*           dest,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

void gp_str_insert(
    GPString*           dest,
    size_t              pos,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

// Returns index to the first occurrence of needle in haystack.
size_t gp_str_replace(
    GPString*           haystack,
    const void*restrict needle,
    size_t              needle_length,
    const void*restrict replacement,
    size_t              replacement_length,
    size_t              start) GP_NONNULL_ARGS();

// Returns number of replacements made.
size_t gp_str_replace_all(
    GPString*           haystack,
    const void*restrict needle,
    size_t              needle_length,
    const void*restrict replacement,
    size_t              replacement_length) GP_NONNULL_ARGS();

// The required buffer size is not calculated precicely to increase preformance.
// This means that sometimes may allocate needlessly.
#define/* size_t */gp_str_print(str_ptr_out, ...) \
    GP_STR_PRINT(str_ptr_out, __VA_ARGS__)

// Does not allocate but limits the amount of printed bytes to n. If n is 0
// str_ptr_out may be NULL.
#define/* size_t */gp_str_n_print(str_ptr_out, n, ...) \
    GP_STR_N_PRINT(str_ptr_out, n, __VA_ARGS__)

#define/* size_t */gp_str_println(str_ptr_out, ...) \
    GP_STR_PRINTLN(str_ptr_out, __VA_ARGS__)

#define/* size_t */gp_str_n_println(str_ptr_out, n, ...) \
    GP_STR_N_PRINTLN(str_ptr_out, n, __VA_ARGS__)

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"
#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

// Flags: 'l' left, 'r' right, 'a' ASCII char set only. Separate flags with |.
// Trims whitespace if char_set is NULL.
void gp_str_trim(
    GPString*    str,
    const char*  optional_char_set,
    int          flags) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
void gp_str_to_upper(
    GPString* str) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
void gp_str_to_lower(
    GPString* str) GP_NONNULL_ARGS();

void gp_str_to_valid(
    GPString*   str,
    const char* replacement) GP_NONNULL_ARGS();

// Returns false if file operations fail. Check errno for the specific error.
bool gp_str_from_path(
    GPString*   str,
    const char* file_path) GP_NONNULL_ARGS() GP_NODISCARD;

// ----------------------------------------------------------------------------
// String examination

#define GP_NOT_FOUND ((size_t)-1)

size_t gp_str_find(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start) GP_NONNULL_ARGS();

size_t gp_str_find_last(
    GPString    haystack,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

size_t gp_str_count(
    GPString    haystack,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

bool gp_str_equal(
    GPString    s1,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

// Locale dependent!
bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

size_t gp_str_codepoint_count(
    GPString str) GP_NONNULL_ARGS();

bool gp_str_is_valid(
    GPString str) GP_NONNULL_ARGS();

size_t gp_str_codepoint_length(
    GPString str) GP_NONNULL_ARGS();


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#ifdef _MSC_VER
// unnamed struct in parenthesis in gp_str_on_stack()
#pragma warning(disable : 4116)
#endif

size_t gp_str_print_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_n_print_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_println_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_n_println_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#define GP_STR_PRINT(OUT, ...) \
    gp_str_print_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_N_PRINT(OUT, N, ...) \
    gp_str_n_print_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_PRINTLN(OUT, ...) \
    gp_str_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_N_PRINTLN(OUT, N, ...) \
    gp_str_n_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_STRING_INCLUDED
