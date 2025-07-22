// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/array.h>
#include <gpc/utils.h>
#include "common.h"
#include <string.h>
#include <assert.h>

GPArray(void) gp_arr_new(
    GPAllocator* allocator,
    const size_t element_size,
    const size_t element_count)
{
    const size_t size = gp_round_to_aligned(element_size * element_count, GP_ALLOC_ALIGNMENT);
    GPArrayHeader* me = gp_mem_alloc(allocator, sizeof(*me) + size);
    *me = (GPArrayHeader) {
        .length = 0,
        .capacity = size / element_size,
        .allocator = allocator,
        .allocation = me
    };
    return me + 1;
}

GPArray(void) gp_arr_reallocate(
    const size_t  element_size,
    GPArray(void) arr,
    size_t        capacity)
{
    capacity = gp_next_power_of_2(capacity);
    GPArrayHeader* new_block;

    if (gp_arr_allocation(arr) != NULL)
        new_block = gp_mem_realloc(
            gp_arr_allocator(arr),
            gp_arr_allocation(arr),
            sizeof*new_block + element_size*gp_arr_capacity(arr),
            sizeof*new_block + element_size*capacity);
    else
        new_block = memcpy(
            gp_mem_alloc(
                gp_arr_allocator(arr),
                sizeof*new_block + element_size*capacity),
            (GPArrayHeader*)arr - 1,
            sizeof*new_block + element_size*gp_arr_length(arr));

    new_block->capacity   = capacity;
    new_block->allocation = new_block;

    return new_block + 1;
}

GPArray(void) gp_arr_reserve(
    const size_t  element_size,
    GPArray(void) arr,
    size_t        capacity)
{
    // This check is here so functions like gp_str_print() can try to
    // guesstimate buffer sizes even for strings on stack when allocator is NULL.
    //
    // TODO once truncating strings and arrays are implemented, this should be
    // an assertion instead.
    if (gp_arr_allocator(arr) == NULL)
        return arr;

    if (capacity <= gp_arr_capacity(arr))
        return arr;
    return gp_arr_reallocate(element_size, arr, capacity);
}

GPArray(void) gp_arr_copy(
    const size_t        element_size,
    GPArray(void)       dest,
    const void*restrict src,
    const size_t        src_length)
{
    dest = gp_arr_reserve(element_size, dest, src_length);
    assert(gp_arr_capacity(dest) >= src_length); // analyzer false positive
    memcpy(dest, src, src_length * element_size);
    ((GPArrayHeader*)dest - 1)->length = src_length;
    return dest;
}

GPArray(void) gp_arr_slice(
    const size_t elem_size,
    GPArray(void)       dest,
    const void*restrict const src,
    const size_t i_start,
    const size_t i_end)
{
    size_t length = i_end - i_start;

    if (src == NULL) {
        memmove(dest, (uint8_t*)dest + i_start * elem_size, length * elem_size);
    } else {
        dest = gp_arr_reserve(elem_size, dest, length);
        memcpy(dest, (uint8_t*)src + i_start * elem_size, length * elem_size);
    }
    ((GPArrayHeader*)dest - 1)->length = length;
    return dest;
}

GPArray(void) gp_arr_push(
    const size_t element_size,
    GPArray(void)       arr,
    const void*restrict element)
{
    const size_t length = gp_arr_length(arr);
    arr = gp_arr_reserve(element_size, arr, length + 1);
    memcpy((uint8_t*)arr + length * element_size, element, element_size);
    ((GPArrayHeader*)arr - 1)->length++;
    return arr;
}

void* gp_arr_pop(
    const size_t element_size,
    GPArray(void) arr)
{
    return (uint8_t*)arr + --((GPArrayHeader*)arr - 1)->length * element_size;
}

GPArray(void) gp_arr_append(
    const size_t element_size,
    GPArray(void)       arr,
    const void*restrict src,
    const size_t n)
{
    const size_t length = gp_arr_length(arr);
    arr = gp_arr_reserve(element_size, arr, length + n);
    memcpy((uint8_t*)arr + length * element_size, src, n * element_size);
    ((GPArrayHeader*)arr - 1)->length += n;
    return arr;
}

