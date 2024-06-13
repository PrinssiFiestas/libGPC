// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#ifndef CONVERSIONS_H_INCLUDED
#define CONVERSIONS_H_INCLUDED 1

#include <printf/format_scanning.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns number of characters written excluding null-terminator. Does not
// write more than n characters.
size_t pf_utoa(size_t n, char* buf, unsigned long long x);
size_t pf_otoa(size_t n, char* buf, unsigned long long x);
size_t pf_xtoa(size_t n, char* buf, unsigned long long x);
size_t pf_Xtoa(size_t n, char* buf, unsigned long long x);
size_t pf_itoa(size_t n, char* buf, long long x);
size_t pf_ftoa(size_t n, char* buf, double x);
size_t pf_Ftoa(size_t n, char* buf, double x);
size_t pf_etoa(size_t n, char* buf, double x);
size_t pf_Etoa(size_t n, char* buf, double x);
size_t pf_gtoa(size_t n, char* buf, double x);
size_t pf_Gtoa(size_t n, char* buf, double x);

size_t pf_strfromd(char* buf, size_t n, PFFormatSpecifier fmt, double f);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CONVERSIONS_H_INCLUDED
