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
 * provided constructor macros #gpstr() and #gpstr_on_stack(). After
 * initialization the members should not be written to but the non-private ones
 * can be read.
 */
typedef struct GPString
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
} GPString;

enum
{
    GPSTR_ALLOCATION_FAILURE,
    GPSTR_ERROR_LENGTH
};
extern const GPString GPSTR_ERROR[GPSTR_ERROR_LENGTH];

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
    const char init_literal[GPC_NONNULL],
    size_t capacity/* = sizeof(init_literal) */,
    Allocator* allocator/* = NULL */);

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
    const char cstr[GPC_NONNULL],
    size_t length/* = strlen(cstr) */,
    Allocator* allocator/* = NULL */);

/** Frees @p str->allocation and sets all fields to 0 @memberof GPString.
 * @note This only frees @p str->allocation. If @p str itself is allocated it
 * will not be freed.
 */
void gpstr_clear(GPString str[GPC_NONNULL]);

/** String copying @memberof GPString.
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 *
 * @return @p dest if copying was successful, NULL otherwise.
 */
GPString* gpstr_copy(GPString dest[GPC_NONNULL], const GPString src);

/** Access character with bounds checking @memberof GPString.
 */
inline char gpstr_at(const GPString s, size_t i)
{
    return i <= s.length ? s.data[i] : '\0';
}

/** Check if string is used as string view @memberof GPString.
 */
inline bool gpstr_is_view(const GPString s) { return s.capacity == 0; }

/** Replace character with bounds checking @memberof GPString.
 * @param s Pointer to string for the character to be inserted.
 * @param i Index where character is to be inserted.
 * @param c The character to be inserted.
 *
 * @return @p s or NULL if i is out of bounds.
 *
 * @note Allocates if @p s is used as string view.
 */
GPString* gpstr_replace_char(GPString s[GPC_NONNULL], size_t i, char c);

/** Preallocate data @memberof GPString.
 * Allocates at least @p requested_capacity if @p requested_capacity is
 * larger than @p s->capacity does nothing otherwise. Used to control when
 * allocation happens or to preallocate string views on performance critical
 * applications.
 */
GPString* gpstr_reserve(
    GPString s[GPC_NONNULL],
    const size_t requested_capacity);

/** Turn to substring @memberof GPString.
 * @return @p str.
 */
GPString* gpstr_slice(
    GPString str[GPC_NONNULL],
    size_t start,
    size_t length);

/** Append string @memberof GPString.
 * Appends string in @p src to string in @p dest allocating if necessary.
 * If allocation fails only characters that fit in will be appended.
 *
 * @return @p dest if appending was successful, NULL otherwise.
 */
GPString* gpstr_append(GPString dest[GPC_NONNULL], const GPString src);

/** Prepend string @memberof GPString.
 * Prepends string in @p src to string in @p dest allocating if necessary.
 * If allocation fails only characters that fit in will be prepended.
 *
 * @return @p dest if prepending was successful, NULL otherwise.
 */
GPString* gpstr_prepend(
    GPString dest[GPC_NONNULL],
    const GPString src);

/** Count substrings @memberof GPString.
 * Counts all occurrences of needle in haystack. */
size_t gpstr_count(const GPString haystack, const GPString needle);

/** Copy substring @memberof GPString.
 * Creates a substring from @p src starting from @p &src.data[start] ending to
 * @p (&src.data[start + length]) and copies it to @p dest allocating if
 * necessary.
 *
 * @param dest Resulting substring will be copied here.
 *
 * @return @p dest or NULL if copying fails.
 */
GPString* gpstr_substr(
    GPString dest[GPC_NONNULL],
    const GPString src,
    size_t start,
    size_t length);

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
    GPString haystack[GPC_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Find and replace last occurrence of substring @memberof GPString.
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
 GPString* str_replace_last(
    GPString haystack[GPC_NONNULL],
    const GPString needle,
    const GPString replacement);

/** Find and replace all occurrences of substring @memberof GPString.
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
GPString* gpstr_replace_all(
    GPString haystack[GPC_NONNULL],
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
#define gpstr(...) GPC_OVERLOAD3(__VA_ARGS__, \
    gpstr_with_allocator, \
    gpstr_with_len, \
    gpstr_wout_len)(__VA_ARGS__)

#define gpstr_on_stack(...) GPC_OVERLOAD3(__VA_ARGS__, \
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

#define gpstr_on_stack_with_allocator(literal, cap, allocator) (GPString) {\
    .data = (char[(cap) + 1]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = (cap), \
    .allocator = (allocator) }

#define gpstr_on_stack_with_cap(literal, cap) (GPString) { \
    .data = (char[(cap) + 1]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = (cap) }

#define gpstr_on_stack_wout_cap(literal) (GPString) { \
    .data = (char[]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = sizeof(literal) - 1 }

#endif // GPSTRING_INCLUDED
