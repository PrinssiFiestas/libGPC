// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

// This library is modified to suit the needs of libGPC. Most notably
// pf_snprintf() does not null-terminate if n is exceeded and custom formats are
// added.
//
// Use %S for GPString
//
// Use B (byte 8 bits), W (word 16 bits), D (double word 32 bits), and
// Q (quad word 64 bits) as length specifier for fixed width integers.
// Example: "%Wi" for int16_t and "%Qx" for uint64_t in hex.

#ifndef PRINTF_H_INCLUDED
#define PRINTF_H_INCLUDED 1

#include <stdio.h>
#include <stdarg.h>
#include "format_scanning.h"

#ifdef __cplusplus
extern "C" {
#endif

int pf_vprintf(
    const char fmt[restrict static 1], va_list args);
int pf_vfprintf(
    FILE stream[restrict static 1], const char fmt[restrict static 1], va_list args);
int pf_vsprintf(
    char buf[restrict static 1], const char fmt[restrict static 1], va_list args);
int pf_vsnprintf(
    char* restrict buf, size_t n, const char fmt[restrict static 1], va_list args);

__attribute__((format (printf, 1, 2)))
int pf_printf(
    const char fmt[restrict static 1], ...);

__attribute__((format (printf, 2, 3)))
int pf_fprintf(
    FILE stream[restrict static 1], const char fmt[restrict static 1], ...);

__attribute__((format (printf, 2, 3)))
int pf_sprintf(char buf[restrict static 1], const char fmt[restrict static 1], ...);

__attribute__((format (printf, 3, 4)))
int pf_snprintf(
    char* restrict buf, size_t n, const char fmt[restrict static 1], ...);

// Functions taking va_list may or may not consume an argument from the list due
// to va_list being implementation defined. This limits their applications so
// pf_vsnprintf() is guranteed to NOT consume an arg from arg list and
// pf_vsnprintf_consuming() is guranteed to consume an arg from arg list.

int pf_vsnprintf_consuming(
    char*restrict out_buf,
    const size_t max_size,
    const char format[restrict static 1],
    struct pf_va_list* args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PRINTF_H_INCLUDED