GPArray(void) gp_arr_insert(
    const size_t elem_size,
    GPArray(void) arr,
    const size_t pos,
    const void*restrict src,
    const size_t n)
{
    const size_t length = gp_arr_length(arr);
    arr = gp_arr_reserve(elem_size, arr, length + n);

    memmove(
        (uint8_t*)arr + (pos + n) * elem_size,
        (uint8_t*)arr +  pos      * elem_size,
        (length - pos)            * elem_size);
    memcpy(
        (uint8_t*)arr +  pos * elem_size, src, n * elem_size);

    ((GPArrayHeader*)arr - 1)->length += n;
    return arr;
}

GPArray(void) gp_arr_erase(
    const size_t  elem_size,
    GPArray(void) arr,
    const size_t  pos,
    const size_t  count)
{
    size_t* length = &((GPArrayHeader*)arr - 1)->length;
    const size_t tail_length = *length - (pos + count);
    memmove(
        (uint8_t*)arr +  pos          * elem_size,
        (uint8_t*)arr + (pos + count) * elem_size,
        tail_length                   * elem_size);
    *length -= count;
    return arr;
}

GPArray(void) gp_arr_map(
    const size_t elem_size,
    GPArray(void) arr,
    const void*restrict optional_src, // mutates arr if NULL
    const size_t src_length,
    void (*const f)(void* out, const void* in))
{
    if (optional_src == NULL) {
        for (size_t i = 0; i < gp_arr_length(arr); i++)
            f((uint8_t*)arr + i * elem_size, (uint8_t*)arr + i * elem_size);
    } else {
        arr = gp_arr_reserve(elem_size, arr, src_length);
        for (size_t i = 0; i < src_length; i++)
            f((uint8_t*)arr + i * elem_size, (uint8_t*)optional_src + i * elem_size);
        ((GPArrayHeader*)arr - 1)->length = src_length;
    }
    return arr;
}

void* gp_arr_fold(
    const size_t elem_size,
    const GPArray(void) arr,
    void* accumulator,
    void* (*const f)(void* accumulator, const void* element))
{
    for (size_t i = 0; i < gp_arr_length(arr); i++)
        accumulator = f(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

void* gp_arr_foldr(
    const size_t elem_size,
    const GPArray(void) arr,
    void* accumulator,
    void* (*const f)(void* accumulator, const void* element))
{
    for (size_t i = gp_arr_length(arr) - 1; i != (size_t)-1; i--)
        accumulator = f(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

static GPArray(void) gp_arr_filter_aliasing(
    const size_t elem_size,
    GPArray(void)restrict const arr,
    const size_t length,
    bool (*const f)(const void* x))
{
    size_t i = 0;
    ((GPArrayHeader*)arr - 1)->length = 0;

    for (; i < length; i++) // skip copying first matching elements
    {
        if (f((uint8_t*)arr + i * elem_size)) {
            ((GPArrayHeader*)arr - 1)->length++;
        } else {
            i++; // after this i > length(arr) so arr[i] and arr[length(arr)]
                 // will not alias
            break;
        }
    }
    for (; i < length; i++)
    {
        if (f((uint8_t*)arr + i * elem_size))
            memcpy(
                (uint8_t*)arr + ((GPArrayHeader*)arr - 1)->length++ * elem_size,
                (uint8_t*)arr + i * elem_size,
                elem_size);
    }
    return arr;
}

static GPArray(void) gp_arr_filter_non_aliasing(
    const size_t elem_size,
    GPArray(void)restrict arr,
    const void*restrict src,
    const size_t src_length,
    bool (*const f)(const void* x))
{
    arr = gp_arr_reserve(elem_size, arr, src_length);
    ((GPArrayHeader*)arr - 1)->length = 0;

    for (size_t i = 0; i < src_length; i++)
    {
        if (f((uint8_t*)src + i * elem_size))
            memcpy(
                (uint8_t*)arr + ((GPArrayHeader*)arr - 1)->length++ * elem_size,
                (uint8_t*)src + i * elem_size,
                elem_size);
    }
    return arr;
}

GPArray(void) gp_arr_filter(
    const size_t elem_size,
    GPArray(void) arr,
    const void*restrict optional_src,
    const size_t src_length,
    bool (*const f)(const void* x))
{
    if (optional_src == NULL)
        return gp_arr_filter_aliasing(
            elem_size,
            arr,
            gp_arr_length(arr),
            f);
    else
        return gp_arr_filter_non_aliasing(
            elem_size,
            arr,
            optional_src,
            src_length,
            f);
}

