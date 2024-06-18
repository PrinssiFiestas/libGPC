// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/generic.h>
#include <gpc/utils.h>
#include "common.h"

extern inline GPArray(void) gp_arr99(const GPAllocator* alc,
    const size_t elem_size, const void*const init, const size_t init_length);

extern inline GPStrIn gp_str_in99(const void* data, const size_t length);
extern inline bool gp_equal99(const GPString a, GPStrIn b);
extern inline size_t gp_count99(GPStrIn haystack, GPStrIn needle);
extern inline GPString gp_repeat99(size_t a_size, const void* a, size_t count, GPStrIn in);
extern inline GPString gp_to_valid99(
    const GPAllocator* alc, const GPString str, const char*const replacement);
extern inline size_t gp_find_first99(const GPString haystack, GPStrIn needle);
extern inline size_t gp_find_last99(const GPString haystack, GPStrIn needle);
extern inline bool gp_equal_case99(const GPString a, GPStrIn b);

extern inline void* gp_push99(const size_t elem_size, void*_parr);

extern inline void* gp_put99(GPHashMap* dict, GPStrIn key);
extern inline void* gp_get99(void* map, GPStrIn key);
extern inline bool gp_remove99(GPHashMap* dict, GPStrIn key);

extern inline GPString gp_file99(size_t a_size, void* a, const char* path, const char* mode);

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

GPString gp_replace_new(const GPAllocator* a, GPStrIn b, GPStrIn c, GPStrIn d, const size_t start)
{
    GPString out = gp_str_new(a, b.length + c.length + d.length, "");
    const size_t pos = gp_bytes_find_first(b.data, b.length, c.data, c.length, start);
    if (pos == GP_NOT_FOUND) {
        memcpy(out, b.data, b.length);
        ((GPStringHeader*)out - 1)->length = b.length;
    } else {
        memcpy(out, b.data, pos);
        memcpy(out + pos, d.data, d.length);
        memcpy(out + pos + d.length, b.data + pos + c.length, b.length - c.length);
        ((GPStringHeader*)out - 1)->length = b.length + d.length - c.length;
    }
    return out;
}

GPString gp_replace99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d,
    const size_t start)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_replace((GPString*)a, b.data, b.length, c.data, c.length, start);
        return *(GPString*)a;
    }
    return gp_replace_new(a, b, c, d, start);
}

GPString gp_replace_all_new(const void* alc, GPStrIn hay, GPStrIn ndl, GPStrIn repl)
{
    // TODO don't copy and replace all, just copy what's needed
    GPString out = gp_str_new(alc, 3 * hay.length / 2, "");
    gp_str_copy(&out, hay.data, hay.length);
    gp_str_replace_all(&out, ndl.data, ndl.length, repl.data, repl.length);
    return out;
}

GPString gp_replace_all99(
    const size_t a_size, const void* a, GPStrIn b, GPStrIn c, GPStrIn d)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_replace_all((GPString*)a, b.data, b.length, c.data, c.length);
        return *(GPString*)a;
    }
    return gp_replace_all_new(a, b, c, d);
}

GPString gp_str_trim_new(const void* alc, GPStrIn str, const char* char_set, const int flags)
{
    GPString out = gp_str_new(alc, str.length, "");
    // TODO don't copy and trim, just copy what's needed!
    gp_str_copy(&out, str.data, str.length);
    gp_str_trim(&out, char_set, flags);
    return out;
}

GPString gp_trim99(
    const size_t a_size, const void* a, GPStrIn b, const char* char_set, int flags)
{
    if (a_size < sizeof(GPAllocator)) {
        gp_str_trim((GPString*)a, char_set, flags);
        return *(GPString*)a;
    }
    return gp_str_trim_new(a, b, char_set, flags);
}

GPString gp_to_upper99(const GPAllocator* alc, const GPString str)
{ // TODO don't copy and process. Read char, process, and write to out
    GPString out = gp_str_new(alc, gp_str_length(str), "");
    memcpy(out, str, gp_str_length(str));
    ((GPStringHeader*)out - 1)->length = gp_str_length(str);
    gp_str_to_upper(&out);
    return out;
}

