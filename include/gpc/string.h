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
typedef struct GPAllocator GPAllocator;

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

typedef struct GPStringAllocation
{
    /** Allocation size. */
    size_t capacity;

    /** Determines if #gpstr_clear() frees data. */
    bool should_free;

    /** #GPString.data lives somewhere here. */
    char data[];
} GPStringAllocation;

/** Generic string data structure.
 *
 * Members can be initialized with struct altough it is recommended to use the
 * provided constructor macros #gpstr() and #gpstr_on_stack(). After
 * initialization the members should not be written to but the non-private ones
 * can be read.
 */
typedef struct GPString
{
    /** String data. @private
     * May not be null-terminated. Use #gpstr_cstr() to get null-terminated data.
     */
    char* data;

    /** Bytes in string.
     * 0 indicates an empty string regardless of contents in @ref data
     */
    size_t length;

    /** Optional allocator.
     * Uses malloc() and free() if NULL.
     */
    const GPAllocator* allocator;

    /** Allocation data including capacity. */
    GPStringAllocation* allocation;
} GPString;

enum
{
    GPSTR_OUT_OF_BOUNDS,
    GPSTR_ALLOCATION_FAILURE,
    GPSTR_ERROR_LENGTH
};
extern const GPString gpstr_error[GPSTR_ERROR_LENGTH];

/** Stack constructor MACRO @memberof GPString.
 * Creates a string on stack initialized with @p init_literal.
 *
 * @param init_literal must be a string literal to compile.
 * @param capacity Determines size of allocated buffer. Should be a literal
 * constant larger than the length of @p init_literal. Optional variable.
 *
 * @return stack allocated string.
 */
GPString gpstr_on_stack(
    const char init_literal[GP_NONNULL],
    size_t capacity/* = sizeof(init_literal) */,
    GPAllocator* allocator/* = NULL */);

/** String view constructor MACRO @memberof GPString.
 * Creates a string view initialized statically with @p cstr. Any modification
 * will allocate.
 *
 * @param cstr C string to be used as GPString. Assumed to be
 * null-terminated if @p length is not provided.
 *
 * @return stack allocated string view.
 */
GPString gpstr(
    const char cstr[GP_NONNULL],
    size_t length/* = strlen(cstr) */,
    GPAllocator* allocator/* = NULL */);

/** GPString to null-terminated C-string conversion @memberof GPString.
 * Alloates if @p str is a string view  or if capacity is not large enough for
 * null-termination.
 */
const char* gpstr_cstr(GPString str[GP_NONNULL]);

/** Frees @p str->allocation and sets all fields to 0 @memberof GPString.
 * @note This only frees @p str->allocation. If @p str itself is allocated it
 * will not be freed.
 */
void gpstr_clear(GPString str[GP_NONNULL]);

/** String copying @memberof GPString.
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 *
 * @return @p dest if copying was successful, NULL otherwise.
 */
GPString* gpstr_copy(GPString dest[GP_NONNULL], const GPString src);

/** Get allocated block size @memberof GPString.
 * @note @p s.data might have some offset from the allocated block so don't
 * do processing to @p s.data based this!
 */
inline size_t gpstr_capacity(const GPString s)
{
    if (s.allocation != NULL)
        return s.allocation->capacity;
    else return 0;
}

/** Determine if @p s should be freed @memberof GPString.
 */
inline bool gpstr_is_allocated(const GPString s)
{
    if (s.allocation != NULL && s.allocation->should_free)
        return true;
    else return false;
}

/** Access character with bounds checking @memberof GPString.
 */
inline char gpstr_at(const GPString s, size_t i)
{
    return i <= s.length ? s.data[i] : '\0';
}

/** Check if string is used as string view @memberof GPString.
 */
inline bool gpstr_is_view(const GPString s) { return s.allocation == NULL; }

/** Replace character with bounds checking @memberof GPString.
 * @param s Pointer to string for the character to be inserted.
 * @param i Index where character is to be inserted.
 * @param c The character to be inserted.
 *
 * @return @p s or error string if i is out of bounds.
 *
 * @note Allocates if @p s is used as string view.
 */
