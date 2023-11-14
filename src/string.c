// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/string.h"
#include "../include/gpc/utils.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

gpc_String gpc_str_new(const char* init, size_t capacity)
{
    gpc_String new_str = {0};
    if (init != NULL)
        new_str.length = strlen(init);
    if (capacity < new_str.length)
        capacity = new_str.length;
    new_str.capacity = gpc_next_power_of_2(capacity);
    new_str.allocation = malloc(new_str.capacity + sizeof('\0'));
    new_str.cstr = strncpy(new_str.allocation, init, new_str.length);
    new_str.cstr[new_str.length] = '\0';
    return new_str;
}

void gpc_str_free(gpc_String* s)
{
    free(s->allocation);
    *s = (gpc_String){0};
}

gpc_String gpc_str_copy(gpc_String* dest, const gpc_String src)
{
    if (dest == NULL)
    {
        gpc_String new_str = { .length = src.length };
        if (src.cstr == NULL)
            new_str.length = 0;
        new_str.capacity = gpc_next_power_of_2(new_str.length);
        new_str.allocation = malloc(new_str.capacity + sizeof('\0'));
        new_str.cstr = strncpy(new_str.allocation, src.cstr, new_str.length);
        new_str.cstr[new_str.length] = '\0';
        return new_str;
    }

    size_t offset = dest->allocation ?
        (size_t)(dest->cstr - (char*)dest->allocation) : 0;
    size_t full_capacity = dest->capacity + offset;
    if (src.length > full_capacity) // allocation needed
    {
        dest->capacity = gpc_next_power_of_2(src.length);
        free(dest->allocation);
        dest->allocation = malloc(dest->capacity + sizeof('\0'));
        if (dest->allocation == NULL)
        {
            perror("malloc() failed in gpc_str_copy()!");
            return *dest = (gpc_String){0};
        }
        dest->cstr = dest->allocation;
    }
    else if (src.length > dest->capacity) // no alloc needed but need more space
    {
        dest->cstr -= offset;
        dest->capacity += offset;
    }
    memcpy(dest->cstr, src.cstr, src.length + sizeof('\0'));
    dest->length = src.length;
    return *dest;
}

bool gpc_str_equal(const gpc_String s1, const gpc_String s2)
{
    if (s1.length != s2.length)
        return false;
    return memcmp(s1.cstr, s2.cstr, s1.length) == 0;
}
