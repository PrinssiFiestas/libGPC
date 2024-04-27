// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <wctype.h>
#include <printf/printf.h>
#include "pfstring.h"

extern inline GPArrayHeader* gp_str_set(GPString* me);

GPString gp_str_clear(GPString me)
{
    if (me == NULL)
        return NULL;
    gp_dealloc(gp_str_allocator(me), gp_str_allocation(me));
    return NULL;
}

const char* gp_cstr(GPString str)
{
    str[gp_str_length(str)].c = '\0';
    return (const char*)str;
}

size_t gp_str_length(const GPString str)
{
    return ((GPArrayHeader*)str - 1)->length;
}

size_t gp_str_capacity(const GPString str)
{
    return ((GPArrayHeader*)str - 1)->capacity;
}

void* gp_str_allocation(const GPString str)
{
    return ((GPArrayHeader*)str - 1)->allocation;
}

const struct gp_allocator* gp_str_allocator(const GPString str)
{
    return (const struct gp_allocator*)(((GPArrayHeader*)str - 1)->allocator);
}

void gp_str_reserve(
    GPString* str,
    size_t capacity)
{
    if (gp_str_capacity(*str) <= capacity)
        gp_realloc(gp_str_allocator(*str), str, gp_str_capacity(*str), capacity);
}

void gp_str_copy(
    GPString* dest,
    GPString src)
{
    memcpy(*dest, src, gp_str_length(src));
    gp_str_set(dest)->length = gp_str_length(src);
}

#if 0
size_t gp_cstr_copy_n(
    char*restrict dest,
    const void*restrict src,
    size_t n)
{
    memcpy(dest, src, n);
    dest[n] = '\0';
    return n;
}

size_t gp_cstr_slice(
    char* str,
    size_t start,
    size_t end)
{
    memmove(str, str + start, end - start);
    str[end - start] = '\0';
    return end - start;
}

size_t gp_big_cstr_slice(
    char** str,
    size_t start,
    size_t end)
{
    *str += start;
    (*str)[end - start] = '\0';
    return end - start;
}

size_t gp_cstr_substr(
    char*restrict dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    memcpy(dest, src + start, end - start);
    dest[end - start] = '\0';
    return end - start;
}

size_t gp_cstr_append(
    char*restrict dest,
    const char*restrict src)
{
    size_t dest_length = strlen(dest);
    size_t src_length  = strlen(src);
    memcpy(dest + dest_length, src, src_length + sizeof(""));
    dest[dest_length + src_length] = '\0';
    return dest_length + src_length;
}

size_t gp_cstr_append_n(
    char*restrict dest,
    const void*restrict src,
    size_t n)
{
    size_t dest_length = strlen(dest);
    memcpy(dest + dest_length, src, n);
    dest[dest_length + n] = '\0';
    return dest_length + n;
}

size_t gp_cstr_insert(
    char*restrict dest,
    size_t pos,
    const char*restrict src)
{
    size_t dest_length = strlen(dest);
    size_t src_length  = strlen(src);
    memmove(dest + pos + src_length, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, src_length);
    dest[dest_length + src_length] = '\0';
    return dest_length + src_length;
}

size_t gp_cstr_insert_n(
    char*restrict dest,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    size_t dest_length = strlen(dest);
    memmove(dest + pos + n, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, n);
    dest[dest_length + n] = '\0';
    return dest_length + n;
}

static size_t cstr_replace_range(
    const size_t me_length,
    char*restrict me,
    const size_t start,
    const size_t end,
    const char* replacement,
    const size_t replacement_length)
{
    memmove(
        me + start + replacement_length,
        me + end,
        me_length - end);

    memcpy(me + start, replacement, replacement_length);
    return me_length + replacement_length - (end - start);
}

