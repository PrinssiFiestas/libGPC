// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file string.h
 * String data type.
 */

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED

#include "memory.h"
#include "bytes.h"
#include "attributes.h"
#include "overload.h"
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <wctype.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gp_string_header
{
    size_t length;
    size_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPStringHeader;
typedef struct gp_char { uint8_t c; } GPChar;

typedef GPChar* GPString;

GP_NONNULL_ARGS_AND_RETURN
GPString gp_str_new(
    const       GPAllocator*,
    size_t      capacity,
    const char* init);

// Not available in C++
#define/* GPString */gp_str_on_stack( \
    optional_allocator_ptr, \
    size_t_capacity, \
    /*optional_cstr_literal_init*/...) (GPString) \
    \
    GP_STR_ON_STACK(optional_allocator_ptr, size_t_capacity,__VA_ARGS__)

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, arrays can be created on stack manually. Capacity should be
// initialized to be actual capacity -1 for null termination.
/*
    struct optional_name{GPStringHeader header; GPChar data[2048];} my_str_mem;
    my_str_mem.header = (GPStringHeader) {.capacity = 2048 - sizeof""};
    GPString my_string = my_str_mem.data;
*/

size_t             gp_str_length    (GPString) GP_NONNULL_ARGS();
size_t             gp_str_capacity  (GPString) GP_NONNULL_ARGS();
const GPAllocator* gp_str_allocator (GPString) GP_NONNULL_ARGS();
void*              gp_str_allocation(GPString) GP_NONNULL_ARGS();

// Passing strings on stack is safe too.
inline void gp_str_delete(GPString optional)
{
    if (optional != NULL && gp_str_allocation(optional) != NULL)
        gp_mem_dealloc(gp_str_allocator(optional), gp_str_allocation(optional));
}

// This should be used as destructor for GPDictionary(GPString) if needed.
inline void gp_str_ptr_delete(GPString* optional)
{
    if (optional != NULL)
        gp_str_delete(*optional);
}

const char* gp_cstr(GPString) GP_NONNULL_ARGS_AND_RETURN;

GP_NONNULL_ARGS()
void gp_str_reserve(
    GPString*,
    size_t capacity);

GP_NONNULL_ARGS()
void gp_str_copy(
    GPString*              dest,
    const void*GP_RESTRICT src,
    size_t                 src_size);

GP_NONNULL_ARGS()
void gp_str_repeat(
    GPString*              dest,
    size_t                 count,
    const void*GP_RESTRICT src,
    size_t                 src_length);

GP_NONNULL_ARGS(1)
void gp_str_slice(
    GPString*              dest,
    const void*GP_RESTRICT optional_src, // mutates dest if NULL
    size_t                 start,
    size_t                 end);

GP_NONNULL_ARGS()
void gp_str_append(
    GPString*              dest,
    const void*GP_RESTRICT src,
    size_t                 src_size);

GP_NONNULL_ARGS()
void gp_str_insert(
    GPString*              dest,
    size_t                 pos,
    const void*GP_RESTRICT src,
    size_t                 src_size);

// Returns index to the first occurrence of needle in haystack.
GP_NONNULL_ARGS()
size_t gp_str_replace(
    GPString*              haystack,
    const void*GP_RESTRICT needle,
    size_t                 needle_length,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_length,
    size_t                 start);

// Returns number of replacements made.
GP_NONNULL_ARGS()
size_t gp_str_replace_all(
    GPString*              haystack,
    const void*GP_RESTRICT needle,
    size_t                 needle_length,
    const void*GP_RESTRICT replacement,
    size_t                 replacement_length);

#define/* size_t */gp_str_print(str_ptr_out, ...) \
    GP_STR_PRINT(str_ptr_out, __VA_ARGS__)

#define/* size_t */gp_str_n_print(str_ptr_out, n, ...) \
    GP_STR_N_PRINT(str_ptr_out, n, __VA_ARGS__)

#define/* size_t */gp_str_println(str_ptr_out, ...) \
    GP_STR_PRINTLN(str_ptr_out, __VA_ARGS__)

