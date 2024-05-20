// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifdef __GNUC__
#define _GNU_SOURCE // memmem()
#endif

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <gpc/array.h>
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <wctype.h>
#include <printf/printf.h>
#include <printf/conversions.h>
#include "pfstring.h"

GPString gp_str_new(
    const GPAllocator* allocator,
    size_t capacity)
{
    GPStringHeader* me = gp_mem_alloc(allocator, sizeof*me + capacity + sizeof"");
    *me = (GPStringHeader) {
        .capacity   = capacity,
        .allocator  = allocator,
        .allocation = me };
    return (GPString)(me + 1);
}

void gp_str_delete(GPString me)
{
    if (me != NULL && gp_str_allocation(me) != NULL)
        gp_dealloc(gp_str_allocator(me), gp_str_allocation(me));
}

static GPStringHeader* gp_str_header(const GPString str)
{
    return (GPStringHeader*)str - 1;
}

size_t             gp_str_length    (GPString s) { return gp_str_header(s)->length;    }
size_t             gp_str_capacity  (GPString s) { return gp_str_header(s)->capacity;  }
void*              gp_str_allocation(GPString s) { return gp_str_header(s)->allocation;}
const GPAllocator* gp_str_allocator (GPString s) { return gp_str_header(s)->allocator; }

size_t gp_str_find(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start)
{
    return gp_bytes_find(haystack, gp_str_length(haystack), needle, needle_size, start);
}

size_t gp_str_find_last(
    GPString haystack,
    const void* needle,
    size_t needle_length)
{
    return gp_bytes_find_last(haystack, gp_str_length(haystack), needle, needle_length);
}

size_t gp_str_count(
    GPString haystack,
    const void* needle,
    size_t     needle_size)
{
    return gp_bytes_count(haystack, gp_str_length(haystack), needle, needle_size);
}

bool gp_str_equal(
    GPString  s1,
    const void* s2,
    size_t      s2_size)
{
    if (gp_str_length(s1) != s2_size)
        return false;
    else
        return memcmp(s1, s2, s2_size) == 0;
}

bool gp_str_equal_case(
    GPString  s1,
    const void* s2,
    size_t      s2_size)
{
    return gp_bytes_equal_case(s1, gp_str_length(s1), s2, s2_size);
}

size_t gp_str_codepoint_count(
    GPString str)
{
    return gp_bytes_codepoint_count(str, gp_str_length(str));
}

bool gp_str_is_valid(
    GPString str)
{
    return gp_bytes_is_valid(str, gp_str_length(str));
}

size_t gp_str_codepoint_length(
    GPString str)
{
    return gp_bytes_codepoint_length(str);
}

const char* gp_cstr(GPString str)
{
    str[gp_str_length(str)].c = '\0';
    return (const char*)str;
}

void gp_str_reserve(
    GPString* str,
    size_t capacity)
{
    *str = gp_arr_reserve(sizeof**str, *str, capacity + sizeof"");
    gp_str_header(*str)->capacity -= sizeof"";
}

void gp_str_copy(
    GPString* dest,
    const void*restrict src,
    size_t n)
{
    gp_str_reserve(dest, n);
    memcpy(*dest, src, n);
    gp_str_header(*dest)->length = n;
}

void gp_str_repeat(
    GPString* dest,
    const size_t n,
    const void*restrict mem,
    const size_t mem_length)
{
    gp_str_reserve(dest, n * mem_length);
    if (mem_length == 1) {
        memset(*dest, *(uint8_t*)mem, n);
    } else for (size_t i = 0; i < n; i++) {
        memcpy(*dest + i * mem_length, mem, mem_length);
    }
    gp_str_header(*dest)->length = n * mem_length;
}

void gp_str_slice(
    GPString* dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    if (src != NULL) {
        gp_str_reserve(dest, end - start);
        memcpy(*dest, src + start, end - start);
        gp_str_header(*dest)->length = end - start;
    } else {
        memmove(*dest, *dest + start,  end - start);
        gp_str_header(*dest)->length = end - start;
    }
}

