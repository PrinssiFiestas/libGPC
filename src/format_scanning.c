// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/format_scanning.h>
#include <string.h>

PFFormatSpecifier
pf_scan_format_string(
    const char* fmt_string,
    pf_va_list* va_args)
{
    PFFormatSpecifier fmt = { fmt_string };

    fmt.string = strchr(fmt.string, '%');
    if (fmt.string == NULL)
    {
        return fmt;
    }
    if (fmt.string[1] == '%')
    {
        fmt.string_length = 2;
        fmt.conversion_format = '%';
    }

    // Iterator
    const char* c = fmt.string + strlen("%");

    // Find all flags if any
    for (const char* flag; (flag = strchr("-+ #0", *c)); c++)
    {
        switch (*flag)
        {
            case '-': fmt.flag.dash  = 1; break;
            case '+': fmt.flag.plus  = 1; break;
            case ' ': fmt.flag.space = 1; break;
            case '#': fmt.flag.hash  = 1; break;
            case '0': fmt.flag.zero  = 1; break;
        }
    }

    // Find field width
    {
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
            {
                fmt.field.asterisk = false;
            }
            c++;
        }
        else if ('1' <= *c && *c <= '9') // can't be 0. Leading 0 is a flag.
        {
            const char* num = c;
            unsigned digits = 0;
            do {
                digits++;
                c++;
            } while ('0' <= *c && *c <= '9');

            unsigned digit = 1;
            while (digits)
            {
                fmt.field.width += (num[digits - 1] - '0') * digit;
                digit *= 10;
                digits--;
            }
        }
    }

    // Find precision
    if (*c == '.')
    {
        c++; // ignore '.'

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
            {
                fmt.precision.option = PF_NONE;
            }

            c++;
        }
        else
        {
            fmt.precision.option = PF_SOME;
            const char* num = c;
            unsigned digits = 0;

            while ('0' <= *c && *c <= '9')
            {
                digits++;
                c++;
            }

            unsigned digit = 1;
            while (digits)
            {
                fmt.precision.width += (num[digits - 1] - '0') * digit;
                digit *= 10;
                digits--;
            }
        }
    }

    // Find length modifier
    const char* modifier = strchr("hljztLBWDQ", *c);
    if (modifier != NULL)
    {
        fmt.length_modifier = *modifier;
        c++;
        if (*modifier == 'h' && *c == 'h') {
            fmt.length_modifier += 'h';
            c++;
        }
        if (*modifier == 'l' && *c == 'l') {
            fmt.length_modifier += 'l';
            c++;
        }
    }

    fmt.conversion_format = *c;
    c++; // get to the end of string
    fmt.string_length = c - fmt.string;

    return fmt;
}