#define/* size_t */gp_str_n_println(str_ptr_out, n, ...) \
    GP_STR_N_PRINTLN(str_ptr_out, n, __VA_ARGS__)

#define GP_WHITESPACE  " \t\n\v\f\r" \
    "\u00A0\u1680\u2000\u2001\u2002\u2003\u2004\u2005\u2006" \
    "\u2007\u2008\u2009\u200A\u2028\u2029\u202F\u205F\u3000\xC2\x85"

// Flags: 'l' left, 'r' right, 'a' ASCII char set only. Separate flags with |.
// Trims whitespace if char_set is NULL.
GP_NONNULL_ARGS(1)
void gp_str_trim(
    GPString*,
    const char* optional_char_set,
    int         flags);

// Only converts Unicode characters with 1:1 mapping.
GP_NONNULL_ARGS()
void gp_str_to_upper(
    GPString*);

// Only converts Unicode characters with 1:1 mapping.
GP_NONNULL_ARGS()
void gp_str_to_lower(
    GPString*);

// Only converts Unicode characters with 1:1 mapping.
GP_NONNULL_ARGS()
void gp_str_to_title(
    GPString*);

// Unicode standard recommends using this as replacement character for invalid
// bytes.
#define GP_REPLACEMENT_CHARACTER "\uFFFD" // �

GP_NONNULL_ARGS()
void gp_str_to_valid(
    GPString*   str,
    const char* replacement);

// Opens file in file_path, performs a file operation, and closes it. If
// operation[0] == 'r', reads the whole file and stores to str. If
// operation[0] == 'w', writes full contents of str to the file. If
// operation[0] == 'a', appends fulll contents of str to the file. Any other
// character is undefined.
// Returns  0 on success.
// Returns -1 if file operations fail. Check errno for the specific error.
// Returns  1 if file size > SIZE_MAX in 32-bit systems.
GP_NONNULL_ARGS() GP_NODISCARD
int gp_str_file(
    GPString*   src_or_dest,
    const char* file_path,
    const char* operation);

// ----------------------------------------------------------------------------
// String examination

#define GP_NOT_FOUND ((size_t)-1)

GP_NONNULL_ARGS()
size_t gp_str_find_first(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start);

GP_NONNULL_ARGS()
size_t gp_str_find_last(
    GPString    haystack,
    const void* needle,
    size_t      needle_size);

GP_NONNULL_ARGS()
size_t gp_str_find_first_of(
    GPString    haystack,
    const char* utf8_char_set,
    size_t      start);

GP_NONNULL_ARGS()
size_t gp_str_find_first_not_of(
    GPString    haystack,
    const char* utf8_char_set,
    size_t      start);

GP_NONNULL_ARGS()
size_t gp_str_count(
    GPString    haystack,
    const void* needle,
    size_t      needle_size);

GP_NONNULL_ARGS()
bool gp_str_equal(
    GPString    s1,
    const void* s2,
    size_t      s2_size);

GP_NONNULL_ARGS()
bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size);

GP_NONNULL_ARGS()
size_t gp_str_codepoint_count(
    GPString str);

GP_NONNULL_ARGS(1)
bool gp_str_is_valid(
    GPString str,
    size_t*  optional_invalid_position);

// Only reads the first byte at str + i
GP_NONNULL_ARGS()
size_t gp_str_codepoint_length(
    GPString str,
    size_t i);

// Locale dependent!
GP_NONNULL_ARGS()
bool gp_str_codepoint_classify(
    GPString str,
    size_t i,
    int (*classifier)(wint_t c));


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
}, {__VA_ARGS__} }.data

#define GP_STR_PRINT(OUT, ...) \
    gp_str_print_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_N_PRINT(OUT, N, ...) \
    gp_str_n_print_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_PRINTLN(OUT, ...) \
    gp_str_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_STR_N_PRINTLN(OUT, N, ...) \
    gp_str_n_println_internal( \
        OUT, \
        N, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
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