void gp_str_append(
    GPString* dest,
    const void* src,
    size_t src_length)
{
    gp_str_reserve(dest, gp_str_length(*dest) + src_length);
    memcpy(*dest + gp_str_length(*dest), src, src_length + sizeof"");
    gp_str_header(*dest)->length += src_length;
}

void gp_str_insert(
    GPString* dest,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    gp_str_reserve(dest, gp_str_length(*dest) + n);
    memmove(*dest + pos + n, *dest + pos, gp_str_length(*dest) - pos);
    memcpy(*dest + pos, src, n);
    gp_str_header(*dest)->length += n;
}

size_t gp_str_replace(
    GPString* haystack,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t start)
{
    if ((start = gp_str_find(*haystack, needle, needle_length, start)) == GP_NOT_FOUND)
        return GP_NOT_FOUND;

    gp_str_reserve(haystack,
        gp_str_length(*haystack) + replacement_length - needle_length);

    const size_t end = start + needle_length;
    gp_str_header(*haystack)->length = gp_bytes_replace_range(
        *haystack,
        gp_str_length(*haystack),
        start,
        end,
        replacement,
        replacement_length);

    return start;
}

size_t gp_str_replace_all(
    GPString* haystack,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length)
{
    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_str_find(*haystack, needle, needle_length, start)) != GP_NOT_FOUND)
    {
        gp_str_reserve(haystack,
            gp_str_length(*haystack) + replacement_length - needle_length);

        gp_str_header(*haystack)->length = gp_bytes_replace_range(
            *haystack,
            gp_str_length(*haystack),
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    return replacement_count;
}

static size_t gp_str_print_object_size(GPPrintable object, pf_va_list _args)
{
    va_list args;
    va_copy(args, _args.list);

    size_t length = 0;
    if (object.identifier[0] == '\"')
    {
        const char* fmt = va_arg(args, char*);
        length = pf_vsnprintf(
            NULL,
            0,
            fmt,
            args);
    } else {
        switch (object.type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                length = 1;
                break;

            case GP_BOOL:
                length = strlen("false");
                break;

            char* p;
            size_t p_len;
            case GP_CHAR_PTR:
                p = va_arg(args, char*);
                p_len = strlen(p);
                length = p_len;
                break;

            GPString s;
            case GP_STRING:
                s = va_arg(args, GPString);
                length = gp_str_length(s);
                break;

            default:
                length = gp_max_digits_in(object.type);
        }
    }
    va_end(args);
    return length;
}

size_t gp_str_print_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    // Avoid many small allocations by estimating a sufficient buffer size. This
    // estimation is currently completely arbitrary.
    gp_str_reserve(out, arg_count * 10);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        gp_str_reserve(out, gp_str_length(*out) + gp_str_print_object_size(objs[i], args));
        gp_str_header(*out)->length += gp_bytes_print_objects(
            (size_t)-1,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    return gp_str_header(*out)->length;
}

size_t gp_str_n_print_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        gp_str_header(*out)->length += gp_bytes_print_objects(
            n >= gp_str_length(*out) ? n - gp_str_length(*out) : 0,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    const size_t out_length = gp_str_length(*out);
    if (out_length > n)
        gp_str_header(*out)->length = n;
    return out_length;
}

size_t gp_str_println_internal(
    GPString* out,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    // Avoid many small allocations by estimating a sufficient buffer size. This
    // estimation is currently completely arbitrary.
    gp_str_reserve(out, arg_count * 10);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        gp_str_reserve(out,
            gp_str_length(*out) + strlen(" ") + gp_str_print_object_size(objs[i], args));

        gp_str_header(*out)->length += gp_bytes_print_objects(
            (size_t)-1,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);

        (*out)[gp_str_header(*out)->length++].c = ' ';
    }
    va_end(_args);
    va_end(args.list);

    (*out)[gp_str_length(*out) - 1].c = '\n';
    return gp_str_header(*out)->length;
}

