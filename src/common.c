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

size_t gp_arr_length(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->length;
}

size_t gp_arr_capacity(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->capacity;
}

void* gp_arr_allocation(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->allocation;
}

const GPAllocator* gp_arr_allocator(const void* arr)
{
    return ((GPArrayHeader*)arr - 1)->allocator;
}

GPArray(void) gp_arr_reserve(
    const size_t element_size,
    GPArray(void) arr,
    size_t        capacity)
{
    if (capacity >= gp_arr_capacity(arr))
    {
        capacity = gp_next_power_of_2(capacity);
        GPArrayHeader* new_block = gp_mem_alloc(
            gp_arr_allocator(arr),
            sizeof*new_block + capacity * element_size);

        memcpy(new_block, (GPArrayHeader*)arr - 1,
            sizeof*new_block + gp_arr_length(arr) * element_size);

        new_block->capacity   = capacity;
        new_block->allocation = new_block;

        gp_mem_dealloc(gp_arr_allocator(arr), gp_arr_allocation(arr));
        arr = new_block + 1;
    }
    return arr;
}

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

size_t gp_bytes_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_bytes_codepoint_length(haystack + i);
        if (cp_length == 0 || i + cp_length > length)
            return i;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
        if ( ! gp_valid_codepoint(codepoint))
            return i;

        i += cp_length;
    }
    return GP_NOT_FOUND;
}

size_t gp_bytes_find_valid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        size_t cp_length = gp_bytes_codepoint_length(haystack + i);
        if (cp_length == 1)
            return i;
        if (cp_length == 0)
            continue;

        if (cp_length + i < length) {
            uint32_t codepoint = 0;
            for (size_t j = 0; j < cp_length; j++)
                codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
            if (gp_valid_codepoint(codepoint))
                return i;
        } // else maybe there's ascii in last bytes so continue
    }
    return length;
}

int gp_bytes_case_compare_alc(
    const void*_s1,
    const size_t s1_length,
    const void*_s2,
    const size_t s2_length,
    const GPAllocator* alc)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;

    size_t buf1_cap  = 1 << 10;
    size_t buf2_cap  = 1 << 10;
    wchar_t stack_buf1[1 << 10];
    wchar_t stack_buf2[1 << 10];
    wchar_t* buf1 = stack_buf1;
    wchar_t* buf2 = stack_buf2;
    if (s1_length + 1 >= buf1_cap) {
        buf1_cap = s1_length + 1;
        buf1 = gp_mem_alloc(alc, buf1_cap * sizeof(wchar_t));
    } if (s2_length + 1 >= buf2_cap) {
        buf2_cap = s2_length + 1;
        buf2 = gp_mem_alloc(alc, buf2_cap * sizeof(wchar_t));
    }
    mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0});
    mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0});

    int result = wcscoll(buf1, buf2);
    if (buf1 != stack_buf1)
        gp_mem_dealloc(alc, buf1);
    if (buf2 != stack_buf2)
        gp_mem_dealloc(alc, buf2);
    return result;
}

