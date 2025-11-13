// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file string.h
 * String data type
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED 1

#include "bytes.h"
#include "array.h"
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


/** Distinct character type */
typedef struct gp_char { uint8_t c; } GPChar;

/** String type.
 * In memory, a string is GPStringHeader followed by the characters. An object
 * of type GPString is a pointer to the first element.
 *     GPStrings are not null-terminated by design to simplify their usage and
 * to discourage the usage of buggy and slow null-terminated strings. However,
 * null-terminated strings are ubiquitous, so conversion functions are provided.
 *     String can be configured to be dynamic or truncating on per object basis.
 * Storing a pointer to an allocator allows the string to reallocate and grow.
 * If the pointer is NULL, the string is considered static and will not
 * reallocate. A static string will be truncated to prevent overflow and the
 * number of truncated elements will be returned by relevant functions.
 *     The weak type system of C makes UTF-8 invariance enforcement too slow and
 * impractical, so it is left for the user (sorry!). A workflow for proper UTF-8
 * handling goes as follows: validate all inputs , do all processing (which
 * may invalidate string due to truncation so beware!), and finally validate
 * the output. Internal functions can deal with invalid UTF-8, but they never
 * automatically convert any invalid sequences, which could lead to data
 * corruption, which is, again, the reason why this is left for the user to do.
 */
typedef GPChar* GPString;

/** Dynamic string.
 * Use this type to signal the reader that this string is supposed to grow by
 * reallocation. This is obviously not enforced by the compiler or even our
 * functions, but you can enforce this simply by asserting that the allcoator is
 * not NULL.
 */
typedef GPString GPStringDynamic;

/** Static string.
 * Use this type to signal the reader that this string is supposed to never
 * reallocate and may truncate or is expected to be truncating.
 */
typedef GPString GPStringStatic;

/** String meta-data.
 * You can edit the fields directly using `gp_str_set(my_str)->field = x`.
 * TODO better docs, which fields are safe to modify and how?
 */
typedef struct gp_string_header
{
    uintptr_t    capacity;
    void*        allocation; // allocated block start or NULL if on stack
    GPAllocator* allocator;  // set this to NULL to make the string truncating (not dynamic)
    uintptr_t    length;
} GPStringHeader;

/** Static string buffer.
 * Used to create a static or stack allocated GPString. Create a variable of
 * this type, then pass it by address to @ref gp_str_buffered() to initialize
 * and convert it to GPString. This type is not meant to be used directly, it is
 * meant to be used as a statically allocated buffer to be converted to
 * GPString.
 */
#define GPStringBuffer(CAPACITY)     \
struct GP_ANONYMOUS_STRUCT(__LINE__)  \
{                                      \
    uintptr_t    capacity;              \
    void*        allocation;             \
    GPAllocator* allocator;               \
    uintptr_t    length;                   \
    GPChar       data[(CAPACITY)+sizeof""]; \
}

/** Getters */
GP_NONNULL_ARGS() GP_NODISCARD static inline size_t       gp_str_length    (GPString str) { return ((GPStringHeader*)str - 1)->length;     }
GP_NONNULL_ARGS() GP_NODISCARD static inline size_t       gp_str_capacity  (GPString str) { return ((GPStringHeader*)str - 1)->capacity;   }
GP_NONNULL_ARGS() GP_NODISCARD static inline GPAllocator* gp_str_allocator (GPString str) { return ((GPStringHeader*)str - 1)->allocator;  }
GP_NONNULL_ARGS() GP_NODISCARD static inline void*        gp_str_allocation(GPString str) { return ((GPStringHeader*)str - 1)->allocation; }

/** Direct access to GPStringHeader fields. */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline GPStringHeader* gp_str_set(GPString str)
{
    return (GPStringHeader*)str - 1;
}

/** Create a new dynamic string */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline GPStringDynamic gp_str_new(
    GPAllocator* allocator,
    size_t       capacity)
{
    return (GPStringDynamic)gp_arr_new(sizeof(GPChar), allocator, capacity);
}

/** Create and initialize a new string */
GP_NONNULL_ARGS_AND_RETURN
static inline GPStringDynamic gp_str_new_init(
    GPAllocator* alc,
    size_t       capacity,
    const char*  init)
{
    size_t len = strlen(init);
    GPStringDynamic s = gp_str_new(alc, gp_max(capacity, len));
    memcpy(s, init, len);
    gp_str_set(s)->length = len;
    return s;
}

