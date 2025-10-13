// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file bytes.h
 * Unsafe and ASCII only, but fast and re-entrant strings.
 */

#ifndef GP_BYTES_INCLUDED
#define GP_BYTES_INCLUDED 1

#include "attributes.h"
#include "overload.h"
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
#include <sstream>
extern "C" {
GP_NONNULL_ARGS(3) GP_PRINTF(3, 4)
size_t pf_snprintf(char*GP_RESTRICT, size_t, const char*GP_RESTRICT, ...);
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// The bytes interface is meant to be used for lower level operations. It is
// inherently unsafe and static by nature. For dynamic memory or bounds
// checking, use GPString or GPArray instead.
//
// None of these functions null-terminate! Non of these functions bounds check
// either! `dest_size`, when present, indicates the data length already present
// in `dest`, not the buffer size!
//
// Unlike GPString, these are not assumed UTF-8, but ASCII or raw bytes instead.

/** Copy or remove characters.
 * Copies characters from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, characters from @p str outside
 * @p start_index and @p end_index are removed and the remaining characters are
 * moved over.
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS(1)
size_t gp_bytes_slice(
    void*GP_RESTRICT       dest,
    const void*GP_RESTRICT optional_src, // mutates dest if NULL
    size_t                 start,
    size_t                 end);

/** Copy source string to destination many times.
 * Copies @p src to @p dest and appends @p src to it count - 1 times.
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS()
size_t gp_bytes_repeat(
    void*GP_RESTRICT       dest,
    size_t                 count,
    const void*GP_RESTRICT src,
    size_t                 src_size);

/** Add characters to the end.
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS()
size_t gp_bytes_append(
    void*GP_RESTRICT       dest,
    size_t                 dest_size,
    const void*GP_RESTRICT src,
    size_t                 src_size);

/** Add characters to specified position.
 * Moves rest of the characters over to make room for added characters.
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS()
size_t gp_bytes_insert(
    void*GP_RESTRICT       dest,
    size_t                 dest_size,
    size_t                 pos,
    const void*GP_RESTRICT src,
    size_t                 src_size);

/** Replace range of characters with other string.
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS()
size_t gp_bytes_replace_range(
    void*GP_RESTRICT       dest,
    size_t                 dest_size,
    size_t                 start,
    size_t                 end,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_length);


/** Replace substring with other string.
 * Find the first occurrence of @p needle in @p haystack starting from @p start
 * and replace it with @p replacement.
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS(1, 3, 5)
size_t gp_bytes_replace(
    void*GP_RESTRICT       haystack,
    size_t                 haystack_size,
    const void*GP_RESTRICT needle,
    size_t                 needle_size,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_size,
    size_t*                optional_in_start_out_first_occurrence_position);

/** Replace all substrings with other string.
 * Find the all occurrences of @p needle in @p haystack and replace them with
 * @p replacement. .
 * @return the length of resultant string.
 */
GP_NONNULL_ARGS(1, 3, 5)
size_t gp_bytes_replace_all(
    void*GP_RESTRICT       haystack,
    size_t                 haystack_size,
    const void*GP_RESTRICT needle,
    size_t                 needle_size,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_size,
    size_t*                optional_replacement_count);

#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

/** Trim characters.
 * Flags: 'l' left, 'r' right, and 'l' | 'r' for both. Trims whitespace if
 * char_set is NULL.
 */
GP_NONNULL_ARGS(1)
size_t gp_bytes_trim(
    void*GP_RESTRICT       bytes,
    size_t                 bytes_size,
    void**GP_RESTRICT      optional_out_ptr, // memmove() if NULL
    const char*GP_RESTRICT optional_char_set,
    int                    flags);

/** ASCII upcasing.*/
GP_NONNULL_ARGS()
size_t gp_bytes_to_upper(
    void*  bytes,
    size_t bytes_size);

/** ASCII downcasing.*/
GP_NONNULL_ARGS()
size_t gp_bytes_to_lower(
    void*  bytes,
    size_t bytes_size);

/** Make string valid ASCII.
 * Converts all invalid bytes with @p replacement.
 */
GP_NONNULL_ARGS()
size_t gp_bytes_to_valid_ascii(
    void*GP_RESTRICT       bytes,
    size_t                 bytes_size,
    const char*GP_RESTRICT replacement);

// ----------------------------------------------------------------------------
// String formatting

// Strings can be formatted without format specifiers with gp_bytes_print()
// family of macros if C11 or higher or C++. If not C++ format specifiers can be
// added optionally for more control. C99 requires format strings. There can be
// multiple format strings with an arbitrary amount of format specifiers.
// Silly example:
/*
    gp_bytes_print(my_str, 1, 2, "%u%u", 3u, 4u, "%x", 5); // copies "12345"
 */
// See the tests for more detailed examples.

/** Copies formatted string without limiting.*/
#define/* size_t */gp_bytes_print(bytes_out, ...) \
    GP_BYTES_PRINT(bytes_out, (size_t)-1, __VA_ARGS__)

/** Copies formattes string limited to n.*/
#define/* size_t */gp_bytes_n_print(bytes_out, n, ...) \
    GP_BYTES_PRINT(bytes_out, n, __VA_ARGS__)

/** Like gp_bytes_print() but add spaces between args and add newline.*/
#define/* size_t */gp_bytes_println(bytes_out, ...) \
    GP_BYTES_PRINTLN(bytes_out, (size_t)-1, __VA_ARGS__)

/** Like gp_bytes_n_print() but add spaces between args and add newline.*/
#define/* size_t */gp_bytes_n_println(bytes_out, n, ...) \
    GP_BYTES_PRINTLN(bytes_out, n, __VA_ARGS__)

// ----------------------------------------------------------------------------
// Bytes examination

// Return value for functions returning indices.
#define GP_NOT_FOUND ((size_t)-1)

/** Find substring.
 * @return index to the first occurrence of @p needle in @p haystack starting
 * from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_bytes_find_first(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size,
    size_t      start);

/** Find substring from right.
 * @return index to the last occurrence of @p needle in @p haystack or
 * GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_bytes_find_last(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size);

/** Find ASCII characters.
 * @return index to the first occurrence of any characters in @p char_set
 * starting from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_bytes_find_first_of(
    const void* haystack,
    size_t      haystack_size,
    const char* char_set,
    size_t      start);

/** Find ASCII characters exclusive.
 * @return index to the first occurrence of any characters not in
 * @p char_set starting from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_bytes_find_first_not_of(
    const void* haystack,
    size_t      haystack_size,
    const char* char_set,
    size_t      start);

/** Count substrings.
 * @return the number of @p needles found in @p haystack.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_bytes_count(
    const void* haystack,
    size_t      haystack_size,
    const void* needle,
    size_t      needle_size);

/** Compare strings.*/
GP_NONNULL_ARGS() GP_NODISCARD
bool gp_bytes_equal(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size);

/** Case insensitive ASCII string comparison.*/
GP_NONNULL_ARGS() GP_NODISCARD
bool gp_bytes_equal_case(
    const void* s1,
    size_t      s1_size,
    const void* s2,
    size_t      s2_size);

/** Check if string is valid UTF-8.
 * @p optional_invalid_index is only modified if invalid segment is found and is
 * not NULL.
 * @return true if valid, false if not.
 */
GP_NONNULL_ARGS(1)
bool gp_bytes_is_valid_utf8(
    const void* str,
    size_t str_length,
    size_t* optional_invalid_index);

/** Check if string is valid ASCII.
 * @p optional_invalid_index is only modified if invalid segment is found and is
 * not NULL.
 * @return true if valid, false if not.
 */
GP_NONNULL_ARGS(1) GP_NODISCARD
bool gp_bytes_is_valid_ascii(
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

#if GP_HAS_C11_GENERIC
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) } // TODO test if this would work in C++ too
#else
#define GP_PRINTABLE(X) { #X, INT_MAX - (int)(sizeof(X)) }
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
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#define GP_BYTES_PRINTLN(OUT, N, ...) \
    gp_bytes_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
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
