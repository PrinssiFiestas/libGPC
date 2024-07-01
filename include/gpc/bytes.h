// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file bytes.h
 * Unsafe and ASCII only, but fast strings.
 */

#ifndef GP_BYTES_INCLUDED
#define GP_BYTES_INCLUDED

#include "attributes.h"
#include "overload.h"
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
#include <printf/printf.h>
#include <sstream>
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

GP_NONNULL_ARGS(1)
size_t gp_bytes_slice(
    void*GP_RESTRICT       dest,
    const void*GP_RESTRICT optional_src, // mutates dest if NULL
    size_t                 start,
    size_t                 end);

GP_NONNULL_ARGS()
size_t gp_bytes_repeat(
    void*GP_RESTRICT       dest,
    size_t                 count,
    const void*GP_RESTRICT src,
    size_t                 src_size);

GP_NONNULL_ARGS()
size_t gp_bytes_append(
    void*GP_RESTRICT       dest,
    size_t                 dest_size,
    const void*GP_RESTRICT src,
    size_t                 src_size);

GP_NONNULL_ARGS()
size_t gp_bytes_insert(
    void*GP_RESTRICT       dest,
    size_t                 dest_size,
    size_t                 pos,
    const void*GP_RESTRICT src,
    size_t                 src_size);

GP_NONNULL_ARGS()
size_t gp_bytes_replace_range(
    void*GP_RESTRICT       dest,
    size_t                 dest_size,
    size_t                 start,
    size_t                 end,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_length);

// Returns index to the first occurrence of needle in haystack.
GP_NONNULL_ARGS(1, 3, 5)
size_t gp_bytes_replace(
    void*GP_RESTRICT       haystack,
    size_t                 haystack_size,
    const void*GP_RESTRICT needle,
    size_t                 needle_size,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_size,
    size_t*                optional_in_start_out_first_occurrence_position);

GP_NONNULL_ARGS(1, 3, 5)
size_t gp_bytes_replace_all(
    void*GP_RESTRICT       haystack,
    size_t                 haystack_size,
    const void*GP_RESTRICT needle,
    size_t                 needle_size,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_size,
    size_t*                optional_replacement_count);

#define/* size_t */gp_bytes_print(bytes_out, ...) \
    GP_BYTES_PRINT(bytes_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_bytes_n_print(bytes_out, n, ...) \
    GP_BYTES_PRINT(bytes_out, n, __VA_ARGS__)

#define/* size_t */gp_bytes_println(bytes_out, ...) \
    GP_BYTES_PRINTLN(bytes_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_bytes_n_println(bytes_out, n, ...) \
    GP_BYTES_PRINTLN(bytes_out, n, __VA_ARGS__)

#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

// Flags: 'l' left, 'r' right, and 'l' | 'r' for both. Trims whitespace if
// char_set is NULL.
GP_NONNULL_ARGS(1)
size_t gp_bytes_trim(
    void*GP_RESTRICT       bytes,
    size_t                 bytes_size,
    void**GP_RESTRICT      optional_out_ptr, // memmove() if NULL
    const char*GP_RESTRICT optional_char_set,
    int                    flags);

GP_NONNULL_ARGS()
size_t gp_bytes_to_upper(
    void*  bytes,
    size_t bytes_size);

GP_NONNULL_ARGS()
size_t gp_bytes_to_lower(
    void*  bytes,
    size_t bytes_size);

GP_NONNULL_ARGS()
size_t gp_bytes_to_valid(
    void*GP_RESTRICT       bytes,
    size_t                 bytes_size,
    const char*GP_RESTRICT replacement);

// ----------------------------------------------------------------------------
// Bytes examination

// Return value for functions returning indices.
#define GP_NOT_FOUND ((size_t)-1)

GP_NONNULL_ARGS()
size_t gp_bytes_find_first(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size,
    size_t      start);

GP_NONNULL_ARGS()
size_t gp_bytes_find_last(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size);

GP_NONNULL_ARGS()
size_t gp_bytes_find_first_of(
    const void* haystack,
    size_t      haystack_size,
    const char* char_set,
    size_t      start);

GP_NONNULL_ARGS()
size_t gp_bytes_find_first_not_of(
    const void* haystack,
    size_t      haystack_size,
    const char* char_set,
    size_t      start);

GP_NONNULL_ARGS()
size_t gp_bytes_count(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size);

GP_NONNULL_ARGS()
bool gp_bytes_equal(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size);

GP_NONNULL_ARGS()
bool gp_bytes_equal_case(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size);

GP_NONNULL_ARGS(1)
bool gp_bytes_is_valid(
    const void* bytes,
    size_t      bytes_size,
    size_t*     optional_invalid_position);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


typedef struct gp_printable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum gp_type type;

    // Actual data is in gp_str_print_internal() variadic args.
} GPPrintable;

#if __STDC_VERSION__ >= 201112L || defined(__COMPCERT__)
#define GP_GENERIC_AVAILABLE 1
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#else
#define GP_PRINTABLE(X) { #X, -1 }
#endif

size_t gp_bytes_print_internal(
    void*GP_RESTRICT out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_bytes_println_internal(
    void*GP_RESTRICT out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#ifndef __cplusplus

#define GP_BYTES_PRINT(OUT, N, ...) \
    gp_bytes_print_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_BYTES_PRINTLN(OUT, N, ...) \
    gp_bytes_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#else // __cplusplus
} // extern "C"
#define GP_STREAM_INSERT(...) <<
#define GP_STREAM_INSERT_SPACE(...) << " " <<

#define GP_BYTES_PRINT(OUT, N, ...) pf_snprintf(OUT, N, "%s", \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT, __VA_ARGS__) \
    ).str().c_str())

#define GP_BYTES_PRINTLN(OUT, N, ...) pf_snprintf(OUT, N, "%s", \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT_SPACE, __VA_ARGS__) << "\n" \
    ).str().c_str())

#endif // __cplusplus

#endif // GP_BYTES_INCLUDED
