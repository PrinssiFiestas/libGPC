// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/bytes.h>
#include <gpc/utils.h>
#include <gpc/overload.h>
#include <gpc/string.h>
#include <printf/conversions.h>
#include "pfstring.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>
#include <limits.h>

static void* gp_memmem(
    const void* haystack, const size_t hlen, const void* needle, const size_t nlen)
{
    #if defined(_GNU_SOURCE) && defined(__linux__)
    return memmem(haystack, hlen, needle, nlen);
    #endif
    if (hlen == 0 || nlen == 0)
        return NULL;

    const char n0 = *(char*)needle;
    for (void* p = memchr(haystack, n0, hlen); p != NULL;)
    {
        if (p + nlen > haystack + hlen)
            return NULL;
        if (memcmp(p, needle, nlen) == 0)
            return p;

        p++;
        p = memchr(p, n0, hlen - (p - haystack));
    }
    return NULL;
}

size_t gp_bytes_find_first(
    const void*  haystack,
    const size_t haystack_size,
    const void*  needle,
    const size_t needle_size,
    const size_t start)
{
    const void* result = gp_memmem(
        haystack + start, haystack_size - start, needle, needle_size);
    return result ? (size_t)(result - haystack) : GP_NOT_FOUND;
}

// Find first occurrence of ch looking from right to left
static const char* gp_memchr_r(const char* ptr_r, const char ch, size_t count)
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

size_t gp_bytes_find_last(
    const void*  _haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_length)
{
    const char* haystack = (const char*)_haystack;

    if (needle_length > haystack_length || needle_length==0 || haystack_length==0)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle_length - 1;
    const char* data = haystack + haystack_length - needle_last;
    size_t to_be_searched = haystack_length - needle_last;

    while ((data = gp_memchr_r(data, *(char*)needle, to_be_searched)))
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

size_t gp_bytes_find_first_of(
    const void*const haystack,
    const size_t haystack_size,
    const char*const char_set,
    const size_t start)
{
    const uint8_t*const hay = haystack;
    for (size_t i = start; i < haystack_size; i++)
        if (strchr(char_set, hay[i]) != NULL)
            return i;
    return GP_NOT_FOUND;
}

size_t gp_bytes_find_first_not_of(
    const void*const haystack,
    const size_t haystack_size,
    const char*const char_set,
    const size_t start)
{
    const uint8_t*const hay = haystack;
    for (size_t i = start; i < haystack_size; i++)
        if (strchr(char_set, hay[i]) == NULL)
            return i;
    return GP_NOT_FOUND;
}

size_t gp_bytes_count(
    const void*  haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_size)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_bytes_find_first(haystack, haystack_length, needle, needle_size, i))
        != GP_NOT_FOUND)
    {
        count++;
        i++;
    }
    return count;
}

bool gp_bytes_equal(
    const void*  s1,
    const size_t s1_size,
    const void*  s2,
    const size_t s2_size)
{
    if (s1_size != s2_size)
        return false;
    else
        return memcmp(s1, s2, s2_size) == 0;
}

bool gp_bytes_equal_case(
    const void* _s1,
    const size_t s1_size,
    const void* _s2,
    const size_t s2_size)
{
    if (s1_size != s2_size)
        return false;

    const char* s1 = _s1;
    const char* s2 = _s2;
    for (size_t i = 0; i < s1_size; i++)
    {
        const char c1 = s1[i] + ('A' <= s1[i] && s1[i] <= 'Z') * ('a' - 'A');
        const char c2 = s2[i] + ('A' <= s2[i] && s2[i] <= 'Z') * ('a' - 'A');
        if (c1 != c2)
            return false;
    }
    return true;
}

bool gp_bytes_is_valid(
    const void* _str,
    const size_t n,
    size_t* invalid_index)
{
    const uint8_t* str = _str;
    const size_t align_offset = (uintptr_t)str     % 8;
    const size_t remaining    = (n - align_offset) % 8;
    size_t i = 0;

    for (size_t len = gp_min(align_offset, n); i < len; i++) {
        if (str[i] & 0x80) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
    }
    for (; i < n - remaining; i += 8) {
        uint64_t x;
        memcpy(&x, str + i, sizeof x);
        if (x & 0x8080808080808080) // invalid detected
            break; // find the index for the invalid in the next loop
    }
    for (; i < n; i++) {
        if (str[i] & 0x80) {
            if (invalid_index != NULL)
                *invalid_index = i;
            return false;
        }
    }
    return true;
}

size_t gp_bytes_slice(
    void*restrict dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    if (src != NULL)
        memcpy(dest, src + start, end - start);
    else
        memmove(dest, (uint8_t*)dest + start, end - start);
    return end - start;
}

size_t gp_bytes_repeat(
    void*restrict dest,
    const size_t n,
    const void*restrict mem,
    const size_t mem_length)
{
    if (mem_length == 1) {
        memset(dest, *(uint8_t*)mem, n);
    } else for (size_t i = 0; i < n; i++) {
        memcpy(dest + i * mem_length, mem, mem_length);
    }
    return n * mem_length;
}

