// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

// Original library:
// https://github.com/PrinssiFiestas/printf
//
// This library is modified to suit the needs of libGPC. Most notably, custom
// formats and portable attributes and restrict are added. Also, return types
// are size_t instead of int. Invalid format strings are asserted in debug
// builds, undefined otherwise.
//
// %n is not supported due to security issues, it is not useful anyway since you
// can just split the calls and read the return value. Long double may also lose
// precision at the time of writing. Other than these limitations and the
// modifications mentioned above, `pf_printf()` is fully ANSI C compatible with
// the following extensions:
//
// S conversion specifier for GPString. Note that GNUC interprets %S as %ls, but
// since it's use as %ls is discouraged anyway, we use it for our purposes.
// However, the compiler may issue a warning, which can be silenced by casting
// the GPString input to wchar_t*. Another option is to disable compiler format
// string checking as described below.
//
// Use B (byte 8 bits), W (word 16 bits), D (double word 32 bits),
// Q (quad word 64 bits), and O (octa word 128 bits for GP[U]Int128) as length
// specifier for fixed width integers. C23 wN ([u]intN_t) and wfN
// ([u]int_fastN_t), where N is 8, 16, 32, or 128, is also supported and
// recommended over non-standard BWDQO.
//
// At the time of writing, C23 wN and wfN length specifiers are not widely
// supported by compilers. Also casting GPString to wchar_t* can be misleading
// to the reader, and BWDQO are not recognized by compilers at all. To disable
// potential compiler warnings, user can #define GP_NO_FORMAT_STRING_CHECK
// before including this header to disable compiler checks. Disabling the checks
// is discouraged, but sometimes necessary.
//
// Extensions examples:
// - "%S":     GPString
// - "%Wi":    int16_t
// - "%Qx":    uint64_t hex
// - "%w128u": GPUInt128
// - "%w16x":  uint16_t hex
//
// TODO C23 %b binary conversion specifier

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
size_t pf_vprintf(
    const char*GP_RESTRICT fmt, va_list args);

GP_NONNULL_ARGS()
size_t pf_vfprintf(
    FILE*GP_RESTRICT stream, const char*GP_RESTRICT fmt, va_list args);

GP_NONNULL_ARGS()
size_t pf_vsprintf(
    char*GP_RESTRICT buf, const char*GP_RESTRICT fmt, va_list args);

GP_NONNULL_ARGS(3)
size_t pf_vsnprintf(
    char*GP_RESTRICT buf, size_t n, const char*GP_RESTRICT fmt, va_list args);

GP_CHECK_FORMAT_STRING(1, 2)
size_t pf_printf(
    const char*GP_RESTRICT fmt, ...);

GP_NONNULL_ARGS() GP_CHECK_FORMAT_STRING(2, 3)
size_t pf_fprintf(
    FILE*GP_RESTRICT stream, const char*GP_RESTRICT fmt, ...);

GP_NONNULL_ARGS() GP_CHECK_FORMAT_STRING(2, 3)
size_t pf_sprintf(char*GP_RESTRICT buf, const char*GP_RESTRICT fmt, ...);

GP_NONNULL_ARGS(3) GP_CHECK_FORMAT_STRING(3, 4)
size_t pf_snprintf(
    char*GP_RESTRICT buf, size_t n, const char*GP_RESTRICT fmt, ...);

// Functions taking va_list may or may not consume an argument from the list due
// to va_list being implementation defined. This limits their applications so
// pf_vsnprintf() is guranteed to NOT consume an arg from arg list and
// pf_vsnprintf_consuming() is guranteed to consume an arg from arg list.

GP_NONNULL_ARGS(3, 4)
size_t pf_vsnprintf_consuming(
    char*GP_RESTRICT out_buf,
    const size_t max_size,
    const char*GP_RESTRICT format,
    struct pf_va_list* args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PRINTF_H_INCLUDED
