// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <gpc/array.h>
#include <gpc/hashmap.h> // integer endianness
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <printf/printf.h>
#include "pfstring.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "thread.h"

#if 0 // unused for now, maybe later
#if _WIN32
typedef _locale_t GPLocale;
#define towlower_l(...) _towlower_l(__VA_ARGS__)
#define towupper_l(...) _towupper_l(__VA_ARGS__)
#else
typedef locale_t GPLocale;
#endif

static GPLocale gp_locale_new(int category, const char*const locale)
{
    #if _WIN32
    return _create_locale(category, locale);
    #else
    return newlocale(category, locale, (GPLocale)0);
    #endif
}
static void gp_locale_delete(GPLocale locale)
{
    #if _WIN32
    _free_locale(locale);
    #else
    freelocale(locale);
    #endif
}
static GPLocale gp_default_locale;
static GPThreadOnce gp_locale_once = GP_THREAD_ONCE_INIT;

static void gp_init_default_locale(void)
{
    gp_default_locale = gp_locale_new(LC_COLLATE, "C.UTF-8");
}
#endif

extern inline void gp_str_delete(GPString);
extern inline void gp_str_ptr_delete(GPString*);

GPString gp_str_new(
    const GPAllocator*const allocator,
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
        .allocation = me };
    return memcpy(me + 1, init, init_length);
}

static GPStringHeader* gp_str_header(const GPString str)
{
    return (GPStringHeader*)str - 1;
}

size_t             gp_str_length    (GPString s) { return gp_str_header(s)->length;    }
size_t             gp_str_capacity  (GPString s) { return gp_str_header(s)->capacity;  }
void*              gp_str_allocation(GPString s) { return gp_str_header(s)->allocation;}
const GPAllocator* gp_str_allocator (GPString s) { return gp_str_header(s)->allocator; }

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
        cplen = gp_str_codepoint_length(haystack, i);
        if (strstr(char_set, memcpy((char[8]){}, haystack + i, cplen)) != NULL)
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
        cplen = gp_str_codepoint_length(haystack, i);
        if (strstr(char_set, memcpy((char[8]){}, haystack + i, cplen)) == NULL)
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

size_t gp_utf8_encode(uint32_t* encoding, const void*const u8, const size_t i);
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

size_t gp_str_codepoint_length(
    GPString _str, const size_t i)
{
    const uint8_t* str = (uint8_t*)_str;
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[str[i] >> 3];
}

bool gp_str_codepoint_classify(
    GPString str,
    const size_t i,
    int (*const classifier)(wint_t c))
{
    const size_t codepoint_length = gp_str_codepoint_length(str, i);
    if (codepoint_length == 0)
        return false;
    wchar_t wc;
    mbrtowc(&wc, (char*)str + i, codepoint_length, &(mbstate_t){0});
    return classifier(wc);
}

const char* gp_cstr(GPString str)
{
    str[gp_str_length(str)].c = '\0';
    return (const char*)str;
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
            size_t size = gp_str_codepoint_length(*str, prefix_length);
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
        while ((size = gp_str_codepoint_length(*str, i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    gp_str_header(*str)->length = length;
}

size_t gp_utf8_encode(uint32_t* encoding, const void*const _u8, const size_t i)
{
    const GPString u8 = (GPString)_u8;
    *encoding = 0;
    const size_t codepoint_length = gp_str_codepoint_length(u8, i);
    for (size_t j = 0; j < codepoint_length; j++)
        *encoding = (*encoding << 8) | u8[i + j].c;

    if (*encoding > 0x7F)
    {
        uint32_t mask = (*encoding <= 0x00EFBFBF) ? 0x000F0000 : 0x003F0000 ;
        *encoding = ((*encoding & 0x07000000) >> 6) |
            ((*encoding & mask )             >> 4) |
            ((*encoding & 0x00003F00)        >> 2) |
            (*encoding & 0x0000003F);
    }
    return codepoint_length;
}

GPArray(uint32_t) gp_utf8_to_utf32(const GPAllocator* allocator, const GPString u8)
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

void gp_utf32_to_utf8(GPString* u8, const GPArray(uint32_t) u32)
{
    size_t required_capacity = 0;
    for (size_t i = 0; i < gp_arr_length(u32); ++i)
    {
        if (u32[i] < 0x0000800) {
            required_capacity += 2;
        } else if (u32[i] < 0x0010000) {
            required_capacity += 3;
        } else {
            required_capacity += 4;
        }
    }
    gp_str_reserve(u8, required_capacity);

    ((GPStringHeader*)*u8 - 1)->length = 0;
    for (size_t i = 0; i < gp_arr_length(u32); ++i)
    {
        if (u32[i] > 0x7F)
        {
            if (u32[i] < 0x800) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x000FC0) >> 6) | 0xC0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x00003F) >> 0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 2;
            } else if (u32[i] < 0x10000) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x03F000) >> 12) | 0xE0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u32[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 3;
            } else {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x1C0000) >> 18) | 0xF0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x03F000) >> 12) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u32[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 3].c = ((u32[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 4;
            }
        }
        else
        {
            (*u8)[((GPStringHeader*)*u8 - 1)->length++].c = (uint8_t)u32[i];
        }
    }
}

