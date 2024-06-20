// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

// This library is modified to suit the needs of libGPC. Most notably
// pf_snprintf() does not null-terminate if n is exceeded and custom formats and
// portable attributes and restrict are added.
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
#include <gpc/attributes.h>
#include "format_scanning.h"

#ifdef __cplusplus
extern "C" {
#endif

GP_NONNULL_ARGS()
int pf_vprintf(
    const char*GP_RESTRICT fmt, va_list args);

GP_NONNULL_ARGS()
int pf_vfprintf(
    FILE*GP_RESTRICT stream, const char*GP_RESTRICT fmt, va_list args);

GP_NONNULL_ARGS()
int pf_vsprintf(
    char*GP_RESTRICT buf, const char*GP_RESTRICT fmt, va_list args);

GP_NONNULL_ARGS(3)
int pf_vsnprintf(
    char*GP_RESTRICT buf, size_t n, const char*GP_RESTRICT fmt, va_list args);

GP_PRINTF(1, 2)
int pf_printf(
    const char*GP_RESTRICT fmt, ...);

GP_NONNULL_ARGS() GP_PRINTF(2, 3)
int pf_fprintf(
    FILE*GP_RESTRICT stream, const char*GP_RESTRICT fmt, ...);

GP_NONNULL_ARGS() GP_PRINTF(2, 3)
int pf_sprintf(char*GP_RESTRICT buf, const char*GP_RESTRICT fmt, ...);

GP_NONNULL_ARGS(3) GP_PRINTF(3, 4)
int pf_snprintf(
    char*GP_RESTRICT buf, size_t n, const char*GP_RESTRICT fmt, ...);

// Functions taking va_list may or may not consume an argument from the list due
// to va_list being implementation defined. This limits their applications so
// pf_vsnprintf() is guranteed to NOT consume an arg from arg list and
// pf_vsnprintf_consuming() is guranteed to consume an arg from arg list.

GP_NONNULL_ARGS(3, 4)
int pf_vsnprintf_consuming(
    char*GP_RESTRICT out_buf,
    const size_t max_size,
    const char*GP_RESTRICT format,
    struct pf_va_list* args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PRINTF_H_INCLUDED