size_t gp_cstr_replace(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_in_start_out_pos)
{
    size_t start = optional_in_start_out_pos != NULL ?
        *optional_in_start_out_pos : 0;

    if ((start = gp_cstr_find(haystack, needle, start)) == GP_NOT_FOUND)
    {
        if (optional_in_start_out_pos != NULL)
            *optional_in_start_out_pos = GP_NOT_FOUND;
        return strlen(haystack);
    }
    const size_t haystack_length    = strlen(haystack);
    const size_t needle_length      = strlen(needle);
    const size_t replacement_length = strlen(replacement);
    const size_t end = start + needle_length;

    const size_t out_length = cstr_replace_range(
        haystack_length,
        haystack,
        start,
        end,
        replacement,
        replacement_length);

    haystack[out_length] = '\0';
    if (optional_in_start_out_pos != NULL)
        *optional_in_start_out_pos = start;

    return out_length;
}

size_t gp_cstr_replace_all(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_replacement_count)
{
          size_t haystack_length    = strlen(haystack);
    const size_t needle_length      = strlen(needle);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_cstr_find(haystack, needle, start)) != GP_NOT_FOUND)
    {
        haystack_length = cstr_replace_range(
            haystack_length,
            haystack,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    haystack[haystack_length] = '\0';
    if (optional_replacement_count != NULL)
        *optional_replacement_count = replacement_count;

    return haystack_length;
}

size_t gp_cstr_print_internal(
    int is_println,
    char*restrict _out,
    const size_t n,
    const size_t arg_count,
    const struct GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    struct PFString out_ = { _out, 0, n };
    struct PFString* out = &out_;
    bool capacity_sufficed_for_trailing_space = false;

    for (size_t i = 0; i < arg_count; i++)
    {
        if (objs[i].identifier[0] == '\"')
        {
            const char* fmt = va_arg(args.list, char*);
            for (const char* c = fmt; (c = strchr(c, '%')) != NULL; c++)
            {
                if (c[1] == '%')
                    c++;
                else // consuming more args
                    i++;
            }
            out->length += pf_vsnprintf_consuming(
                out->data + out->length,
                capacity_left(*out),
                fmt,
                &args);

            if (is_println)
                capacity_sufficed_for_trailing_space = push_char(out, ' ');
            continue;
        }

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                push_char(out, (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                if (va_arg(args.list, int))
                    concat(out, "true", strlen("true"));
                else
                    concat(out, "false", strlen("false"));
                break;

            case GP_SHORT:
            case GP_INT:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, int));
                break;

            case GP_LONG:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, long int));
                break;

            case GP_LONG_LONG:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, long long int));
                break;

            case GP_FLOAT:
            case GP_DOUBLE:
                out->length += pf_ftoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, double));
                break;

            char* p;
            case GP_CHAR_PTR:
                p = va_arg(args.list, char*);
                concat(out, p, strlen(p));
                break;

            GPString s;
            case GP_STRING:
                s = va_arg(args.list, GPString);
                concat(out, (char*)s, gp_str_length(s));
                break;

            case GP_PTR:
                p = va_arg(args.list, void*);
                if (p != NULL) {
                    concat(out, "0x", strlen("0x"));
                    out->length += pf_xtoa(
                        capacity_left(*out),
                        out->data + out->length,
                        (uintptr_t)p);
                } else {
                    concat(out, "(nil)", strlen("(nil)"));
                } break;
        }
        if (is_println)
            capacity_sufficed_for_trailing_space = push_char(out, ' ');
    }
    va_end(_args);
    va_end(args.list);
    if (out->capacity > 0) {
        if (capacity_sufficed_for_trailing_space)
            out->data[out->length - 1] = '\n';
        out->data[capacity_left(*out) ? out->length : out->capacity - 1] = '\0';
    }
    return out->length;
}

size_t gp_cstr_codepoint_length(
    const char* str)
{
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[(uint8_t)*str >> 3];
}

size_t gp_cstr_trim(
    char*restrict str,
    const char*restrict optional_char_set,
    int flags)
{
    char* str_start = str;
    size_t length = gp_big_cstr_trim(&str_start, optional_char_set, flags);
    memmove(str, str_start, length);
    str[length] = '\0';
    return length;
}

