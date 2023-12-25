// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file string.h
 * @brief String data type
 */

#ifndef GPC_STRING_INCLUDED
#define GPC_STRING_INCLUDED

#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
size_t strlen(const char*);

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/// Generic string data structure
/**
 * Members can be initialized with struct altough it is recommended to use the
 * provided constructor macros #gpc_str() and #gpc_str_on_stack(). After
 * initialization the members should not be written to but the non-private ones
 * can be read.
 */
typedef struct gpc_String
{
    /// @private String data
    /** May not be null-terminated. Use #gpc_cstr() to get null-terminated data.
     */
    char* data;

    /// Bytes in string
    /** 0 indicates an empty string regardless of contents in @ref data */
    size_t length : CHAR_BIT * sizeof(size_t) - 2;

    /// @private
    /** Leave this as 0 when using struct initializer. */
    size_t has_offset : 2;

    /// Allocation size
    /** Bytes allocated to string data. 0 indicates that the string is used as
     * string view and any attempt to modify will allocate. */
    size_t capacity : CHAR_BIT * sizeof(size_t) - 1;

    /// Determines if has to be freed
    /** Should be false if string is static and true otherwise. Any mutating
     * function will set this to true if they allocate using malloc() or the
     * provided optional allocator. */
    bool is_allocated : 1;

    /// Optional allocator
    /** Uses malloc() and free() if NULL. */
    struct Allocator* allocator;
} gpc_String;

/// Stack constructor MACRO @memberof gpc_String
/**
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
    size_t capacity/* = sizeof(init_literal) */);

/// Static constructor MACRO @memberof gpc_String
/**
 * Creates a string initialized statically with @p init. Any modification will
 * allocate.
 *
 * @return stack allocated instance of @ref gpc_String with static data.
 */
gpc_String gpc_str(const char init[GPC_NONNULL]);

/// Frees @p str.allocation and sets all fields to 0. @memberof gpc_String
/** @note This only frees @p str.allocation but does not free the pointer to
 * @p str!
 */
void gpc_str_clear(gpc_String str[GPC_NONNULL]);

/// String copying @memberof gpc_String
/** Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 * If allocation fails only @p dest->capacity characters will be copied.
 *
 * @return pointer to @p dest if copying was successful, NULL otherwise.
 */
gpc_String* gpc_str_copy(gpc_String dest[GPC_NONNULL], const gpc_String src);

/// @brief Access character with bounds checking @memberof gpc_String
inline char gpc_str_at(const gpc_String s, size_t i)
{
    return i <= s.length ? s.data[i] : '\0';
}

/// @brief Check if string is used as string view @memberof gpc_String
inline bool gpc_str_is_view(const gpc_String s) { return s.capacity == 0; }

/// Insert character with bounds checking @memberof gpc_String
/**
 * @param s Pointer to string for the character to be inserted.
 * @param i Index where character is to be inserted.
 * @param c The character to be inserted.
 *
 * @return Pointer to @p s or NULL if i is out of bounds.
 *
 * @note Allocates if @p s is used as string view.
 */
gpc_String* gpc_str_insert_char(gpc_String s[GPC_NONNULL], size_t i, char c);

/// Truncate string @memberof gpc_String
/**
 * Cuts @p length characters from the end of the string.
 *
 * @return pointer to str.
 */
gpc_String* gpc_str_cut_end(gpc_String str[GPC_NONNULL], size_t length);

/// Truncate beginning of string @memberof gpc_String
/**
 * Cuts @p length characters from start. Does not move the remaining string but
 * instead just moves the @p str.data pointer by @p length greatly increasing
 * perfromance. This also allows optimizing gpc_str_prepend().
 *
 * @return pointer to @p str.
 */
gpc_String* gpc_str_cut_start(gpc_String str[GPC_NONNULL], size_t length);

/// Append string @memberof gpc_String
/**
 * Appends string in @p src to string in @p dest allocating if necessary.
 * If allocation fails only characters that fit in will be appended.
 *
 * @return pointer to @p dest if appending was successful, NULL otherwise.
 */
gpc_String* gpc_str_append(gpc_String dest[GPC_NONNULL], const gpc_String src);

