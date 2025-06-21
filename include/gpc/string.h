// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file string.h
 * String data type
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED 1

#include "memory.h"
#include "bytes.h"
#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


/** Distinct character type.
 * This should not be used directly. This only exist to make _Generic()
 * selection work in C and overloads in C++. This should only be used to get
 * byte values from GPString e.g. my_string[i].c.
 */
typedef struct gp_char { uint8_t c; } GPChar;

/** Dynamic string.
 * In memory, a string is GPStringHeader followed by the characters. An object
 * of type GPString is a pointer to the first element.
 *     GPStrings are not null-terminated by design to simplify their usage and
 * to discourage the usage of buggy and slow null-terminated strings. However,
 * null-terminated strings are ubiquitous, so conversion functions are provided.
 */
typedef GPChar* GPString;

/** String meta-data.
 * You can edit the fields directly with ((GPStringHeader)my_string - 1)->field.
 * This might be useful for micro-optimizations, but it is mostly recommended to
 * use the provided functions instead.
 */
typedef struct gp_string_header
{
    uintptr_t          capacity;
    void*              allocation; // pointer to self or NULL if on stack
    const GPAllocator* allocator;
    uintptr_t          length;
} GPStringHeader;

/** Create and initialize a new string.*/
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
GPString gp_str_new(
    const       GPAllocator*,
    size_t      capacity,
    const char* init);

/** Create a new dynamic string on stack.
 * @p allocator_ptr determines how the string will be reallocated if length
 * exceeds capacity. If it is known that length will not exceed capacity,
 * @p allocator_ptr can be left NULL.
 * Not available in C++.
 */
#define/* GPString */gp_str_on_stack( \
    optional_allocator_ptr, \
    size_t_capacity, \
    /*optional_cstr_literal_init*/...) (GPString) \
    \
    GP_STR_ON_STACK(optional_allocator_ptr, size_t_capacity,__VA_ARGS__)

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, strings can be created on stack manually. This is required in
// C++ which does not have gp_str_on_stack(). Capacity should be initialized to
// be actual capacity - 1 for null-termination. Example using memset():
/*
    struct optional_name{GPStringHeader header; GPChar data[2048];} my_str_mem;
    memset(&my_str_mem.header, 0, sizeof my_str_mem.header);
    my_str_mem.header.capacity = 2048 - sizeof"";
    GPString my_string = my_str_mem.data;
*/
// or more concisely: (C only)
/*
    struct { GPStringHeader h; GPChar data[2048];} str_mem;
    str_mem.h = (GPStringHeader){.capacity = 2048 };
    GPString str = str_mem.data;
*/

/** Getters */
size_t             gp_str_length    (GPString) GP_NONNULL_ARGS() GP_NODISCARD;
size_t             gp_str_capacity  (GPString) GP_NONNULL_ARGS() GP_NODISCARD;
const GPAllocator* gp_str_allocator (GPString) GP_NONNULL_ARGS() GP_NODISCARD;
void*              gp_str_allocation(GPString) GP_NONNULL_ARGS() GP_NODISCARD;

/** Free string memory.
 * Passing strings on stack is safe too.
 */
inline void gp_str_delete(GPString optional)
{
    if (optional != NULL && gp_str_allocation(optional) != NULL)
        gp_mem_dealloc(gp_str_allocator(optional), gp_str_allocation(optional));
}

/** Free string memory trough pointer.
 * This should be used as destructor for GPDictionary(GPString) if needed.
 */
inline void gp_str_ptr_delete(GPString* optional)
{
    if (optional != NULL)
        gp_str_delete(*optional);
}

/** Convert to null-terminated string.
 * Adds null-terminator at the end of the string without allocating.
 */
const char* gp_cstr(GPString) GP_NONNULL_ARGS_AND_RETURN;

/** Set length to 0 without changing capacity. */
void gp_str_clear(GPString*) GP_NONNULL_ARGS();

/** Reserve capacity.
 * If @p capacity > gp_str_capacity(@p *str), reallocates, does nothing
 * otherwise. In case of reallocation, capacity will be rounded up to the next
 * power of two.
 */
GP_NONNULL_ARGS()
void gp_str_reserve(
    GPString* str,
    size_t    capacity);

/** Copy source string to destination.*/
GP_NONNULL_ARGS()
void gp_str_copy(
    GPString*              dest,
    const void*GP_RESTRICT src,
    size_t                 src_size);

/** Copy source string to destination many times.
 * Copies @p src to @p dest and appends @p src to it count - 1 times.
 */
