// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <gpc/array.h>
#include <gpc/unicode.h>
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <locale.h>
#include <printf/printf.h>
#include <sys/types.h>
#include <sys/stat.h>

GPString gp_str_new(
    GPAllocator*const allocator,
    size_t capacity,
    const char*const init)
{
    const size_t init_length = strlen(init);
    capacity = gp_max(init_length, capacity);
    GPStringHeader* me = gp_mem_alloc(allocator, sizeof*me + capacity + sizeof"");
    *me = (GPStringHeader) {
        .length     = init_length,
        .capacity   = capacity,
        .allocator  = allocator,
        .allocation = me
    };
    return memcpy(me + 1, init, init_length);
}

static GPStringHeader* gp_str_header(const GPString str)
{
    return (GPStringHeader*)str - 1;
}

size_t gp_str_find_first(
    GPString    haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start)
{
    return gp_bytes_find_first(haystack, gp_str_length(haystack), needle, needle_size, start);
}

size_t gp_str_find_last(
    GPString    haystack,
    const void* needle,
    size_t      needle_length)
{
    return gp_bytes_find_last(haystack, gp_str_length(haystack), needle, needle_length);
}

size_t gp_str_find_first_of(
    const GPString   haystack,
    const char*const char_set,
    const size_t     start)
{
    for (size_t cplen, i = start; i < gp_str_length(haystack); i += cplen) {
        cplen = gp_utf8_codepoint_length(haystack, i);
        if (strstr(char_set, memcpy((char[8]){""}, haystack + i, cplen)) != NULL)
            return i;
    }
    return GP_NOT_FOUND;
}

size_t gp_str_find_first_not_of(
    const GPString   haystack,
    const char*const char_set,
    const size_t     start)
{
    for (size_t cplen, i = start; i < gp_str_length(haystack); i += cplen) {
        cplen = gp_utf8_codepoint_length(haystack, i);
        if (strstr(char_set, memcpy((char[8]){""}, haystack + i, cplen)) == NULL)
            return i;
    }
    return GP_NOT_FOUND;
}

size_t gp_str_count(
    GPString haystack,
    const void* needle,
    size_t      needle_size)
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

static uint32_t gp_u32_simple_fold(uint32_t r);

bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size)
{
    const size_t s1_length = gp_bytes_codepoint_count(s1, gp_str_length(s1));
    const size_t s2_length = gp_bytes_codepoint_count(s2, s2_size);
    if (s1_length != s2_length)
        return false;

    for (size_t i = 0; i < s1_length; i++)
    {
        uint32_t codepoint1;
        uint32_t codepoint2;
        const size_t s1_codepoint_size = gp_utf8_encode(&codepoint1, s1, 0);
        const size_t s2_codepoint_size = gp_utf8_encode(&codepoint2, s2, 0);
        s1 += s1_codepoint_size;
        s2 = (uint8_t*)s2 + s2_codepoint_size;

        if (codepoint1 == codepoint2)
            continue;
        if (codepoint2 < codepoint1) { // simplify the following
            uint32_t swap = codepoint1;
            codepoint1 = codepoint2;
            codepoint2 = swap;
        }
        if (codepoint2 < 0x80) {
            if ('A' <= codepoint1 && codepoint1 <= 'Z' &&
                codepoint2 == codepoint1 + 'a' - 'A')
                continue;
            return false;
        }
        uint32_t cp = gp_u32_simple_fold(codepoint1);
        while (cp != codepoint1 && cp < codepoint2)
            cp = gp_u32_simple_fold(cp);
        if (cp == codepoint2)
            continue;

        return false;
    }
    return true;
}

size_t gp_str_codepoint_count(
    GPString str)
{
    return gp_bytes_codepoint_count(str, gp_str_length(str));
}

bool gp_str_is_valid(
    GPString str,
    size_t* invalid_index)
{
    return gp_bytes_is_valid_utf8(str, gp_str_length(str), invalid_index);
}

