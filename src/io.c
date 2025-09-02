// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/io.h>
#include <gpc/string.h>
#include <gpc/unicode.h>
#include <printf/printf.h>
#include <printf/conversions.h>
#include <printf/format_scanning.h>
#include "common.h"

#if !(defined(__COMPCERT__) && defined(GPC_IMPLEMENTATION))
extern inline void gp_file_close(FILE*);
#endif

FILE* gp_file_open(const char* path, const char* mode)
{
    size_t len = 0;
    char mode_buf[4] = { mode[len++] };
    if ( ! strchr(mode, 'x'))
        mode_buf[len++] = 'b';
    if (strchr(mode, '+'))
        mode_buf[len++] = '+';
    return fopen(path, mode_buf);
}

bool gp_file_read_line(GPString* out, FILE* in)
{
    int c = fgetc(in);
    if (c == EOF)
        return false;

    (*out)[0].c = c;
    ((GPStringHeader*)*out - 1)->length = 1;

    while (true)
    {
        while (gp_str_length(*out) < gp_str_capacity(*out))
        {
            if (c == '\n')
                goto end;
            c = fgetc(in);
            if (c == EOF)
                goto end;
            (*out)[((GPStringHeader*)*out - 1)->length++].c = c;
        }
        gp_str_reserve(out, gp_str_capacity(*out) + 1); // doubles cap
    }
    end:
    return true;
}

bool gp_file_read_until(
    GPString*   out,
    FILE*       in,
    const char* delimiter)
{
    int c = fgetc(in);
    if (c == EOF)
        return false;

    (*out)[0].c = c;
    ((GPStringHeader*)*out - 1)->length = 1;

    const char* match = delimiter + (delimiter[0] == c);
    while (true)
    {
        while (gp_str_length(*out) < gp_str_capacity(*out))
        {
            if (*match == '\0')
                goto end;
            c = fgetc(in);
            if (c == EOF)
                goto end;
            (*out)[((GPStringHeader*)*out - 1)->length++].c = c;
            if (c == *match)
                ++match;
            else
                match = delimiter;
        }
        gp_str_reserve(out, gp_str_capacity(*out) + 1); // doubles cap
    }
    end:
    return true;
}

bool gp_file_read_strip(
    GPString*   out,
    FILE*       in,
    const char* char_set)
{
    if (char_set == NULL)
        char_set = GP_WHITESPACE;

    ((GPStringHeader*)*out - 1)->length = 0;

    while (true) // strip left
    {
        int c = fgetc(in);
        if (c == EOF)
            return false;
        char codepoint[8] = {c};
        size_t codepoint_length = gp_utf8_codepoint_length(codepoint, 0);
        for (size_t i = 1; i < codepoint_length; i++) {
            if ((c = fgetc(in)) == EOF)
                return false;
            codepoint[i] = c;
        }
        if (strstr(char_set, codepoint) == NULL) {
            gp_str_append(out, codepoint, codepoint_length);
            break;
        }
    }
    while (true) // write until codepoint found in char set
    {
        int c = fgetc(in);
        if (c == EOF)
            return false;
        char codepoint[8] = {c};
        size_t codepoint_length = gp_utf8_codepoint_length(codepoint, 0);
        for (size_t i = 1; i < codepoint_length; i++) {
            if ((c = fgetc(in)) == EOF)
                return false;
            codepoint[i] = c;
        }
        if (strstr(char_set, codepoint) != NULL)
            break;
        gp_str_append(out, codepoint, codepoint_length);
    }
    return true;
}

static size_t gp_print_va_arg(
    FILE* out,
    pf_va_list*restrict const args,
    const GPType type)
{
    size_t length = 0;
    switch (type)
    {
    case GP_CHAR:
    case GP_SIGNED_CHAR:
    case GP_UNSIGNED_CHAR:
        length = 1;
        fputc(va_arg(args->list, int), out);
        break;

    char buf[64];
    case GP_UNSIGNED_SHORT:
    case GP_UNSIGNED:
        length = pf_utoa(sizeof buf, buf, va_arg(args->list, unsigned));
        fwrite(buf, 1, length, out);
        break;

    case GP_UNSIGNED_LONG:
        length = pf_utoa(sizeof buf, buf, va_arg(args->list, unsigned long));
        fwrite(buf, 1, length, out);
        break;

    case GP_UNSIGNED_LONG_LONG:
        length = pf_utoa(sizeof buf, buf, va_arg(args->list, unsigned long long));
        fwrite(buf, 1, length, out);
        break;

    case GP_UINT128:
        length = pf_u128toa(sizeof buf, buf, va_arg(args->list, GPUInt128));
        fwrite(buf, 1, length, out);
        break;

    case GP_BOOL:
        if (va_arg(args->list, int)) {
            length = strlen("true");
            fputs("true", out);
        } else {
            length = strlen("false");
            fputs("false", out);
        }
        break;

    case GP_SHORT:
    case GP_INT:
        length = pf_itoa(sizeof buf, buf, va_arg(args->list, int));
        fwrite(buf, 1, length, out);
        break;

    case GP_LONG:
        length = pf_itoa(sizeof buf, buf, va_arg(args->list, long));
        fwrite(buf, 1, length, out);
        break;

    case GP_LONG_LONG:
        length = pf_itoa(sizeof buf, buf, va_arg(args->list, long long));
        fwrite(buf, 1, length, out);
        break;

    case GP_INT128:
        length = pf_i128toa(sizeof buf, buf, va_arg(args->list, GPInt128));
        fwrite(buf, 1, length, out);
        break;

    case GP_FLOAT:
    case GP_DOUBLE:
    case GP_LONG_DOUBLE: {
        PFFormatSpecifier fmt = {0};
        fmt.conversion_format = 'g';
        #if GP_HAS_LONG_DOUBLE // again, may lose precision due to missing pf_strfroml()
        if (fmt.length_modifier == 'L')
            length = pf_strfromd(buf, sizeof buf, fmt, va_arg(args->list, long double));
        else
        #endif
            length = pf_strfromd(buf, sizeof buf, fmt, va_arg(args->list, double));
        fwrite(buf, 1, length, out);
        } break;

    case GP_CHAR_PTR: {
        const char* cstr = va_arg(args->list, char*);
        length = strlen(cstr);
        fwrite(cstr, 1, length, out);
        } break;

    case GP_STRING: {
        GPString s = va_arg(args->list, GPString);
        length = gp_arr_length(s);
        fwrite(s, 1, length, out);
        } break;

    case GP_PTR: {
        const uintptr_t p = va_arg(args->list, uintptr_t);
        if (p != 0) {
            strcpy(buf, "0x");
            length = strlen("0x") + pf_xtoa(sizeof buf - strlen("0x"), buf + strlen("0x"), p);
            fwrite(buf, 1, length, out);
        } else {
            length = strlen("(nil)");
            fwrite("(nil)", 1, length, out);
        }
        } break;
    }
    return length;
}

