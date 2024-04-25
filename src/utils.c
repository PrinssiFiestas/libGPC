// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// License for random funcitons
// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <gpc/utils.h>
#include "pcg_basic.h"

inline size_t gp_next_power_of_2(size_t n);
inline bool gp_fapproxf(float x, float y, float max_relative_diff);
inline bool gp_fapprox(double x, double y, double max_relative_diff);
inline bool gp_fapproxl(long double x, long double y, long double max_rel_diff);
inline bool gp_mem_equal(const void* rhs, const void* lhs, size_t count);
inline bool gp_clip_range(size_t* start, size_t* end, size_t limit);



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

void gp_random_seed(GPRandomState* state, uint64_t seed)
{
    pcg32_srandom_r((pcg32_random_t*)state, seed, 0xf35d3918378e53c4ULL);
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
