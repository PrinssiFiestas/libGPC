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
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <gpc/memory.h> // TODO just get def for GPAllocator
size_t strlen(const char*);

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
typedef struct GPString
{
    /** String data.
     * May not be null-terminated. Use #gpstr_cstr() to get null-terminated data.
     */
    char* data;

    /** Bytes in string.
     * 0 indicates an empty string regardless of contents in @ref data
     */
    size_t length;

    /** Bytes that can be stored without reallocation.
     */
    size_t capacity;
} GPString;

#if GP_DOXYGEN
/** Stack constructor MACRO @memberof GPString.
 *
 * Creates a string on stack initialized with @p init_literal.
 *
 * @param init_literal must be a string literal to compile.
 * @param square_bracket_enclosed_capacity must be square bracket enclosed e.g.
 * [5] or empty [] to infere capacity from @p init_literal.
 *
 * @return stack allocated string.
 */
GPString gpstr_on_stack(
    square_bracket_enclosed_capacity,
    const char init_literal[GP_NONNULL],
    GPAllocator* allocator/* = NULL */);
#endif // GP_DOXYGEN

// TODO remove static
/** Create string view @memberof GPString.
 */
static inline GPString gpstr(const char cstr[GP_NONNULL])
{
    return (GPString){ (char*)cstr, strlen(cstr) };
}

/** GPString to null-terminated C-string conversion @memberof GPString.
 */
inline const char* gpcstr(GPString str)
{
    str.data[str.length] = '\0';
    return str.data;
}

/** Frees @p str->allocation and sets all fields to 0 @memberof GPString.
 *
 * @note This only frees @p str->allocation. If @p str itself is allocated, it
 * will not be freed.
 */
void gpstr_clear(GPString s[GP_NONNULL], GPAllocator* allocator);

/** String copying @memberof GPString.
 *
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 */
GPString*
gpstr_copy(GPString dest[GP_NONNULL], const GPString src);

/** Check if string is used as string view @memberof GPString.
 */
inline bool gpstr_is_view(GPString s)
{
    return s.capacity == 0;
}

/** Preallocate data @memberof GPString.
 *
 * Allocates at least @p requested_capacity if @p requested_capacity is
 * larger than @p s->capacity does nothing otherwise. Used to control when
 * allocation happens or to preallocate string views.
 *
 * @return @p s or error string if allocation fails.
 */
GPString* gpstr_reserve(
    GPString s[GP_NONNULL],
    size_t requested_capacity,
    GPAllocator allocator[GP_NONNULL]);

/** Turn to substring @memberof GPString.
 *
 * Creates a substring from @p str starting from @p (str.data[start]) ending to
 * @p (str.data[end]).
 *
 * @return @p str on success. If indices are out of bounds the original string
 * will be unmutated and an error string is returned.
 */
GPString* gpstr_slice(
    GPString str[GP_NONNULL],
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
GPString* gpstr_substr(
    GPString dest[GP_NONNULL],
    const GPString src,
    size_t start,
    size_t end);

GPString* gpstr_insert(
    GPString dest[GP_NONNULL],
    size_t pos,
    const GPString src);

/** Count substrings @memberof GPString.
 *
 * Counts all occurrences of needle in haystack. */
size_t gpstr_count(GPString haystack, const GPString needle);

/** Return value for gpstr_find_first() and gpstr_find_last() when not found.
 * @memberof GPString
 */
#define GPC_NOT_FOUND ((size_t)-1)

/** Find substring @memberof GPString.
 *
 * @return index of first occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpstr_find(GPString haystack, const GPString needle);

/** Find last substring @memberof GPString.
 *
 * @return index of last occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpstr_find_last(GPString haystack, const GPString needle);

/** Find and replace substring @memberof GPString.
 *
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
GPString* gpstr_replace(
    GPString haystack[GP_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Find and replace last occurrence of substring @memberof GPString.
 *
 * Allocates if necessary.
 *
 * @return @p haystack or an error string if allocation failed.
 */
 GPString* str_replace_last(
    GPString haystack[GP_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Find and replace all occurrences of substring @memberof GPString.
 *
 * Allocates if necessary.
 *
 * @return @p haystack or an error string if allocation failed.
 */
GPString* gpstr_replace_all(
    GPString haystack[GP_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Compare strings @memberof GPString.
 *
 * @return true if strings in @p s1 and @p s2 are equal, false otherwise
 */
bool gpstr_eq(GPString s1, const GPString s2);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

/**@cond */
#define gpstr_on_stack(cap_in_sqr_brackets, literal) \
(GPString) { \
    .data      = (char cap_in_sqr_brackets){literal}, \
    .length    = sizeof(literal) - 1, \
    .capacity  = sizeof((char cap_in_sqr_brackets){literal}) }
/**@endcond */

#endif // GPSTRING_INCLUDED
