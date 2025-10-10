// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Do NOT define GP_NO_TYPE_SAFE_MACRO_SHADOWING here, it will break the
// generated single header! All the macros must be #undef'd BELOW the #includes.

#include <gpc/array.h>
#include <gpc/utils.h>
#include "common.h"
#include <string.h>
#include <assert.h>

#ifdef gp_arr_reallocate
#undef gp_arr_reallocate
#undef gp_arr_filter
#undef gp_arr_push // gp_arr_filter() needs this
#endif

void gp_arr_reallocate(
    const size_t   element_size,
    GPArrayAnyAddr _parr,
    size_t         capacity)
{
    GPArrayAny* parr = _parr;
    gp_assert(gp_arr_allocator(*parr) != NULL, "Cannot reallocate truncating array.");
    GPArrayHeader* new_block;

    if (gp_arr_allocation(*parr) != NULL)
        new_block = gp_mem_realloc(
            gp_arr_allocator(*parr),
            gp_arr_allocation(*parr),
            sizeof*new_block + element_size*gp_arr_capacity(*parr),
            sizeof*new_block + element_size*capacity);
    else
        new_block = memcpy(
            gp_mem_alloc(
                gp_arr_allocator(*parr),
                sizeof*new_block + element_size*capacity),
            (GPArrayHeader*)*parr - 1,
            sizeof*new_block + element_size*gp_arr_length(*parr));

    new_block->capacity   = capacity - (element_size==sizeof(char));
    new_block->allocation = new_block;

    *parr = new_block + 1;
}

size_t gp_arr_filter(
    size_t                   element_size,
    GPArrayAnyAddr           dest_address,
    const void*GP_RESTRICT   optional_src, // mutates arr if NULL
    size_t                   optional_src_length,
    gp_arr_filter_callback_t f)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    bool(*func)(const void* x) = (bool(*)(const void* x))f;

    size_t length = optional_src == NULL ? gp_arr_length(*parr) : optional_src_length;
    gp_arr_set(*parr)->length = 0;
    size_t trunced = 0;

    if (optional_src == NULL) {
        for (size_t i = 0; i < length; ++i)
            if (func((const uint8_t*)*parr + i*element_size))
               memmove(
                   (uint8_t*)*parr + gp_arr_set(*parr)->length++ * element_size,
                   (uint8_t*)*parr + i*element_size,
                   element_size);
    } else {
        if (length <= gp_arr_capacity(*parr)) { // don't need reallocating or bounds checks
            for (size_t i = 0; i < length; ++i)
                if (func((const uint8_t*)optional_src + i*element_size))
                    memcpy(
                        (uint8_t*)*parr + gp_arr_set(*parr)->length++ * element_size,
                        (const uint8_t*)optional_src + i*element_size,
                        element_size);
        } else { // TODO unnecessary allocations and bounds checks can be optimized away by storing indices
            for (size_t i = 0; i < length; ++i) {
                if (func((const uint8_t*)optional_src + i*element_size)) {
                    trunced += gp_arr_push(
                        element_size, parr, (const uint8_t*)optional_src + i*element_size);
                }
            }
        }
    }
    return trunced;
}
