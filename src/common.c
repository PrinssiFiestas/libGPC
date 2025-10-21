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

void gp_internal_arena_dealloc(GPAllocator* arena, void* mem)
{
    (void)arena;
    GP_TRY_POISON_MEMORY_REGION(mem, GP_ALLOC_ALIGNMENT);
}

void gp_internal_carena_dealloc(GPAllocator* arena, void* mem)
{
    (void)arena;
    (void)mem;
}

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
// https://stackoverflow.com/questions/66715611/check-for-valid-utf-8-encoding-in-c/66723102#66723102
bool gp_internal_bytes_is_valid_codepoint(
    const void*_str,
    const size_t i)
{
    // Instead of checking byte by byte, check all all bits in parallel.

    const char* str = _str;

    uint32_t cp_bits = 0; // not UTF-32, just raw bits
    for (size_t j = 0; j < gp_utf8_decode_codepoint_length(str, i); ++j)
        cp_bits = cp_bits << 8 | (uint8_t)str[i + j];

    if (cp_bits <= 0x7Fu)
        return true;

    if (0xC280u <= cp_bits && cp_bits <= 0xDFBFu)
       return ((cp_bits & 0xE0C0u) == 0xC080u);

    if (0xEDA080u <= cp_bits && cp_bits <= 0xEDBFBFu)
       return false; // Reject UTF-16 surrogates

    if (0xE0A080u <= cp_bits && cp_bits <= 0xEFBFBFu)
       return ((cp_bits & 0xF0C0C0u) == 0xE08080u);

    if (0xF0908080u <= cp_bits && cp_bits <= 0xF48FBFBFu)
       return ((cp_bits & 0xF8C0C0C0u) == 0xF0808080u);

    return false;
}

size_t gp_internal_bytes_codepoint_count_unsafe(
    const void* _str,
    const size_t n)
{
    size_t count = 0;
    const char* str = _str;
    static const size_t valid_leading_nibble[] = {
        1,1,1,1, 1,1,1,1, 0,0,0,0, 1,1,1,1
    };
    if (n <= 8) // Not worth optimizing. Also GCC miscompiles small string
    {           // inputs with the optimized code using -O3 in x86_64 Linux.
        for (size_t i = 0; i < n; ++i)
            count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];
        return count;
    }
    // else process in parallel

    const size_t align_offset = (uintptr_t)str     & 7;
    const size_t remaining    = (n - align_offset) & 7;
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

size_t gp_internal_convert_va_arg(
    const size_t limit,
    void*restrict const out,
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
        length++;
        if (limit > 0)
            *(uint8_t*)out = va_arg(args->list, gp_promoted_arg_char_t);
        break;

    case GP_TYPE_UNSIGNED_SHORT:
    case GP_TYPE_UNSIGNED:
        length += pf_utoa(limit, out, va_arg(args->list, unsigned));
        break;

    case GP_TYPE_UNSIGNED_LONG:
        length += pf_utoa(limit, out, va_arg(args->list, unsigned long));
        break;

    case GP_TYPE_UNSIGNED_LONG_LONG:
        length += pf_utoa(limit, out, va_arg(args->list, unsigned long long));
        break;

    case GP_TYPE_UINT128:
        length += pf_u128toa(limit, out, va_arg(args->list, GPUInt128));
        break;

    case GP_TYPE_BOOL:
        if (va_arg(args->list, gp_promoted_arg_bool_t)) {
            length += sizeof"true"-sizeof"";
            memcpy(out, "true", gp_min(4llu, limit));
        } else {
            length += sizeof"false"-sizeof"";
            memcpy(out, "false", gp_min(5llu, limit));
        } break;

    case GP_TYPE_SHORT:
    case GP_TYPE_INT:
        length += pf_itoa(limit, out, va_arg(args->list, int));
        break;

    case GP_TYPE_LONG:
        length += pf_itoa(limit, out, va_arg(args->list, long int));
        break;

    case GP_TYPE_LONG_LONG:
        length += pf_itoa(limit, out, va_arg(args->list, long long int));
        break;

    case GP_TYPE_INT128:
        length += pf_i128toa(limit, out, va_arg(args->list, GPInt128));
        break;

    case GP_TYPE_FLOAT:
        length += pf_gtoa(limit, out, va_arg(args->list, gp_promoted_arg_float_t));
        break;

    case GP_TYPE_DOUBLE:
        length += pf_gtoa(limit, out, va_arg(args->list, gp_promoted_arg_double_t));
        break;

    #if GP_HAS_LONG_DOUBLE // pf_Lgtoa() missing, may lose precision, but better than nothing
    case GP_TYPE_LONG_DOUBLE:
        length += pf_gtoa(limit, out, va_arg(args->list, long double));
        break;
    #else
    case GP_TYPE_LONG_DOUBLE: GP_UNREACHABLE("long double not supported.");
    #endif

    char* p;
    size_t p_len;
    case GP_TYPE_CHAR_PTR:
        p = va_arg(args->list, char*);
        p_len = strlen(p);
        memcpy(out, p, gp_min(p_len, limit));
        length += p_len;
        break;

    GPString s;
    case GP_TYPE_STRING:
        s = va_arg(args->list, GPString);
        memcpy(out, s, gp_min(gp_arr_length(s), limit));
        length += gp_arr_length(s);
        break;

    case GP_TYPE_PTR:
        p = va_arg(args->list, void*);
        if (p != NULL) {
            memcpy(out, "0x", gp_min(2llu, limit));
            length += sizeof"0x"-sizeof"" + pf_xtoa(
                limit > 2 ? limit - 2 : 0,
                (char*)out + sizeof"0x"-sizeof"", (uintptr_t)p);
        } else {
            length += sizeof"(nil)"-sizeof"";
            memcpy(out, "(nil)", gp_min(sizeof"(nil)"-sizeof"", limit));
        } break;

    case GP_NO_TYPE:
    case GP_TYPE_LENGTH:
        GP_UNREACHABLE("");
    }
    return length;
}

size_t gp_internal_bytes_print_objects(
    const size_t limit,
    void*restrict out,
    pf_va_list* args,
    size_t*const i,
    GPInternalReflectionData obj)
{
    size_t length = 0;
    if (obj.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args->list, char*);
        *i += gp_internal_count_fmt_specs(fmt);

        length += pf_vsnprintf_consuming_no_null_termination(out, limit, fmt, args);
    } else
        length += gp_internal_convert_va_arg(limit, out, args, obj.type);

    return length;
}
