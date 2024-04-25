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

typedef GPChar* GPString;

// init : C string literal or GPString if n not given, any char array otherwise.
#define gp_str_new(allocator,/*size_t capacity, init, size_t n=0*/...) \
GP_OVERLOAD2(__VA_ARGS__, \
    gp_str_new_init_n, \
    gp_str_new_init)(__VA_ARGS__)

#define gp_str_on_stack(allocator, const_size_t_capacity, cstr_literal_init) \

void gp_str_clear(GPString*);

const char* gp_cstr(GPString);

size_t gp_str_length(GPString) GP_NONNULL_ARGS();
size_t gp_str_capacity(GPString) GP_NONNULL_ARGS();
bool   gp_str_is_allocated(GPString) GP_NONNULL_ARGS();
const struct gp_allocator* gp_str_allocator(GPString) GP_NONNULL_ARGS();

#if 0
size_t gp_cstr_copy(
    char*restrict dest,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_copy_n(
    char*restrict dest,
    const void*restrict src,
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
    const void*restrict src,
    size_t start,
    size_t end) GP_NONNULL_ARGS();

size_t gp_cstr_append(
    char*restrict dest,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_append_n(
    char*restrict dest,
    const void*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_insert(
    char*restrict dest,
    size_t pos,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_insert_n(
    char*restrict dest,
    size_t pos,
    const void*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_replace(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_in_start_out_pos) GP_NONNULL_ARGS(1, 2, 3);

size_t gp_cstr_replace_n(
    char*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    size_t needle_length,
    const void*restrict replacement,
    size_t replacement_length,
    size_t* optional_in_start_out_pos) GP_NONNULL_ARGS(1, 3, 5);

size_t gp_cstr_replace_all(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_replacement_count) GP_NONNULL_ARGS(1, 2, 3);

size_t gp_cstr_replace_all_n(
    char*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    size_t needle_length,
    const void*restrict replacement,
    size_t replacement_length,
    size_t* optional_replacement_count) GP_NONNULL_ARGS(1, 3, 5);

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

size_t gp_cstr_trim_n(
    char*restrict str,
    size_t str_length,
    const char*restrict optional_char_set,
    int flags) GP_NONNULL_ARGS(1);

size_t gp_big_cstr_trim(
    char*restrict* str,
    const char*restrict optional_char_set,
    int mode) GP_NONNULL_ARGS(1);

size_t gp_big_cstr_trim_n(
    char*restrict str,
    size_t str_length,
    const char*restrict optional_char_set,
    int mode) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent. Result length might differ from input string length.
size_t gp_cstr_to_upper(
    char* str) GP_NONNULL_ARGS();

size_t gp_cstr_to_upper_n(
    char* str,
    size_t n) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent. Result length might differ from input string length.
size_t gp_cstr_to_lower(
    char* str) GP_NONNULL_ARGS();

size_t gp_cstr_to_lower_n(
    char* str,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_to_valid(
    char*restrict str,
    const char*restrict replacement) GP_NONNULL_ARGS();

size_t gp_cstr_to_valid_n(
    char*restrict str,
    size_t n,
    const char*restrict replacement) GP_NONNULL_ARGS();

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

#endif // 0

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#ifndef GP_ARRAY_INCLUDED
typedef struct gp_array_header
{
    size_t length;
    size_t capacity;
    struct gp_allocator* allocator;
    void* allocation;
} GPArrayHeader;
#endif

GPString gp_str_new_init_n(
    void* allocator,
    size_t capacity,
    void* init,
    size_t n) GP_NONNULL_ARGS_AND_RETURN;

#define gp_str_new_init(ALLOCATOR, CAPACITY, INIT) \
    gp_str_new_init_n( \
        ALLOCATOR, \
        CAPACITY, \
        INIT, \
        #INIT[0] == '\"' ? sizeof(INIT) - 1 : gp_str_length(INIT))

#ifdef TEST_THIS_LATER // __GNUC__
#define gp_str_on_stack_init(ALLOCATOR, CAPACITY, INIT) \
    ({ \
        GPArrayHeader _gp_header = { \
            .length    = sizeof(INIT) - sizeof"", \
            .capacity  = sizeof((char[CAPACITY]){ INIT }) + sizeof"", \
            .allocator = ALLOCATOR }; \
        GPChar* _gp_str = alloca(sizeof(GPArrayHeader) + _gp_header.capacity); \
        char* strcpy(char*, const char*); \
        strcpy(_gp_str + sizeof _gp_header, INIT); \
        _gp_str + sizeof _gp_header;
    )}
#else

static inline GPString gp_str_build(
    char* buf,
    void* allocator,
    size_t capacity,
    const char* init)
{
    void* memcpy(void*, const void*, size_t);
    size_t strlen(const char*);
    GPArrayHeader header =
        {.length = strlen(init), .capacity = capacity, .allocator = allocator };
    memcpy(buf, &header, sizeof header);
    return memcpy(buf + sizeof header, init, header.length + sizeof"");
}
#define gp_str_on_stack_init(ALLOCATOR, CAPACITY, INIT) \
    gp_str_build( \
        (char[sizeof(GPArrayHeader) + (CAPACITY) + sizeof""]){""}, \
        ALLOCATOR, \
        sizeof((char[CAPACITY]){ INIT }) + sizeof"", \
        INIT)
#endif // __GNUC__ gp_str_on_stack()

#if 0

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
#if __STDC_VERSION__ >= 201112L
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#else
#define GP_PRINTABLE(X) { #X, -1 }
#endif

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

#endif // 0





























// Null terminated C strings TODO GET RID OF THESE

size_t gp_cstr_copy(
    char*restrict dest,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_copy_n(
    char*restrict dest,
    const void*restrict src,
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
    const void*restrict src,
    size_t start,
    size_t end) GP_NONNULL_ARGS();

size_t gp_cstr_append(
    char*restrict dest,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_append_n(
    char*restrict dest,
    const void*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_insert(
    char*restrict dest,
    size_t pos,
    const char*restrict src) GP_NONNULL_ARGS();

size_t gp_cstr_insert_n(
    char*restrict dest,
    size_t pos,
    const void*restrict src,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_replace(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_in_start_out_pos) GP_NONNULL_ARGS(1, 2, 3);

size_t gp_cstr_replace_n(
    char*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    size_t needle_length,
    const void*restrict replacement,
    size_t replacement_length,
    size_t* optional_in_start_out_pos) GP_NONNULL_ARGS(1, 3, 5);

size_t gp_cstr_replace_all(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_replacement_count) GP_NONNULL_ARGS(1, 2, 3);

size_t gp_cstr_replace_all_n(
    char*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    size_t needle_length,
    const void*restrict replacement,
    size_t replacement_length,
    size_t* optional_replacement_count) GP_NONNULL_ARGS(1, 3, 5);

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

size_t gp_cstr_trim_n(
    char*restrict str,
    size_t str_length,
    const char*restrict optional_char_set,
    int flags) GP_NONNULL_ARGS(1);

size_t gp_big_cstr_trim(
    char*restrict* str,
    const char*restrict optional_char_set,
    int mode) GP_NONNULL_ARGS(1);

size_t gp_big_cstr_trim_n(
    char*restrict str,
    size_t str_length,
    const char*restrict optional_char_set,
    int mode) GP_NONNULL_ARGS(1);

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent. Result length might differ from input string length.
size_t gp_cstr_to_upper(
    char* str) GP_NONNULL_ARGS();

size_t gp_cstr_to_upper_n(
    char* str,
    size_t n) GP_NONNULL_ARGS();

// Only converts Unicode characters with 1:1 mapping. Result is locale
// dependent. Result length might differ from input string length.
size_t gp_cstr_to_lower(
    char* str) GP_NONNULL_ARGS();

size_t gp_cstr_to_lower_n(
    char* str,
    size_t n) GP_NONNULL_ARGS();

size_t gp_cstr_to_valid(
    char*restrict str,
    const char*restrict replacement) GP_NONNULL_ARGS();

size_t gp_cstr_to_valid_n(
    char*restrict str,
    size_t n,
    const char*restrict replacement) GP_NONNULL_ARGS();

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
    const enum gp_type type;

    // Actual data is in pr_cstr_print_internal() variadic args.
};
#if __STDC_VERSION__ >= 201112L
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#else
#define GP_PRINTABLE(X) { #X, -1 }
#endif

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
