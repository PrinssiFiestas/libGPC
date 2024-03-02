/* MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

/**@file string.h
 * String data type.
 */

#ifndef GPSTRING_INCLUDED
#define GPSTRING_INCLUDED

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

// TODO fix all docs

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

/**@cond */
#define gpstr_on_stack(cap_in_sqr_brackets, literal) \
(struct GPString) { \
    .data      = (char cap_in_sqr_brackets){literal}, \
    .length    = sizeof(literal) - 1, }

static inline size_t gp_btoa(size_t n, char* buf, bool x)
{
    char* strcpy(char* restrict, const char* restrict);
    (void)n; // n is just here for the macro below and can be ignored
    strcpy(buf, x ? "true" : "false");
    return x ? 4 : 5;
}

static inline size_t gp_ptoa(size_t n, char* buf, void* x)
{
    return pf_xtoa(n, buf, (uintptr_t)x);
}

static inline size_t gp_ctoa(size_t n, char* buf, unsigned char x)
{
    if (n)
        buf[0] = x + '0';
    return 1;
}

// Since length of x is not known, it must not be copied to buf but directly to
// destination string instead. We have to store the pointer value for lazy
// reading though.
static inline size_t gp_stoa(size_t n, char* buf, const char* x)
{
    (void)n;
    buf[0] = 0; // This tells gpstr_print_internal() to read as char*
    void* memcpy(void* restrict, const void* restrict, size_t);
    memcpy(buf + 1, &x, sizeof(x));
    return sizeof(uintptr_t) + 1;
}

static inline size_t gp_Stoa(size_t n, char* buf, const struct GPString x)
{
    (void)n;
    buf[0] = 1; // This tells gpstr_print_internal() to read as GPString
    void* memcpy(void* restrict, const void* restrict, size_t);
    memcpy(buf + 1, &x, sizeof(x));
    return sizeof(x) + 1;
}

#define GPSTR_TO_CSTR(VAR)       \
_Generic(VAR,                    \
    bool:               gp_btoa, \
    short:              pf_itoa, \
    int:                pf_itoa, \
    long:               pf_itoa, \
    long long:          pf_itoa, \
    unsigned short:     pf_utoa, \
    unsigned int:       pf_utoa, \
    unsigned long:      pf_utoa, \
    unsigned long long: pf_utoa, \
    float:              pf_gtoa, \
    double:             pf_gtoa, \
    char:               gp_ctoa, \
    unsigned char:      gp_ctoa, \
    signed char:        gp_ctoa, \
    char*:              gp_stoa, \
    const char*:        gp_stoa, \
    struct GPString:    gp_Stoa, \
    default:            gp_ptoa) \
        (12, (char[sizeof(struct GPString) + 4]){""}, (VAR))

#define gpstr_print(me, ...) gpstr_print_internal(me, \
    GP_COUNT_ARGS(__VA_ARGS__), \
    GP_PROCESS_ALL_ARGS(GPSTR_TO_CSTR, GP_COMMA, __VA_ARGS__))

struct GPString*
gpstr_print_internal(struct GPString me[GP_NONNULL], unsigned arg_count, ...);
/**@endcond */

#endif // GPSTRING_INCLUDED
