// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/generic.h>
#include <gpc/utils.h>
#include "common.h"

GPString gp_str_make(struct gp_str_maker maker)
{
    if (maker.init == NULL)
        maker.init = "";
    return gp_str_new(
        maker.allocator, gp_max((size_t)16, gp_next_power_of_2(strlen(maker.init))), maker.init);
}

// ----------------------------------------------------------------------------
// String

size_t gp_codepoint_count99(GPStrIn s)
{
    return gp_bytes_codepoint_count(s.data, s.length);
}

bool gp_is_valid99(GPStrIn s, size_t*i)
{
    return gp_bytes_is_valid_utf8(s.data, s.length, i);
}

// ----------------------------------------------------------------------------
// Srtings and arrays

void gp_reserve99(const size_t elem_size, void* px, const size_t capacity)
{
    if (gp_arr_allocator(*(void**)px) == NULL)
        return;

    if (elem_size == sizeof(GPChar))
        gp_str_reserve(px, capacity);
    else
        *(void**)px = gp_arr_reserve(elem_size, *(void**)px, capacity);
}

static size_t gp_length99(const void* x, const char* ident, const size_t length, const size_t size)
{
    return ident == NULL ?
        length            : ident[0] == '"' ?
        length - sizeof"" : strchr(ident, '{') ?
        length / size     : gp_arr_length(x);
}

void* gp_copy99(const size_t y_size, void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size)
{
    x_length = gp_length99(x, x_ident, x_length, x_size);
    if (y_size >= sizeof(GPAllocator)) {
        void* out = gp_arr_new(y, x_size, x_length + sizeof"");
        ((GPArrayHeader*)out - 1)->length = x_length;
        return memcpy(out, x, x_size * x_length);
    }

    if (x_size == 1)
        gp_str_copy(y, x, x_length);
    else
        *(void**)y = gp_arr_copy(x_size, *(void**)y, x, x_length);
    return *(void**)y;
}

void* gp_slice99(
    const size_t y_size, const void* y,
    const size_t x_size, const void* x,
    const size_t start, const size_t end)
{
    if (y_size >= sizeof(GPAllocator)) {
        void* out = gp_arr_new(y, x_size, end - start + sizeof"");
        ((GPArrayHeader*)out - 1)->length = end - start;
        return memcpy(out, (uint8_t*)x + start * x_size, (end - start) * x_size);
    }
    return gp_arr_slice(x_size, *(void**)y, x, start, end);
}

void* gp_append99(
    const size_t a_size, void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length)
{
    b_length = gp_length99(b, b_ident, b_length, b_size);
    if (a_size < sizeof(GPAllocator))
    {
        if (b_size == 1) {
            gp_str_append(a, b, b_length);
            return *(GPString*)a;
        } else {
            return gp_arr_append(b_size, *(void**)a, b, b_length);
        }
    }
    c_length = gp_length99(c, c_ident, c_length, b_size);
    void* out = gp_arr_new(a, b_size, b_length + c_length + sizeof"");
    memcpy(out, b, b_length * b_size);
    memcpy((uint8_t*)out + b_length * b_size, c, c_length * b_size);
    ((GPArrayHeader*)out - 1)->length = b_length + c_length;
    return out;
}

void* gp_insert99(
    const size_t a_size, void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length)
{
    b_length = gp_length99(b, b_ident, b_length, b_size);
    if (a_size < sizeof(GPAllocator))
    {
        if (b_size == 1) {
            gp_str_insert(a, pos, b, b_length);
            return *(GPString*)a;
        } else {
            return gp_arr_insert(b_size, *(void**)a, pos, b, b_length);
        }
    }
    c_length = gp_length99(c, c_ident, c_length, b_size);
    void* out = gp_arr_new(a, b_size, b_length + c_length + sizeof"");
    memcpy(out, b, pos * b_size);
    memcpy((uint8_t*)out + pos * b_size, c, c_length * b_size);
    memcpy((uint8_t*)out + (pos + c_length) * b_size, (uint8_t*)b + pos * b_size, (b_length - pos) * b_size);
    ((GPArrayHeader*)out - 1)->length = b_length + c_length;
    return out;
}

// ----------------------------------------------------------------------------
// Arrays

GPArray(void) gp_map99(const size_t a_size, const void* a,
    const GPArray(void)const src, const char*const src_ident,
    const size_t src_size, const size_t src_elem_size,
    void(*f)(void*,const void*))
{
    const size_t src_length = gp_length99(src, src_ident, src_size, src_elem_size);
    if (a_size < sizeof(GPAllocator))
        return gp_arr_map(src_elem_size, *(GPArray(void)*)a, src, src_length, f);

    GPArray(void) out = gp_arr_new(a, src_elem_size, src_length);
    return gp_arr_map(src_elem_size, out, src, src_length, f);
}