size_t gp_big_cstr_trim(
    char*restrict* str,
    const char*restrict optional_char_set,
    int flags)
{
    size_t length = strlen(*str);
    const bool left  = flags & 0x04;
    const bool right = flags & 0x02;
    const bool ascii = flags & 0x01;

    if (ascii)
    {
        const char* char_set = optional_char_set != NULL ?
            optional_char_set :
            GP_ASCII_WHITESPACE;

        if (left)
        {
            const size_t prefix_length = strspn(*str, char_set);
            length -= prefix_length;
            *str += prefix_length;
        }
        if (right && length > 0)
        {
            while (strchr(char_set, (*str)[length - 1]) != NULL) {
                length--;
                if (length == 0)
                    break;
            }
        }
        (*str)[length] = '\0';
        return length;
    }
    // else utf8

    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_WHITESPACE;

    if (left)
    {
        size_t prefix_length = 0;
        while (true)
        {
            char codepoint[8] = "";
            size_t size = gp_cstr_codepoint_length(*str + prefix_length);
            memcpy(codepoint, *str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
        }
        length -= prefix_length;
        *str += prefix_length;
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_cstr_codepoint_length(*str + i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    (*str)[length] = '\0';
    return length;
}

static size_t cstr_to_something(
    char* str,
    wint_t(*towsomething)(wint_t))
{
    size_t length = strlen(str);
    size_t buf_cap  = 1 << 10;
    wchar_t stack_buf[1 << 10];
    wchar_t* buf = stack_buf;
    if (length + 1 >= buf_cap) {
        buf_cap = length + 1;
        buf = malloc(buf_cap * sizeof(wchar_t));
    }
    size_t buf_length = mbsrtowcs(buf,
        &(const char*){str}, buf_cap, &(mbstate_t){0});
    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    length = wcsrtombs(str,
        (const wchar_t**)&buf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        free(buf);
    return length;
}

size_t gp_cstr_to_upper(
    char* str)
{
    return cstr_to_something(str, towupper);
}

size_t gp_cstr_to_lower(
    char* str)
{
    return cstr_to_something(str, towlower);
}

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
static bool gp_cstr_valid_codepoint(
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

static size_t gp_cstr_find_invalid(
    const char* haystack,
    const size_t start,
    const size_t length)
{
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_cstr_codepoint_length(haystack + i);
        if (cp_length == 0 || i + cp_length > length)
            return i;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
        if ( ! gp_cstr_valid_codepoint(codepoint))
            return i;

        i += cp_length;
    }
    return GP_NOT_FOUND;
}

static size_t gp_cstr_find_valid(
    const char* haystack,
    const size_t start,
    const size_t length)
{
    for (size_t i = start; i < length; i++)
    {
        size_t cp_length = gp_cstr_codepoint_length(haystack + i);
        if (cp_length == 1)
            return i;
        if (cp_length == 0)
            continue;

        if (cp_length + i < length) {
            uint32_t codepoint = 0;
            for (size_t j = 0; j < cp_length; j++)
                codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
            if (gp_cstr_valid_codepoint(codepoint))
                return i;
        } // else maybe there's ascii in last bytes so continue
    }
    return length;
}

size_t gp_cstr_to_valid(
    char* str,
    const char* replacement)
{
          size_t length = strlen(str);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_cstr_find_invalid(str, start, length)) != GP_NOT_FOUND)
    {
        length = cstr_replace_range(
            length,
            str,
            start,
            gp_cstr_find_valid(str, start, length),
            replacement,
            replacement_length);

        start += replacement_length;
    }

    str[length] = '\0';
    return length;
}

bool gp_cstr_is_valid(
    const char* str)
{
    const size_t length = strlen(str);
    for (size_t i = 0; i < length;)
    {
        if (i + sizeof(uint64_t) < length) // ascii optimization
        {
            uint64_t bytes;
            memcpy(&bytes, str + i, sizeof bytes);
            if ((~bytes & 0x8080808080808080llu) == 0x8080808080808080llu) {
                i += 8;
                continue;
            }
        }
        size_t cp_length = gp_cstr_codepoint_length(str + i);
        if (cp_length == 0 || i + cp_length > length)
            return false;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)str[i + j];
        if ( ! gp_cstr_valid_codepoint(codepoint))
            return false;

        i += cp_length;
    }
    return true;
}

size_t gp_cstr_codepoint_count(
    const char* str)
{
    size_t count = 0;
    for (size_t i = 0; str[i] != '\0'; i++)
        count += gp_cstr_codepoint_length(str + i) != 0;
    return count;
}

size_t gp_cstr_find(const char* haystack, const char* needle, size_t start)
{
    const char* result = strstr(haystack + start, needle);
    return result ? (size_t)(result - haystack) : GP_NOT_FOUND;
}

// Find first occurrence of ch looking from right to left
static const char* memchr_r(const char* ptr_r, const char ch, size_t count)
{
    const char* position = NULL;
    while (--ptr_r, --count != (size_t)-1) // <=> count >= 0
    {
        if (*ptr_r == ch) {
            position = ptr_r;
            break;
        }
    }
    return position;
}

size_t gp_cstr_find_last(const char* haystack, const char* needle)
{
    size_t haystack_length = strlen(haystack);
    size_t needle_length = strlen(needle);

    if (needle_length > haystack_length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle_length - 1;
    const char* data = haystack + haystack_length - needle_last;
    size_t to_be_searched = haystack_length - needle_last;

    while ((data = memchr_r(data, needle[0], to_be_searched)))
    {
        if (memcmp(data, needle, needle_length) == 0)
        {
            position = (size_t)(data - haystack);
            break;
        }
        data--;
        const char* haystack_end = haystack + haystack_length;
        to_be_searched = haystack_length - (size_t)(haystack_end - data);
    }
    return position;
}

size_t gp_cstr_count(const char* haystack, const char* needle)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_cstr_find(haystack, needle, i)) != GP_NOT_FOUND) {
        count++;
        i++;
    }
    return count;
}

