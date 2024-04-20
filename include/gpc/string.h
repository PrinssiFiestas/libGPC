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

size_t gp_cstr_make_valid(
    char* str) GP_NONNULL_ARGS();

// String examination

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
    const char* str,
    size_t* optional_out_codepoint_count) GP_NONNULL_ARGS(1);

bool gp_cstr_is_valid_index(
    const char* str,
    size_t i) GP_NONNULL_ARGS();

// Assumes valid index
size_t gp_cstr_codepoint_size(
    const char* str,
    size_t i) GP_NONNULL_ARGS();





















// TODO GET RID OF OLD STUFF

/** String data structure.
 */
struct GPString
{
    /** String data.
     * May not be null-terminated. Use #gpstr_cstr() to get null-terminated data.
     */
    char* data;

    /** Bytes in string.
     * 0 indicates an empty string regardless of contents in @ref data
     */
    size_t length;
};

#if GP_DOXYGEN
/** Stack constructor MACRO @memberof strGPString.
 *
 * Creates a string on stack initialized with @p init_literal.
 *
 * @param init_literal must be a string literal to compile.
 * @param square_bracket_enclosed_capacity must be square bracket enclosed e.g.
 * [5] or empty [] to infere capacity from @p init_literal.
 *
 * @return stack allocated string.
 */
struct GPString gpstr_on_stack(
    square_bracket_enclosed_capacity,
    const char init_literal[GP_NONNULL],
    GPAllocator* allocator/* = NULL */);
#endif // GP_DOXYGEN

/** Create string view @memberof GPString.
 */
inline struct GPString gpstr(const char cstr[GP_NONNULL])
{
    size_t strlen(const char*);
    return (struct GPString){ (char*)cstr, strlen(cstr) };
}

/** GPString to null-terminated C-string conversion @memberof GPString.
 */
inline const char* gpcstr(struct GPString str)
{
    str.data[str.length] = '\0';
    return str.data;
}

/** String copying @memberof GPString.
 *
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 */
struct GPString*
gpstr_copy(struct GPString dest[GP_NONNULL], const struct GPString src);

/** Turn to substring @memberof GPString.
 *
 * Creates a substring from @p str starting from @p (str.data[start]) ending to
 * @p (str.data[end]).
 */
struct GPString* gpstr_slice(
    struct GPString str[GP_NONNULL],
    size_t start,
    size_t end);

/** Copy substring @memberof GPString.
 *
 * Creates a substring from @p src starting from @p (src.data[start]) ending to
 * @p (src.data[end]) and copies it to @p dest allocating if necessary.
 *
 * @return @p dest or an error string if possible allocation fails.
 * @note Unlike #gpstr_slice() out of bounds indices will not result in to an
 * error but an empty string in dest instead.
 */
struct GPString* gpstr_substr(
    struct GPString dest[GP_NONNULL],
    const struct GPString src,
    size_t start,
    size_t end);

struct GPString* gpstr_insert(
    struct GPString dest[GP_NONNULL],
    size_t pos,
    const struct GPString src);

/** Return value for gpstr_find_first() and gpstr_find_last() when not found.
 * @memberof GPString
 */
#define GP_NOT_FOUND ((size_t)-1)

/** Find substring @memberof GPString.
 *
 * @return index of first occurrence of @p needle in @p haystack, GP_NOT_FOUND
 * if not found.
 */
size_t gpstr_find(
    const struct GPString haystack, const struct GPString needle, size_t start);

/** Find last substring @memberof GPString.
 *
 * @return index of last occurrence of @p needle in @p haystack, GP_NOT_FOUND
 * if not found.
 */
size_t gpstr_find_last(
    const struct GPString haystack, const struct GPString needle);

/** Count substrings @memberof GPString.
 *
 * Counts all occurrences of needle in haystack. */
size_t gpstr_count(const struct GPString haystack, const struct GPString needle);

/** Find and replace substring @memberof GPString.
 *
 * @return index of first occurrence of @p needle in @p me, GP_NOT_FOUND if not
 * found.
 */
size_t gpstr_replace(
    struct GPString me[GP_NONNULL],
    const struct GPString needle,
    const struct GPString replacement,
    size_t start);

