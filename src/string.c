// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/string.h"
#include "../include/gpc/utils.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

extern inline char gpc_str_at(const gpc_String s, size_t i);
extern inline bool gpc_str_is_view(const gpc_String s);

// Offset from allocation address to the beginning of string data
static size_t gpc_l_capacity(const gpc_String s[GPC_NONNULL])
{
    switch (s->has_offset)
    {
        case 1: return *((unsigned char*)s->data - 1);
        case 2: return *((size_t*)s->data - 1);
    }
    return 0;
}

static size_t gpc_r_capacity(const gpc_String s[GPC_NONNULL])
{
    return s->capacity - gpc_l_capacity(s);
}

static char* gpc_allocation_address(const gpc_String s[GPC_NONNULL])
{
    return s->data - gpc_l_capacity(s);
}

static void gpc_str_free(gpc_String s[GPC_NONNULL])
{
    if (s->is_allocated)
    {
        if (s->allocator == NULL)
            free(gpc_allocation_address(s));
        // TODO else use allocator when it's implemented
    }
}

void gpc_str_clear(gpc_String s[GPC_NONNULL])
{
    gpc_str_free(s);
    *s = (gpc_String){0};
}

gpc_String* gpc_str_copy(gpc_String dest[GPC_NONNULL], const gpc_String src)
{
    if (src.length > dest->capacity) // allocation needed
    {
        size_t new_capacity = gpc_next_power_of_2(src.length);
        char* buf = malloc(new_capacity); // TODO use allocator
        if (buf == NULL)
        {
            perror("malloc() failed in gpc_str_copy()!");
            dest->data -= gpc_l_capacity(dest);
            memcpy(dest->data, src.data, dest->capacity);
            dest->length = dest->capacity;
            dest->has_offset = false;
            return NULL;
        }
        gpc_str_free(dest);
        dest->data = buf;
        dest->capacity = new_capacity;
        dest->is_allocated = true;
    }
    else if (src.length > gpc_r_capacity(dest)) // no alloc needed but need more space
    {
        dest->data -= gpc_l_capacity(dest);
    }

    memcpy(dest->data, src.data, src.length);
    dest->length = src.length;
    dest->has_offset = false;
    return dest;
}

gpc_String* gpc_str_reserve(
    gpc_String s[GPC_NONNULL],
    const size_t requested_capacity)
{
    if (requested_capacity > s->capacity)
    {
        s->capacity = gpc_next_power_of_2(requested_capacity);
        char* buf = malloc(s->capacity); // TODO use allocator
        if (buf == NULL)
        {
            perror("malloc() failed in gpc_str_reserve()!");
            return NULL;
        }
        memcpy(buf, s->data, s->length);
        gpc_str_free(s);
        s->data = buf;
        s->is_allocated = true;
        s->has_offset   = false;
    }
    return s;
}

bool gpc_str_eq(const gpc_String s1, const gpc_String s2)
{
    if (s1.length != s2.length)
        return false;
    return gpc_mem_eq(s1.data, s2.data, s1.length);
}

gpc_String* gpc_str_replace_char(gpc_String s[GPC_NONNULL], size_t i, char c)
{
    if (i >= s->length)
        return NULL;

    if (gpc_str_reserve(s, s->length) == NULL)
    {
        perror("gpc_str_is_view() calling gpc_str_reserve()"); // TODO better error handling
        return NULL;
    }

    s->data[i] = c;
    return s;
}