/** Create and initialize a string from a static string buffer.
 * Passing an allocator makes the string reallocateable (dynamic), static if
 * NULL.
 */
#define/* GPString */gp_str_buffered(      \
    GPAllocator_ptr_OPTIONAL,               \
    GPStringBuffer_ptr_BUFFER,               \
    /* optional initial string literal */...) \
( \
    (GPStringBuffer_ptr_BUFFER)->capacity =                            \
        sizeof((GPStringBuffer_ptr_BUFFER)->data) - sizeof"",           \
    (GPStringBuffer_ptr_BUFFER)->allocation = NULL,                      \
    (GPStringBuffer_ptr_BUFFER)->allocator  = (GPAllocator_ptr_OPTIONAL), \
    GP_STR_CHECK_INIT_SIZE((GPStringBuffer_ptr_BUFFER)->data, __VA_ARGS__),\
    (GPStringBuffer_ptr_BUFFER)->length     = sizeof(""__VA_ARGS__)-1,      \
    strcpy((char*)((GPStringBuffer_ptr_BUFFER)->data), ""__VA_ARGS__),       \
    /* return */(GPStringBuffer_ptr_BUFFER)->data                             \
)

/** Free string memory.
 * Passing strings on stack is safe too.
 */
static inline void gp_str_delete(GPString optional)
{
    if (optional != NULL && gp_str_allocation(optional) != NULL)
        gp_mem_dealloc(gp_str_allocator(optional), gp_str_allocation(optional));
}

/** Free string memory trough pointer.
 * Useful for some destructor callbacks.
 */
static inline void gp_str_ptr_delete(GPString* optional)
{
    if (optional != NULL)
        gp_str_delete(*optional);
}

/** Convert to null-terminated string.
 * Adds null-terminator at the end of the string without allocating.
 */
GP_NONNULL_ARGS_AND_RETURN
static inline char* gp_cstr(GPString str)
{
    str[gp_str_length(str)].c = '\0'; // safe, using implicit capacity
    return (char*)str;
}

/** Reserve capacity.
 * If @p capacity > gp_str_capacity(@p *str), reallocates, does nothing
 * otherwise. In case of reallocation, capacity will be rounded up
 * exponentially.
 * @return 0 if capacity will be large enough to hold @p capacity bytes, which
 * is always the case for dynamic strings. Otherwise returns the difference of
 * @p capacity and current capacity.
 */
GP_NONNULL_ARGS()
static inline size_t gp_str_reserve(
    GPString* str,
    size_t    capacity)
{
    return gp_arr_reserve(sizeof(GPChar), str, capacity);
}

/** Copy source string to destination.*/
GP_NONNULL_ARGS()
static inline size_t gp_str_copy(
    GPString*              dest,
    const void*GP_RESTRICT src,
    size_t                 src_size)
{
    return gp_arr_copy(sizeof(GPChar), dest, (GPString)src, src_size);
}

/** Copy source string to destination many times.
 * Copies @p src to @p dest and appends @p src to it count - 1 times.
 */
GP_NONNULL_ARGS()
static inline size_t gp_str_repeat(
    GPString*              dest,
    size_t                 count,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    size_t trunced = gp_str_reserve(dest, count * src_length);
    if (src_length == 1) {
        memset(*dest, *(uint8_t*)src, count - trunced);
    }
    else if ( ! trunced) for (size_t i = 0; i < count; ++i) {
        memcpy(*dest + i*src_length, src, src_length);
    }
    else for (size_t i = 0; i < count; ++i) {
        if ((i+1)*src_length > gp_str_capacity(*dest)) {
            memcpy(*dest + i*src_length, src, gp_str_capacity(*dest) - i*src_length);
            break;
        }
        memcpy(*dest + i*src_length, src, src_length);
    }
    gp_str_set(*dest)->length = count*src_length - trunced;
    return trunced;
}

/** Copy or remove characters.
 * Copies characters from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, characters from @p str outside
 * @p start_index and @p end_index are removed and the remaining characters are
 * moved over.
 */
GP_NONNULL_ARGS(1)
static inline size_t gp_str_slice(
    GPString*              str,
    const void*GP_RESTRICT optional_src,
    size_t                 start,
    size_t                 end)
{
    return gp_arr_slice(sizeof(GPChar), str, (GPString)optional_src, start, end);
}