static uint32_t gp_u32_to_upper(uint32_t);
static uint32_t gp_u32_to_lower(uint32_t);

void gp_str_to_upper(GPString* str)
{
    GPArena* scratch = gp_scratch_arena();
    GPArray(uint32_t) u32 = gp_utf8_to_utf32((GPAllocator*)scratch, *str);
    for (size_t i = 0; i < gp_arr_length(u32); i++)
        u32[i] = gp_u32_to_upper(u32[i]);
    gp_utf32_to_utf8(str, u32);
    gp_arena_rewind(scratch, gp_arr_allocation(u32));
}

void gp_str_to_lower(GPString* str)
{
    GPArena* scratch = gp_scratch_arena();
    GPArray(uint32_t) u32 = gp_utf8_to_utf32((GPAllocator*)scratch, *str);
    for (size_t i = 0; i < gp_arr_length(u32); i++)
        u32[i] = gp_u32_to_lower(u32[i]);
    gp_utf32_to_utf8(str, u32);
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
        size_t cp_length = gp_str_codepoint_length((GPString)haystack, i);
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
        size_t cp_length = gp_str_codepoint_length((GPString)haystack, i);
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
          size_t length = gp_str_length(*str);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_str_find_invalid(*str, start, length)) != GP_NOT_FOUND)
    {
        const size_t end = gp_str_find_valid(*str, start, length);
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

int gp_str_case_compare( // TODO figure the API out and make this public
    const GPString _s1,
    const void*const _s2,
    const size_t s2_length)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;

    wchar_t stack_buf1[1 << 10];
    wchar_t stack_buf2[sizeof stack_buf1 / sizeof*stack_buf1];
    size_t  buf1_cap = sizeof stack_buf1 / sizeof*stack_buf1;
    size_t  buf2_cap = buf1_cap;
    wchar_t*buf1     = stack_buf1;
    wchar_t*buf2     = stack_buf2;

    GPArena arena;
    const GPAllocator* scope = NULL;
    const size_t max_length = gp_max(gp_str_length(_s1), s2_length);
    if (max_length + 1 >= buf1_cap)
    {
        arena = gp_arena_new(2 * max_length * sizeof*buf1 +/*internals*/64);
        scope = (const GPAllocator*)&arena;
    }
    if (gp_str_length(_s1) + 1 >= buf1_cap) {
        buf1_cap = gp_str_length(_s1) + 1;
        buf1 = gp_mem_alloc(scope, buf1_cap * sizeof(wchar_t));
    }
    if (s2_length + 1 >= buf2_cap) {
        buf2_cap = s2_length + 1;
        buf2 = gp_mem_alloc(scope, buf2_cap * sizeof(wchar_t));
    }
    mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0});
    mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0});

    #if 1
    int result = wcscoll(buf1, buf2);
    #elif _WIN32
    int result = _wcsicoll_l(buf1, buf2, gp_default_locale);
    #else
    int result = wcscasecmp_l(buf1, buf2, gp_default_locale);
    #endif
    gp_arena_delete((GPArena*)scope);
    return result;
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
                return -1;
            fclose(f);
        }
    }
    return 0;
}

// ----------------------------------------------------------------------------
// towupper() and towlower() from
// https://chromium.googlesource.com/native_client/nacl-newlib/+/refs/heads/main/newlib/libc/ctype

