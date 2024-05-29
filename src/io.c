// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/io.h>
#include <gpc/string.h>
#include <printf/printf.h>
#include "common.h"

extern inline void gp_file_close(void* optional_file);
extern inline int gp_stat(GPStat* s, const char* path);

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
            c = fgetc(in);
            if (c == EOF)
                goto end;
            (*out)[((GPStringHeader*)*out - 1)->length++].c = c;
            if (c == '\n')
                goto end;
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

    const char* match = delimiter;
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
                match++;
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
    while (true) // strip left
    {
        int c = fgetc(in);
        if (c == EOF)
            return false;
        char codepoint[8] = {c};
        size_t codepoint_length = gp_char_codepoint_length(codepoint);
        for (size_t i = 1; i < codepoint_length; i++) {
            if ((c = fgetc(in)) == EOF)
                return false;
            codepoint[i] = c;
        }
        if (strstr(char_set, codepoint) != NULL)
            break;
    }
    while (true) // write until codepoint found in char set
    {
        int c = fgetc(in);
        if (c == EOF)
            return false;
        char codepoint[8] = {c};
        size_t codepoint_length = gp_char_codepoint_length(codepoint);
        for (size_t i = 1; i < codepoint_length; i++) {
            if ((c = fgetc(in)) == EOF)
                return false;
            codepoint[i] = c;
        }
        if (strstr(char_set, codepoint) == NULL)
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

        case GP_UNSIGNED_SHORT:
        case GP_UNSIGNED:
            length = fprintf(out, "%u", va_arg(args->list, unsigned));
            break;

        case GP_UNSIGNED_LONG:
            length = fprintf(out, "%lu", va_arg(args->list, unsigned long));
            break;

        case GP_UNSIGNED_LONG_LONG:
            length = fprintf(out, "%llu", va_arg(args->list, unsigned long long));
            break;

        case GP_BOOL:
            if (va_arg(args->list, int)) {
                length = strlen("true");
                fputs("true", out);
            } else {
                length = strlen("false");
                fputs("false", out);
            } break;

        case GP_SHORT:
        case GP_INT:
            length = fprintf(out, "%i", va_arg(args->list, int));
            break;

        case GP_LONG:
            length = fprintf(out, "%li", va_arg(args->list, long));
            break;

        case GP_LONG_LONG:
            length = fprintf(out, "%lli", va_arg(args->list, long long));
            break;

        case GP_FLOAT:
        case GP_DOUBLE:
            length = fprintf(out, "%g", va_arg(args->list, double));
            break;

        case GP_CHAR_PTR:
            length = fprintf(out, "%s", va_arg(args->list, char*));
            break;

        GPString s;
        case GP_STRING:
            s = va_arg(args->list, GPString);
            length = gp_arr_length(s);
            fwrite(s, 1, length, out);
            break;

        case GP_PTR:
            length = fprintf(out, "%p", va_arg(args->list, void*));
            break;
    }
    return length;
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
        *i += gp_count_fmt_specs(fmt);

        length += pf_vfprintf (out,     fmt, args->list);

        // Dummy consumption. TODO this is useless work, write a dedicated dummy
        // consumer.
        pf_vsnprintf_consuming(NULL, 0, fmt, args);
    } else {
        length += gp_print_va_arg(out, args, obj.type);
    }
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
    {
        length += gp_print_objects(
            out,
            &args,
            &i,
            objs[i]);
    }
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
        length += strlen(" ") + gp_print_objects(
            out,
            &args,
            &i,
            objs[i]);

        if (i < arg_count - 1)
            fputs(" ",  out);
    }
    fputs("\n", out);
    va_end(_args);
    va_end(args.list);

    return length;
}
