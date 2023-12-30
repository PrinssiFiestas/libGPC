/* MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

/**@file string.h
 * String data type.
 */

#ifndef GPC_STRING_INCLUDED
#define GPC_STRING_INCLUDED

#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
size_t strlen(const char*);
typedef struct Allocator Allocator;

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/** Generic string data structure.
 *
 * Members can be initialized with struct altough it is recommended to use the
 * provided constructor macros #gpc_str() and #gpc_str_on_stack(). After
 * initialization the members should not be written to but the non-private ones
 * can be read.
 */
typedef struct gpc_String
{
    /** String data. @private
     * May not be null-terminated. Use #gpc_cstr() to get null-terminated data.
     */
    char* data;

    /** Bytes in string.
     * 0 indicates an empty string regardless of contents in @ref data
     */
    size_t length : CHAR_BIT * sizeof(size_t) - 1;

    /** Determines if data has offset from allocation. @private
     */
    bool has_offset : 1;

    /** Allocation size.
     * Bytes allocated to string data. 0 indicates that the string is used as
     * string view and any attempt to modify will allocate.
     */
    size_t capacity : CHAR_BIT * sizeof(size_t) - 1;

    /** Determines if has to be freed.
     * Should be false if string is static and true otherwise. Any mutating
     * function will set this to true if they allocate using malloc() or the
     * provided optional allocator.
     */
    bool is_allocated : 1;

    /** Optional allocator.
     * Uses malloc() and free() if NULL.
     */
    Allocator* allocator;
} gpc_String;

enum
{
    GPC_STR_ALLOCATION_FAILURE,
    GPC_STR_ERROR_LENGTH
};
extern const gpc_String GPC_STR_ERROR[GPC_STR_ERROR_LENGTH];

/** Stack constructor MACRO @memberof gpc_String.
 * Creates a string on stack initialized with @p init_literal.
 *
 * @param init_literal must be a string literal to compile.
 * @param capacity Determines size of allocated buffer. Should be a literal
 * constant larger than the length of @p init_literal. Optional variable.
 *
 * @return stack allocated string.
 */
gpc_String gpc_str_on_stack(
    const char init_literal[GPC_NONNULL],
    size_t capacity/* = sizeof(init_literal) */,
    Allocator* allocator/* = NULL */);

/** String view constructor MACRO @memberof gpc_String.
 * Creates a string view initialized statically with @p cstr. Any modification
 * will allocate.
 *
 * @param cstr C string to be used as gpc_String. Assumed to be
 * null-terminated if @p length is not provided.
 *
 * @return stack allocated string view.
 */
gpc_String gpc_str(
    const char cstr[GPC_NONNULL],
    size_t length/* = strlen(cstr) */,
    Allocator* allocator/* = NULL */);

/** Frees @p str->allocation and sets all fields to 0 @memberof gpc_String.
 * @note This only frees @p str->allocation. If @p str itself is allocated it
 * will not be freed.
 */
void gpc_str_clear(gpc_String str[GPC_NONNULL]);

/** String copying @memberof gpc_String.
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 *
 * @return @p dest if copying was successful, NULL otherwise.
 */
gpc_String* gpc_str_copy(gpc_String dest[GPC_NONNULL], const gpc_String src);

/** Access character with bounds checking @memberof gpc_String.
 */
inline char gpc_str_at(const gpc_String s, size_t i)
{
    return i <= s.length ? s.data[i] : '\0';
}

/** Check if string is used as string view @memberof gpc_String.
 */
inline bool gpc_str_is_view(const gpc_String s) { return s.capacity == 0; }

/** Replace character with bounds checking @memberof gpc_String.
 * @param s Pointer to string for the character to be inserted.
 * @param i Index where character is to be inserted.
 * @param c The character to be inserted.
 *
 * @return @p s or NULL if i is out of bounds.
 *
 * @note Allocates if @p s is used as string view.
 */
gpc_String* gpc_str_replace_char(gpc_String s[GPC_NONNULL], size_t i, char c);

/** Preallocate data @memberof gpc_String.
 * Allocates at least @p requested_capacity if @p requested_capacity is
 * larger than @p s->capacity does nothing otherwise. Used to control when
 * allocation happens or to preallocate string views on performance critical
 * applications.
 */
gpc_String* gpc_str_reserve(
    gpc_String s[GPC_NONNULL],
    const size_t requested_capacity);

