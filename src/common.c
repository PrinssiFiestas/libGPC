// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "common.h"
#include <gpc/string.h>
#include <gpc/array.h>
#include <gpc/utils.h>
#include <printf/printf.h>
#include <stdint.h>
#include <wchar.h>

extern inline void   gp_arena_dealloc(const GPAllocator*, void*);
extern inline size_t gp_max_digits_in  (const GPType T);
extern inline size_t gp_count_fmt_specs(const char* fmt);

size_t gp_convert_va_arg(
    const size_t limit,
    void*restrict const out,
    pf_va_list*restrict const args,
    const GPType type)
{
    size_t length = 0;
    switch (type)
    {
        case GP_CHAR:
        case GP_SIGNED_CHAR:
        case GP_UNSIGNED_CHAR:
            length++;
            if (limit > 0)
                *(uint8_t*)out = (char)va_arg(args->list, int);
            break;

        case GP_UNSIGNED_SHORT:
        case GP_UNSIGNED:
            length += pf_utoa(
                limit,
                out,
                va_arg(args->list, unsigned));
            break;

        case GP_UNSIGNED_LONG:
            length += pf_utoa(
                limit,
                out,
                va_arg(args->list, unsigned long));
            break;

        case GP_UNSIGNED_LONG_LONG:
            length += pf_utoa(
                limit,
                out,
                va_arg(args->list, unsigned long long));
            break;

        case GP_BOOL:
            if (va_arg(args->list, int)) {
                length += strlen("true");
                memcpy(out, "true", gp_min(4llu, limit));
            } else {
                length += strlen("false");
                memcpy(out, "false", gp_min(5llu, limit));
            } break;

        case GP_SHORT:
        case GP_INT:
            length += pf_itoa(
                limit,
                out,
                va_arg(args->list, int));
            break;

        case GP_LONG:
            length += pf_itoa(
                limit,
                out,
                va_arg(args->list, long int));
            break;

        case GP_LONG_LONG:
            length += pf_itoa(
                limit,
                out,
                va_arg(args->list, long long int));
            break;

        case GP_FLOAT:
        case GP_DOUBLE:
            length += pf_gtoa(
                limit,
                out,
                va_arg(args->list, double));
            break;

        char* p;
        size_t p_len;
        case GP_CHAR_PTR:
            p = va_arg(args->list, char*);
            p_len = strlen(p);
            memcpy(out, p, gp_min(p_len, limit));
            length += p_len;
            break;

        GPString s;
        case GP_STRING:
            s = va_arg(args->list, GPString);
            memcpy(out, s, gp_min(gp_arr_length(s), limit));
            length += gp_arr_length(s);
            break;

        case GP_PTR:
            p = va_arg(args->list, void*);
            if (p != NULL) {
                memcpy(out, "0x", gp_min(2llu, limit));
                length += strlen("0x") + pf_xtoa(
                    limit > 2 ? limit - 2 : 0, (char*)out + strlen("0x"), (uintptr_t)p);
            } else {
                length += strlen("(nil)");
                memcpy(out, "(nil)", gp_min(strlen("(nil)"), limit));
            } break;
    }
    return length;
}

size_t gp_bytes_print_objects(
    const size_t limit,
    void*restrict out,
    pf_va_list* args,
    size_t*const i,
    GPPrintable obj)
{
    size_t length = 0;
    if (obj.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args->list, char*);
        *i += gp_count_fmt_specs(fmt);

        length += pf_vsnprintf_consuming(
            out,
            limit,
            fmt,
            args);
    } else {
        length += gp_convert_va_arg(limit, out, args, obj.type);
    }
    return length;
}

