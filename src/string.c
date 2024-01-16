// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <signal.h>
#ifdef __GNUC__
    #define GP_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
    #define GP_ALWAYS_INLINE inline
#endif
GP_ALWAYS_INLINE void gp_debug_segfault(void)
{
    #if GP_DEBUG
        raise(SIGSEGV);
    #endif
}

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

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
    if (gpstr_is_view(*str))
        str->data += start;
    else
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

