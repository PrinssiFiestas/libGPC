// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/string.h"
#include "../include/gpc/utils.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

gpc_String gpc_str_ctor(gpc_String* s)
{
    if (s == NULL || gpc_mem_eq(s, &(gpc_String){0}, sizeof(*s)))
        return (gpc_String){""};

    if (s->capacity > 0)
    {
        size_t cstrlen;
        if (s->cstr != NULL && s->capacity < (cstrlen = strlen(s->cstr)))
            s->capacity = cstrlen;
        s->capacity = gpc_next_power_of_2(s->capacity);

        char* buf = malloc(s->capacity + sizeof('\0')); // TODO use gpc_malloc()
        if (s->cstr != NULL)
            strcpy(buf, s->cstr);
        else
            buf[0] = '\0';

        s->cstr = buf;
        s->allocation = buf;
        return *s;
    }

    if (s->length == 0)
        s->length = strlen(s->cstr);
    return *s;
}

void gpc_str_free(gpc_String str)
{
    free(str.allocation); // TODO use gpc_free()
}

void gpc_str_clear(gpc_String* s)
{
    free(s->allocation); // TODO use gpc_free()
    *s = (gpc_String){0};
}

gpc_String* gpc_str_copy(gpc_String dest[GPC_NONNULL], const gpc_String src)
{
    size_t offset = dest->allocation ?
        (size_t)(dest->cstr - (char*)dest->allocation) : 0;
    size_t full_capacity = dest->capacity + offset;

    if (src.length > full_capacity) // allocation needed
    {
        dest->capacity = gpc_next_power_of_2(src.length);
        char* buf = malloc(dest->capacity + sizeof('\0'));
        if (buf == NULL)
        {
            perror("malloc() failed in gpc_str_copy()!");
            return NULL;
        }
        free(dest->allocation);
        dest->allocation = buf;
        dest->cstr = dest->allocation;
    }
    else if (src.length > dest->capacity) // no alloc needed but need more space
    {
        dest->cstr -= offset;
        dest->capacity += offset;
    }
    memcpy(dest->cstr, src.cstr, src.length + sizeof('\0'));
    dest->length = src.length;
    return dest;
}

bool gpc_str_equal(const gpc_String s1, const gpc_String s2)
{
    if (s1.length != s2.length)
        return false;
    return gpc_mem_eq(s1.cstr, s2.cstr, s1.length);
}
