// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#ifndef FORMAT_SCANNING_H_INCLUDED
#define FORMAT_SCANNING_H_INCLUDED 1

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return type of scan_format_string(). Can also be filled manually to be used
// with pf_strfromd().
typedef struct PFFormatSpecifier
{
    // Pointer to the first occurrence of '%' in fmt_string passed to
    // scan_format_string(). NULL if fmt_string does not contain a format
    // specifier.
    const char* string;
    size_t string_length;

    struct // field
    {
        unsigned width;
        bool asterisk;
    } field;

    struct // precision
    {
        unsigned width;
        enum // option
        {
            PF_NONE,
            PF_SOME,
            PF_ASTERISK
        } option;
    } precision;

    struct // flag
    {
        bool dash;
        bool plus;
        bool space;
        bool hash;
        bool zero;
    } flag;

    unsigned char length_modifier;   // any of "hljztLwBWDQO" or 2*'h' or 2*'l' or any of "BWDQO"+'f'
    unsigned char conversion_format; // any of GP_FORMAT_SPECIFIERS. 'n' not supported.
} PFFormatSpecifier;

// Portability wrapper.
// https://stackoverflow.com/questions/8047362/is-gcc-mishandling-a-pointer-to-a-va-list-passed-to-a-function
typedef struct pf_va_list
{
    va_list list;
} pf_va_list;

PFFormatSpecifier
pf_scan_format_string(
    const char* fmt_string, // should be null-terminated
    pf_va_list* optional_asterisks);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORMAT_SCANNING_H_INCLUDED
