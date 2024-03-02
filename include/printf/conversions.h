// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#ifndef CONVERSIONS_H_INCLUDED
#define CONVERSIONS_H_INCLUDED 1

#include <printf/format_scanning.h>
#include <stdint.h>
#include <stddef.h>

// Returns number of characters written excluding null-terminator. Does not
// write more than n characters.
unsigned pf_utoa(size_t n, char* buf, uintmax_t x);
unsigned pf_otoa(size_t n, char* buf, uintmax_t x);
unsigned pf_xtoa(size_t n, char* buf, uintmax_t x);
unsigned pf_Xtoa(size_t n, char* buf, uintmax_t x);
unsigned pf_itoa(size_t n, char* buf, intmax_t x);
unsigned pf_ftoa(size_t n, char* buf, double x);
unsigned pf_Ftoa(size_t n, char* buf, double x);
unsigned pf_etoa(size_t n, char* buf, double x);
unsigned pf_Etoa(size_t n, char* buf, double x);
unsigned pf_gtoa(size_t n, char* buf, double x);
unsigned pf_Gtoa(size_t n, char* buf, double x);

unsigned pf_strfromd(char* buf, size_t n, PFFormatSpecifier fmt, double f);

#endif // CONVERSIONS_H_INCLUDED
