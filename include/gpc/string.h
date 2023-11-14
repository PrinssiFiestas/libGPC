// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

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

// All functions mutating strings will also null terminate them so strings can
// be safely used when null termination is expected. Mutating functions also may
// allocate so it's generally recommended to free() all strings.
typedef struct gpc_String
{
    // String data. Should be null terminated.
    char* cstr;

    // Pass this to free() if .cstr may be allocated. Should be NULL if .cstr is
    // not allocated.
    void* allocation;

    // Bytes in string excluding null terminator.
    size_t length;

    // Bytes allocated to string data excluding null terminator. 0 indicates
    // temporary buffer with no bounds checking.
    size_t capacity;
} gpc_String;

// Creates a string on stack initialized with s. s must be a string literal.
// This string has automatic lifetime so it should not be returned or used after
// scope.
#define/* gpc_String */gpc_str_on_stack(/* char[] s, cap = sizeof(s)-1 */...) \
GPC_OVERLOAD2(__VA_ARGS__, gpc_str_on_stack_with_cap, gpc_str_on_stack_wout_cap)(__VA_ARGS__)

// Creates a string on heap initialized with init with capacity being the next
// power of 2 of the larger one of strlen(init) and capacity. init must be null
// terminated.
GPC_NODISCARD gpc_String gpc_str_new(const char* init, size_t capacity);

// Constructs string based on values in s. Constructed string will be stored to
// s and returned.
// s.cstr
// - C string that constructed string will be initialized with.
// - NULL means that string possibly passed to s.allocation will be moved as is.
//   If s.allocation is NULL the newly allocated string will be initialized with
//   an empty string -> "".
// s.allocation
// - Any non NULL pointer tells the constructor that memory is allocated by the
//   user on e.g. stack or arena and no memory needs to be allocated.
// - Buffer size should be required capacity + 1 byte for null terminator!
// - Will be turned to NULL in constructed string and moved to s.cstr.
// - If s.cstr is NULL this will be moved to s.cstr along with it's contents.
// - If left NULL the constructor allocates memory in it which should be freed.
// s.length
// - Determines the length of the initializer string in s.cstr.
// - Does not make any difference if s.cstr is NULL.
// - If left 0 s.cstr is assumed to be null terminated and it's value will be
// - calculated to be strlen(s.cstr).
// s.capacity
// - If s.allocation is not NULL should be size of buffer. The final constructed
//   value will be decremented by 1 to reserve space for null terminator.
// - If s.allocation is NULL determines the amount of memory to be allocated.
// - If smaller than strlen(s.cstr) the final value and the amount of memory to
//   be allocated is determined by the length of s.cstr. This means that can be
//   left 0 too if s.cstr is not NULL.
// Checking examples in ../../tests/test_string.c is highly recommended.
gpc_String gpc_str_constructor(gpc_String* s);

// String constructor that only requires values to be passed. Check docs for
// gpc_str_constructor() for details.
#define/* gpc_String */gpc_str_ctor(...) \
gpc_constructor((gpc_String){__VA_ARGS__})

// Frees str.allocation and sets all fields in str to 0.
void gpc_str_free(gpc_String* str);

// Copies src to dest allocating if dest->capacity is not large enough or dest
// is NULL. Creates new string that should be free()'d if dest is NULL.
// Returns dest or newly allocated string if dest is NULL.
gpc_String gpc_str_copy(gpc_String* dest, const gpc_String src);

// Cuts length characters from the end of the string.
void gpc_str_cut_end(gpc_String str[GPC_NONNULL], size_t length);

// Cuts length characters from start. Does not move the remaining string but
// instead just moves the .cstr pointer by length greatly increasing
// perfromance. This also allows optimizing gpc_str_prepend().
void gpc_str_cut_start(gpc_String str[GPC_NONNULL], size_t length);

// Appends string in src to string in dest.
void gpc_str_append(gpc_String dest[GPC_NONNULL], const gpc_String src);

// Prepends string in src to string in dest. dest_original_start is used to
// determine if there's capacity before the string which would greatly improve
// performance. This might be the case after using gpc_str_cut_start() or
// gpc_str_slice(). If dest is created on the heap or it is known that there is
// no additional capacity dest_original_start can be left NULL otherwise pass
// the original char* used to create dest.
void gpc_str_prepend(
    gpc_String dest[GPC_NONNULL],
    const gpc_String src,
    const char* dest_original_start);

// Counts all occurrences of needle in haystack.
size_t gpc_str_count(
    const char haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL]);

// Turns str into substring starting from &str.cstr[start] ending to
// &str.cstr[start + length].
inline void gpc_str_slice(
    gpc_String str[GPC_NONNULL],
    size_t start,
    size_t length)
{
    str->cstr += start;
    str->length = length;
    str->capacity -= start;
}

// Creates a substring from src starting from &src.cstr[start] ending to
// &src.cstr[start + length] and copies it to dest allocating if dest.capacity
// is not large enough or dest is NULL. Returns dest or newly allocated string
// if dest is NULL.
gpc_String gpc_str_substr(
    gpc_String* dest,
    const gpc_String src,
    size_t start,
    size_t length);

// Return value for gpc_str_find_first() and gpc_str_find_last() when not found.
extern const size_t GPC_NOT_FOUND; // = (size_t)-1;

// Returns index of first occurrence of needle in haystack, GPC_NOT_FOUND if not
// found.
size_t gpc_str_find_first(
    const char haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL]);

// Returns index of last occurrence of needle in haystack, GPC_NOT_FOUND if not
// found.
size_t gpc_str_find(
    const char haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL]);

