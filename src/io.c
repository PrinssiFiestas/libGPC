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
    const gp_type_t type)
{
    gp_db_assert((int)type < INT_MAX-16, "gp_print() family of macros require format strings in C99.");

    size_t length = 0;
    switch (type)
    {
    case GP_TYPE_CHAR:
    case GP_TYPE_SIGNED_CHAR:
    case GP_TYPE_UNSIGNED_CHAR:
        length = 1;
        fputc(va_arg(args->list, gp_promoted_arg_char_t), out);
        break;

    char buf[64];
    case GP_TYPE_UNSIGNED_SHORT:
    case GP_TYPE_UNSIGNED:
        length = pf_utoa(sizeof buf, buf, va_arg(args->list, unsigned));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_UNSIGNED_LONG:
        length = pf_utoa(sizeof buf, buf, va_arg(args->list, unsigned long));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_UNSIGNED_LONG_LONG:
        length = pf_utoa(sizeof buf, buf, va_arg(args->list, unsigned long long));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_UINT128:
        length = pf_u128toa(sizeof buf, buf, va_arg(args->list, GPUInt128));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_BOOL:
        if (va_arg(args->list, gp_promoted_arg_bool_t)) {
            length = sizeof"true"-sizeof"";
            fputs("true", out);
        } else {
            length = sizeof"false"-sizeof"";
            fputs("false", out);
        }
        break;

    case GP_TYPE_SHORT:
    case GP_TYPE_INT:
        length = pf_itoa(sizeof buf, buf, va_arg(args->list, int));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_LONG:
        length = pf_itoa(sizeof buf, buf, va_arg(args->list, long));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_LONG_LONG:
        length = pf_itoa(sizeof buf, buf, va_arg(args->list, long long));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_INT128:
        length = pf_i128toa(sizeof buf, buf, va_arg(args->list, GPInt128));
        fwrite(buf, 1, length, out);
        break;

    case GP_TYPE_FLOAT:
    case GP_TYPE_DOUBLE:
    case GP_TYPE_LONG_DOUBLE: {
        PFFormatSpecifier fmt = {0};
        fmt.conversion_format = 'g';
        #if GP_HAS_LONG_DOUBLE // again, may lose precision due to missing pf_strfroml()
        if (fmt.length_modifier == 'L')
            length = pf_strfromd(buf, sizeof buf, fmt, va_arg(args->list, long double));
        else
        #endif
            length = pf_strfromd(
                buf,
                sizeof buf,
                fmt,
                va_arg(args->list, gp_promoted_arg_double_t));

        fwrite(buf, 1, length, out);
        } break;

    case GP_TYPE_CHAR_PTR: { // TODO wide strings!
        const char* cstr = va_arg(args->list, char*);
        length = strlen(cstr);
        fwrite(cstr, 1, length, out);
        } break;

    case GP_TYPE_STRING: {
        GPString s = va_arg(args->list, GPString);
        length = gp_str_length(s);
        fwrite(s, 1, length, out);
        } break;

    case GP_TYPE_PTR: {
        const uintptr_t p = (uintptr_t)va_arg(args->list, void*);
        if (p != 0) {
            strcpy(buf, "0x");
            const size_t strlen0x = sizeof"0x"-sizeof"";
            length = strlen0x + pf_xtoa(sizeof buf - strlen0x, buf + strlen0x, p);
            fwrite(buf, 1, length, out);
        } else {
            length = sizeof"(nil)"-sizeof"";
            fwrite("(nil)", 1, length, out);
        }
        } break;

    case GP_NO_TYPE:
    case GP_TYPE_LENGTH:
        GP_UNREACHABLE("");
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
        case 'c':
            if (fmt.length_modifier != 'l')
                (void)va_arg(args->list, gp_promoted_arg_char_t);
            else
                (void)va_arg(args->list, gp_promoted_arg_wint_t);
            break;

        case 'i': case 'd': switch (fmt.length_modifier)
            {
            case 2 * 'h': (void)va_arg(args->list, gp_promoted_arg_signed_char_t); break;
            case 'h':     (void)va_arg(args->list, gp_promoted_arg_short_t      ); break;
            case  0 :     (void)va_arg(args->list, gp_promoted_arg_int_t        ); break;
            case 'l':     (void)va_arg(args->list, gp_promoted_arg_long_t       ); break;
            case 2 * 'l': (void)va_arg(args->list, gp_promoted_arg_long_long_t  ); break;
            case 'B':     (void)va_arg(args->list, gp_promoted_arg_int8_t       ); break;
            case 'W':     (void)va_arg(args->list, gp_promoted_arg_int16_t      ); break;
            case 'D':     (void)va_arg(args->list, gp_promoted_arg_int32_t      ); break;
            case 'Q':     (void)va_arg(args->list, gp_promoted_arg_int64_t      ); break;
            case 'O':     (void)va_arg(args->list, GPInt128                     ); break;
            case 'j':     (void)va_arg(args->list, gp_promoted_arg_ptrdiff_t    ); break;
            case 'B'+'f': (void)va_arg(args->list, gp_promoted_arg_int_fast8_t  ); break;
            case 'W'+'f': (void)va_arg(args->list, gp_promoted_arg_int_fast16_t ); break;
            case 'D'+'f': (void)va_arg(args->list, gp_promoted_arg_int_fast32_t ); break;
            case 'Q'+'f': (void)va_arg(args->list, gp_promoted_arg_int_fast64_t ); break;
            #ifdef SSIZE_MAX
            case 'z':     (void)va_arg(args->list, gp_promoted_arg_ssize_t      ); break;
            #endif
            default: GP_UNREACHABLE("");
            }
            break;

        case 'o':
        case 'x': case 'X':
        case 'u': switch (fmt.length_modifier)
            {
            case 2 * 'h': (void)va_arg(args->list, gp_promoted_arg_unsigned_char_t     ); break;
            case 'h':     (void)va_arg(args->list, gp_promoted_arg_unsigned_short_t    ); break;
            case  0:      (void)va_arg(args->list, gp_promoted_arg_unsigned_t          ); break;
            case 'l':     (void)va_arg(args->list, gp_promoted_arg_unsigned_long_t     ); break;
            case 2 * 'l': (void)va_arg(args->list, gp_promoted_arg_unsigned_long_long_t); break;
            case 'B':     (void)va_arg(args->list, gp_promoted_arg_uint8_t             ); break;
            case 'W':     (void)va_arg(args->list, gp_promoted_arg_uint16_t            ); break;
            case 'D':     (void)va_arg(args->list, gp_promoted_arg_uint32_t            ); break;
            case 'Q':     (void)va_arg(args->list, gp_promoted_arg_uint64_t            ); break;
            case 'O':     (void)va_arg(args->list, GPUInt128                           ); break;
            case 'z':     (void)va_arg(args->list, gp_promoted_arg_size_t              ); break;
            case 'B'+'f': (void)va_arg(args->list, gp_promoted_arg_uint_fast8_t        ); break;
            case 'W'+'f': (void)va_arg(args->list, gp_promoted_arg_uint_fast16_t       ); break;
            case 'D'+'f': (void)va_arg(args->list, gp_promoted_arg_uint_fast32_t       ); break;
            case 'Q'+'f': (void)va_arg(args->list, gp_promoted_arg_uint_fast64_t       ); break;
            default: GP_UNREACHABLE("");
            }
            break;

        case 's': case 'S':
        case 'p': (void)va_arg(args->list, void*); break;

        case 'f': case 'F':
        case 'e': case 'E':
        case 'g': case 'G':
            #if GP_HAS_LONG_DOUBLE
            if (fmt.length_modifier == 'L')
                (void)va_arg(args->list, long double);
            else
            #endif
                (void)va_arg(args->list, gp_promoted_arg_double_t);
            break;

        default:
            GP_UNREACHABLE("");
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
        length += sizeof" "-sizeof"" + gp_print_objects(out, &args, &i, objs[i]);

        if (i < arg_count - 1)
            fputs(" ",  out);
    }
    fputs("\n", out);
    va_end(_args);
    va_end(args.list);

    return length;
}