/** Turn to substring @memberof gpc_String.
 * @return @p str.
 */
gpc_String* gpc_str_slice(
    gpc_String str[GPC_NONNULL],
    size_t start,
    size_t length);

/** Append string @memberof gpc_String.
 * Appends string in @p src to string in @p dest allocating if necessary.
 * If allocation fails only characters that fit in will be appended.
 *
 * @return @p dest if appending was successful, NULL otherwise.
 */
gpc_String* gpc_str_append(gpc_String dest[GPC_NONNULL], const gpc_String src);

/** Prepend string @memberof gpc_String.
 * Prepends string in @p src to string in @p dest allocating if necessary.
 * If allocation fails only characters that fit in will be prepended.
 *
 * @return @p dest if prepending was successful, NULL otherwise.
 */
gpc_String* gpc_str_prepend(
    gpc_String dest[GPC_NONNULL],
    const gpc_String src);

/** Count substrings @memberof gpc_String.
 * Counts all occurrences of needle in haystack. */
size_t gpc_str_count(const gpc_String haystack, const gpc_String needle);

/** Copy substring @memberof gpc_String.
 * Creates a substring from @p src starting from @p &src.data[start] ending to
 * @p (&src.data[start + length]) and copies it to @p dest allocating if
 * necessary.
 *
 * @param dest Resulting substring will be copied here.
 *
 * @return @p dest or NULL if copying fails.
 */
gpc_String* gpc_str_substr(
    gpc_String dest[GPC_NONNULL],
    const gpc_String src,
    size_t start,
    size_t length);

/** Return value for gpc_str_find_first() and gpc_str_find_last() when not.
 * found. @memberof gpc_String
 */
#define GPC_NOT_FOUND ((size_t)-1)

/** Find substring @memberof gpc_String.
 * @return index of first occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpc_str_find(const gpc_String haystack, const gpc_String needle);

/** Find last substring @memberof gpc_String.
 * @return index of last occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpc_str_find_last(const gpc_String haystack, const gpc_String needle);

/** Find and replace substring @memberof gpc_String.
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
gpc_String* gpc_str_replace(
    gpc_String haystack[GPC_NONNULL],
    const gpc_String needle,
    const gpc_String replacement);

/** Find and replace last occurrence of substring @memberof gpc_String.
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
 gpc_String* str_replace_last(
    gpc_String haystack[GPC_NONNULL],
    const gpc_String needle,
    const gpc_String replacement);

/** Find and replace all occurrences of substring @memberof gpc_String.
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
gpc_String* gpc_str_replace_all(
    gpc_String haystack[GPC_NONNULL],
    const gpc_String needle,
    const gpc_String replacement);

/** Compare strings @memberof gpc_String.
 * @return true if strings in @p s1 and @p s2 are equal, false otherwise
 */
bool gpc_str_eq(const gpc_String s1, const gpc_String s2);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

/**@cond */
#define gpc_str(...) GPC_OVERLOAD3(__VA_ARGS__, \
    gpc_str_with_allocator, \
    gpc_str_with_len, \
    gpc_str_wout_len)(__VA_ARGS__)

#define gpc_str_on_stack(...) GPC_OVERLOAD3(__VA_ARGS__, \
    gpc_str_on_stack_with_allocator, \
    gpc_str_on_stack_with_cap, \
    gpc_str_on_stack_wout_cap)(__VA_ARGS__)
/**@endcond */

#define gpc_str_with_allocator(cstr, len, allocator) (gpc_String) { \
    .data = (cstr), \
    .length = (len), \
    .allocator = (allocator) }

#define gpc_str_with_len(cstr, len) (gpc_String) { \
    .data = (cstr), \
    .length = (len) }

#define gpc_str_wout_len(cstr) (gpc_String) { \
    .data = (cstr), \
    .length = strlen(cstr) }

#define gpc_str_on_stack_with_allocator(literal, cap, allocator) (gpc_String) {\
    .data = (char[(cap) + 1]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = (cap), \
    .allocator = (allocator) }

#define gpc_str_on_stack_with_cap(literal, cap) (gpc_String) { \
    .data = (char[(cap) + 1]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = (cap) }

#define gpc_str_on_stack_wout_cap(literal) (gpc_String) { \
    .data = (char[]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = sizeof(literal) - 1 }

#endif // GPC_STRING_INCLUDED