GP_NONNULL_ARGS()
void gp_str_repeat(
    GPString*              dest,
    size_t                 count,
    const void*GP_RESTRICT src,
    size_t                 src_length);

/** Copy or remove characters.
 * Copies characters from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, characters from @p str outside
 * @p start_index and @p end_index are removed and the remaining characters are
 * moved over.
 */
GP_NONNULL_ARGS(1)
void gp_str_slice(
    GPString*              str,
    const void*GP_RESTRICT optional_src,
    size_t                 start,
    size_t                 end);

/** Add characters to the end.*/
GP_NONNULL_ARGS()
void gp_str_append(
    GPString*              dest,
    const void*GP_RESTRICT src,
    size_t                 src_size);

/** Add characters to specified position.
 * Moves rest of the characters over to make room for added characters.
 */
GP_NONNULL_ARGS()
void gp_str_insert(
    GPString*              dest,
    size_t                 pos,
    const void*GP_RESTRICT src,
    size_t                 src_size);

/** Replace substring with other string.
 * Find the first occurrence of @p needle in @p haystack starting from @p start
 * and replace it with @p replacement.
 * @return index to the first occurrence of needle in haystack.
 */
GP_NONNULL_ARGS()
size_t gp_str_replace(
    GPString*              haystack,
    const void*GP_RESTRICT needle,
    size_t                 needle_length,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_length,
    size_t                 start);

/** Replace all substrings with other string.
 * Find the all occurrences of @p needle in @p haystack and replace them with
 * @p replacement. .
 * @return number of replacements made.
 */
GP_NONNULL_ARGS()
size_t gp_str_replace_all(
    GPString*              haystack,
    const void*GP_RESTRICT needle,
    size_t                 needle_length,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_length);

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"

#define GP_LEFT  'l'
#define GP_RIGHT 'r'
#define GP_ASCII 'a'

/** Trim characters.
 * @p flags: 'l' or GP_LEFT, 'r' or GP_RIGHT, 'a' or GP_ASCII for ASCII char set
 * only. Separate flags with |. Trims whitespace if @p char_set is NULL.
 */
GP_NONNULL_ARGS(1)
void gp_str_trim(
    GPString*,
    const char* optional_char_set,
    int         flags);

/** Simple Unicode upcasing.
 * Only converts Unicode characters with 1:1 mapping.
 */
GP_NONNULL_ARGS()
void gp_str_to_upper(
    GPString*);

/** Simple Unicode downcasing.
 * Only converts Unicode characters with 1:1 mapping.
 */
GP_NONNULL_ARGS()
void gp_str_to_lower(
    GPString*);

// Unicode standard recommends using this as replacement character for invalid
// bytes.
#define GP_REPLACEMENT_CHARACTER "\uFFFD" // �

/** Make string valid UTF-8.
 * Converts all invalid bytes with @p replacement.
 */
GP_NONNULL_ARGS()
void gp_str_to_valid(
    GPString*   str,
    const char* replacement);

/** Quick file operations.
 * Opens file in file_path, performs a file operation, and closes it. If
 * operation[0] == 'r', reads the whole file and stores to str. If
 * operation[0] == 'w', writes full contents of str to the file. If
 * operation[0] == 'a', appends fulll contents of str to the file.
 * Files are always opened in binary mode by default. Add "x" or "text" anywhere
 * in operation string to open in text mode. This makes no difference in POSIX,
 * but in Windows adds processing to "\n" which is unnecessary in 2024.
 * Returns  0 on success.
 * Returns -1 if file operations fail. Check errno for the specific error.
 * Returns  1 if file size > SIZE_MAX in 32-bit systems.
 */
GP_NONNULL_ARGS() GP_NODISCARD
int gp_str_file(
    GPString*   src_or_dest,
    const char* file_path,
    const char* operation);

// ----------------------------------------------------------------------------
// String formatting

// Strings can be formatted without format specifiers with gp_str_print()
// family of macros if C11 or higher or C++. If not C++ format specifiers can be
// added optionally for more control. C99 requires format strings. There can be
// multiple format strings with an arbitrary amount of format specifiers.
// Silly example:
/*
    gp_str_print(&my_str, 1, 2, "%u%u", 3u, 4u, "%x", 5); // copies "12345"
 */
// See the tests for more detailed examples.

/** Copy formatted string allocating as needed.*/
#define/* size_t */gp_str_print(str_ptr_out, ...) \
    GP_STR_PRINT(str_ptr_out, __VA_ARGS__)

/** Copy max n formatted characters allocating max 1 times.*/
#define/* size_t */gp_str_n_print(str_ptr_out, n, ...) \
    GP_STR_N_PRINT(str_ptr_out, n, __VA_ARGS__)