bool gp_cstr_equal(const char* s1, const char* s2)
{
    return strcmp(s1, s2) == 0;
}

bool gp_cstr_equal_case(
    const char* s1,
    const char* s2)
{
    size_t s1_length = gp_cstr_codepoint_count(s1);
    size_t s2_length = gp_cstr_codepoint_count(s2);
    if (s1_length != s2_length)
        return false;

    mbstate_t state1 = {0};
    mbstate_t state2 = {0};
    wchar_t wc1;
    wchar_t wc2;
    for (size_t i = 0; i < s1_length; i++)
    {
        size_t wc1_length = mbrtowc(&wc1, s1, sizeof(wchar_t), &state1);
        size_t wc2_length = mbrtowc(&wc2, s2, sizeof(wchar_t), &state2);
        if (sizeof(wchar_t) < sizeof(uint32_t)/* Windows probably */&&
            (wc1_length == (size_t)-2) != (wc2_length == (size_t)-2))
        { // one fits to wchar_t and other doesn't so most likely different
            return false;
        }
        else if (sizeof(wchar_t) < sizeof(uint32_t) &&
                 wc1_length == (size_t)-2) // char wider than sizeof(wchar_t)
        {                                  // so just compare raw bytes
            size_t s1_codepoint_size = gp_cstr_codepoint_length(s1);
            size_t s2_codepoint_size = gp_cstr_codepoint_length(s2);
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

int gp_cstr_case_compare(
    const char* s1,
    const char* s2)
{
    size_t s1_length = strlen(s1);
    size_t s2_length = strlen(s2);
    if (s1_length != s2_length)
        return false;

    size_t buf1_cap  = 1 << 10;
    size_t buf2_cap  = 1 << 10;
    wchar_t stack_buf1[1 << 10];
    wchar_t stack_buf2[1 << 10];
    wchar_t* buf1 = stack_buf1;
    wchar_t* buf2 = stack_buf2;
    if (s1_length + 1 >= buf1_cap) {
        buf1_cap = s1_length + 1;
        buf1 = malloc(buf1_cap * sizeof(wchar_t));
    } if (s2_length + 1 >= buf2_cap) {
        buf2_cap = s2_length + 1;
        buf2 = malloc(buf2_cap * sizeof(wchar_t));
    }
    if (mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0}) !=
        mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0})) {
        return false;
    }
    int result = wcscoll(buf1, buf2);
    if (buf1 != stack_buf1)
        free(buf1);
    if (buf2 != stack_buf2)
        free(buf2);
    return result;
}

