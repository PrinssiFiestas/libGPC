// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

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
#include "pfstring.h"
#include <sys/types.h>
#include <sys/stat.h>

GPString gp_str_new(
    const GPAllocator* allocator,
    size_t capacity,
    const char* init)
{
    GPStringHeader* me = gp_mem_alloc(allocator, sizeof*me + capacity + sizeof"");
    *me = (GPStringHeader) {
        .capacity   = capacity,
        .allocator  = allocator,
        .allocation = me };
    return memcpy(me + 1, init, strlen(init));
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

static size_t gp_bytes_codepoint_count(
    const void* _str,
    const size_t n)
{
    size_t count = 0;
    const char* str = _str;
    static const size_t valid_leading_nibble[] = {
        1,1,1,1, 1,1,1,1, 0,0,0,0, 1,1,1,1
    };
    const size_t align_offset = (uintptr_t)str     % 8;
    const size_t remaining    = (n - align_offset) % 8;
    size_t i = 0;

    for (size_t len = gp_min(align_offset, n); i < len; i++)
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

static size_t gp_bytes_codepoint_length(
    const void* str)
{
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[*(uint8_t*)str >> 3];
}

bool gp_str_equal_case(
    GPString    s1,
    const void* s2,
    size_t      s2_size)
{
    size_t s1_length = gp_bytes_codepoint_count(s1, gp_str_length(s1));
    size_t s2_length = gp_bytes_codepoint_count(s2, s2_size);
    if (s1_length != s2_length)
        return false;

    mbstate_t state1 = {0};
    mbstate_t state2 = {0};
    wchar_t wc1;
    wchar_t wc2;
    for (size_t i = 0; i < s1_length; i++)
    {
        size_t wc1_length = mbrtowc(&wc1, (char*)s1, sizeof(wchar_t), &state1);
        size_t wc2_length = mbrtowc(&wc2, (char*)s2, sizeof(wchar_t), &state2);
        if (sizeof(wchar_t) < sizeof(uint32_t)/* Windows probably */&&
            (wc1_length == (size_t)-2) != (wc2_length == (size_t)-2))
        { // one fits to wchar_t and other doesn't so most likely different
            return false;
        }
        else if (sizeof(wchar_t) < sizeof(uint32_t) &&
                 wc1_length == (size_t)-2) // char wider than sizeof(wchar_t)
        {                                  // so just compare raw bytes
            size_t s1_codepoint_size = gp_bytes_codepoint_length(s1);
            size_t s2_codepoint_size = gp_bytes_codepoint_length(s2);
            if (s1_codepoint_size != s2_codepoint_size ||
                memcmp(s1, s2, s1_codepoint_size) != 0)
            {
                return false;
            }
            s1 += s1_codepoint_size;
            s2 += s2_codepoint_size;
        }
        else
        {
            wc1 = towlower(wc1);
            wc2 = towlower(wc2);
            if (wc1 != wc2)
                return false;

            s1 += wc1_length;
            s2 += wc2_length;
        }
    }
    return true;
}

size_t gp_str_codepoint_count(
    GPString str)
{
    return gp_bytes_codepoint_count(str, gp_str_length(str));
}

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
static bool gp_valid_codepoint(
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

bool gp_str_is_valid(
    GPString _str,
    size_t* invalid_index)
{
    const char* str = (const char*)_str;
    const size_t length = gp_str_length(_str);
    for (size_t i = 0; i < length;)
    {
        size_t cp_length = gp_bytes_codepoint_length(str + i);
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
    if (gp_str_allocator(*out) != NULL)
        gp_str_reserve(out, arg_count * 10);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        if (gp_str_allocator(*out) != NULL)
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
    if (gp_str_allocator(*out) != NULL)
        gp_str_reserve(out, arg_count * 10);

    gp_str_header(*out)->length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        if (gp_str_allocator(*out) != NULL)
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
            size_t size = gp_bytes_codepoint_length(*str + prefix_length);
            memcpy(codepoint, *str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
        }
        length -= prefix_length;

        memmove(*str, *str + prefix_length, length);
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_bytes_codepoint_length(*str + i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    gp_str_header(*str)->length = length;
}

static void gp_str_to_something(
    GPString* str,
    wint_t(*const towsomething)(wint_t))
{
    size_t length = gp_str_length(*str);

    wchar_t  stack_buf[1 << 10];
    size_t   buf_cap = sizeof stack_buf / sizeof*stack_buf;
    wchar_t* buf = stack_buf;
    if (length + sizeof"" > buf_cap) {
        buf_cap = length + sizeof"";
        buf = gp_mem_alloc(gp_heap, buf_cap * sizeof*buf);
    }
    const char* src = gp_cstr(*str);
    size_t buf_length = mbsrtowcs(buf, &src, buf_cap, &(mbstate_t){0});

    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    const wchar_t* pbuf = (const wchar_t*)buf;
    gp_str_reserve(str, wcsrtombs(NULL, &pbuf, 0, &(mbstate_t){0}));

    gp_str_header(*str)->length = wcsrtombs((char*)*str,
        &pbuf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        gp_mem_dealloc(gp_heap, buf);
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

static size_t gp_str_find_invalid(
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

static size_t gp_str_find_valid(
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

int gp_str_case_compare(
    const GPString _s1,
    const GPString _s2)
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
    const size_t max_length = gp_max(gp_str_length(_s1), gp_str_length(_s2));
    if (max_length + 1 >= buf1_cap)
    {
        arena = gp_arena_new(2 * max_length * sizeof*buf1 +/*internals*/64);
        scope = (const GPAllocator*)&arena;
    }
    if (gp_str_length(_s1) + 1 >= buf1_cap) {
        buf1_cap = gp_str_length(_s1) + 1;
        buf1 = gp_mem_alloc(scope, buf1_cap * sizeof(wchar_t));
    }
    if (gp_str_length(_s2) + 1 >= buf2_cap) {
        buf2_cap = gp_str_length(_s2) + 1;
        buf2 = gp_mem_alloc(scope, buf2_cap * sizeof(wchar_t));
    }
    mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0});
    mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0});

    int result = wcscoll(buf1, buf2);
    gp_arena_delete((GPArena*)scope);
    return result;
}

int gp_str_file(
    GPString*   str,
    const char* file_path,
    const char* operation)
{
    switch (operation[0])
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
            const char mode[2] = { operation[0], '\0' };
            FILE* f = fopen(file_path, mode);
            if (f == NULL)
                return -1;
            if (fwrite(*str, sizeof**str, gp_str_length(*str), f) != gp_str_length(*str))
                return -1;
            fclose(f);
        }
    }
    return 0;
}