const char* gp_cstr(GPString str)
{
    str[gp_str_length(str)].c = '\0';
    return (const char*)str;
}

void gp_str_clear(GPString* str)
{
    ((GPStringHeader*)*str - 1)->length = 0;
}

void gp_str_reserve(
    GPString* pstr,
    size_t capacity)
{
    GPString str = gp_arr_reserve(sizeof**pstr, *pstr, capacity + sizeof"");
    if (str != *pstr) // allocation happened
        gp_str_header(str)->capacity -= sizeof"";
    *pstr = str;
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
        memcpy(*dest, (uint8_t*)src + start, end - start);
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
    memcpy(*dest + gp_str_length(*dest), src, src_length);
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
    if ((start = gp_str_find_first(*haystack, needle, needle_length, start)) == GP_NOT_FOUND)
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
    while ((start = gp_str_find_first(*haystack, needle, needle_length, start)) != GP_NOT_FOUND)
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

    gp_str_reserve(out, n);
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

    gp_str_reserve(out, n);
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
    if (gp_str_length(*str) == 0)
        return;

    const bool ascii = flags & 0x01;
    if (ascii) {
        gp_str_header(*str)->length = gp_bytes_trim(
            *str, gp_str_length(*str), NULL, optional_char_set, flags);
        return;
    }
    // else utf8

    size_t      length   = gp_str_length(*str);
    const bool  left     = flags & 0x04;
    const bool  right    = flags & 0x02;
    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_WHITESPACE;

    if (left)
    {
        size_t prefix_length = 0;
        while (true)
        {
            char codepoint[8] = "";
            size_t size = gp_utf8_codepoint_length(*str, prefix_length);
            memcpy(codepoint, *str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
            if (prefix_length >= gp_str_length(*str)) {
                gp_str_header(*str)->length = 0;
                return;
            }
        }
        length -= prefix_length;

        memmove(*str, *str + prefix_length, length);
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_utf8_codepoint_length(*str, i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    gp_str_header(*str)->length = length;
}

GPArray(uint32_t) gp_utf8_to_utf32_new(GPAllocator* allocator, const GPString u8)
{
    GPArray(uint32_t) u32 = gp_arr_new(allocator, sizeof u32[0], gp_str_length(u8));
    for (size_t i = 0, codepoint_length; i < gp_str_length(u8); i += codepoint_length)
    {
        uint32_t encoding;
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        u32[((GPArrayHeader*)u32 - 1)->length++] = encoding;
    }
    return u32;
}

uint32_t gp_u32_to_upper(uint32_t);
uint32_t gp_u32_to_lower(uint32_t);
uint32_t gp_u32_to_title(uint32_t);

void gp_str_to_upper(GPString* str)
{
    GPArena* scratch = gp_scratch_arena();
    GPArray(uint32_t) u32 = gp_utf8_to_utf32_new((GPAllocator*)scratch, *str);
    for (size_t i = 0; i < gp_arr_length(u32); i++)
        u32[i] = gp_u32_to_upper(u32[i]);
    gp_utf32_to_utf8(str, u32, gp_arr_length(u32));
    gp_arena_rewind(scratch, gp_arr_allocation(u32));
}

void gp_str_to_lower(GPString* str)
{
    GPArena* scratch = gp_scratch_arena();
    GPArray(uint32_t) u32 = gp_utf8_to_utf32_new((GPAllocator*)scratch, *str);
    for (size_t i = 0; i < gp_arr_length(u32); i++)
        u32[i] = gp_u32_to_lower(u32[i]);
    gp_utf32_to_utf8(str, u32, gp_arr_length(u32));
    gp_arena_rewind(scratch, gp_arr_allocation(u32));
}

void gp_str_to_title(GPString* str)
{
    GPArena* scratch = gp_scratch_arena();
    GPArray(uint32_t) u32 = gp_utf8_to_utf32_new((GPAllocator*)scratch, *str);
    for (size_t i = 0; i < gp_arr_length(u32); i++)
        u32[i] = gp_u32_to_title(u32[i]);
    gp_utf32_to_utf8(str, u32, gp_arr_length(u32));
    gp_arena_rewind(scratch, gp_arr_allocation(u32));
}

static size_t gp_str_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_utf8_codepoint_length((GPString)haystack, i);
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

static size_t gp_str_find_valid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        size_t cp_length = gp_utf8_codepoint_length((GPString)haystack, i);
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

void gp_str_to_valid(
    GPString* str,
    const char* replacement)
{
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    if (replacement_length == 0) while ((start = gp_str_find_invalid(*str, start, gp_str_length(*str))) != GP_NOT_FOUND)
    {
        const size_t end = gp_str_find_valid(*str, start, gp_str_length(*str));
        memmove(*str + start, *str + end, gp_str_length(*str) - end);
        gp_str_header(*str)->length -= end - start;
    }
    else if (replacement_length == 1) while ((start = gp_str_find_invalid(*str, start, gp_str_length(*str))) != GP_NOT_FOUND)
    {
        const size_t end = gp_str_find_valid(*str, start, gp_str_length(*str));
        memset(*str + start, replacement[0], end - start);
        start = end;
    }
    else while ((start = gp_str_find_invalid(*str, start, gp_str_length(*str))) != GP_NOT_FOUND)
    {
        const size_t end = gp_str_find_valid(*str, start, gp_str_length(*str));

        gp_str_reserve(str, gp_str_length(*str) + (end - start) * (replacement_length - 1));
        memmove(*str + start + (end - start) * replacement_length, *str + end, gp_str_length(*str) - end);
        gp_bytes_repeat(*str + start, end - start, replacement, replacement_length);
        gp_str_header(*str)->length += (end - start) * (replacement_length - 1);
        start += (end - start) * replacement_length;
    }
}

int gp_str_file(
    GPString*   str,
    const char* file_path,
    const char* mode)
{
    switch (mode[0])
    {
        case 'r':
        {
            #if _WIN32
            struct __stat64 s;
            if (_stat64(file_path, &s) != 0)
            #elif _GNU_SOURCE
            struct stat64 s;
            if (stat64(file_path, &s) != 0)
            #else
            struct stat s;
            if (stat(file_path, &s) != 0)
            #endif
                return -1;

            if ((uint64_t)s.st_size > SIZE_MAX)
                return 1;

            FILE* f = fopen(file_path, "r");
            if (f == NULL)
                return -1;

            gp_str_reserve(str, s.st_size);
            if (fread(*str, sizeof**str, s.st_size, f) != (size_t)s.st_size) {
                fclose(f);
                return -1;
            }
            gp_str_header(*str)->length = s.st_size;

            fclose(f);
        } break;

        default:
        {
            size_t len = 0;
            char mode_buf[4] = { mode[len++] };
            if ( ! strchr(mode, 'x'))
                mode_buf[len++] = 'b';
            if (strchr(mode, '+'))
                mode_buf[len++] = '+';

            FILE* f = fopen(file_path, mode_buf);
            if (f == NULL)
                return -1;
            if (fwrite(*str, sizeof**str, gp_str_length(*str), f) != gp_str_length(*str))
                return fclose(f), -1;
            fclose(f);
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
// Case folding from Go source code
// Copyright (c) 2009 The Go Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

static const uint16_t gp_ascii_fold[] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
    0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
    0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
    0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x212A, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x017F, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F,
};

typedef struct gp_fold_pair
{
    uint16_t from;
    uint16_t to;
} GPFoldPair;

static GPFoldPair gp_case_orbit[] = {
    {0x004B, 0x006B}, {0x0053, 0x0073}, {0x006B, 0x212A}, {0x0073, 0x017F},
    {0x00B5, 0x039C}, {0x00C5, 0x00E5}, {0x00DF, 0x1E9E}, {0x00E5, 0x212B},
    {0x0130, 0x0130}, {0x0131, 0x0131}, {0x017F, 0x0053}, {0x01C4, 0x01C5},
    {0x01C5, 0x01C6}, {0x01C6, 0x01C4}, {0x01C7, 0x01C8}, {0x01C8, 0x01C9},
    {0x01C9, 0x01C7}, {0x01CA, 0x01CB}, {0x01CB, 0x01CC}, {0x01CC, 0x01CA},
    {0x01F1, 0x01F2}, {0x01F2, 0x01F3}, {0x01F3, 0x01F1}, {0x0345, 0x0399},
    {0x0392, 0x03B2}, {0x0395, 0x03B5}, {0x0398, 0x03B8}, {0x0399, 0x03B9},
    {0x039A, 0x03BA}, {0x039C, 0x03BC}, {0x03A0, 0x03C0}, {0x03A1, 0x03C1},
    {0x03A3, 0x03C2}, {0x03A6, 0x03C6}, {0x03A9, 0x03C9}, {0x03B2, 0x03D0},
    {0x03B5, 0x03F5}, {0x03B8, 0x03D1}, {0x03B9, 0x1FBE}, {0x03BA, 0x03F0},
    {0x03BC, 0x00B5}, {0x03C0, 0x03D6}, {0x03C1, 0x03F1}, {0x03C2, 0x03C3},
    {0x03C3, 0x03A3}, {0x03C6, 0x03D5}, {0x03C9, 0x2126}, {0x03D0, 0x0392},
    {0x03D1, 0x03F4}, {0x03D5, 0x03A6}, {0x03D6, 0x03A0}, {0x03F0, 0x039A},
    {0x03F1, 0x03A1}, {0x03F4, 0x0398}, {0x03F5, 0x0395}, {0x0412, 0x0432},
    {0x0414, 0x0434}, {0x041E, 0x043E}, {0x0421, 0x0441}, {0x0422, 0x0442},
    {0x042A, 0x044A}, {0x0432, 0x1C80}, {0x0434, 0x1C81}, {0x043E, 0x1C82},
    {0x0441, 0x1C83}, {0x0442, 0x1C84}, {0x044A, 0x1C86}, {0x0462, 0x0463},
    {0x0463, 0x1C87}, {0x1C80, 0x0412}, {0x1C81, 0x0414}, {0x1C82, 0x041E},
    {0x1C83, 0x0421}, {0x1C84, 0x1C85}, {0x1C85, 0x0422}, {0x1C86, 0x042A},
    {0x1C87, 0x0462}, {0x1C88, 0xA64A}, {0x1E60, 0x1E61}, {0x1E61, 0x1E9B},
    {0x1E9B, 0x1E60}, {0x1E9E, 0x00DF}, {0x1FBE, 0x0345}, {0x2126, 0x03A9},
    {0x212A, 0x004B}, {0x212B, 0x00C5}, {0xA64A, 0xA64B}, {0xA64B, 0x1C88},
};

static uint32_t gp_u32_simple_fold(uint32_t r)
{
	if (r < sizeof gp_ascii_fold / sizeof*gp_ascii_fold) {
		return gp_ascii_fold[r];
	}

	// Consult caseOrbit table for special cases.
	uint32_t lo = 0;
	uint32_t hi = sizeof gp_case_orbit / sizeof*gp_case_orbit;
	for (; lo < hi;) {
		uint32_t m = (lo+hi) >> 1;
		if (gp_case_orbit[m].from < r) {
			lo = m + 1;
		} else {
			hi = m;
		}
	}
	if (lo < sizeof gp_case_orbit / sizeof*gp_case_orbit && gp_case_orbit[lo].from == r) {
		return gp_case_orbit[lo].to;
	}

	// No folding specified. This is a one- or two-element
	// equivalence class containing rune and ToLower(rune)
	// and ToUpper(rune) if they are different from rune.
        uint32_t l = gp_u32_to_lower(r);
	if (l != r) {
		return l;
	}
	return gp_u32_to_upper(r);
}