#endif

















size_t gp_cstr_copy(
    char*restrict dest,
    const char*restrict src)
{
    size_t len = strlen(src);
    memcpy(dest, src, len + sizeof(""));
    return len;
}

size_t gp_cstr_copy_n(
    char*restrict dest,
    const void*restrict src,
    size_t n)
{
    memcpy(dest, src, n);
    dest[n] = '\0';
    return n;
}

size_t gp_cstr_slice(
    char* str,
    size_t start,
    size_t end)
{
    memmove(str, str + start, end - start);
    str[end - start] = '\0';
    return end - start;
}

size_t gp_big_cstr_slice(
    char** str,
    size_t start,
    size_t end)
{
    *str += start;
    (*str)[end - start] = '\0';
    return end - start;
}

size_t gp_cstr_substr(
    char*restrict dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    memcpy(dest, src + start, end - start);
    dest[end - start] = '\0';
    return end - start;
}

size_t gp_cstr_append(
    char*restrict dest,
    const char*restrict src)
{
    size_t dest_length = strlen(dest);
    size_t src_length  = strlen(src);
    memcpy(dest + dest_length, src, src_length + sizeof(""));
    dest[dest_length + src_length] = '\0';
    return dest_length + src_length;
}

size_t gp_cstr_append_n(
    char*restrict dest,
    const void*restrict src,
    size_t n)
{
    size_t dest_length = strlen(dest);
    memcpy(dest + dest_length, src, n);
    dest[dest_length + n] = '\0';
    return dest_length + n;
}

size_t gp_cstr_insert(
    char*restrict dest,
    size_t pos,
    const char*restrict src)
{
    size_t dest_length = strlen(dest);
    size_t src_length  = strlen(src);
    memmove(dest + pos + src_length, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, src_length);
    dest[dest_length + src_length] = '\0';
    return dest_length + src_length;
}

size_t gp_cstr_insert_n(
    char*restrict dest,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    size_t dest_length = strlen(dest);
    memmove(dest + pos + n, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, n);
    dest[dest_length + n] = '\0';
    return dest_length + n;
}

static size_t cstr_replace_range(
    const size_t me_length,
    char*restrict me,
    const size_t start,
    const size_t end,
    const char* replacement,
    const size_t replacement_length)
{
    memmove(
        me + start + replacement_length,
        me + end,
        me_length - end);

    memcpy(me + start, replacement, replacement_length);
    return me_length + replacement_length - (end - start);
}

size_t gp_cstr_replace(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_in_start_out_pos)
{
    size_t start = optional_in_start_out_pos != NULL ?
        *optional_in_start_out_pos : 0;

    if ((start = gp_cstr_find(haystack, needle, start)) == GP_NOT_FOUND)
    {
        if (optional_in_start_out_pos != NULL)
            *optional_in_start_out_pos = GP_NOT_FOUND;
        return strlen(haystack);
    }
    const size_t haystack_length    = strlen(haystack);
    const size_t needle_length      = strlen(needle);
    const size_t replacement_length = strlen(replacement);
    const size_t end = start + needle_length;

    const size_t out_length = cstr_replace_range(
        haystack_length,
        haystack,
        start,
        end,
        replacement,
        replacement_length);

    haystack[out_length] = '\0';
    if (optional_in_start_out_pos != NULL)
        *optional_in_start_out_pos = start;

    return out_length;
}

