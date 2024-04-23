/* MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

/**@file string.h
 * String data type.
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED

#include "attributes.h"
#include "overload.h"
#include <printf/conversions.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

// TODO move this to appropriate header
static inline bool gp_clip_range(size_t* start, size_t* end, size_t limit)
{
    bool clipped = false;

    // TODO confirm that this works with any start and end when limit == 0.

    if (end != NULL && *end > limit) {
        *end = limit;
        clipped = true;
    }

    if (limit == 0) // prevent underflow for start
        limit = 1;

    if (start != NULL && *start >= limit) {
        *start = limit - 1;
        clipped = true;
    }

    return clipped;
}

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

size_t gp_cstr_copy(
    char*restrict dest,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_copy_n(
    char*restrict dest,
    const char*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_slice(
    char* str,
    size_t start,
    size_t end) GP_NONNULL_ARGS();

size_t gp_big_cstr_slice(
    char** str,
    size_t start,
    size_t end) GP_NONNULL_ARGS();

size_t gp_cstr_substr(
    char*restrict dest,
    const char*restrict src,
    size_t start,
    size_t end) GP_NONNULL_ARGS();

size_t gp_cstr_append(
    char*restrict dest,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_append_n(
    char*restrict dest,
    const char*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_insert(
    char*restrict dest,
    size_t pos,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_insert_n(
    char*restrict dest,
    size_t pos,
    const char*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_replace(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_in_start_out_pos) GP_NONNULL_ARGS(1, 2, 3);

size_t gp_cstr_replace_all(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_replacement_count) GP_NONNULL_ARGS(1, 2, 3);

#define/* size_t */gp_cstr_print(char_ptr_out, ...) \
    GP_CSTR_PRINT(false, char_ptr_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_cstr_print_n(char_ptr_out, n, ...) \
    GP_CSTR_PRINT(false, char_ptr_out, n, __VA_ARGS__)

#define/* size_t */gp_cstr_println(char_ptr_out, ...) \
    GP_CSTR_PRINT(true, char_ptr_out, (size_t)-1, __VA_ARGS__)

#define/* size_t */gp_cstr_println_n(char_ptr_out, n, ...) \
    GP_CSTR_PRINT(true, char_ptr_out, n, __VA_ARGS__)

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"
#define GP_ASCII_WHITESPACE " \t\n\v\f\r"

// Flags: 'l' left, 'r' right, 'a' ASCII char set only. Separate flags with |.
size_t gp_cstr_trim(
    char*restrict str,
    const char*restrict optional_char_set, // whitespace if NULL
    int flags) GP_NONNULL_ARGS(1);

size_t gp_big_cstr_trim(
    char*restrict* str,
    const char*restrict optional_char_set,
    int mode) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent. Result length might differ from input string length.
size_t gp_cstr_to_upper(
    char* str) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent. Result length might differ from input string length.
size_t gp_cstr_to_lower(
    char* str) GP_NONNULL_ARGS();

size_t gp_cstr_to_valid(
    char* str,
    const char* replacement) GP_NONNULL_ARGS();

// String examination

#define GP_NOT_FOUND ((size_t)-1)

size_t gp_cstr_find(
    const char* haystack,
    const char* needle,
    size_t start) GP_NONNULL_ARGS();

size_t gp_cstr_find_last(
    const char* haystack,
    const char* needle) GP_NONNULL_ARGS();

size_t gp_cstr_count(
    const char* haystack,
    const char* needle) GP_NONNULL_ARGS();

bool gp_cstr_equal(
    const char* s1,
    const char* s2) GP_NONNULL_ARGS();

bool gp_cstr_equal_case(
    const char* s1,
    const char* s2) GP_NONNULL_ARGS();

size_t gp_cstr_codepoint_count(
    const char* str) GP_NONNULL_ARGS();

bool gp_cstr_is_valid(
    const char* str) GP_NONNULL_ARGS();

size_t gp_cstr_codepoint_length(
    const char* str) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#ifndef GP_ASSERT_INCLUDED
//
struct GPPrintable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum GPType type;

    // Actual data is in pr_cstr_print_internal() variadic args.
};
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#endif

size_t gp_cstr_print_internal(
    int is_println,
    char*restrict out,
    size_t n,
    size_t arg_count,
    const struct GPPrintable* objs,
    ...);

#define GP_CSTR_PRINT(IS_PRINTLN, OUT, N, ...) \
    gp_cstr_print_internal( \
        IS_PRINTLN, \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (struct GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#endif // GP_STRING_INCLUDED
