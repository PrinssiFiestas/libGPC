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
 * be safely used when null termination is expected. Mutating functions also
 * may allocate so all strings should be freed even if originally allocated on
 * stack.
 */
typedef struct gpc_String
{
    /// String data
    /** Should be null terminated. */
    char* cstr;

    /// To be passed to gpc_free()
    /** Contains possible heap allocation where cstr lives. NULL if cstr lives
     *  on stack or on an arena. Can be passed to gpc_free() but not free(). */
    void* allocation;

    /** Bytes in string excluding null terminator. */
    size_t length;

    /// Allocation size
    /** Bytes allocated to string data excluding null terminator. */
    size_t capacity;
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
    size_t capacity/* = sizeof(init_literal) - 1 */);

/// Heap constructor @memberof gpc_String
/**
 * Creates a string on heap initialized with capacity being the next power of 2
 * of the larger one of length of @p init and @p capacity.
 *
 * @param init A null-terminated string used to initialize created string.
 * @param capacity Minimum capacity for newly created string. Can be left 0.
 *
 * @return newly created string.
 */
GPC_NODISCARD gpc_String gpc_str_new(const char* init, size_t capacity);

/// Generic constructor
/**
 * Constructs string based on values in @p s. Constructed string will be stored
 * to @p s and a copy of it is returned. The return value is useful when @p s is
 * anonymous or NULL.
 *
 * @p s.cstr
 * - C string that constructed string will be initialized with.
 * - NULL means that string possibly passed to @p s.allocation will be moved as
 *   is. If @p s.allocation is NULL the newly allocated string will be
 *   initialized with an empty string.
 *
 * @p s.allocation
 * - Any non NULL pointer tells the constructor that memory is allocated by the
 *   user on e.g. stack or arena and no memory needs to be allocated.
 * - Buffer size should be required capacity + 1 byte for null terminator!
 * - Will be turned to NULL in constructed string and moved to s.cstr.
 * - If s.cstr is NULL this will be moved to s.cstr along with it's contents.
 * - If left NULL the constructor allocates memory in it which should be freed.
 *
 * @p s.length
 * - Determines the length of the initializer string in @p s.cstr.
 * - Does not make any difference if @p s.cstr is NULL.
 * - If left 0 @p s.cstr is assumed to be null terminated and it's value will be
 * - calculated with strlen().
 *
 * @p s.capacity
 * - If @p s.allocation is not NULL should be size of buffer. The final
 *   constructed value will be decremented by 1 to reserve space for null
 *   terminator.
 * - If @p s.allocation is NULL determines the amount of memory to be allocated.
 * - If smaller than length of @p s.cstr the final value and the amount of
 *   memory to be allocated is determined by the length of @p s.cstr. This means
 *   that can be left 0 too if @p s.cstr is not NULL.
 *
 * @return newly constructed string. Should be ignored if constructed string is
 * stored in @p s.
 *
 * @warning If @p s is not NULL or a temporary and return value is stored in
 * another variable the 2 variables are INDEPENDENT copies of each other than
 * they point to the same string data. This WILL lead to bugs! Either use @p s
 * and ignore the return value or use NULL or a temporary as the initializer
 * string.
 */
gpc_String gpc_str_constructor(gpc_String* s);

/**
 * @brief Generic string constructor @memberof gpc_String
 *
 * Creates an string by passing a temporary gpc_String object to
 * gpc_str_constructor(). Check docs for gpc_str_constructor() for parameter
 * details.
 *
 * @return newly constructed string which should not be ignored!
 */
#define gpc_str_ctor(...) \
gpc_constructor(&(gpc_String){__VA_ARGS__})

/// Frees @p str.allocation and sets all fields to 0. @memberof gpc_String
/**
 * @note This only frees @p str.allocation but does not free the pointer to
 * @p str!
 */
void gpc_str_free(gpc_String* str);

/// String copying @memberof gpc_String
/**
 * Copies @p src to @p dest allocating if @p dest->capacity is not large enough.
 * If allocation fails only @p dest->capacity characters will be copied.
 *
 * @return pointer to @p dest if copying was successful, NULL otherwise.
 */
gpc_String* gpc_str_copy(gpc_String dest[GPC_NONNULL], const gpc_String src);

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
bool gpc_str_equal(const gpc_String s1, const gpc_String s2);

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
// Generic string macros
// Requires C11 or a compiler that supports _Generic()

// Read-only string for generic string macros.
#define/* const gpc_String */gpc_any_str(s) \
_Generic(s, char*: gpc_temp_str(s), \
            const char*: gpc_temp_str(s), \
            gpc_String: (s))

// Mutable string for generic string macros.
#define/* gpc_String* */gpc_any_mutable_str(s) \
_Generic(s, char*: &gpc_temp_mutable_str(s), \
            gpc_String*: (s), \
            void*: (s)) // NULL