/** Add characters to the end.*/
GP_NONNULL_ARGS()
static inline size_t gp_str_append(
    GPString*              dest,
    const void*GP_RESTRICT src,
    size_t                 src_size)
{
    return gp_arr_append(sizeof(GPChar), dest, (GPString)src, src_size);
}

/** Add characters to specified position.
 * Moves rest of the characters over to make room for added characters.
 */
GP_NONNULL_ARGS()
static inline size_t gp_str_insert(
    GPString*              dest,
    size_t                 pos,
    const void*GP_RESTRICT src,
    size_t                 src_size)
{
    return gp_arr_insert(sizeof(GPChar), dest, pos, (GPString)src, src_size);
}

/** Remove elements.
 * Removes @p count bytes starting from @p pos moving the rest of the bytes
 * over. Will not reallocate, only takes the array by address to signal mutation
 * and to be consistent with other mutating functions.
 */
GP_NONNULL_ARGS()
static inline void gp_str_erase(
    GPString* dest,
    size_t    position,
    size_t    count)
{
    gp_arr_erase(sizeof(GPChar), dest, position, count);
}

GP_NONNULL_ARGS()
size_t gp_str_replace(
    GPString*   dest,
    size_t      position,
    size_t      count,
    const void* replacement,
    size_t      replacement_length);

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"

#define GP_LEFT  'l'
#define GP_RIGHT 'r'
#define GP_TRIM_INVALID 0x1

/** Trim characters.
 * @p flags: 'l' or GP_LEFT, 'r' or GP_RIGHT, 'a' or GP_ASCII for ASCII char set
 * only. Combine flags with |.
 */
GP_NONNULL_ARGS(1, 4)
size_t gp_str_trim(
    GPString*   dest,
    const void* optional_src,
    size_t      optional_src_length,
    const char* utf8_char_set,
    int         flags);

/** Simple Unicode upcasing.
 * Only converts Unicode characters with 1:1 mapping.
 */
GP_NONNULL_ARGS()
size_t gp_str_to_upper(
    GPString*);

/** Simple Unicode downcasing.
 * Only converts Unicode characters with 1:1 mapping.
 */
GP_NONNULL_ARGS()
size_t gp_str_to_lower(
    GPString*);

// Unicode standard recommends using this as replacement character for invalid
// UTF-8.
#define GP_REPLACEMENT_CHARACTER "\uFFFD" // �

/** Make string valid UTF-8.
 * Copy @p optional_src to @p dest replacing all invalid bytes with
 * @p replacement, which may be empty, but it is recommended to use
 * GP_REPLACEMENT_CHARACTER (�) instead, which is more secure and designed for
 * this purpose. See https://www.unicode.org/reports/tr36/tr36-15.html.
 * If @p optional_src is NULL, @p *dest will be used as input instead.
 */
GP_NONNULL_ARGS(1)
size_t gp_str_to_valid(
    GPString*              dest,
    const void*GP_RESTRICT optional_src,
    size_t                 optional_src_length,
    const char*            replacement);

/** Remove potentially invalid bytes at the end of truncated string.
 * A truncating string may partially truncate a codepoint at the end of the
 * string. A truncating string will not automatically truncate these invalid
 * bytes, this could lead into further data corruption and unexpected results.
 * Use this function at the end of processing a truncating string and before
 * using it as input to any Unicode sensitive processing. Only a maximum of 3
 * bytes will be processed and @p dest assumed valid before initial truncation.
 * @return number of truncated bytes at the end.
 */
GP_NONNULL_ARGS()
static inline size_t gp_str_truncate_invalid_tail(
    GPStringStatic* dest)
{
    size_t gp_utf8_decode_codepoint_length(const void*, size_t);
    size_t len = gp_imax(0, gp_str_length(*dest) - 3);
    for (size_t cp_len; len < gp_str_length(*dest); len += cp_len + !cp_len) {
        cp_len = gp_utf8_decode_codepoint_length(*dest, len);
        if (len + cp_len > gp_str_length(*dest))
            break;
    }
    size_t trunced = gp_str_length(*dest) - len;
    gp_str_set(*dest)->length -= trunced;
    return trunced;
}

