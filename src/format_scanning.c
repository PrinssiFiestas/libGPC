// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <gpc/assert.h>
#include <printf/format_scanning.h>
#include "common.h"
#include <string.h>
#include <limits.h>

PFFormatSpecifier
pf_scan_format_string(
    const char* fmt_string,
    pf_va_list* va_args)
{
    PFFormatSpecifier fmt = {0};
    fmt.string = fmt_string;

    fmt.string = strchr(fmt.string, '%');
    if (fmt.string == NULL)
        return fmt;

    if (fmt.string[1] == '%') {
        fmt.string_length = 2;
        fmt.conversion_format = '%';
        return fmt;
    }

    // Iterator
    const char* c = fmt.string;
    ++c; // ignore '%'

    // Find all flags if any
    for (const char* flag; (flag = strchr("-+ #0", *c)); ++c) switch (*flag) {
        case '-': fmt.flag.dash  = true; break;
        case '+': fmt.flag.plus  = true; break;
        case ' ': fmt.flag.space = true; break;
        case '#': fmt.flag.hash  = true; break;
        case '0': fmt.flag.zero  = true; break;
        default: GP_UNREACHABLE("Missing conversion specifier in format string.");
    }

    // Find field width
    if (*c == '*')
    {
        fmt.field.asterisk = true;

        int width = 0;
        if (va_args != NULL && (width = va_arg(va_args->list, int)) >= 0)
        {
            fmt.field.asterisk = false; // prevent recalling va_arg()
            fmt.field.width = width;
        }
        else if (width < 0)
            fmt.field.asterisk = false;
        ++c;
    }
    else if ('1' <= *c && *c <= '9') // can't be 0. Leading 0 is a flag.
    do {
        fmt.field.width = 10*fmt.field.width + *c - '0';
        gp_db_assert(fmt.field.width <= INT_MAX, "Format string field width too large.");
        ++c;
    } while ('0' <= *c && *c <= '9');

    // Find precision
    if (*c == '.')
    {
        ++c; // ignore '.'

        if (*c == '*')
        {
            fmt.precision.option = PF_ASTERISK;
            int width = 0;
            if (va_args != NULL && (width = va_arg(va_args->list, int)) >= 0)
            {
                fmt.precision.option = PF_SOME;
                fmt.precision.width = width;
            }
            else if (width < 0)
                fmt.precision.option = PF_NONE;
            ++c;
        }
        else {
            fmt.precision.option = PF_SOME;
            for (; '0' <= *c && *c <= '9'; ++c) {
                fmt.precision.width = 10*fmt.precision.width + *c - '0';
                gp_db_assert(fmt.precision.width <= INT_MAX, "Format string precision too large.");
            }
        }
    }

    // Find length modifier
    const char* modifier = strchr("hljztLBWDQOw", *c);
    if (modifier != NULL)
    {
        ++c;

        if ((*modifier == 'h' || *modifier == 'l') && *c == *modifier)
            fmt.length_modifier = 2 * *c++;
        else if (*modifier == 'w')
        {
            if (*c == 'f')
                fmt.length_modifier = *c++;

            size_t width = 0;
            for (; '0' <= *c && *c <= '9'; ++c) {
                width = 10*width + *c - '0';
                gp_db_assert(width <= 128, "N in wN format string length modifier too large.");
            }
            gp_db_assert(width != 0, "Invalid format string.");
            gp_db_assert((width & (width-1)) == 0, // check if power of 2
                "N in wN format string length modifier must be 8, 16, 32, 64, or 128.");
            if (fmt.length_modifier == 'f')
                gp_db_assert(width != 128, "No fast 128-bit integer available.");

            switch (width) {
            case 8:   fmt.length_modifier += 'B'; break;
            case 16:  fmt.length_modifier += 'W'; break;
            case 32:  fmt.length_modifier += 'D'; break;
            case 64:  fmt.length_modifier += 'Q'; break;
            case 128: fmt.length_modifier += 'O'; break;
            default: GP_UNREACHABLE("Missing conversion specifier in format string.");
            }
        }
        else
            fmt.length_modifier = *modifier;
    }

    fmt.conversion_format = *c++;
    fmt.string_length = c - fmt.string;

    gp_db_assert(fmt.conversion_format != 'n', "Unsupported format specifier.");
    gp_db_assert(strchr(GP_FORMAT_SPECIFIERS, fmt.conversion_format),
        "Missing conversion specifier in format string.");

    // TODO assert invalid combinations

    return fmt;
}