/** Find and replace all occurrences of substring @memberof GPString.
 *
 * @return the amount of replacements.
 */
unsigned gpstr_replace_all(
    struct GPString me[GP_NONNULL],
    const struct GPString needle,
    const struct GPString replacement);

#define GPSTR_WHITESPACE " \t\n\v\f\r"

/** Trim chars from the start, end, or both sides of string @memberof GPString.
 *
 * @param mode 'l' to trim from left, 'r' to trim from right, 'l' + 'r' for both
 */
struct GPString* gpstr_trim(
    struct GPString me[GP_NONNULL], const char char_set[GP_NONNULL], int mode);

/** Compare strings @memberof GPString.
 *
 * @return true if strings in @p s1 and @p s2 are equal, false otherwise
 */
bool gpstr_eq(struct GPString s1, const struct GPString s2);

#if GP_DOXYGEN

/** Format string MACRO @memberof GPString.
 */
struct GPString* gpstr_print(struct GPString me[GP_NONNULL], ...);

#endif // GP_DOXYGEN

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










// TODO get rid of old stuff

/**@cond */
#define gpstr_on_stack(cap_in_sqr_brackets, literal) \
(struct GPString) { \
    .data      = (char cap_in_sqr_brackets){literal}, \
    .length    = sizeof(literal) - 1, }

static inline char* gp_btoa(char* buf, bool x)
{
    char* strcpy(char* restrict, const char* restrict);
    return strcpy(buf, x ? "true" : "false");
}

static inline char* gp_ptoa(char* buf, void* x)
{
    pf_xtoa(12, buf, (uintptr_t)x);
    return buf;
}

static inline char* gp_ctoa(char* buf, unsigned char x)
{
    buf[0] = x;
    return buf;
}

// Since length of x is not known, it must not be copied to buf but directly to
// destination string instead. We have to store the pointer value for lazy
// reading though.
static inline char* gp_stoa(char* buf, const char* x)
{
    buf[0] = 0; // This tells gpstr_print_internal() to read as char*
    void* memcpy(void* restrict, const void* restrict, size_t);
    memcpy(buf + 1, &x, sizeof(x));
    return buf;
}

static inline char* gp_Stoa(char* buf, const struct GPString x)
{
    buf[0] = 1; // This tells gpstr_print_internal() to read as GPString
    void* memcpy(void* restrict, const void* restrict, size_t);
    memcpy(buf + 1, &x, sizeof(x));
    return buf;
}

static inline char* gp_itoa(char* buf, const long long x)
{
    pf_itoa(12, buf, x);
    return buf;
}

static inline char* gp_utoa(char* buf, const long long x)
{
    pf_utoa(12, buf, x);
    return buf;
}

static inline char* gp_gtoa(char* buf, const double x)
{
    pf_gtoa(12, buf, x);
    return buf;
}

#define GPSTR_TO_CSTR(VAR)       \
_Generic(VAR,                    \
    bool:               gp_btoa, \
    short:              gp_itoa, \
    int:                gp_itoa, \
    long:               gp_itoa, \
    long long:          gp_itoa, \
    unsigned short:     gp_utoa, \
    unsigned int:       gp_utoa, \
    unsigned long:      gp_utoa, \
    unsigned long long: gp_utoa, \
    float:              gp_gtoa, \
    double:             gp_gtoa, \
    char:               gp_ctoa, \
    unsigned char:      gp_ctoa, \
    signed char:        gp_ctoa, \
    char*:              gp_stoa, \
    const char*:        gp_stoa, \
    struct GPString:    gp_Stoa, \
    default:            gp_ptoa) \
        ((char[sizeof(struct GPString) + 4]){""}, (VAR))

#define gpstr_print(me, ...) gpstr_print_internal(me, \
    GP_COUNT_ARGS(__VA_ARGS__), \
    (char*[]){GP_PROCESS_ALL_ARGS(GPSTR_TO_CSTR, GP_COMMA, __VA_ARGS__)})

struct GPString*
gpstr_print_internal(
    struct GPString me[GP_NONNULL], size_t arg_count, char** args);
/**@endcond */

#endif // GP_STRING_INCLUDED
