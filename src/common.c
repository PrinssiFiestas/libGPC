// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "common.h"
#include <gpc/string.h>
#include <gpc/unicode.h>
#include <gpc/array.h>
#include <gpc/utils.h>
#include <printf/printf.h>
#include <stdint.h>
#include <wchar.h>

extern inline void   gp_arena_dealloc(const GPAllocator*, void*);
extern inline size_t gp_max_digits_in  (const GPType T);
extern inline size_t gp_count_fmt_specs(const char* fmt);

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
bool gp_valid_codepoint(
    const uint32_t c)
{
    if (c <= 0x7Fu)
        return true;

    if (0xC280u <= c && c <= 0xDFBFu)
       return ((c & 0xE0C0u) == 0xC080u);

    if (0xEDA080u <= c && c <= 0xEDBFBFu)
       return 0; // Reject UTF-16 surrogates

    if (0xE0A080u <= c && c <= 0xEFBFBFu)
       return ((c & 0xF0C0C0u) == 0xE08080u);

    if (0xF0908080u <= c && c <= 0xF48FBFBFu)
       return ((c & 0xF8C0C0C0u) == 0xF0808080u);

    return false;
}

bool gp_bytes_is_valid_utf8(
    const void*_str,
    const size_t length,
    size_t* invalid_index)
{
    const char* str = (const char*)_str;
    for (size_t i = 0; i < length;)
    {
        size_t cp_length = gp_utf8_codepoint_length(str, i);
        if (cp_length == 0 || i + cp_length > length) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)str[i + j];
        if ( ! gp_valid_codepoint(codepoint)) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
        i += cp_length;
    }
    return true;
}

size_t gp_bytes_codepoint_count(
    const void* _str,
    const size_t n)
{
    size_t count = 0;
    const char* str = _str;
    static const size_t valid_leading_nibble[] = {
        1,1,1,1, 1,1,1,1, 0,0,0,0, 1,1,1,1
    };
    if (n <= 8) { // Not worth optimizing. Also GCC miscompiles on x86_64 Linux.
        for (size_t i = 0; i < n; ++i)
            count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];
        return count;
    }
    // else process in parallel for blazing speed

    const size_t align_offset = (uintptr_t)str     % 8;
    const size_t remaining    = (n - align_offset) % 8;
    size_t i = 0;

    for (size_t len = gp_min(align_offset, n); i < len; ++i)
        count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];

    for (; i < n - remaining; i += 8)
    {
        // Read 8 bytes to be processed in parallel
        uint64_t x;
        memcpy(&x, str + i, sizeof x);

        // Extract bytes that start with 0b10
        const uint64_t a =   x & 0x8080808080808080llu;
        const uint64_t b = (~x & 0x4040404040404040llu) << 1;

        // Each byte in c is either 0 or 0b10000000
        uint64_t c = a & b;

        uint32_t bit_count;
        #ifdef __clang__ // only Clang seems to benefit from popcount()
        bit_count = __builtin_popcountll(c);
        #else
        //https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
        uint32_t v0 = c & 0xffffffffllu;
        uint32_t v1 = c >> 32;

        v0 = v0 - (v0 >> 1);
        v0 = (v0 & 0x33333333) + ((v0 >> 2) & 0x33333333);
        bit_count = (((v0 + (v0 >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;

        v1 = v1 - (v1 >> 1);
        v1 = (v1 & 0x33333333) + ((v1 >> 2) & 0x33333333);
        bit_count += (((v1 + (v1 >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
        #endif

        count += 8 - bit_count;
    }
    for (; i < n; i++)
        count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];

    return count;
}

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

