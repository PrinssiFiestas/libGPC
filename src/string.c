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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

extern inline const char* gpcstr(struct GPString str);
extern inline struct GPString gpstr(const char cstr[GP_NONNULL]);

// ----------------------------------------------------------------------------

struct GPString* gpstr_copy(struct GPString dest[GP_NONNULL], const struct GPString src)
{
    memcpy(dest->data, src.data, src.length);
    dest->length = src.length;
    return dest;
}

bool gpstr_eq(const struct GPString s1, const struct GPString s2)
{
    if (s1.length != s2.length)
        return false;
    return memcmp(s1.data, s2.data, s1.length) == 0;
}

struct GPString* gpstr_slice(
    struct GPString str[GP_NONNULL],
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

struct GPString* gpstr_substr(
    struct GPString dest[GP_NONNULL],
    const struct GPString src,
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

struct GPString* gpstr_insert(
    struct GPString dest[GP_NONNULL],
    const size_t pos,
    const struct GPString src)
{
    if (pos >= dest->length + 1) // +1 because +0 is allowed for appending
        gp_debug_segfault();

    memmove(dest->data + pos + src.length, dest->data + pos, dest->length - pos);
    memcpy(dest->data + pos, src.data, src.length);
    dest->length += src.length;

    return dest;
}

size_t gpstr_find(const struct GPString haystack, const struct GPString needle, const size_t start)
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

size_t gpstr_find_last(const struct GPString haystack, const struct GPString needle)
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

size_t gpstr_count(const struct GPString haystack, const struct GPString needle)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gpstr_find(haystack, needle, i)) != GP_NOT_FOUND) {
        count++;
        i++;
    }
    return count;
}

static struct GPString* replace_range(
    struct GPString me[GP_NONNULL],
    const size_t start,
    const size_t end,
    const struct GPString replacement)
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
    struct GPString me[GP_NONNULL],
    const struct GPString needle,
    const struct GPString replacement,
    size_t start)
{
    if ((start = gpstr_find(*me, needle, start)) == GP_NOT_FOUND)
        return GP_NOT_FOUND;
    const size_t end = start + needle.length;
    replace_range(me, start, end, replacement);
    return start;
}

unsigned gpstr_replace_all(
    struct GPString me[GP_NONNULL],
    const struct GPString needle,
    const struct GPString replacement)
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

struct GPString*
gpstr_trim(struct GPString me[GP_NONNULL], const char char_set[GP_NONNULL], int mode)
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

struct GPString*
gpstr_print_internal(
    struct GPString me[GP_NONNULL], const size_t arg_count, char** args)
{
    for (size_t i = 0; i < arg_count; i++)
    {
        if (args[i][0] != 0 && args[i][0] != 1)
        {
            gpstr_insert(me, me->length, gpstr(args[i]));
        }
        else if (args[i][0] == 0) // cstr
        {
            const char* real_arg;
            memcpy(&real_arg, args[i] + 1, sizeof(real_arg));
            gpstr_insert(me, me->length, gpstr(real_arg));
        }
        else // gpstr
        {
            struct GPString real_arg;
            memcpy(&real_arg, args[i] + 1, sizeof(real_arg));
            gpstr_insert(me, me->length, real_arg);
        }
    }
    return me;
}