GPString gp_to_lower99(const GPAllocator* alc, const GPString str)
{ // TODO don't copy and process. Read char, process, and write to out
    GPString out = gp_str_new(alc, gp_str_length(str), "");
    memcpy(out, str, gp_str_length(str));
    ((GPStringHeader*)out - 1)->length = gp_str_length(str);
    gp_str_to_lower(&out);
    return out;
}

GPString gp_to_valid99(
    const GPAllocator* alc, const GPString str, const char*const replacement)
{ // TODO don't copy and process. Read char, process, and write to out
    GPString out = gp_str_new(alc, gp_str_length(str), "");
    memcpy(out, str, gp_str_length(str));
    ((GPStringHeader*)out - 1)->length = gp_str_length(str);
    gp_str_to_valid(&out, replacement);
    return out;
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

static size_t gp_length99(const void* x, const char* ident, const size_t length)
{
    return
        ident == NULL ? length : ident[0] == '"' ? length - sizeof"" : gp_arr_length(x);
}

void* gp_copy99(const size_t y_size, const void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size)
{
    x_length = gp_length99(x, x_ident, x_length);
    if (y_size >= sizeof(GPAllocator)) {
        void* out = gp_arr_new(y, x_size, x_length + sizeof"");
        ((GPArrayHeader*)out - 1)->length = x_length;
        return memcpy(out, x, x_size * x_length);
    }

    if (x_size == 1)
        gp_str_copy((GPString*)y, x, x_length);
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
    GPArray(void*) parr = (GPArray(void*))y;
    return *parr = gp_arr_slice(x_size, *parr, x, start, end);
}

void* gp_append99(
    const size_t a_size, const void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length)
{
    b_length = gp_length99(b, b_ident, b_length);
    if (a_size < sizeof(GPAllocator))
    {
        if (b_size == 1) {
            gp_str_append((GPString*)a, b, b_length);
            return *(GPString*)a;
        } else {
            GPArray(void)* parr = (GPArray(void)*)a;
            return *parr = gp_arr_append(b_size, *parr, b, b_length);
        }
    }
    c_length = gp_length99(c, c_ident, c_length);
    void* out = gp_arr_new(a, b_size, b_length + c_length + sizeof"");
    memcpy(out, b, b_length * b_size);
    memcpy((uint8_t*)out + b_length * b_size, c, c_length * b_size);
    ((GPArrayHeader*)out - 1)->length = b_length + c_length;
    return out;
}

void* gp_insert99(
    const size_t a_size, const void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length)
{
    b_length = gp_length99(b, b_ident, b_length);
    if (a_size < sizeof(GPAllocator))
    {
        if (b_size == 1) {
            gp_str_insert((GPString*)a, pos, b, b_length);
            return *(GPString*)a;
        } else {
            GPArray(void)* parr = (GPArray(void)*)a;
            return *parr = gp_arr_insert(b_size, *parr, pos, b, b_length);
        }
    }
    c_length = gp_length99(c, c_ident, c_length);
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
    const size_t src_length = gp_length99(src, src_ident, src_size);
    if (a_size < sizeof(GPAllocator))
        return gp_arr_map(src_elem_size, *(GPArray(void)*)a, src, src_length, f);

    GPArray(void) out = gp_arr_new(a, src_elem_size, src_length);
    return out = gp_arr_map(src_elem_size, out, src, src_length, f);
}

GPArray(void) gp_filter99(size_t a_size, const void* a,
    const GPArray(void) src, const char*src_ident, size_t src_size, size_t src_elem_size,
    bool(*f)(const void* element))
{
    const size_t src_length = gp_length99(src, src_ident, src_size);
    if (a_size < sizeof(GPAllocator))
        return gp_arr_filter(src_elem_size, *(GPArray(void)*)a, src, src_length, f);

    GPArray(void) out = gp_arr_new(a, src_elem_size, src_length);
    return out = gp_arr_filter(src_elem_size, out, src, src_length, f);
}