size_t gp_cstr_replace_all(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_replacement_count)
{
          size_t haystack_length    = strlen(haystack);
    const size_t needle_length      = strlen(needle);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_cstr_find(haystack, needle, start)) != GP_NOT_FOUND)
    {
        haystack_length = cstr_replace_range(
            haystack_length,
            haystack,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    haystack[haystack_length] = '\0';
    if (optional_replacement_count != NULL)
        *optional_replacement_count = replacement_count;

    return haystack_length;
}

size_t gp_cstr_print_internal(
    int is_println,
    char*restrict _out,
    const size_t n,
    const size_t arg_count,
    const struct GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    struct PFString out_ = { _out, 0, n };
    struct PFString* out = &out_;
    bool capacity_sufficed_for_trailing_space = false;

    for (size_t i = 0; i < arg_count; i++)
    {
        if (objs[i].identifier[0] == '\"')
        {
            const char* fmt = va_arg(args.list, char*);
            for (const char* c = fmt; (c = strchr(c, '%')) != NULL; c++)
            {
                if (c[1] == '%')
                    c++;
                else // consuming more args
                    i++;
            }
            out->length += pf_vsnprintf_consuming(
                out->data + out->length,
                capacity_left(*out),
                fmt,
                &args);

            if (is_println)
                capacity_sufficed_for_trailing_space = push_char(out, ' ');
            continue;
        }

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                push_char(out, (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                if (va_arg(args.list, int))
                    concat(out, "true", strlen("true"));
                else
                    concat(out, "false", strlen("false"));
                break;

            case GP_SHORT:
            case GP_INT:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, int));
                break;

            case GP_LONG:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, long int));
                break;

            case GP_LONG_LONG:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, long long int));
                break;

            case GP_FLOAT:
            case GP_DOUBLE:
                out->length += pf_ftoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, double));
                break;

            char* p;
            case GP_CHAR_PTR:
                p = va_arg(args.list, char*);
                concat(out, p, strlen(p));
                break;

            GPString s;
            case GP_STRING:
                s = va_arg(args.list, GPString);
                concat(out, (char*)s, gp_str_length(s));
                break;

            case GP_PTR:
                p = va_arg(args.list, void*);
                if (p != NULL) {
                    concat(out, "0x", strlen("0x"));
                    out->length += pf_xtoa(
                        capacity_left(*out),
                        out->data + out->length,
                        (uintptr_t)p);
                } else {
                    concat(out, "(nil)", strlen("(nil)"));
                } break;
        }
        if (is_println)
            capacity_sufficed_for_trailing_space = push_char(out, ' ');
    }
    va_end(_args);
    va_end(args.list);
    if (out->capacity > 0) {
        if (capacity_sufficed_for_trailing_space)
            out->data[out->length - 1] = '\n';
        out->data[capacity_left(*out) ? out->length : out->capacity - 1] = '\0';
    }
    return out->length;
}

size_t gp_cstr_codepoint_length(
    const char* str)
{
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[(uint8_t)*str >> 3];
}

size_t gp_cstr_trim(
    char*restrict str,
    const char*restrict optional_char_set,
    int flags)
{
    char* str_start = str;
    size_t length = gp_big_cstr_trim(&str_start, optional_char_set, flags);
    memmove(str, str_start, length);
    str[length] = '\0';
    return length;
}

