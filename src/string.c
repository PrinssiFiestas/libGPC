// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <signal.h>
#ifdef __GNUC__
    #define GP_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
    #define GP_ALWAYS_INLINE inline
#endif
GP_ALWAYS_INLINE void gp_debug_segfault(void) // TODO just put this in a macro
{
    #if GP_DEBUG
        raise(SIGSEGV);
    #endif
}

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdio.h>
#include <stddef.h> // TODO needed?
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

extern inline bool gpstr_is_view(GPString s);
extern inline const char* gpcstr(GPString str);

// ----------------------------------------------------------------------------

GPString* gpstr_reserve(
    GPString s[GP_NONNULL],
    const size_t requested_capacity,
    GPAllocator allocator[GP_NONNULL])
{
    if (requested_capacity > s->capacity)
    {
        const size_t final_cap = gp_next_power_of_2(requested_capacity);
        char* buf = gpmem_alloc(allocator, final_cap);
        if (buf == NULL)
            return NULL;
        memcpy(buf, s->data, s->length);
        gpmem_dealloc(allocator, s->data);
        s->data = buf;
        s->capacity = final_cap;
    }
    return s;
}

void gpstr_clear(GPString s[GP_NONNULL], GPAllocator* allocator)
{
    if (allocator != NULL)
        gpmem_dealloc(allocator, s);
    *s = (GPString){0};
}

GPString* gpstr_copy(GPString dest[GP_NONNULL], const GPString src)
{
    memcpy(dest->data, src.data, src.length);
    dest->length = src.length;
    return dest;
}

bool gpstr_eq(const GPString s1, const GPString s2)
{
    if (s1.length != s2.length)
        return false;
    return memcmp(s1.data, s2.data, s1.length) == 0;
}

GPString* gpstr_slice(
    GPString str[GP_NONNULL],
    const size_t start,
    const size_t end) // NOT inclusive!
{
    if (start >= str->length || end > str->length || end < start)
        gp_debug_segfault();

    const size_t length = end - start;
    memmove(str->data, str->data + start, length);
    str->length = length;

    return str;
}

GPString* gpstr_substr(
    GPString dest[GP_NONNULL],
    const GPString src,
    const size_t start,
    const size_t end)
{
    if (start >= src.length || end > src.length || end < start)
        gp_debug_segfault();

    const size_t length = end - start;
    memcpy(dest->data, src.data + start, length);
    dest->length = length;

    return dest;
}

GPString* gpstr_insert(
    GPString dest[GP_NONNULL],
    const size_t pos,
    const GPString src)
{
    if (pos >= dest->length + 1) // +1 because +0 is allowed for appending
        gp_debug_segfault();

    memmove(dest->data + pos + src.length, dest->data + pos, dest->length - pos);
    memcpy(dest->data + pos, src.data, src.length);
    dest->length += src.length;

    return dest;
}

size_t gpstr_find(const GPString haystack, const GPString needle, const size_t start)
{
    if (needle.length > haystack.length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const char* data = haystack.data + start;
    size_t to_be_searched = haystack.length - start;

    while ((data = memchr(data, needle.data[0], to_be_searched)))
    {
        if (memcmp(data, needle.data, needle.length) == 0)
        {
            position = (size_t)(data - haystack.data);
            break;
        }
        data++;
        to_be_searched = haystack.length - (size_t)(data - haystack.data);
    }
    return position;
}

// Find first occurrence of ch looking from right to left
static const char* memchr_r(const char ptr_r[GP_NONNULL], const char ch, size_t count)
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

size_t gpstr_find_last(const GPString haystack, const GPString needle)
{
    if (needle.length > haystack.length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle.length - 1;
    const char* data = haystack.data + haystack.length - needle_last;
    size_t to_be_searched = haystack.length - needle_last;

    while ((data = memchr_r(data, needle.data[0], to_be_searched)))
    {
        if (memcmp(data, needle.data, needle.length) == 0)
        {
            position = (size_t)(data - haystack.data);
            break;
        }
        data--;
        const char* haystack_end = haystack.data + haystack.length;
        to_be_searched = haystack.length - (size_t)(haystack_end - data);
    }
    return position;
}

size_t gpstr_count(const GPString haystack, const GPString needle)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gpstr_find(haystack, needle, i)) != GP_NOT_FOUND) {
        count++;
        i++;
    }
    return count;
}

static GPString* replace_range(
    GPString me[GP_NONNULL],
    const size_t start,
    const size_t end,
    const GPString replacement)
{
    memmove(
        me->data + start + replacement.length,
        me->data + end,
        me->length - end);

    memcpy(me->data + start, replacement.data, replacement.length);
    me->length += replacement.length - (end - start);
    return me;
}

size_t gpstr_replace(
    GPString me[GP_NONNULL],
    const GPString needle,
    const GPString replacement,
    size_t start)
{
    if ((start = gpstr_find(*me, needle, start)) == GP_NOT_FOUND)
        return GP_NOT_FOUND;
    const size_t end = start + needle.length;
    replace_range(me, start, end, replacement);
    return start;
}

unsigned gpstr_replace_all(
    GPString me[GP_NONNULL],
    const GPString needle,
    const GPString replacement)
{
    size_t start = 0;
    unsigned replacement_count = 0;
    while ((start = gpstr_find(*me, needle, start)) != GP_NOT_FOUND)
    {
        replace_range(me, start, start + needle.length, replacement);
        start += replacement.length;
        replacement_count++;
    }
    return replacement_count;
}

GPString*
gpstr_trim(GPString me[GP_NONNULL], const char char_set[GP_NONNULL], int mode)
{
    size_t i_l = 0;
    size_t i_r = me->length;

    if (mode == 'l' || mode == 'l' + 'r')
        while (strchr(char_set, me->data[i_l]))
            i_l++;

    if (mode == 'r' || mode == 'l' + 'r') {
        i_r--;
        while (strchr(char_set, me->data[i_r]))
            i_r--;
    }

    memmove(me->data, me->data + i_l, me->length - i_l);
    me->length -= i_l + (me->length - i_r);
    if (mode == 'r' || mode == 'l' + 'r')
        me->length++;
    return me;
}

size_t
gpstr_interpolate_internal(GPString* me, unsigned arg_count, ...)
{
    size_t chars_written = 0;
    size_t cap_left = 0;
    char* data = NULL;
    if (me != NULL)
    {
        cap_left = me->capacity;
        data = me->data;
    }

    va_list args;
    va_start(args, arg_count);
    while (arg_count && cap_left)
    {
        const char* fmt = va_arg(args, const char*);
        size_t copied = (size_t)vsnprintf(data, cap_left, fmt, args);
        // vsnprintf() didn't consume arg from args so here's a dummy consumption
        (void)va_arg(args, void*);

        data += copied;
        cap_left -= copied;
        chars_written += copied;
        arg_count--;
    }
    va_end(args);

    if (me != NULL)
        me->length = chars_written;
    return chars_written;
}
