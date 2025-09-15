// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Warning: do NOT define GP_NO_TYPE_SAFE_MACRO_SHADOWING here, it will break
// the generated single header! All the macros must be #undef'd BELOW the
// #includes. Ugly, but user experience must be maximized!

#include <gpc/array.h>
#include <gpc/utils.h>
#include "common.h"
#include <string.h>
#include <assert.h>

#ifdef gp_arr_reallocate
#undef gp_arr_reallocate
#endif
void gp_arr_reallocate(
    const size_t   element_size,
    GPArrayAnyAddr _parr,
    size_t         capacity)
{
    GPArrayAny* parr = _parr;
    capacity = gp_next_power_of_2(capacity);
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

    new_block->capacity   = capacity;
    new_block->allocation = new_block;

    *parr = new_block + 1;
}

// TODO use this to implement truncating strings and arrays
// use attributes GP_NONNULL_ARGS(2) GP_NODISCARD
size_t gp_arr_try_reserve(
    const size_t   element_size,
    GPArrayAnyAddr _parr,
    size_t         capacity)
{
    GPArrayAny* parr = _parr;
    if (capacity <= gp_arr_capacity(*parr))
        return 0;
    else if (gp_arr_allocator(*parr) == NULL)
        return capacity - gp_arr_capacity(*parr);
    gp_arr_reallocate(element_size, parr, capacity);
    return 0;
}

