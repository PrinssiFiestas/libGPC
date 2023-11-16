// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file gpc/string.h
 * @brief String data type
 */

#ifndef GPC_STRING_INCLUDED
#define GPC_STRING_INCLUDED

#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/**
 * All functions mutating strings will also null terminate them so strings can
 * be safely used when null-termination is expected. Mutating functions also
 * may allocate so all strings should be freed even if originally allocated on
 * stack.
 *
 * String functions have requirements for all strings passed to them. Easiest
 * way of fulfilling these requirements is creating string using the provided
 * constructors #gpc_str(), #gpc_str_ctor(), #gpc_str_on_stack(), and only
 * modifying strings with the provided functions.
 *
 * However, it might be useful to manually initialize strings for better control
 * on memory usage. Maybe the string data could live in an arena instead of a
 * pointer returned by malloc() or gpc_malloc(). Also accessing members directly
 * might be useful when writing optimized string functions.
 *
 * Here is a checklist of all requirements for all strings:
 * - cstr should never be NULL. The only exceptions are when constructing string
 *   using #gpc_str() or #gpc_str_ctor(). Check their docs for details.
 * - cstr should be null-terminated unless length is provided and capacity is 0.
 * - capacity may be 0 but this means that any mutating string function will
 *   always allocate.
 * - Maximum value for capacity should be 1 byte smaller than the actual
 *   allocated buffer size. This extra byte is reserved for null-terminator.
 * - length should match the calculated length of strlen() e.g. the length of
 *   "trun\0cated string" should be 4.
 * - allocation is reserved for allocations made by string operations. The only
 *   valid values are NULL or pointers returned by gpc_malloc() family of
 *   functions.
 * - allocation should be NULL if cstr lives in stack, an arena, or other user
 *   managed memory block.
 */
typedef struct gpc_String
{
    /// String data
    /** Should be null-terminated if not used as string view and never NULL. */
    char* cstr;

    /// Allocation size
    /**
     * Bytes allocated to string data excluding null-terminator. 0 means that
     * the string is used as string view and any attempt to modify will
     * allocate.
     */
    size_t capacity;

    /** Bytes in string excluding null-terminator. */
    size_t length;

    /// To be passed to gpc_free()
    /**
     * Contains possible heap allocation where cstr lives. NULL if cstr lives
     * on stack or on an arena. Can be passed to gpc_free() but not free().
     * Should always be NULL when initialized.
     */
    void* allocation;
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
 *
 * @warning The returned string has automatic (block scoped) lifetime so it
 * should not be returned or referenced after block!
 */
gpc_String gpc_str_on_stack(
    const char init_literal[GPC_NONNULL],
    size_t capacity/* = sizeof(init_literal) - sizeof('\0') */);

/// Generic constructor
/**
 * Constructs string based on values in @p s. Constructed string will be stored
 * to @p s and will be returned. Passing NULL creates an empty string.
 *
 * This constructor does not try to obfuscate the code by trying to guess what
 * the user wants. Therefore @p s only has a limited set of valid values for any
 * combination of members with defined output listed here:
 * - All members are 0: Creates an empty string without allocating. Allocates
 *   immediately on any modification.
 * - All members are 0 except @p s.cstr: Creates a string view from
 *   null-terminated @p s.cstr with calculated length without allocating.
 *   Allocates immediately on any modification.
 * - All members are 0 except @p s.capacity: Allocates @p s.capacity + 1 bytes
 *   which will be initialized with an empty string. The extra byte will be
 *   reserved for null-terminator but the resulting @p s.capacity will not
 *   include it.
 * - All members are 0 except @p s.cstr and @p s.capacity: Like above but will
 *   be initialized with @p s.cstr.
 * - All members are 0 except @p s.cstr and @p s.length: Creates a string view
 *   with length @p s.length. In this case @p s.cstr does not need to be
 *   null-terminated. Allocates immediately on any modification.
 *
 * For every other combination of values in @p s the output of the constructor
 * is undefined. Use struct initializer if more complex initialization is
 * required.
 *
 * @return newly constructed string. Should be ignored or treated as a read only
 * temporary variable if constructed string is stored in @p s.
 *
 * @warning If @p s is not NULL or a temporary and return value is stored in
 * another variable the 2 variables are INDEPENDENT copies of each other than
 * they point to the same string data. This WILL lead to bugs! Either use @p s
 * and ignore the return value or use NULL or a temporary as the initializer
 * string.
 */
gpc_String gpc_str_ctor(gpc_String* s);

/**
 * @brief Generic string constructor @memberof gpc_String
 *
 * Creates an string by passing a temporary gpc_String object to
 * #gpc_str_ctor(). Check docs for #gpc_str_ctor() for details!
 *
 * @return newly constructed string which should not be ignored!
 */
#define gpc_str(...) \
gpc_str_ctor(&(gpc_String){__VA_ARGS__})

/// @brief Frees recources allocated by @p str @memberof gpc_String
void gpc_str_free(gpc_String str);

/// Frees @p str.allocation and sets all fields to 0. @memberof gpc_String
/**
 * @note This only frees @p str.allocation but does not free the pointer to
 * @p str!
 */
void gpc_str_clear(gpc_String* str);

/// String copying @memberof gpc_String
/**
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 * If allocation fails only @p dest->capacity characters will be copied.
 *
 * @return pointer to @p dest if copying was successful, NULL otherwise.
 */
gpc_String* gpc_str_copy(gpc_String dest[GPC_NONNULL], const gpc_String src);

/// @brief Access character with bounds checking @memberof gpc_String
inline char gpc_str_at(const gpc_String s, size_t i)
{
    return i <= s.length ? s.cstr[i] : '\0';
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
 * instead just moves the @p str.cstr pointer by @p length greatly increasing
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
    str->cstr += start;
    str->length = length;
    str->capacity -= start;
    return str;
}

/// Copy substring @memberof gpc_String
/**
 * Creates a substring from @p src starting from @p &src.cstr[start] ending to
 * @p &src.cstr[start + @p length] and copies it to @p dest allocating if
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

/// Calculate length of C string
/**
 * Same as strlen().
 *
 * @param s should be null-terminated.
 *
 * @return lengt of string in bytes not counting null-terminator.
 */
size_t gpc_strlen(const char s[GPC_NONNULL]);

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
///@endcond

#define gpc_str_on_stack_wout_cap(literal) (gpc_String) \
{ \
    .cstr = (char[]){literal}, \
    .allocation = NULL, \
    .length = sizeof(literal) - 1, \
    .capacity = sizeof(literal) - 1 \
}

#define gpc_str_on_stack_with_cap(literal, cap) (gpc_String) \
{ \
    .cstr = (char[cap + sizeof('\0')]){literal}, \
    .allocation = NULL, \
    .length = sizeof(literal) - 1, \
    .capacity = cap \
}

#endif // GPC_STRING_INCLUDED
