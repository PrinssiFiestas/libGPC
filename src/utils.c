// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/utils.h"
#include <string.h>

size_t gpc_next_power_of_2(size_t n)
{
    // prevent integer overflow
    if (n >= SIZE_MAX/2)
        return SIZE_MAX;

    size_t result = 1;
    while (result <= n)
        result *= 2;
    return result;
}

bool gpc_mem_eq(const void* rhs, const void* lhs, size_t count)
{
    return memcmp(lhs, rhs, count) == 0;
}