#define/* char* */gpc_any_cstr(s) \
_Generic(s, char*: (s), const char: (s), gpc_String: (s).cstr)

#ifndef gpc_length
#define gpc_length(x) \
_Generic(x, \
    char*: gpc_strlen(x), \
    const char*: gpc_strlen(x), \
    gpc_String: (x).length, \
    gpc_StringView: (x).length, \
    gpc_Array: (x).length, \
    default: gpc_arr_length(x))
#endif

#ifndef gpc_capacity
#define gpc_capacity(x) \
_Generic(x, \
    gpc_String: (x).capacity, \
    gpc_Array: (x).length, \
    default: gpc_arr_capacity(x))
#endif

// Cuts length characters from the end of the string.
#define/* void */gpc_cut_end(s, /* size_t */length) \
_Generic(s, \
    gpc_String*: gpc_str_cut_end(s, length), \
    char*: (void)((s)[gpc_strlen(s) - length] = '\0'))

// Cuts length characters from start.
#define/* void */gpc_cut_start(s, /* size_t */length) \
_Generic(s, \
    gpc_String*: gpc_str_cut_start(s, length), \
    char*: (void)((s) += length))

// Appends string in src to string in dest.
#define/* void */gpc_append(dest, src) \
_Generic(dest, \
    gpc_String*: gpc_str_append(dest, gpc_any_str(src)), \
    char*: strcat(dest, gpc_any_cstr(src)))

// Prepends string in src to string in dest.
#define/* void */gpc_prepend(dest, src) \
gpc_str_prepend(gpc_any_mutable_str(dest), gpc_any_str(src), NULL)

// Counts all occurrences of needle in haystack.
#define/* size_t */gpc_count(haystack, needle) \
gpc_str_count(gpc_any_cstr(haystack), gpc_any_cstr(needle))

// Turns str into substring starting from &str.cstr[start] ending to
// &str.cstr[start + length].
#define/* void */gpc_slice(str, /* size_t */ start, /* size_t */ length) \
gpc_str_slice(gpc_any_mutable_str(str), start, length)

// Creates a substring from src starting from &src.cstr[s] ending to
// &src.cstr[s + l] and copies it to dest allocating if dest.capacity
// is not large enough or dest is NULL. Returns dest or newly allocated string
// if dest is NULL.
#define/* gpc_String */gpc_substr(dest, src, /* size_t */s, /* size_t */l) \
gpc_str_substr(gpc_any_mutable_str(dest), gpc_any_str(src), s, l)

// Returns index of first occurrence of needle in haystack, GPC_NOT_FOUND if not
// found.
#define/* size_t */gpc_find(haystack, needle) \
gpc_str_find(gpc_any_cstr(haystack), gpc_any_cstr(needle))

// Returns index of last occurrence of needle in haystack, GPC_NOT_FOUND if not
// found.
#define/* size_t */gpc_find_last(haystack, needle) \
gpc_str_find_last(gpc_any_cstr(haystack), gpc_any_cstr(needle))

// Replaces first occurrence of needle in haystack with replacement.
#define/* void */gpc_replace(haystack, needle, replacement) \
gpc_str_replace( \
    gpc_any_mutable_str(haystack), \
    gpc_any_cstr(needle), \
    gpc_any_str(replacement))

// Replaces last occurrence of needle in haystack with replacement.
#define/* void */gpc_replace_last(haystack, needle, replacement) \
gpc_str_replace_last( \
    gpc_any_mutable_str(haystack), \
    gpc_any_cstr(needle), \
    gpc_any_str(replacement))

// Replaces all occurrences of needle in haystack with replacement.
#define/* void */gpc_replace_all(haystack, needle, replacement) \
gpc_str_replace_all( \
    gpc_any_mutable_str(haystack), \
    gpc_any_cstr(needle), \
    gpc_any_str(replacement))

// Returns true if contents in s1 and s2 are equal.
#define/* bool */gpc_str_eq(s1, s2) \
gpc_str_equal(gpc_any_str(s1), gpc_any_str(s2))

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

// To be used in function argument by generic string macros.
inline gpc_String gpc_temp_str(const char cstr[GPC_NONNULL])
{
    return (gpc_String){
        .cstr = (char*)cstr, // Should not get mutated!
        .length = gpc_strlen(cstr)};
}

// To be used in function argument by generic string macros. size_in_bytes is
// sizeof(cstr) which is gpc_strlen(cstr) + 1 assuming cstr is null terminated
// char[].
inline gpc_String gpc_temp_mutable_str(
    char cstr[GPC_NONNULL])
{
    return (gpc_String){
        .cstr = cstr,
        .length = gpc_strlen(cstr)};
}

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