/* Copyright (c) 2002 Red Hat Incorporated.
   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
     Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
     Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
     The name of Red Hat Incorporated may not be used to endorse
     or promote products derived from this software without specific
     prior written permission.
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL RED HAT INCORPORATED BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

static uint32_t gp_u32_to_upper(uint32_t c)
{
  /* Based on and tested against Unicode 5.2 */
  /* Expression used to filter out the characters for the below code:
     awk -F\; '{ if ( $13 != "" ) print $1; }' UnicodeData.txt
  */
  if (c < 0x100)
    {
      if (c == 0x00b5)
	return 0x039c;

      if ((c >= 0x00e0 && c <= 0x00fe && c != 0x00f7) ||
	  (c >= 0x0061 && c <= 0x007a))
	return (c - 0x20);

      if (c == 0xff)
	return 0x0178;

      return c;
    }
  else if (c < 0x300)
    {
      if ((c >= 0x0101 && c <= 0x012f) ||
	  (c >= 0x0133 && c <= 0x0137) ||
	  (c >= 0x014b && c <= 0x0177) ||
	  (c >= 0x01df && c <= 0x01ef) ||
	  (c >= 0x01f9 && c <= 0x021f) ||
	  (c >= 0x0223 && c <= 0x0233) ||
	  (c >= 0x0247 && c <= 0x024f))
	{
	  if (c & 0x01)
	    return (c - 1);
	  return c;
	}
      if ((c >= 0x013a && c <= 0x0148) ||
	  (c >= 0x01ce && c <= 0x01dc) ||
	  c == 0x023c || c == 0x0242)
	{
	  if (!(c & 0x01))
	    return (c - 1);
	  return c;
	}

      if (c == 0x0131)
	return 0x0049;

      if (c == 0x017a || c == 0x017c || c == 0x017e)
	return (c - 1);

      if (c >= 0x017f && c <= 0x0292)
	{
	  wint_t k;
	  switch (c)
	    {
	    case 0x017f:
	      k = 0x0053;
	      break;
	    case 0x0180:
	      k = 0x0243;
	      break;
	    case 0x0183:
	      k = 0x0182;
	      break;
	    case 0x0185:
	      k = 0x0184;
	      break;
	    case 0x0188:
	      k = 0x0187;
	      break;
	    case 0x018c:
	      k = 0x018b;
	      break;
	    case 0x0192:
	      k = 0x0191;
	      break;
	    case 0x0195:
	      k = 0x01f6;
	      break;
	    case 0x0199:
	      k = 0x0198;
	      break;
	    case 0x019a:
	      k = 0x023d;
	      break;
	    case 0x019e:
	      k = 0x0220;
	      break;
	    case 0x01a1:
	    case 0x01a3:
	    case 0x01a5:
	    case 0x01a8:
	    case 0x01ad:
	    case 0x01b0:
	    case 0x01b4:
	    case 0x01b6:
	    case 0x01b9:
	    case 0x01bd:
	    case 0x01c5:
	    case 0x01c8:
	    case 0x01cb:
	    case 0x01f2:
	    case 0x01f5:
	      k = c - 1;
	      break;
	    case 0x01bf:
	      k = 0x01f7;
	      break;
	    case 0x01c6:
	    case 0x01c9:
	    case 0x01cc:
	      k = c - 2;
	      break;
	    case 0x01dd:
	      k = 0x018e;
	      break;
	    case 0x01f3:
	      k = 0x01f1;
	      break;
	    case 0x023f:
	      k = 0x2c7e;
	      break;
	    case 0x0240:
	      k = 0x2c7f;
	      break;
	    case 0x0250:
	      k = 0x2c6f;
	      break;
	    case 0x0251:
	      k = 0x2c6d;
	      break;
	    case 0x0252:
	      k = 0x2c70;
	      break;
	    case 0x0253:
	      k = 0x0181;
	      break;
	    case 0x0254:
	      k = 0x0186;
	      break;
	    case 0x0256:
	      k = 0x0189;
	      break;
	    case 0x0257:
	      k = 0x018a;
	      break;
	    case 0x0259:
	      k = 0x018f;
	      break;
	    case 0x025b:
	      k = 0x0190;
	      break;
	    case 0x0260:
	      k = 0x0193;
	      break;
	    case 0x0263:
	      k = 0x0194;
	      break;
	    case 0x0268:
	      k = 0x0197;
	      break;
	    case 0x0269:
	      k = 0x0196;
	      break;
	    case 0x026b:
	      k = 0x2c62;
	      break;
	    case 0x026f:
	      k = 0x019c;
	      break;
	    case 0x0271:
	      k = 0x2c6e;
	      break;
	    case 0x0272:
	      k = 0x019d;
	      break;
	    case 0x0275:
	      k = 0x019f;
	      break;
	    case 0x027d:
	      k = 0x2c64;
	      break;
	    case 0x0280:
	      k = 0x01a6;
	      break;
	    case 0x0283:
	      k = 0x01a9;
	      break;
	    case 0x0288:
	      k = 0x01ae;
	      break;
	    case 0x0289:
	      k = 0x0244;
	      break;
	    case 0x028a:
	      k = 0x01b1;
	      break;
	    case 0x028b:
	      k = 0x01b2;
	      break;
	    case 0x028c:
	      k = 0x0245;
	      break;
	    case 0x0292:
	      k = 0x01b7;
	      break;
	    default:
	      k = 0;
	    }
	  if (k != 0)
	    return k;
	}
    }
  else if (c < 0x0400)
    {
      wint_t k;
      if (c >= 0x03ad && c <= 0x03af)
      	return (c - 0x25);
      if (c >= 0x03b1 && c <= 0x03cb && c != 0x03c2)
	return (c - 0x20);

      if (c >= 0x03d9 && c <= 0x03ef && (c & 1))
	return (c - 1);
      switch (c)
	{
	case 0x0345:
	  k = 0x0399;
	  break;
	case 0x0371:
	case 0x0373:
	case 0x0377:
	case 0x03f8:
	case 0x03fb:
	  k = c - 1;
	  break;
	case 0x037b:
	case 0x037c:
	case 0x037d:
	  k = c + 0x82;
	  break;
	case 0x03ac:
	  k = 0x0386;
	  break;
	case 0x03c2:
	  k = 0x03a3;
	  break;
	case 0x03cc:
	  k = 0x038c;
	  break;
	case 0x03cd:
	case 0x03ce:
	  k = c - 0x3f;
	  break;
	case 0x03d0:
	  k = 0x0392;
	  break;
	case 0x03d1:
	  k = 0x0398;
	  break;
	case 0x03d5:
	  k = 0x03a6;
	  break;
	case 0x03d6:
	  k = 0x03a0;
	  break;
	case 0x03d7:
	  k = 0x03cf;
	  break;
	case 0x03f0:
	  k = 0x039a;
	  break;
	case 0x03f1:
	  k = 0x03a1;
	  break;
	case 0x03f2:
	  k = 0x03f9;
	  break;
	case 0x03f5:
	  k = 0x0395;
	  break;
	default:
	  k = 0;
	}
      if (k != 0)
	return k;
    }
  else if (c < 0x500)
    {
      if (c >= 0x0430 && c <= 0x044f)
	return (c - 0x20);

      if (c >= 0x0450 && c <= 0x045f)
	return (c - 0x50);

      if ((c >= 0x0461 && c <= 0x0481) ||
	  (c >= 0x048b && c <= 0x04bf) ||
	  (c >= 0x04d1 && c <= 0x04ff))
	{
	  if (c & 0x01)
	    return (c - 1);
	  return c;
	}

      if (c >= 0x04c2 && c <= 0x04ce)
	{
	  if (!(c & 0x01))
	    return (c - 1);
	  return c;
	}

      if (c == 0x04cf)
      	return 0x04c0;
      if (c >= 0x04f7 && c <= 0x04f9)
	return (c - 1);
    }
  else if (c < 0x0600)
    {
      if (c >= 0x0501 && c <= 0x0525 && (c & 1))
      	return c - 1;
      if (c >= 0x0561 && c <= 0x0586)
	return (c - 0x30);
    }
  else if (c < 0x1f00)
    {
      if (c == 0x1d79)
      	return 0xa77d;
      if (c == 0x1d7d)
      	return 0x2c63;
      if ((c >= 0x1e01 && c <= 0x1e95) ||
	  (c >= 0x1ea1 && c <= 0x1eff))
	{
	  if (c & 0x01)
	    return (c - 1);
	  return c;
	}

      if (c == 0x1e9b)
	return 0x1e60;
    }
  else if (c < 0x2000)
    {

      if ((c >= 0x1f00 && c <= 0x1f07) ||
	  (c >= 0x1f10 && c <= 0x1f15) ||
	  (c >= 0x1f20 && c <= 0x1f27) ||
	  (c >= 0x1f30 && c <= 0x1f37) ||
	  (c >= 0x1f40 && c <= 0x1f45) ||
	  (c >= 0x1f60 && c <= 0x1f67) ||
	  (c >= 0x1f80 && c <= 0x1f87) ||
	  (c >= 0x1f90 && c <= 0x1f97) ||
	  (c >= 0x1fa0 && c <= 0x1fa7))
	return (c + 0x08);
      if (c >= 0x1f51 && c <= 0x1f57 && (c & 0x01))
	return (c + 0x08);

      if (c >= 0x1f70 && c <= 0x1ff3)
	{
	  wint_t k;
	  switch (c)
	    {
	    case 0x1fb0:
	      k = 0x1fb8;
	      break;
	    case 0x1fb1:
	      k = 0x1fb9;
	      break;
	    case 0x1f70:
	      k = 0x1fba;
	      break;
	    case 0x1f71:
	      k = 0x1fbb;
	      break;
	    case 0x1fb3:
	      k = 0x1fbc;
	      break;
	    case 0x1fbe:
	      k = 0x0399;
	      break;
	    case 0x1f72:
	      k = 0x1fc8;
	      break;
	    case 0x1f73:
	      k = 0x1fc9;
	      break;
	    case 0x1f74:
	      k = 0x1fca;
	      break;
	    case 0x1f75:
	      k = 0x1fcb;
	      break;
	    case 0x1fc3:
	      k = 0x1fcc;
	      break;
	    case 0x1fd0:
	      k = 0x1fd8;
	      break;
	    case 0x1fd1:
	      k = 0x1fd9;
	      break;
	    case 0x1f76:
	      k = 0x1fda;
	      break;
	    case 0x1f77:
	      k = 0x1fdb;
	      break;
	    case 0x1fe0:
	      k = 0x1fe8;
	      break;
	    case 0x1fe1:
	      k = 0x1fe9;
	      break;
	    case 0x1f7a:
	      k = 0x1fea;
	      break;
	    case 0x1f7b:
	      k = 0x1feb;
	      break;
	    case 0x1fe5:
	      k = 0x1fec;
	      break;
	    case 0x1f78:
	      k = 0x1ff8;
	      break;
	    case 0x1f79:
	      k = 0x1ff9;
	      break;
	    case 0x1f7c:
	      k = 0x1ffa;
	      break;
	    case 0x1f7d:
	      k = 0x1ffb;
	      break;
	    case 0x1ff3:
	      k = 0x1ffc;
	      break;
	    default:
	      k = 0;
	    }
	  if (k != 0)
	    return k;
	}
    }
  else if (c < 0x3000)
    {
      if (c == 0x214e)
      	return 0x2132;
      if (c == 0x2184)
      	return 0x2183;
      if (c >= 0x2170 && c <= 0x217f)
	return (c - 0x10);

      if (c >= 0x24d0 && c <= 0x24e9)
	return (c - 0x1a);

      if (c >= 0x2c30 && c <= 0x2c5e)
	return (c - 0x30);
      if ((c >= 0x2c68 && c <= 0x2c6c && !(c & 1)) ||
	  (c >= 0x2c81 && c <= 0x2ce3 &&  (c & 1)) ||
	  c == 0x2c73 || c == 0x2c76 ||
	  c == 0x2cec || c == 0x2cee)
      	return (c - 1);
      if (c >= 0x2c81 && c <= 0x2ce3 && (c & 1))
	return (c - 1);
      if (c >= 0x2d00 && c <= 0x2d25)
      	return (c - 0x1c60);
      switch (c)
      	{
	case 0x2c61:
	  return 0x2c60;
	case 0x2c65:
	  return 0x023a;
	case 0x2c66:
	  return 0x023e;
	}
    }
  else if (c >= 0xa000 && c < 0xb000)
    {
      if (((c >= 0xa641 && c <= 0xa65f) ||
           (c >= 0xa663 && c <= 0xa66d) ||
           (c >= 0xa681 && c <= 0xa697) ||
           (c >= 0xa723 && c <= 0xa72f) ||
           (c >= 0xa733 && c <= 0xa76f) ||
           (c >= 0xa77f && c <= 0xa787)) &&
	  (c & 1))
	return (c - 1);

      if (c == 0xa77a || c == 0xa77c || c == 0xa78c)
	return (c - 1);
    }
  else
    {
      if (c >= 0xff41 && c <= 0xff5a)
	return (c - 0x20);

      if (c >= 0x10428 && c <= 0x1044f)
	return (c - 0x28);
    }
  return c;
}

