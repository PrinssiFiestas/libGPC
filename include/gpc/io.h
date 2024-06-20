// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_IO_INCLUDED
#define GP_IO_INCLUDED

#include "bytes.h"
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

// This is mostly for completeness
FILE* gp_file_open(const char* path, const char* mode);

// To be passed to gp_defer() with correct function type
inline void gp_file_close(FILE* file)
{
    fclose(file);
}

#define/* size_t */gp_print(...) \
    GP_FILE_PRINT(stdout, __VA_ARGS__)

#define/* size_t */gp_println(...) \
    GP_FILE_PRINTLN(stdout, __VA_ARGS__)

#define/* size_t */gp_file_print(FILE_ptr, ...) \
    GP_FILE_PRINT(FILE_ptr, __VA_ARGS__)

#define/* size_t */gp_file_println(FILE_ptr, ...) \
    GP_FILE_PRINTLN(FILE_ptr, __VA_ARGS__)

typedef struct gp_char* GPString;

bool gp_file_read_line(
    GPString* dest,
    FILE*     in) GP_NONNULL_ARGS();

bool gp_file_read_until(
    GPString*   dest,
    FILE*       in,
    const char* delimiter) GP_NONNULL_ARGS();

bool gp_file_read_strip(
    GPString*   dest,
    FILE*       in,
    const char* optional_utf8_char_set) GP_NONNULL_ARGS(1, 2);

// Portability wrappers for stat

#if _WIN32
typedef struct __stat64 GPStat;
#elif _GNU_SOURCE
typedef struct stat64 GPStat;
#else // 64-bit in 64-bit Linux
typedef struct stat GPStat;
#endif

GP_NONNULL_ARGS() GP_NODISCARD
inline int gp_stat(GPStat* s, const char* path)
{
    #if _WIN32
    return _stat64(path, s);
    #elif _GNU_SOURCE
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
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

#define GP_FILE_PRINTLN(OUT, ...) \
    gp_file_println_internal( \
        OUT, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
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