static void gp_va_list_dummy_consume(
    const char* format,
    pf_va_list* args)
{
    while (1)
    {
        const PFFormatSpecifier fmt = pf_scan_format_string(format, args);
        if (fmt.string == NULL)
            return;

        // Jump over format specifier for next iteration
        format = fmt.string + fmt.string_length;

        switch (fmt.conversion_format)
        {
        case 'c': va_arg(args->list, int); break;

        case 'i': case 'd': switch (fmt.length_modifier)
            {
            case 2 * 'h':
            case 'h':
            case 'B': case 'B'+'f':
            case 'W':
            case  0 :     va_arg(args->list, int         ); break;
            case 'D':     va_arg(args->list, int32_t     ); break;
            case 'Q':     va_arg(args->list, int64_t     ); break;
            case 'O':     va_arg(args->list, GPInt128    ); break;
            case 'l':     va_arg(args->list, long        ); break;
            case 2 * 'l': va_arg(args->list, long long   ); break;
            case 'j':     va_arg(args->list, ptrdiff_t   ); break;
            case 'W'+'f': va_arg(args->list, int_fast16_t); break;
            case 'D'+'f': va_arg(args->list, int_fast32_t); break;
            case 'Q'+'f': va_arg(args->list, int_fast64_t); break;
            #if GP_HAS_SSIZE_T
            case 'z':     va_arg(args->list, ssize_t     ); break;
            #endif
            }
            break;

        case 'o':
        case 'x': case 'X':
        case 'u': switch (fmt.length_modifier)
            {
            case 2 * 'h':
            case 'h':
            case 'B': case 'B'+'f':
            case 'W':
            case 'D':
            case 0:       va_arg(args->list, unsigned          ); break;
            case 'Q':     va_arg(args->list, uint64_t          ); break;
            case 'O':     va_arg(args->list, GPUInt128         ); break;
            case 'l':     va_arg(args->list, unsigned long     ); break;
            case 2 * 'l': va_arg(args->list, unsigned long long); break;
            case 'z':     va_arg(args->list, size_t            ); break;
            case 'W'+'f': va_arg(args->list, uint_fast16_t     ); break;
            case 'D'+'f': va_arg(args->list, uint_fast32_t     ); break;
            case 'Q'+'f': va_arg(args->list, uint_fast64_t     ); break;
            }
            break;

        case 's': case 'S':
        case 'p': va_arg(args->list, void*); break;

        case 'f': case 'F':
        case 'e': case 'E':
        case 'g': case 'G':
            #if GP_HAS_LONG_DOUBLE
            if (fmt.length_modifier == 'L')
                va_arg(args->list, long double);
            else
            #endif
                va_arg(args->list, double);
            break;
        }
    }
}

static size_t gp_print_objects(
    FILE* out,
    pf_va_list* args,
    size_t*const i,
    GPPrintable obj)
{
    size_t length = 0;
    if (obj.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args->list, char*);
        *i += gp_count_fmt_specs(fmt); // skip args printed below

        length += pf_vfprintf(out, fmt, args->list); // remember, no va_arg consumption here,
        // so they must be consumed manually. // TODO we should implement pf_vfprintf_consuming() instead
        gp_va_list_dummy_consume(fmt, args);
    } else
        length += gp_print_va_arg(out, args, obj.type);

    return length;
}

size_t gp_file_print_internal(
    FILE* out,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
        length += gp_print_objects(out, &args, &i, objs[i]);

    va_end(_args);
    va_end(args.list);

    return length;
}

size_t gp_file_println_internal(
    FILE* out,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += strlen(" ") + gp_print_objects(out, &args, &i, objs[i]);

        if (i < arg_count - 1)
            fputs(" ",  out);
    }
    fputs("\n", out);
    va_end(_args);
    va_end(args.list);

    return length;
}