static uint32_t gp_u32_to_lower(uint32_t c)
{
  /* Based on and tested against Unicode 5.2 */
  /* Expression used to filter out the characters for the below code:
     awk -F\; '{ if ( $14 != "" ) print $1; }' UnicodeData.txt
  */
  if (c < 0x100)
    {
      if ((c >= 0x0041 && c <= 0x005a) ||
	  (c >= 0x00c0 && c <= 0x00d6) ||
	  (c >= 0x00d8 && c <= 0x00de))
	return (c + 0x20);
      return c;
    }
  else if (c < 0x300)
    {
      if ((c >= 0x0100 && c <= 0x012e) ||
	  (c >= 0x0132 && c <= 0x0136) ||
	  (c >= 0x014a && c <= 0x0176) ||
	  (c >= 0x01de && c <= 0x01ee) ||
	  (c >= 0x01f8 && c <= 0x021e) ||
	  (c >= 0x0222 && c <= 0x0232))
	{
	  if (!(c & 0x01))
	    return (c + 1);
	  return c;
	}
      if (c == 0x0130)
	return 0x0069;
      if ((c >= 0x0139 && c <= 0x0147) ||
	  (c >= 0x01cd && c <= 0x01db))
	{
	  if (c & 0x01)
	    return (c + 1);
	  return c;
	}

      if (c >= 0x178 && c <= 0x01f7)
	{
	  wint_t k;
	  switch (c)
	    {
	    case 0x0178:
	      k = 0x00ff;
	      break;
	    case 0x0179:
	    case 0x017b:
	    case 0x017d:
	    case 0x0182:
	    case 0x0184:
	    case 0x0187:
	    case 0x018b:
	    case 0x0191:
	    case 0x0198:
	    case 0x01a0:
	    case 0x01a2:
	    case 0x01a4:
	    case 0x01a7:
	    case 0x01ac:
	    case 0x01af:
	    case 0x01b3:
	    case 0x01b5:
	    case 0x01b8:
	    case 0x01bc:
	    case 0x01c5:
	    case 0x01c8:
	    case 0x01cb:
	    case 0x01cd:
	    case 0x01cf:
	    case 0x01d1:
	    case 0x01d3:
	    case 0x01d5:
	    case 0x01d7:
	    case 0x01d9:
	    case 0x01db:
	    case 0x01f2:
	    case 0x01f4:
	      k = c + 1;
	      break;
	    case 0x0181:
	      k = 0x0253;
	      break;
	    case 0x0186:
	      k = 0x0254;
	      break;
	    case 0x0189:
	      k = 0x0256;
	      break;
	    case 0x018a:
	      k = 0x0257;
	      break;
	    case 0x018e:
	      k = 0x01dd;
	      break;
	    case 0x018f:
	      k = 0x0259;
	      break;
	    case 0x0190:
	      k = 0x025b;
	      break;
	    case 0x0193:
	      k = 0x0260;
	      break;
	    case 0x0194:
	      k = 0x0263;
	      break;
	    case 0x0196:
	      k = 0x0269;
	      break;
	    case 0x0197:
	      k = 0x0268;
	      break;
	    case 0x019c:
	      k = 0x026f;
	      break;
	    case 0x019d:
	      k = 0x0272;
	      break;
	    case 0x019f:
	      k = 0x0275;
	      break;
	    case 0x01a6:
	      k = 0x0280;
	      break;
	    case 0x01a9:
	      k = 0x0283;
	      break;
	    case 0x01ae:
	      k = 0x0288;
	      break;
	    case 0x01b1:
	      k = 0x028a;
	      break;
	    case 0x01b2:
	      k = 0x028b;
	      break;
	    case 0x01b7:
	      k = 0x0292;
	      break;
	    case 0x01c4:
	    case 0x01c7:
	    case 0x01ca:
	    case 0x01f1:
	      k = c + 2;
	      break;
	    case 0x01f6:
	      k = 0x0195;
	      break;
	    case 0x01f7:
	      k = 0x01bf;
	      break;
	    default:
	      k = 0;
	    }
	  if (k != 0)
	    return k;
	}
      else if (c == 0x0220)
      	return 0x019e;
      else if (c >= 0x023a && c <= 0x024e)
      	{
	  wint_t k;
	  switch (c)
	    {
	    case 0x023a:
	      k = 0x2c65;
	      break;
	    case 0x023b:
	    case 0x0241:
	    case 0x0246:
	    case 0x0248:
	    case 0x024a:
	    case 0x024c:
	    case 0x024e:
	      k = c + 1;
	      break;
	    case 0x023d:
	      k = 0x019a;
	      break;
	    case 0x023e:
	      k = 0x2c66;
	      break;
	    case 0x0243:
	      k = 0x0180;
	      break;
	    case 0x0244:
	      k = 0x0289;
	      break;
	    case 0x0245:
	      k = 0x028c;
	      break;
	    default:
	      k = 0;
	    }
	  if (k != 0)
	    return k;
	}
    }
  else if (c < 0x0400)
    {
      if (c == 0x0370 || c == 0x0372 || c == 0x0376)
      	return (c + 1);
      if (c >= 0x0391 && c <= 0x03ab && c != 0x03a2)
	return (c + 0x20);
      if (c >= 0x03d8 && c <= 0x03ee && !(c & 0x01))
	return (c + 1);
      if (c >= 0x0386 && c <= 0x03ff)
	{
	  wint_t k;
	  switch (c)
	    {
	    case 0x0386:
	      k = 0x03ac;
	      break;
	    case 0x0388:
	      k = 0x03ad;
	      break;
	    case 0x0389:
	      k = 0x03ae;
	      break;
	    case 0x038a:
	      k = 0x03af;
	      break;
	    case 0x038c:
	      k = 0x03cc;
	      break;
	    case 0x038e:
	      k = 0x03cd;
	      break;
	    case 0x038f:
	      k = 0x03ce;
	      break;
	    case 0x03cf:
	      k = 0x03d7;
	      break;
	    case 0x03f4:
	      k = 0x03b8;
	      break;
	    case 0x03f7:
	      k = 0x03f8;
	      break;
	    case 0x03f9:
	      k = 0x03f2;
	      break;
	    case 0x03fa:
	      k = 0x03fb;
	      break;
	    case 0x03fd:
	      k = 0x037b;
	      break;
	    case 0x03fe:
	      k = 0x037c;
	      break;
	    case 0x03ff:
	      k = 0x037d;
	      break;
	    default:
	      k = 0;
	    }
	  if (k != 0)
	    return k;
	}
    }
  else if (c < 0x500)
    {
      if (c >= 0x0400 && c <= 0x040f)
	return (c + 0x50);

      if (c >= 0x0410 && c <= 0x042f)
	return (c + 0x20);

      if ((c >= 0x0460 && c <= 0x0480) ||
	  (c >= 0x048a && c <= 0x04be) ||
	  (c >= 0x04d0 && c <= 0x04fe))
	{
	  if (!(c & 0x01))
	    return (c + 1);
	  return c;
	}

      if (c == 0x04c0)
	return 0x04cf;
      if (c >= 0x04c1 && c <= 0x04cd)
	{
	  if (c & 0x01)
	    return (c + 1);
	  return c;
	}
    }
  else if (c < 0x1f00)
    {
      if ((c >= 0x0500 && c <= 0x050e) ||
	  (c >= 0x0510 && c <= 0x0524) ||
	  (c >= 0x1e00 && c <= 0x1e94) ||
	  (c >= 0x1ea0 && c <= 0x1ef8))
	{
	  if (!(c & 0x01))
	    return (c + 1);
	  return c;
	}

      if (c >= 0x0531 && c <= 0x0556)
	return (c + 0x30);
      if (c >= 0x10a0 && c <= 0x10c5)
	return (c + 0x1c60);
      if (c == 0x1e9e)
	return 0x00df;
      if (c >= 0x1efa && c <= 0x1efe && !(c & 0x01))
	return (c + 1);
    }
  else if (c < 0x2000)
    {
      if ((c >= 0x1f08 && c <= 0x1f0f) ||
	  (c >= 0x1f18 && c <= 0x1f1d) ||
	  (c >= 0x1f28 && c <= 0x1f2f) ||
	  (c >= 0x1f38 && c <= 0x1f3f) ||
	  (c >= 0x1f48 && c <= 0x1f4d) ||
	  (c >= 0x1f68 && c <= 0x1f6f) ||
	  (c >= 0x1f88 && c <= 0x1f8f) ||
	  (c >= 0x1f98 && c <= 0x1f9f) ||
	  (c >= 0x1fa8 && c <= 0x1faf))
	return (c - 0x08);
      if (c >= 0x1f59 && c <= 0x1f5f)
	{
	  if (c & 0x01)
	    return (c - 0x08);
	  return c;
	}

      if (c >= 0x1fb8 && c <= 0x1ffc)
	{
	  wint_t k;
	  switch (c)
	    {
	    case 0x1fb8:
	    case 0x1fb9:
	    case 0x1fd8:
	    case 0x1fd9:
	    case 0x1fe8:
	    case 0x1fe9:
	      k = c - 0x08;
	      break;
	    case 0x1fba:
	    case 0x1fbb:
	      k = c - 0x4a;
	      break;
	    case 0x1fbc:
	      k = 0x1fb3;
	      break;
	    case 0x1fc8:
	    case 0x1fc9:
	    case 0x1fca:
	    case 0x1fcb:
	      k = c - 0x56;
	      break;
	    case 0x1fcc:
	      k = 0x1fc3;
	      break;
	    case 0x1fda:
	    case 0x1fdb:
	      k = c - 0x64;
	      break;
	    case 0x1fea:
	    case 0x1feb:
	      k = c - 0x70;
	      break;
	    case 0x1fec:
	      k = 0x1fe5;
	      break;
	    case 0x1ff8:
	    case 0x1ff9:
	      k = c - 0x80;
	      break;
	    case 0x1ffa:
	    case 0x1ffb:
	      k = c - 0x7e;
	      break;
	    case 0x1ffc:
	      k = 0x1ff3;
	      break;
	    default:
	      k = 0;
	    }
	  if (k != 0)
	    return k;
	}
    }
  else if (c < 0x2c00)
    {
      if (c >= 0x2160 && c <= 0x216f)
	return (c + 0x10);
      if (c >= 0x24b6 && c <= 0x24cf)
	return (c + 0x1a);

      switch (c)
      	{
	case 0x2126:
	  return 0x03c9;
	case 0x212a:
	  return 0x006b;
	case 0x212b:
	  return 0x00e5;
	case 0x2132:
	  return 0x214e;
	case 0x2183:
	  return 0x2184;
	}
    }
  else if (c < 0x2d00)
    {
      if (c >= 0x2c00 && c <= 0x2c2e)
	return (c + 0x30);
      if (c >= 0x2c80 && c <= 0x2ce2 && !(c & 0x01))
	return (c + 1);
      switch (c)
      	{
	case 0x2c60:
	  return 0x2c61;
	case 0x2c62:
	  return 0x026b;
	case 0x2c63:
	  return 0x1d7d;
	case 0x2c64:
	  return 0x027d;
	case 0x2c67:
	case 0x2c69:
	case 0x2c6b:
	case 0x2c72:
	case 0x2c75:
	case 0x2ceb:
	case 0x2ced:
	  return c + 1;
	case 0x2c6d:
	  return 0x0251;
	case 0x2c6e:
	  return 0x0271;
	case 0x2c6f:
	  return 0x0250;
	case 0x2c70:
	  return 0x0252;
	case 0x2c7e:
	  return 0x023f;
	case 0x2c7f:
	  return 0x0240;
	}
    }
  else if (c >= 0xa600 && c < 0xa800)
    {
      if ((c >= 0xa640 && c <= 0xa65e) ||
	  (c >= 0xa662 && c <= 0xa66c) ||
	  (c >= 0xa680 && c <= 0xa696) ||
	  (c >= 0xa722 && c <= 0xa72e) ||
	  (c >= 0xa732 && c <= 0xa76e) ||
	  (c >= 0xa77f && c <= 0xa786))
	{
	  if (!(c & 1))
	    return (c + 1);
	  return c;
	}
      switch (c)
      	{
	case 0xa779:
	case 0xa77b:
	case 0xa77e:
	case 0xa78b:
	  return (c + 1);
	case 0xa77d:
	  return 0x1d79;
	}
    }
  else
    {
      if (c >= 0xff21 && c <= 0xff3a)
	return (c + 0x20);

      if (c >= 0x10400 && c <= 0x10427)
	return (c + 0x28);
    }
  return c;
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
