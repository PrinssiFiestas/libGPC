// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file bytes.h
 * Guts of string and array.
 */

#ifndef GP_BYTES_INCLUDED
#define GP_BYTES_INCLUDED

#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>

size_t gp_bytes_copy(
    void*restrict       dest,
    const void*restrict src,
    size_t              src_start,
    size_t              src_end) GP_NONNULL_ARGS();

size_t gp_bytes_repeat(
    void*restrict       dest,
    size_t              count,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

size_t gp_bytes_append(
    void*restrict       dest,
    size_t              dest_size,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

size_t gp_bytes_insert(
    void*restrict       dest,
    size_t              dest_size,
    size_t              pos,
    const void*restrict src,
    size_t              src_size) GP_NONNULL_ARGS();

size_t gp_bytes_replace_range(
    void*restrict       dest,
    const size_t        dest_size,
    const size_t        start,
    const size_t        end,
    const void*restrict replacement,
    const size_t        replacement_length);

// Returns index to the first occurrence of needle in haystack.
size_t gp_bytes_replace(
    void*restrict       haystack,
    size_t              haystack_size,
    const void*restrict needle,
    size_t              needle_size,
    const void*restrict replacement,
    size_t              replacement_size,
    size_t*             optional_in_start_out_first_occurrence_position)
    GP_NONNULL_ARGS(1, 3, 5);

// Returns number of replacements made.
size_t gp_bytes_replace_all(
    void*restrict       haystack,
    size_t              haystack_size,
    const void*restrict needle,
    size_t              needle_size,
    const void*restrict replacement,
    size_t              replacement_size,
    size_t*             optional_replacement_count) GP_NONNULL_ARGS(1, 3, 5);

// The required buffer size is not calculated precicely to increase preformance.
// This means that sometimes may allocate needlessly.
#define/* size_t */gp_bytes_print(bytes_out, ...) \
    GP_BYTES_PRINT(false, false, bytes_out, (size_t)-1, __VA_ARGS__)

// Does not allocate but limits the amount of printed bytes to n. If n is 0
// bytes_out may be NULL.
#define/* size_t */gp_bytes_n_print(bytes_out, n, ...) \
    GP_BYTES_PRINT(false, true, bytes_out, n, __VA_ARGS__)

#define/* size_t */gp_bytes_println(bytes_out, ...) \
    GP_BYTES_PRINT(true, false, bytes_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_bytes_n_println(bytes_out, n, ...) \
    GP_BYTES_PRINT(true, true, bytes_out, n, __VA_ARGS__)

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"
#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

// Flags: 'l' left, 'r' right, 'a' ASCII char set only. Separate flags with |.
// Trims whitespace if char_set is NULL.
size_t gp_bytes_trim(
    void*restrict       bytes,
    size_t              bytes_size,
    void**restrict      optional_out_ptr, // memmoves() if NULL
    const char*restrict optional_char_set,
    int                 flags) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
size_t gp_bytes_to_upper(
    void*restrict bytes,
    size_t        bytes_size) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent.
size_t gp_bytes_to_lower(
    void*restrict bytes,
    size_t        bytes_size) GP_NONNULL_ARGS();

size_t gp_bytes_to_valid(
    void*restrict bytes,
    size_t        bytes_size,
    const char*   replacement) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// Bytes examination

#define GP_NOT_FOUND ((size_t)-1)

size_t gp_bytes_find(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size,
    size_t      start) GP_NONNULL_ARGS();

size_t gp_bytes_find_last(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

size_t gp_bytes_count(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size) GP_NONNULL_ARGS();

bool gp_bytes_equal(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

// Locale dependent!
bool gp_bytes_equal_case(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size) GP_NONNULL_ARGS();

size_t gp_bytes_codepoint_count(
    const void* bytes,
    size_t      bytes_size) GP_NONNULL_ARGS();

bool gp_bytes_is_valid(
    const void* bytes,
    size_t bytes_length) GP_NONNULL_ARGS();

size_t gp_bytes_codepoint_length(
    const void* min_4_bytes) GP_NONNULL_ARGS();

int gp_bytes_case_compare(
    const void*restrict s1,
    size_t              s1_length,
    const void*restrict s2,
    size_t              s2_length) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// Printing

#ifndef GP_ASSERT_INCLUDED
//
struct GPBytesPrintable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum gp_type type;

    // Actual data is in gp_bytes_print_internal() variadic args.
};
#if __STDC_VERSION__ >= 201112L || defined(__COMPCERT__)
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#else
#define GP_PRINTABLE(X) { #X, -1 }
#endif

#endif

size_t gp_bytes_print_internal(
    bool is_println,
    bool is_n,
    void*restrict out,
    size_t n,
    size_t arg_count,
    const struct GPBytesPrintable* objs,
    ...);

#define GP_BYTES_PRINT(IS_PRINTLN, IS_N, OUT, N, ...) \
    gp_bytes_print_internal( \
        IS_PRINTLN, \
        IS_N, \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (struct GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_BYTES_INCLUDED