/** Like gp_str_print() but add spaces between args and add newline.*/
#define/* size_t */gp_str_println(str_ptr_out, ...) \
    GP_STR_PRINTLN(str_ptr_out, __VA_ARGS__)

/** Like gp_str_n_print() but add spaces between args and add newline.*/
#define/* size_t */gp_str_n_println(str_ptr_out, n, ...) \
    GP_STR_N_PRINTLN(str_ptr_out, n, __VA_ARGS__)

// ----------------------------------------------------------------------------
// String examination

/** Find substring.
 * @return index to the first occurrence of @p needle in @p haystack starting
 * from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_find_first(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start);

/** Find substring from right.
 * @return index to the last occurrence of @p needle in @p haystack or
 * GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_find_last(
    GPString    haystack,
    const void* needle,
    size_t      needle_size);

/** Find codepoints.
 * @return index to the first occurrence of any codepoints in @p utf8_char_set
 * starting from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_find_first_of(
    GPString    haystack,
    const char* utf8_char_set,
    size_t      start);

/** Find codepoints exclusive.
 * @return index to the first occurrence of any codepoints not in
 * @p utf8_char_set starting from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_find_first_not_of(
    GPString    haystack,
    const char* utf8_char_set,
    size_t      start);

/** Count substrings.
 * @return the number of @p needles found in @p haystack.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_count(
    GPString    haystack,
    const void* needle,
    size_t      needle_size);

/** Compare strings.*/
GP_NONNULL_ARGS() GP_NODISCARD
bool gp_str_equal(
    GPString    s1,
    const void* s2,
    size_t      s2_size);

/** Case insensitive string comparison.
 * Compare strings according to Unicode simple case folding rules.
 */
GP_NONNULL_ARGS() GP_NODISCARD
bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size);

/** Count Unicode code points.*/
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_codepoint_count(
    GPString str);

/** Check if string is valid UTF-8.*/
GP_NONNULL_ARGS(1) GP_NODISCARD
bool gp_str_is_valid(
    GPString str,
    size_t*  optional_out_invalid_position);

// More string functions in unicode.h


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#ifdef _MSC_VER
// unnamed struct in parenthesis in gp_str_on_stack()
#pragma warning(disable : 4116)
#endif

size_t gp_str_print_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_n_print_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_println_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_str_n_println_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#ifndef __cplusplus

#define/* GPString */GP_STR_ON_STACK( \
    optional_allocator_ptr, \
    size_t_capacity, \
    ...) (GPString) \
(struct GP_C99_UNIQUE_STRUCT(__LINE__) \
{ GPStringHeader header; char data[ (size_t_capacity) + sizeof"" ]; }) { \
{ \
    .length     = sizeof(""__VA_ARGS__) - 1, \
    .capacity   = size_t_capacity, \
    .allocator  = optional_allocator_ptr, \
    .allocation = NULL \
}, {""__VA_ARGS__} }.data

#define GP_STR_PRINT(OUT, ...) \
    gp_str_print_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#define GP_STR_N_PRINT(OUT, N, ...) \
    gp_str_n_print_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#define GP_STR_PRINTLN(OUT, ...) \
    gp_str_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#define GP_STR_N_PRINTLN(OUT, N, ...) \
    gp_str_n_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#else // __cplusplus
} // extern "C"

static inline std::ostream& operator<<(std::ostream& out, GPString str)
{
    out.write((const char*)str, gp_str_length(str));
    return out;
}

static inline size_t gp_str_copy_cppstr(GPString* out, const size_t n, std::string s)
{
    const size_t length = n < s.length() ? n : s.length();
    gp_str_reserve(out, length);
    memcpy(*out, s.data(), length);
    ((GPStringHeader*)*out - 1)->length = length;
    return s.length();
}

#define GP_STR_PRINT(OUT, ...) GP_STR_N_PRINT(OUT, SIZE_MAX, __VA_ARGS__)
#define GP_STR_N_PRINT(OUT, N, ...) gp_str_copy_cppstr(OUT, N, \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT, __VA_ARGS__) \
    ).str())

#define GP_STR_PRINTLN(OUT, ...) GP_STR_N_PRINTLN(OUT, SIZE_MAX, __VA_ARGS__)
#define GP_STR_N_PRINTLN(OUT, N, ...) gp_str_copy_cppstr(OUT, N, \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT_SPACE, __VA_ARGS__) << "\n" \
    ).str())

#endif // __cplusplus

#endif // GP_STRING_INCLUDED
