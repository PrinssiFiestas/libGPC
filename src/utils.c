// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <gpc/utils.h>
#include "pcg_basic.h"

#if !(defined(__COMPCERT__) && defined(GPC_IMPLEMENTATION))
extern inline uintptr_t gp_round_to_aligned(uintptr_t x, uintptr_t boundary);
extern inline bool gp_approxf(float x, float y, float max_relative_diff);
extern inline bool gp_approx(double x, double y, double max_relative_diff);
#ifndef __COMPCERT__
extern inline bool gp_approxl(long double x, long double y, long double max_rel_diff);
#endif

extern inline int                gp_imin(int x, int y);
extern inline long               gp_lmin(long x, long y);
extern inline long long          gp_llmin(long long x, long long y);
extern inline unsigned           gp_umin(unsigned x, unsigned y);
extern inline unsigned long      gp_lumin(unsigned long x, unsigned long y);
extern inline unsigned long long gp_llumin(unsigned long long x, unsigned long long y);
extern inline float              gp_fminf(float x, float y);
extern inline double             gp_fmin(double x, double y);
#ifndef __COMPCERT__
extern inline long double        gp_fminl(long double x, long double y);
#endif

extern inline int                gp_imax(int x, int y);
extern inline long               gp_lmax(long x, long y);
extern inline long long          gp_llmax(long long x, long long y);
extern inline unsigned           gp_umax(unsigned x, unsigned y);
extern inline unsigned long      gp_lumax(unsigned long x, unsigned long y);
extern inline unsigned long long gp_llumax(unsigned long long x, unsigned long long y);
extern inline float              gp_fmaxf(float x, float y);
extern inline double             gp_fmax(double x, double y);
#ifndef __COMPCERT__
extern inline long double        gp_fmaxl(long double x, long double y);
#endif
#endif // CompCert inline stuff

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