/** Quick file operations.
 * Opens file in file_path, performs a file operation, and closes it. If
 * operation[0] == 'r', reads the whole file and stores to str. If
 * operation[0] == 'w', writes full contents of str to the file. If
 * operation[0] == 'a', appends fulll contents of str to the file.
 * Files are always opened in binary mode by default. Add "x" or "text" anywhere
 * in operation string to open in text mode. This makes no difference in POSIX,
 * but in Windows adds processing to "\n" which is unnecessary in 2024.
 * @return  0 on success, (size_t)-1 if file operations fail (errno will be
 * set). If reading file to a truncating string without errors, number of
 * truncated bytes will be returned.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_str_file(
    GPString*   src_or_dest,
    const char* file_path,
    const char* operation);

// ----------------------------------------------------------------------------
// String formatting

// Strings can be formatted without format specifiers with gp_str_print()
// family of macros if C11 or higher or C++. If not C++ format specifiers can be
// added optionally for more control. C99 requires format strings. There can be
// multiple format strings with an arbitrary amount of format specifiers.
// Simple example:
/*
    gp_str_print(&my_str, 1, 2, "%u%u", 3u, 4u, "%x", 5); // copies "12345"
 */
// See the tests for more detailed examples.

/** Copy formatted string allocating as needed.*/
#define/* size_t */gp_str_print(str_ptr_out, ...) \
    GP_STR_PRINT(str_ptr_out, __VA_ARGS__)

/** Like gp_str_print() but add spaces between args and add newline.*/
#define/* size_t */gp_str_println(str_ptr_out, ...) \
    GP_STR_PRINTLN(str_ptr_out, __VA_ARGS__)

// ----------------------------------------------------------------------------
// String examination

/** Find substring.
 * @return index to the first occurrence of @p needle in @p haystack starting
 * from @p start or GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
static inline size_t gp_str_find_first(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start)
{
    return gp_bytes_find_first(haystack, gp_str_length(haystack), needle, needle_size, start);
}

/** Find substring from right.
 * @return index to the last occurrence of @p needle in @p haystack or
 * GP_NOT_FOUND if not found.
 */
GP_NONNULL_ARGS() GP_NODISCARD
static inline size_t gp_str_find_last(
    GPString    haystack,
    const void* needle,
    size_t      needle_size)
{
    return gp_bytes_find_last(haystack, gp_str_length(haystack), needle, needle_size);
}

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
    size_t*  optional_invalid_position);

// More string functions in unicode.h


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


size_t gp_internal_str_print(
    GPString* out,
    size_t arg_count,
    const GPInternalReflectionData* objs,
    ...);

size_t gp_internal_str_println(
    GPString* out,
    size_t arg_count,
    const GPInternalReflectionData* objs,
    ...);

#ifndef __cplusplus

#define GP_STR_PRINT(OUT, ...) \
    gp_internal_str_print( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPInternalReflectionData[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#define GP_STR_PRINTLN(OUT, ...) \
    gp_internal_str_println( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPInternalReflectionData[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

// Portably issue warning. Also gives better error message than strcpy().
#define GP_STR_CHECK_INIT_SIZE(DATA_BUF, ...) \
    (char*){0} = (char[sizeof DATA_BUF]){""__VA_ARGS__}

#else // __cplusplus
} // extern "C"

// Major compilers already know to issue warning based on strcpy() alone.
#define GP_STR_CHECK_INIT_SIZE(...)

static inline std::ostream& operator<<(std::ostream& out, GPString str)
{
    out.write((const char*)str, gp_str_length(str));
    return out;
}

static inline size_t gp_internal_str_copy_cppstr(GPString* out, const size_t n, std::string s)
{
    const size_t length = n < s.length() ? n : s.length();
    gp_str_reserve(out, length);
    memcpy(*out, s.data(), length);
    ((GPStringHeader*)*out - 1)->length = length;
    return s.length();
}

#define GP_STR_PRINT(OUT, ...) GP_STR_N_PRINT(OUT, SIZE_MAX, __VA_ARGS__)
#define GP_STR_N_PRINT(OUT, N, ...) gp_internal_str_copy_cppstr(OUT, N, \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT, __VA_ARGS__) \
    ).str())

#define GP_STR_PRINTLN(OUT, ...) GP_STR_N_PRINTLN(OUT, SIZE_MAX, __VA_ARGS__)
#define GP_STR_N_PRINTLN(OUT, N, ...) gp_internal_str_copy_cppstr(OUT, N, \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT_SPACE, __VA_ARGS__) << "\n" \
    ).str())

#endif // __cplusplus

#endif // GP_STRING_INCLUDED