// Replaces first occurrence of needle in haystack with replacement.
void gpc_str_replace(
    gpc_String haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL],
    const gpc_String replacement);

// Replaces last occurrence of needle in haystack with replacement.
void gpc_str_replace_last(
    gpc_String haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL],
    const gpc_String replacement);

// Replaces all occurrences of needle in haystack with replacement.
void gpc_str_replace_all(
    gpc_String haystack[GPC_NONNULL],
    const char needle[GPC_NONNULL],
    const gpc_String replacement);

// Returns true if contents in s1 and s2 are equal.
bool gpc_str_equal(const gpc_String s1, const gpc_String s2);

// Calculate length of C string.
size_t gpc_strlen(const char s[GPC_NONNULL]);

// ----------------------------------------------------------------------------
// Generic string macros
// Requires C11 or a compiler that supports _Generic()

// Read-only string for generic string macros.
#define/* const gpc_String */gpc_any_str(s) \
_Generic(s, char*: gpc_temp_str(s, sizeof(s)), \
            const char*: gpc_temp_str(s, sizeof(s)), \
            gpc_String: (s))

// Mutable string for generic string macros.
#define/* gpc_String* */gpc_any_mutable_str(s) \
_Generic(s, char*: &gpc_temp_mutable_str(s, sizeof(s)), \
            gpc_String*: (s), \
            void*: (s)) // NULL

// Generic string constructor. Returned string should be free()'d if dest is
// NULL.
#define/* gpc_String */gpc_str(/* gpc_String* */dest, str) \
gpc_str_copy(dest, gpc_any_str(str))

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
#define/* void */gpc_cut_end(s, /* size_t */ length) \
_Generic(s, \
    gpc_String*: gpc_str_cut_end(s, length), \
    char*: (void)((s)[gpc_strlen(s) - length] = '\0'))

// Cuts length characters from start.
#define/* void */gpc_cut_start(s, /* size_t */ length) \
_Generic(s, \
    gpc_String*: gpc_str_cut_start(s, length), \
    char*: (void)((s) += length))

// Appends string in src to string in dest.
#define/* void */gpc_append(dest, src) \
_Generic(dest, \
    gpc_String*: gpc_str_append(dest, gpc_any_str(src)), \
    char*: strcat(dest, (char*)(src)))

// Prepends string in src to string in dest.
#define/* void */gpc_prepend(dest, src) \
gpc_str_prepend(gpc_any_mutable_str(dest), gpc_any_str(src), NULL)

// Counts all occurrences of needle in haystack.
#define/* size_t */gpc_count(haystack, needle) \
gpc_str_count((char*)(haystack), (char*)(needle))

// Turns str into substring starting from &str.cstr[start] ending to
// &str.cstr[start + length].
#define/* void */gpc_slice(str, /* size_t */ start, /* size_t */ length) \
gpc_str_slice(gpc_any_mutable_str(str), start, length)

// Creates a substring from src starting from &src.cstr[s] ending to
// &src.cstr[s + l] and copies it to dest allocating if dest.capacity
// is not large enough or dest is NULL. Returns dest or newly allocated string
// if dest is NULL.
#define/* gpc_String */gpc_substr(dest, src, /* size_t */ s, /* size_t */ l) \
gpc_str_substr(gpc_any_mutable_str(dest), gpc_any_str(src), s, l)

// Returns index of first occurrence of needle in haystack, GPC_NOT_FOUND if not
// found.
#define/* size_t */gpc_find(haystack, needle) \
gpc_str_find((char*)(haystack), (char*)(needle))

// Returns index of last occurrence of needle in haystack, GPC_NOT_FOUND if not
// found.
#define/* size_t */gpc_find_last(haystack, needle) \
gpc_str_find_last((char*)(haystack), (char*)(needle))

// Replaces first occurrence of needle in haystack with replacement.
#define/* void */gpc_replace(haystack, needle, replacement) \
gpc_str_replace( \
    gpc_any_mutable_str(haystack), \
    (char*)(needle), \
    gpc_any_str(replacement))

// Replaces last occurrence of needle in haystack with replacement.
#define/* void */gpc_replace_last(haystack, needle, replacement) \
gpc_str_replace_last( \
    gpc_any_mutable_str(haystack), \
    (char*)(needle), \
    gpc_any_str(replacement))

// Replaces all occurrences of needle in haystack with replacement.
#define/* void */gpc_replace_all(haystack, needle, replacement) \
gpc_str_replace_all( \
    gpc_any_mutable_str(haystack), \
    (char*)(needle), \
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

// To be used in function argument by generic string macros. size_in_bytes is
// sizeof(cstr) which is gpc_strlen(cstr) + 1 assuming cstr is null terminated
// char[].
inline gpc_String gpc_temp_str(const char cstr[GPC_NONNULL], size_t size_in_bytes)
{
    return (gpc_String){
        .cstr = (char*)cstr, // Should not get mutated!
        .length = size_in_bytes == sizeof(char*) ? gpc_strlen(cstr) : size_in_bytes - 1 };
}

// To be used in function argument by generic string macros. size_in_bytes is
// sizeof(cstr) which is gpc_strlen(cstr) + 1 assuming cstr is null terminated
// char[].
inline gpc_String gpc_temp_mutable_str(
    char cstr[GPC_NONNULL],
    size_t size_in_bytes)
{
    return (gpc_String){
        .cstr = cstr,
        .length = size_in_bytes == sizeof(char*) ? gpc_strlen(cstr) : size_in_bytes - 1 };
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
