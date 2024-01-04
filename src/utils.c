// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// License for random funcitons
// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include "../include/gpc/utils.h"
#include "pcg_basic.h"
#include <string.h>
#include <math.h>

size_t gp_next_power_of_2(size_t n)
{
    // prevent integer overflow
    if (n >= SIZE_MAX/2)
        return SIZE_MAX;

    size_t result = 1;
    while (result <= n)
        result *= 2;
    return result;
}

bool gp_mem_eq(const void* rhs, const void* lhs, size_t count)
{
    return memcmp(lhs, rhs, count) == 0;
}

// Random stuff

//static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

void gp_g_random_seed(uint64_t seed)
{
    pcg32_srandom(seed, 0xf35d3918378e53c4ULL);
}

uint32_t gp_g_random(void)
{
    return pcg32_random();
}

double gp_g_frandom(void)
{
    return ldexp(pcg32_random(), -32);
}

int32_t gp_g_random_range(int32_t min, int32_t max)
{
    if(max - min > 0)
        return  (int32_t)pcg32_boundedrand((uint32_t) (max - min + 1)) + min;
    else
        return -(int32_t)pcg32_boundedrand((uint32_t)(-max + min - 1)) + min;
}

#include "../include/gpc/memory.h"
void gp_random_seed(gp_RandomState* state, uint64_t seed)
{
    pcg32_srandom_r((pcg32_random_t*)state, seed, 0xf35d3918378e53c4ULL);
}

uint32_t gp_random(gp_RandomState* state)
{
    return pcg32_random_r((pcg32_random_t*)state);
}

double gp_frandom(gp_RandomState* state)
{
    return ldexp(pcg32_random_r((pcg32_random_t*)state), -32);
}

int32_t gp_random_range(gp_RandomState* state, int32_t min, int32_t max)
{
    if (max - min > 0)
        return  (int32_t)pcg32_boundedrand_r((pcg32_random_t*)state,(uint32_t)( max - min + 1)) + min;
    else
        return -(int32_t)pcg32_boundedrand_r((pcg32_random_t*)state,(uint32_t)(-max + min - 1)) + min;
}