size_t gp_bytes_append(
    void*restrict dest,
    const size_t dest_length,
    const void* src,
    const size_t src_length)
{
    memcpy(dest + dest_length, src, src_length + sizeof(""));
    return dest_length + src_length;
}

size_t gp_bytes_insert(
    void*restrict dest,
    const size_t dest_length,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    memmove(dest + pos + n, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, n);
    return dest_length + n;
}

size_t gp_bytes_replace_range(
    void*restrict me,
    const size_t me_length,
    const size_t start,
    const size_t end,
    const void*restrict replacement,
    const size_t replacement_length)
{
    memmove(
        me + start + replacement_length,
        me + end,
        me_length - end);

    memcpy(me + start, replacement, replacement_length);
    return me_length + replacement_length - (end - start);
}

size_t gp_bytes_replace(
    void*restrict haystack,
    const size_t haystack_length,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t* in_start_out_pos)
{
    size_t start = in_start_out_pos != NULL ? *in_start_out_pos : 0;
    if ((start = gp_bytes_find_first(haystack, haystack_length, needle, needle_length, start))
        == GP_NOT_FOUND) {
        return GP_NOT_FOUND;
    }

    if (in_start_out_pos != NULL)
        *in_start_out_pos = start;

    const size_t end = start + needle_length;
    return gp_bytes_replace_range(
        haystack,
        haystack_length,
        start,
        end,
        replacement,
        replacement_length);
}

size_t gp_bytes_replace_all(
    void*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t* optional_replacement_count)
{
    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_bytes_find_first(haystack, haystack_length, needle, needle_length, start))
        != GP_NOT_FOUND)
    {
        haystack_length = gp_bytes_replace_range(
            haystack,
            haystack_length,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    if (optional_replacement_count != NULL)
        *optional_replacement_count = replacement_count;
    return haystack_length;
}

size_t gp_bytes_print_internal(
    void*restrict out,
    const size_t n,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += gp_bytes_print_objects(
            n >= length ? n - length : 0,
            (uint8_t*)out + length,
            &args,
            &i,
            objs[i]);
    }
    va_end(_args);
    va_end(args.list);

    return length;
}

size_t gp_bytes_println_internal(
    void*restrict out,
    const size_t n,
    const size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    size_t length = 0;
    for (size_t i = 0; i < arg_count; i++)
    {
        length += gp_bytes_print_objects(
            n >= length ? n - length : 0,
            (uint8_t*)out + length,
            &args,
            &i,
            objs[i]);

        if (n > length)
            ((char*)out)[length++] = ' ';
    }
    va_end(_args);
    va_end(args.list);

    if (n > (length - !!length)) // overwrite last space
        ((char*)out)[length - 1] = '\n';

    return length;
}

size_t gp_bytes_trim(
    void*restrict _str,
    size_t length,
    void**restrict optional_out_ptr,
    const char*restrict optional_char_set,
    int flags)
{
    char* str = _str;
    const bool left  = flags & 0x04;
    const bool right = flags & 0x02;

    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_ASCII_WHITESPACE;

    if (left)
    {
        char last = str[length - 1];
        str[length - 1] = '\0';
        size_t prefix_length = strspn(str, char_set);
        str[length - 1] = last;

        if (prefix_length == length - 1 && strchr(char_set, last) != NULL)
            prefix_length++;

        length -= prefix_length;

        if (optional_out_ptr != NULL)
            *optional_out_ptr = str + prefix_length;
        else
            memmove(str, str + prefix_length, length);
    }

    if (right && length > 0)
    {
        while (strchr(char_set, ((char*)str)[length - 1]) != NULL) {
            length--;
            if (length == 0)
                break;
        }
    }
    return length;
}

size_t gp_bytes_to_upper(
    void* _bytes,
    size_t bytes_size)
{
    char* bytes = _bytes;
    for (size_t i = 0; i < bytes_size; i++)
    {
        if ('a' <= bytes[i] && bytes[i] <= 'z')
            bytes[i] -= 'a' - 'A';
    }
    return bytes_size;
}

size_t gp_bytes_to_lower(
    void* _bytes,
    size_t bytes_size)
{
    char* bytes = _bytes;
    for (size_t i = 0; i < bytes_size; i++)
    {
        if ('A' <= bytes[i] && bytes[i] <= 'Z')
            bytes[i] += 'a' - 'A';
    }
    return bytes_size;
}

static size_t gp_bytes_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const uint8_t* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        if (haystack[i] >= 0x80)
            return i;
    }
    return GP_NOT_FOUND;
}

static size_t gp_bytes_find_valid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const uint8_t* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        if (haystack[i] < 0x80)
            return i;
    }
    return length;
}

size_t gp_bytes_to_valid(
    void*restrict str,
    size_t length,
    const char* replacement)
{
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_bytes_find_invalid(str, start, length)) != GP_NOT_FOUND)
    {
        length = gp_bytes_replace_range(
            str,
            length,
            start,
            gp_bytes_find_valid(str, start, length),
            replacement,
            replacement_length);

        start += replacement_length;
    }
    return length;
}