size_t gp_big_cstr_trim(
    char*restrict* str,
    const char*restrict optional_char_set,
    int flags)
{
    size_t length = strlen(*str);
    const bool left  = flags & 0x04;
    const bool right = flags & 0x02;
    const bool ascii = flags & 0x01;

    if (ascii)
    {
        const char* char_set = optional_char_set != NULL ?
            optional_char_set :
            GP_ASCII_WHITESPACE;

        if (left)
        {
            const size_t prefix_length = strspn(*str, char_set);
            length -= prefix_length;
            *str += prefix_length;
        }
        if (right && length > 0)
        {
            while (strchr(char_set, (*str)[length - 1]) != NULL) {
                length--;
                if (length == 0)
                    break;
            }
        }
        (*str)[length] = '\0';
        return length;
    }
    // else utf8

    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_WHITESPACE;

    if (left)
    {
        size_t prefix_length = 0;
        while (true)
        {
            char codepoint[8] = "";
            size_t size = gp_cstr_codepoint_length(*str + prefix_length);
            memcpy(codepoint, *str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
        }
        length -= prefix_length;
        *str += prefix_length;
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_cstr_codepoint_length(*str + i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    (*str)[length] = '\0';
    return length;
}

static size_t cstr_to_something(
    char* str,
    wint_t(*towsomething)(wint_t))
{
    size_t length = strlen(str);
    size_t buf_cap  = 1 << 10;
    wchar_t stack_buf[1 << 10];
    wchar_t* buf = stack_buf;
    if (length + 1 >= buf_cap) {
        buf_cap = length + 1;
        buf = malloc(buf_cap * sizeof(wchar_t));
    }
    size_t buf_length = mbsrtowcs(buf,
        &(const char*){str}, buf_cap, &(mbstate_t){0});
    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    length = wcsrtombs(str,
        (const wchar_t**)&buf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        free(buf);
    return length;
}

size_t gp_cstr_to_upper(
    char* str)
{
    return cstr_to_something(str, towupper);
}

size_t gp_cstr_to_lower(
    char* str)
{
    return cstr_to_something(str, towlower);
}

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
static bool gp_cstr_valid_codepoint(
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

static size_t gp_cstr_find_invalid(
    const char* haystack,
    const size_t start,
    const size_t length)
{
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_cstr_codepoint_length(haystack + i);
        if (cp_length == 0 || i + cp_length > length)
            return i;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
        if ( ! gp_cstr_valid_codepoint(codepoint))
            return i;

        i += cp_length;
    }
    return GP_NOT_FOUND;
}

static size_t gp_cstr_find_valid(
    const char* haystack,
    const size_t start,
    const size_t length)
{
    for (size_t i = start; i < length; i++)
    {
        size_t cp_length = gp_cstr_codepoint_length(haystack + i);
        if (cp_length == 1)
            return i;
        if (cp_length == 0)
            continue;

        if (cp_length + i < length) {
            uint32_t codepoint = 0;
            for (size_t j = 0; j < cp_length; j++)
                codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
            if (gp_cstr_valid_codepoint(codepoint))
                return i;
        } // else maybe there's ascii in last bytes so continue
    }
    return length;
}

size_t gp_cstr_to_valid(
    char* str,
    const char* replacement)
{
          size_t length = strlen(str);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_cstr_find_invalid(str, start, length)) != GP_NOT_FOUND)
    {
        length = cstr_replace_range(
            length,
            str,
            start,
            gp_cstr_find_valid(str, start, length),
            replacement,
            replacement_length);

        start += replacement_length;
    }

    str[length] = '\0';
    return length;
}

bool gp_cstr_is_valid(
    const char* str)
{
    const size_t length = strlen(str);
    for (size_t i = 0; i < length;)
    {
        if (i + sizeof(uint64_t) < length) // ascii optimization
        {
            uint64_t bytes;
            memcpy(&bytes, str + i, sizeof bytes);
            if ((~bytes & 0x8080808080808080llu) == 0x8080808080808080llu) {
                i += 8;
                continue;
            }
        }
        size_t cp_length = gp_cstr_codepoint_length(str + i);
        if (cp_length == 0 || i + cp_length > length)
            return false;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)str[i + j];
        if ( ! gp_cstr_valid_codepoint(codepoint))
            return false;

        i += cp_length;
    }
    return true;
}

size_t gp_cstr_codepoint_count(
    const char* str)
{
    size_t count = 0;
    for (size_t i = 0; str[i] != '\0'; i++)
        count += gp_cstr_codepoint_length(str + i) != 0;
    return count;
}

size_t gp_cstr_find(const char* haystack, const char* needle, size_t start)
{
    const char* result = strstr(haystack + start, needle);
    return result ? (size_t)(result - haystack) : GP_NOT_FOUND;
}

// Find first occurrence of ch looking from right to left
static const char* memchr_r(const char* ptr_r, const char ch, size_t count)
{
    const char* position = NULL;
    while (--ptr_r, --count != (size_t)-1) // <=> count >= 0
    {
        if (*ptr_r == ch) {
            position = ptr_r;
            break;
        }
    }
    return position;
}

size_t gp_cstr_find_last(const char* haystack, const char* needle)
{
    size_t haystack_length = strlen(haystack);
    size_t needle_length = strlen(needle);

    if (needle_length > haystack_length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle_length - 1;
    const char* data = haystack + haystack_length - needle_last;
    size_t to_be_searched = haystack_length - needle_last;

    while ((data = memchr_r(data, needle[0], to_be_searched)))
    {
        if (memcmp(data, needle, needle_length) == 0)
        {
            position = (size_t)(data - haystack);
            break;
        }
        data--;
        const char* haystack_end = haystack + haystack_length;
        to_be_searched = haystack_length - (size_t)(haystack_end - data);
    }
    return position;
}

size_t gp_cstr_count(const char* haystack, const char* needle)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_cstr_find(haystack, needle, i)) != GP_NOT_FOUND) {
        count++;
        i++;
    }
    return count;
}