GPString* gpstr_replace_char(GPString s[GP_NONNULL], size_t i, char c);

/** Preallocate data @memberof GPString.
 * Allocates at least @p requested_capacity if @p requested_capacity is
 * larger than @p s->capacity does nothing otherwise. Used to control when
 * allocation happens or to preallocate string views.
 *
 * @return @p s or error string if allocation fails.
 */
GPString* gpstr_reserve(
    GPString s[GP_NONNULL],
    const size_t requested_capacity);

/** Turn to substring @memberof GPString.
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
 * Counts all occurrences of needle in haystack. */
size_t gpstr_count(const GPString haystack, const GPString needle);

/** Return value for gpstr_find_first() and gpstr_find_last() when not.
 * found. @memberof GPString
 */
#define GPC_NOT_FOUND ((size_t)-1)

/** Find substring @memberof GPString.
 * @return index of first occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpstr_find(const GPString haystack, const GPString needle);

/** Find last substring @memberof GPString.
 * @return index of last occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpstr_find_last(const GPString haystack, const GPString needle);

/** Find and replace substring @memberof GPString.
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
GPString* gpstr_replace(
    GPString haystack[GP_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Find and replace last occurrence of substring @memberof GPString.
 * Allocates if necessary.
 *
 * @return @p haystack or an error string if allocation failed.
 */
 GPString* str_replace_last(
    GPString haystack[GP_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Find and replace all occurrences of substring @memberof GPString.
 * Allocates if necessary.
 *
 * @return @p haystack or an error string if allocation failed.
 */
GPString* gpstr_replace_all(
    GPString haystack[GP_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Compare strings @memberof GPString.
 * @return true if strings in @p s1 and @p s2 are equal, false otherwise
 */
bool gpstr_eq(const GPString s1, const GPString s2);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

/**@cond */
#define gpstr(...) GP_OVERLOAD3(__VA_ARGS__, \
    gpstr_with_allocator, \
    gpstr_with_len, \
    gpstr_wout_len)(__VA_ARGS__)

#define gpstr_on_stack(...) GP_OVERLOAD3(__VA_ARGS__, \
    gpstr_on_stack_with_allocator, \
    gpstr_on_stack_with_cap, \
    gpstr_on_stack_wout_cap)(__VA_ARGS__)
/**@endcond */

#define gpstr_with_allocator(cstr, len, allocator) (GPString) { \
    .data = (cstr), \
    .length = (len), \
    .allocator = (allocator) }

#define gpstr_with_len(cstr, len) (GPString) { \
    .data = (cstr), \
    .length = (len) }

#define gpstr_wout_len(cstr) (GPString) { \
    .data = (cstr), \
    .length = strlen(cstr) }

#define gpstr_on_stack_with_allocator(literal, cap, allocator)  gpstr_ctor( \
    sizeof(GPStringAllocation) + (cap), \
    (char[sizeof(GPStringAllocation) + (cap)]){""}, \
    sizeof((char[cap]){literal}) - 1, \
    literal, \
    allocator)

#define gpstr_on_stack_with_cap(literal, cap)  gpstr_ctor( \
    sizeof(GPStringAllocation) + (cap), \
    (char[sizeof(GPStringAllocation) + (cap)]){""}, \
    sizeof((char[cap]){literal}) - 1, \
    literal, \
    NULL)

#define gpstr_on_stack_wout_cap(literal) gpstr_ctor( \
    sizeof(GPStringAllocation) + sizeof(literal), \
    (char[sizeof(GPStringAllocation) + sizeof(literal)]){""}, \
    sizeof((char[]){literal}) - 1, \
    literal, \
    NULL)

GPString gpstr_ctor(
    size_t mem_size,
    char mem[GP_STATIC sizeof(GPStringAllocation)],
    size_t init_len,
    const char init[GP_NONNULL],
    GPAllocator* allocator);

#endif // GPSTRING_INCLUDED