/// Prepend string @memberof gpc_String
/**
 * Prepends string in @p src to string in @p dest allocating if necessary.
 * If allocation fails only characters that fit in will be prepended.
 *
 * @param dest_original_start is used to determine if there's capacity before
 * the string which would greatly improve performance. This might be the case
 * after using gpc_str_cut_start() or gpc_str_slice(). Can be left NULL.
 *
 * @return pointer to @p dest if prepending was successful, NULL otherwise.
 */
gpc_String* gpc_str_prepend(
    gpc_String dest[GPC_NONNULL],
    const gpc_String src,
    const char* dest_original_start);

/// Count substrings @memberof gpc_String
/** Counts all occurrences of needle in haystack. */
size_t gpc_str_count(
    const char haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL]);

/// Turn to substring @memberof gpc_String
/** @return pointer to to modified @p str. */
inline gpc_String* gpc_str_slice(
    gpc_String str[GPC_NONNULL],
    size_t start,
    size_t length)
{
    str->data += start;
    str->length = length;
    str->capacity -= start;
    return str;
}

/// Copy substring @memberof gpc_String
/**
 * Creates a substring from @p src starting from @p &src.data[start] ending to
 * @p &src.data[start + @p length] and copies it to @p dest allocating if
 * necessary.
 *
 * @param dest Resulting substring will be copied here. Can be left NULL to
 * create a new string.
 *
 * @return pointer to @p dest or to a newly created string if @p string is NULL
 * or NULL if allocation fails.
 *
 * @warning If @p dest is not NULL or a temporary and return value is stored in
 * another variable the 2 variables are INDEPENDENT copies of each other than
 * they point to the same string data. This WILL lead to bugs! Either use
 * @p dest and ignore the return value or use NULL or a temporary as the
 * destination string.
 */
gpc_String gpc_str_substr(
    gpc_String* dest,
    const gpc_String src,
    size_t start,
    size_t length);

/**
 * @brief Return value for gpc_str_find_first() and gpc_str_find_last() when not
 * found. @memberof gpc_String
 */
#define GPC_NOT_FOUND ((size_t)-1)

/// Find substring @memberof gpc_String
/**
 * @return index of first occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpc_str_find(
    const char haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL]);

/// Find last substring @memberof gpc_String
/**
 * @return index of last occurrence of @p needle in @p haystack, GPC_NOT_FOUND
 * if not found.
 */
size_t gpc_str_find_last(
    const char haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL]);

/// Find and replace substring @memberof gpc_String
/**
 * Allocates if necessary.
 *
 * @return pointer to @p haystack and NULL if allocation failed.
 */
gpc_String* gpc_str_replace(
    gpc_String haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL],
    const gpc_String replacement);

/// Find and replace last occurrence of substring @memberof gpc_String
/**
 * Allocates if necessary.
 *
 * @return pointer to @p haystack and NULL if allocation failed.
 */
 gpc_String* _str_replace_last(
    gpc_String haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL],
    const gpc_String replacement);

/// Find and replace all occurrences of substring @memberof gpc_String
/**
 * Allocates if necessary.
 *
 * @return pointer to @p haystack and NULL if allocation failed.
 */
gpc_String* gpc_str_replace_all(
    gpc_String haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL],
    const gpc_String replacement);

/// Compare strings @memberof gpc_String
/** @return true if strings in @p s1 and @p s2 are equal, false otherwise */
bool gpc_str_eq(const gpc_String s1, const gpc_String s2);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

///@cond
#define gpc_str_on_stack(...) \
GPC_OVERLOAD2(__VA_ARGS__, gpc_str_on_stack_with_cap, gpc_str_on_stack_wout_cap)(__VA_ARGS__)

#define gpc_str(init) (gpc_String){ .data = (init), .length = strlen(init) }
///@endcond

#define gpc_str_on_stack_wout_cap(literal) (gpc_String) \
{ \
    .data = (char[]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = sizeof(literal) \
}

#define gpc_str_on_stack_with_cap(literal, cap) (gpc_String) \
{ \
    .data = (char[cap + 1]){literal}, \
    .length = sizeof(literal) - 1, \
    .capacity = cap \
}

#endif // GPC_STRING_INCLUDED