bool gp_cstr_equal(const char* s1, const char* s2)
{
    return strcmp(s1, s2) == 0;
}

bool gp_cstr_equal_case(
    const char* s1,
    const char* s2)
{
    size_t s1_length = gp_cstr_codepoint_count(s1);
    size_t s2_length = gp_cstr_codepoint_count(s2);
    if (s1_length != s2_length)
        return false;

    mbstate_t state1 = {0};
    mbstate_t state2 = {0};
    wchar_t wc1;
    wchar_t wc2;
    for (size_t i = 0; i < s1_length; i++)
    {
        size_t wc1_length = mbrtowc(&wc1, s1, sizeof(wchar_t), &state1);
        size_t wc2_length = mbrtowc(&wc2, s2, sizeof(wchar_t), &state2);
        if (sizeof(wchar_t) < sizeof(uint32_t)/* Windows probably */&&
            (wc1_length == (size_t)-2) != (wc2_length == (size_t)-2))
        { // one fits to wchar_t and other doesn't so most likely different
            return false;
        }
        else if (sizeof(wchar_t) < sizeof(uint32_t) &&
                 wc1_length == (size_t)-2) // char wider than sizeof(wchar_t)
        {                                  // so just compare raw bytes
            size_t s1_codepoint_size = gp_cstr_codepoint_length(s1);
            size_t s2_codepoint_size = gp_cstr_codepoint_length(s2);
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

int gp_cstr_case_compare(
    const char* s1,
    const char* s2)
{
    size_t s1_length = strlen(s1);
    size_t s2_length = strlen(s2);
    if (s1_length != s2_length)
        return false;

    size_t buf1_cap  = 1 << 10;
    size_t buf2_cap  = 1 << 10;
    wchar_t stack_buf1[1 << 10];
    wchar_t stack_buf2[1 << 10];
    wchar_t* buf1 = stack_buf1;
    wchar_t* buf2 = stack_buf2;
    if (s1_length + 1 >= buf1_cap) {
        buf1_cap = s1_length + 1;
        buf1 = malloc(buf1_cap * sizeof(wchar_t));
    } if (s2_length + 1 >= buf2_cap) {
        buf2_cap = s2_length + 1;
        buf2 = malloc(buf2_cap * sizeof(wchar_t));
    }
    if (mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0}) !=
        mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0})) {
        return false;
    }
    int result = wcscoll(buf1, buf2);
    if (buf1 != stack_buf1)
        free(buf1);
    if (buf2 != stack_buf2)
        free(buf2);
    return result;
}

