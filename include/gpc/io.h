// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file io.h
 * Input/output
 */

#ifndef GP_IO_INCLUDED
#define GP_IO_INCLUDED 1

#include <gpc/string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Outputs can be formatted without format specifiers with gp_print()
// family of macros if C11 or higher or C++. If not C++ format specifiers can be
// added optionally for more control. C99 requires format strings. There can be
// multiple format strings with an arbitrary amount of format specifiers.
// Silly example:
/*
    gp_print(&my_str, 1, 2, "%u%u", 3u, 4u, "%x", 5); // prints "12345"
 */
// See the tests for more detailed examples.

#define/* size_t */gp_print(...) \
    GP_FILE_PRINT(stdout, __VA_ARGS__)

#define/* size_t */gp_println(...) \
    GP_FILE_PRINTLN(stdout, __VA_ARGS__)

#define/* size_t */gp_file_print(FILE_ptr, ...) \
    GP_FILE_PRINT(FILE_ptr, __VA_ARGS__)

#define/* size_t */gp_file_println(FILE_ptr, ...) \
    GP_FILE_PRINTLN(FILE_ptr, __VA_ARGS__)

// ----------------------------------------------------------------------------

/** Opens file.
 * This exists mostly for completeness.
 * Like fopen(), but handles mode differently. Checks the first character in
 * mode string: 'r' for read, 'w' for write. Then checks if 'x' exists in mode
 * string for text mode. Default is binary mode. Also checks for '+' for
 * read/write or write/read like fopen().
 */
GP_NONNULL_ARGS() GP_NODISCARD
FILE* gp_file_open(const char* path, const char* mode);

/** To be passed to gp_defer() with correct function type.*/
inline void gp_file_close(FILE* optional)
{
    if (optional != NULL)
        fclose(optional);
}

/** Reads line from file.
 * Overwrites any contents in @p dest. Newline will be included in the resultant
 * string.
 * @return `false` when no more bytes to be read from @p in.
 */
GP_NONNULL_ARGS()
bool gp_file_read_line(
    GPString* dest,
    FILE*     in);

/** Reads segment from file.
 * Overwrites any contents in @p dest. Reads until @p delimiter is found in
 * file. @p delimiter will be included in @p dest. The file pointer will point
 * past the occurrence of @p delimiter.
 * @return `false` when no more bytes to be read from @p in.
 */
GP_NONNULL_ARGS()
bool gp_file_read_until(
    GPString*   dest,
    FILE*       in,
    const char* delimiter);

/** Reads segment from file.
 * Overwrites any contents in @p dest. Skips all codepoints in @p in that are in
 * @p char_set. Then, reads until a codepoint found from @p char_set in @p in.
 * No codepoints in @p char_set are stored in @p dest.
 * @return `false` when no more bytes to be read from @p in.
 */
GP_NONNULL_ARGS(1, 2)
bool gp_file_read_strip(
    GPString*   dest,
    FILE*       in,
    const char* optional_utf8_char_set);

// Portability wrappers for stat. Check the man-pages.

#if _WIN32
typedef struct __stat64 GPStat;
#elif defined(_GNU_SOURCE)
typedef struct stat64 GPStat;
#else // 64-bit in 64-bit Linux
typedef struct stat GPStat;
#endif

GP_NONNULL_ARGS() GP_NODISCARD
static inline int gp_stat(GPStat* s, const char* path)
{
    #if _WIN32
    return _stat64(path, s);
    #elif defined(_GNU_SOURCE)
    return stat64(path, s);
    #else
    return stat(path, s);
    #endif
}


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


size_t gp_file_print_internal(
    FILE* file,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

size_t gp_file_println_internal(
    FILE* file,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#if !__cplusplus

#define GP_FILE_PRINT(OUT, ...) \
    gp_file_print_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#define GP_FILE_PRINTLN(OUT, ...) \
    gp_file_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)

#else // __cplusplus
} // extern "C"

#define GP_FILE_PRINT(OUT, ...) fputs( \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT, __VA_ARGS__) \
    ).str().c_str(), OUT)

#define GP_FILE_PRINTLN(OUT, ...) fputs( \
    (std::ostringstream() << \
        GP_PROCESS_ALL_ARGS(GP_EVAL, GP_STREAM_INSERT_SPACE, __VA_ARGS__) << "\n" \
    ).str().c_str(), OUT)

#endif // __cplusplus

#endif // GP_IO_INCLUDED
