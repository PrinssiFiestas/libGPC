// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/string.h"
#include "../include/gpc/memory.h"
#include "../include/gpc/utils.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define PROPAGATE_ERROR(strptr) \
    if (gpstr_error <= strptr && strptr <= gpstr_error + GPSTR_ERROR_LENGTH) \
        return strptr;

const GPString gpstr_error[] =
{
    [GPSTR_OUT_OF_BOUNDS] = {
        "Index out of bounds.",
        .allocator = &gpmem_null_allocator
    },
    [GPSTR_ALLOCATION_FAILURE] = {
        "Allocating string failed.",
        .allocator = &gpmem_null_allocator
    }
};

extern inline char gpstr_at(const GPString s, size_t i);
extern inline bool gpstr_is_view(const GPString s);
extern inline size_t gpstr_capacity(const GPString s);
extern inline bool gpstr_is_allocated(const GPString s);

// Offset from allocation address to the beginning of string data
static size_t l_capacity(const GPString s)
{
    if (s.allocation)
        return (size_t)(s.data - s.allocation->data);
    else return 0;
}

static size_t r_capacity(const GPString s)
{
    if (s.allocation)
        return s.allocation->capacity - l_capacity(s);
    else return 0;
}

static GPStringAllocation* allocate_string(GPString s[GP_NONNULL], size_t cap)
{
    cap = gp_next_power_of_2(cap);
    GPStringAllocation* buf;

    if (s->allocator == NULL)
        buf = malloc(cap + sizeof(GPStringAllocation));
    else
        buf = gpmem_alloc(s->allocator, cap + sizeof(GPStringAllocation));

    if (buf == NULL)
        return NULL;

    buf->capacity = cap;
    buf->should_free = true;
    return buf;
}

static void free_string(GPString s[GP_NONNULL])
{
    if (s->allocation != NULL && s->allocation->should_free)
    {
        if (s->allocator == NULL)
            free(s->allocation);
        else
            gpmem_dealloc(s->allocator, s->allocation);
    }
}

GPString gpstr_ctor(
    size_t mem_size,
    char mem[GP_STATIC sizeof(GPStringAllocation)],
    size_t init_len,
    const char init[GP_NONNULL],
    GPAllocator* allocator)
{
    GPStringAllocation* alloc = (GPStringAllocation*)mem;
    alloc->capacity = mem_size - sizeof(GPStringAllocation);
    memcpy(alloc->data, init, init_len);
    return (GPString)
    {
        alloc->data,
        init_len,
        allocator,
        alloc
    };
}

const char* gpstr_cstr(GPString str[GP_NONNULL])
{
    if (gpstr_error <= str && str <= gpstr_error + GPSTR_ERROR_LENGTH)
        return str->data;

    GPString* result = gpstr_reserve(str, str->length + 1);
    // gpstr_reserve() returns modifyable string or error string so code below
    // is safe.
    if (result == str)
        result->data[result->length] = '\0';
    return result->data;
}

void gpstr_clear(GPString s[GP_NONNULL])
{
    free_string(s);
    *s = (GPString){0};
}

GPString* gpstr_copy(GPString dest[GP_NONNULL], const GPString src)
{
    PROPAGATE_ERROR(dest);

    if (src.length > gpstr_capacity(*dest)) // allocation needed
    {
        GPStringAllocation* buf = allocate_string(dest, src.length);
        if (buf == NULL)
            return (GPString*)gpstr_error + GPSTR_ALLOCATION_FAILURE;
        free_string(dest);
        dest->data = buf->data;
        dest->allocation = buf;
    }
    else if (src.length > r_capacity(*dest)) // no alloc needed but need more space
    {
        dest->data -= l_capacity(*dest);
    }

    memcpy(dest->data, src.data, src.length);
    dest->length = src.length;
    return dest;
}

GPString* gpstr_reserve(
    GPString s[GP_NONNULL],
    const size_t requested_capacity)
{
    PROPAGATE_ERROR(s);

    if (requested_capacity > gpstr_capacity(*s))
    {
        GPStringAllocation* buf = allocate_string(s, requested_capacity);
        if (buf == NULL)
            return (GPString*)gpstr_error + GPSTR_ALLOCATION_FAILURE;
        memcpy(buf->data, s->data, s->length);
        free_string(s);
        s->data = buf->data;
        s->allocation = buf;
    }
    return s;
}

bool gpstr_eq(const GPString s1, const GPString s2)
{
    if (s1.length != s2.length)
        return false;
    return gp_mem_eq(s1.data, s2.data, s1.length);
}

GPString* gpstr_replace_char(GPString s[GP_NONNULL], size_t i, char c)
{
    PROPAGATE_ERROR(s);

    if (i >= s->length)
        return (GPString*)gpstr_error + GPSTR_OUT_OF_BOUNDS;

    if (gpstr_reserve(s, s->length) != s)
        return (GPString*)gpstr_error + GPSTR_ALLOCATION_FAILURE;

    s->data[i] = c;
    return s;
}

GPString* gpstr_slice(
    GPString str[GP_NONNULL],
    const size_t start,
    const size_t end)
{
    PROPAGATE_ERROR(str);

    if (start > str->length || end > str->length || end < start)
        return (GPString*)gpstr_error + GPSTR_OUT_OF_BOUNDS;

    str->data  += start;
    str->length = end - start;

    return str;
}

GPString* gpstr_substr(
    GPString dest[GP_NONNULL],
    const GPString src,
    const size_t start,
    const size_t end)
{
    PROPAGATE_ERROR(dest);

    if (start > dest->length || end > dest->length || end < start)
    {
        dest->length = 0;
    }
    else
    {
        if (gpstr_reserve(dest, end - start) != dest)
            return (GPString*)gpstr_error + GPSTR_ALLOCATION_FAILURE;
        memcpy(dest->data, src.data + start, end - start);
        dest->length = end - start;
    }
    return dest;
}
