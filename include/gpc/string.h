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
size_t strlen(const char*);

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/** Return values for most functions @memberof GPString.
 */
enum GPStringError
{
    GPSTR_NO_ERROR,
    GPSTR_OUT_OF_BOUNDS,
    GPSTR_ALLOCATION_FAILURE,
    GPSTR_ERROR_LENGTH
};

/** Return value for #GPString.error_handler @memberof GPString.
 */
enum GPStringErrorHandling
{
    /** Abort processing and return.
     */
    GPSTR_RETURN   = -1,

    /** Let the function decide.
     */
    GPSTR_DEFAULT  =  0,

    /** Try to continue processing dispite error.
     * E.g. if allocation fails during string copying, just copy as much as the
     * original string can hold.
     */
    GPSTR_CONTINUE =  1,
};

/** Mutable string data structure.
 *
 * Members can be initialized with struct altough it is recommended to use the
 * provided constructor macros #gpstr() and #gpstr_on_stack().
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

    /** Memory allocated by @ref allocator.
     * NULL for stack allocated strings.
     */
    char* allocation;

    /** Optional explicit allocator required for dynamic strings.
     * NULL is not a valid allocator so allocation attempts will lead to
     * GPSTR_ALLOCATION_FAILURE. Use a allocator from memory.h to make strings
     * dynamic.
     */
    const GPAllocator* allocator;

    /** Optional error handling callback.
     * The return value is used by processing functions to determine if they
     * should try to continue processing dispite errors. Also any other debug
     * code can be executed e.g. writing debug logs, aborting execution etc.
     */
    enum GPStringErrorHandling (*error_handler)(
        struct GPString* me,
        enum GPStringError code);
} GPString;

/** Call error_handler callback @memberof GPString.
 */
inline enum GPStringErrorHandling
gpstr_handle_error(GPString me[GP_NONNULL], enum GPStringError code)
{
    if (me->error_handler != NULL)
        return me->error_handler(me, code);
    else return GPSTR_DEFAULT;
}

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

/** GPString to null-terminated C-string conversion @memberof GPString.
 *
 * Alloates if capacity is not large enough for null-termination.
 */
const char* gpcstr(GPString str[GP_NONNULL]);

/** Frees @p str->allocation and sets all fields to 0 @memberof GPString.
 *
 * @note This only frees @p str->allocation. If @p str itself is allocated, it
 * will not be freed.
 */
void gpstr_clear(GPString str[GP_NONNULL]);

/** String copying @memberof GPString.
 *
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 */
enum GPStringError
gpstr_copy(GPString dest[GP_NONNULL], const char src[GP_NONNULL]);

/** Access character with bounds checking @memberof GPString.
 */
inline char gpstr_at(GPString s, size_t i)
{
    return i <= s.length ? s.data[i] : '\0';
}

/** Check if string is used as string view @memberof GPString.
 */
inline bool gpstr_is_view(GPString s)
{
    return s.capacity == 0;
}

/** Replace character with bounds checking @memberof GPString.
 *
 * @param s Pointer to string for the character to be inserted.
 * @param i Index where character is to be inserted.
 * @param c The character to be inserted.
 *
 * @return @p s or error string if i is out of bounds.
 *
 * @note Allocates if @p s is used as string view.
 */
enum GPStringError gpstr_replace_char(GPString s[GP_NONNULL], size_t i, char c);

/** Preallocate data @memberof GPString.
 *
 * Allocates at least @p requested_capacity if @p requested_capacity is
 * larger than @p s->capacity does nothing otherwise. Used to control when
 * allocation happens or to preallocate string views.
 *
 * @return @p s or error string if allocation fails.
 */
enum GPStringError gpstr_reserve(
    GPString s[GP_NONNULL],
    const size_t requested_capacity);

/** Turn to substring @memberof GPString.
 *
 * Creates a substring from @p str starting from @p (str.data[start]) ending to
 * @p (str.data[end]).
 *
 * @return @p str on success. If indices are out of bounds the original string
 * will be unmutated and an error string is returned.
 */
enum GPStringError gpstr_slice(
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
enum GPStringError gpstr_substr(
    GPString dest[GP_NONNULL],
    const char src[GP_NONNULL],
    size_t start,
    size_t end);

enum GPStringError gpstr_insert(
    GPString dest[GP_NONNULL],
    size_t pos,
    const char src[GP_NONNULL]);

/** Count substrings @memberof GPString.
 *
 * Counts all occurrences of needle in haystack. */
size_t gpstr_count(GPString haystack, const char needle[GP_NONNULL]);

/** Return value for gpstr_find_first() and gpstr_find_last() when not found.
 * @memberof GPString
 */
#define GPC_NOT_FOUND ((size_t)-1)

/** Find substring @memberof GPString.
 *
 * @return index of first occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpstr_find(GPString haystack, const char needle[GP_NONNULL]);

/** Find last substring @memberof GPString.
 *
 * @return index of last occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpstr_find_last(GPString haystack, const char needle[GP_NONNULL]);

/** Find and replace substring @memberof GPString.
 *
 * Allocates if necessary.
 *
 * @return @p haystack and NULL if allocation failed.
 */
enum GPStringError gpstr_replace(
    GPString haystack[GP_NONNULL],
    const char needle[GP_NONNULL],
    const char replacement[GP_NONNULL]);

/** Find and replace last occurrence of substring @memberof GPString.
 *
 * Allocates if necessary.
 *
 * @return @p haystack or an error string if allocation failed.
 */
 enum GPStringError str_replace_last(
    GPString haystack[GP_NONNULL],
    const char needle[GP_NONNULL],
    const char replacement[GP_NONNULL]);

/** Find and replace all occurrences of substring @memberof GPString.
 *
 * Allocates if necessary.
 *
 * @return @p haystack or an error string if allocation failed.
 */
enum GPStringError gpstr_replace_all(
    GPString haystack[GP_NONNULL],
    const char needle[GP_NONNULL],
    const char replacement[GP_NONNULL]);

/** Compare strings @memberof GPString.
 *
 * @return true if strings in @p s1 and @p s2 are equal, false otherwise
 */
bool gpstr_eq(GPString s1, const char s2[GP_NONNULL]);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

/**@cond */
#define gpstr_on_stack(cap_in_sqr_brackets, ...) GP_OVERLOAD2(__VA_ARGS__, \
    gpstr_on_stack_with_allocator, \
    gpstr_on_stack_wout_allocator)(cap_in_sqr_brackets, __VA_ARGS__)
/**@endcond */

#define gpstr_on_stack_wout_allocator(cap_in_sqr_brackets, literal) \
(GPString) { \
    .data      = (char cap_in_sqr_brackets){literal}, \
    .length    = sizeof(literal) - 1, \
    .capacity  = sizeof((char cap_in_sqr_brackets){literal}) }

#define gpstr_on_stack_with_allocator(cap_in_sqr_brackets, literal, mallocator) \
(GPString) { \
    .data      = (char cap_in_sqr_brackets){literal}, \
    .length    = sizeof(literal) - 1, \
    .capacity  = sizeof((char cap_in_sqr_brackets){literal}), \
    .allocator = (mallocator) }

#endif // GPSTRING_INCLUDED
