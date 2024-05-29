// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <gpc/utils.h>
#include "pcg_basic.h"

extern inline uintptr_t gp_round_to_aligned(uintptr_t x, uintptr_t boundary);
extern inline bool gp_fapproxf(float x, float y, float max_relative_diff);
extern inline bool gp_fapprox(double x, double y, double max_relative_diff);
extern inline bool gp_fapproxl(long double x, long double y, long double max_rel_diff);

size_t gp_next_power_of_2(size_t x)
{
    return sizeof x == sizeof(uint32_t) ?
        gp_next_power_of_2_32(x) : gp_next_power_of_2_64(x);
}

uint32_t gp_next_power_of_2_32(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

uint64_t gp_next_power_of_2_64(uint64_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

bool gp_check_bounds(size_t* start, size_t* end, size_t limit)
{
    bool clipped = false;
    end = end != NULL ? end : &(size_t){ limit };
    if (*end > limit) {
        *end = limit;
        clipped = true;
    }
    if (start != NULL && *start >= *end) {
        *start  = *end - (limit != 0);
        clipped = true;
    }
    return ! clipped;
}



// Random stuff

//static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

GPRandomState gp_new_random_state(uint64_t seed)
{
    GPRandomState state;
    pcg32_srandom_r((pcg32_random_t*)&state, seed, 0xf35d3918378e53c4ULL);
    return state;
}

uint32_t gp_random(GPRandomState* state)
{
    return pcg32_random_r((pcg32_random_t*)state);
}

double gp_frandom(GPRandomState* state)
{
    return ldexp(pcg32_random_r((pcg32_random_t*)state), -32);
}

int32_t gp_random_range(GPRandomState* state, int32_t min, int32_t max)
{
    if (max - min > 0)
        return  (int32_t)pcg32_boundedrand_r((pcg32_random_t*)state,(uint32_t)( max - min + 1)) + min;
    else
        return -(int32_t)pcg32_boundedrand_r((pcg32_random_t*)state,(uint32_t)(-max + min - 1)) + min;
}
