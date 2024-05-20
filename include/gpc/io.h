// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_IO_INCLUDED
#define GP_IO_INCLUDED

#include "bytes.h"
#include <stdio.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Returns (size_t)-1 on errors. Check errno for specific error.
size_t gp_file_size(const char* file_name) GP_NONNULL_ARGS();

#define/* size_t */gp_print(...) \
    GP_FILE_PRINT(stdout, __VA_ARGS__)

#define/* size_t */gp_println(...) \
    GP_FILE_PRINTLN(stdout, __VA_ARGS__)

#define/* size_t */gp_file_print(FILE_ptr, ...) \
    GP_FILE_PRINT(FILE_ptr, __VA_ARGS__)

#define/* size_t */gp_file_println(FILE_ptr, ...) \
    GP_FILE_PRINTLN(FILE_ptr, __VA_ARGS__)

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

#endif // GP_IO_INCLUDED