size_t gp_str_n_println_internal(
    GPString* out,
    size_t n,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        gp_str_header(*out)->length += gp_bytes_print_objects(
            n >= gp_str_length(*out) ? n - gp_str_length(*out) : 0,
            *out + gp_str_length(*out),
            &args,
            &i,
            objs[i]);

        if (n > gp_str_length(*out))
            (*out)[gp_str_header(*out)->length++].c = ' ';
    }
    va_end(_args);
    va_end(args.list);

    if (n > (gp_str_length(*out) - !!gp_str_length(*out))) // overwrite last space
        (*out)[gp_str_length(*out) - 1].c = '\n';

    const size_t out_length = gp_str_length(*out);
    if (out_length > n)
        gp_str_header(*out)->length = n;
    return out_length;
}

void gp_str_trim(
    GPString* str,
    const char*restrict optional_char_set,
    int flags)
{
    gp_str_header(*str)->length = gp_bytes_trim(
        *str, gp_str_length(*str), NULL, optional_char_set, flags);
}

static void gp_str_to_something(
    GPString* str,
    wint_t(*towsomething)(wint_t))
{
    // Worst case: all single-byte characters map to two-byte characters e.g.
    // I to ı in Turkish locale. TODO measure wcsrtombs() performance to see if
    // it's worth to call it twice, first to calculate length, second to do the
    // actual conversion.
    gp_str_reserve(str, gp_str_length(*str) * 2);

    size_t length = gp_str_length(*str);
    size_t buf_cap  = 1 << 10;
    wchar_t stack_buf[1 << 10];
    wchar_t* buf = stack_buf;
    if (length + 1 >= buf_cap) {
        buf_cap = length + 1;
        buf = gp_mem_alloc(gp_str_allocator(*str), buf_cap * sizeof(wchar_t));
    }
    const char* src = (char*)*str;
    size_t buf_length = mbsrtowcs(buf,
        &src, buf_cap, &(mbstate_t){0});
    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    length = wcsrtombs((char*)*str,
        (const wchar_t**)&buf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        gp_mem_dealloc(gp_str_allocator(*str), buf);
    gp_str_header(*str)->length = length;
}

void gp_str_to_upper(
    GPString* str)
{
    gp_str_to_something(str, towupper);
}

void gp_str_to_lower(
    GPString* str)
{
    gp_str_to_something(str, towlower);
}

void gp_str_to_valid(
    GPString* str,
    const char* replacement)
{
          size_t length = gp_str_length(*str);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_bytes_find_invalid(*str, start, length)) != GP_NOT_FOUND)
    {
        const size_t end = gp_bytes_find_valid(*str, start, length);
        gp_str_reserve(str,
            gp_str_length(*str) + replacement_length - (end - start));

        length = gp_bytes_replace_range(
            *str,
            length,
            start,
            end,
            replacement,
            replacement_length);

        start += replacement_length;
    }
    gp_str_header(*str)->length = length;
}

int gp_str_case_compare(
    const GPString s1,
    const GPString s2)
{
    const GPAllocator* alc = gp_str_allocator(s1) != NULL ?
        gp_str_allocator(s1) : gp_str_allocator(s2);
    return gp_bytes_case_compare_alc(
        s1, gp_str_length(s1), s2, gp_str_length(s2), alc);
}

bool gp_str_from_path(
    GPString*   str,
    const char* file_path)
{
    FILE* f = fopen(file_path, "r");
    if (f == NULL)
        return false;

    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    gp_str_reserve(str, file_size);

    rewind(f);
    fread(*str, 1, file_size, f);
    gp_str_header(*str)->length = file_size;

    fclose(f);
    return true;
}
