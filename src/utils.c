// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <gpc/assert.h>
#include <gpc/utils.h>
#include "pcg_basic.h"
#include <string.h>

#if !(defined(__COMPCERT__) && defined(GPC_IMPLEMENTATION))
extern inline uintptr_t gp_round_to_aligned(uintptr_t, uintptr_t);
#endif

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


// Random stuff

//static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

GPRandomState gp_random_state(uint64_t seed)
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
{ // TODO optimize for power of 2 ranges (avoid integer modulus)
    gp_db_assert(max > min, "Invalid range.");
    union punner { uint32_t u; int32_t i; } r; // avoid implementation defined cast when u>INT32_MAX
    r.u = pcg32_boundedrand_r((pcg32_random_t*)state, max - min) + min;
    return r.i;
}

void gp_random_bytes(GPRandomState* state, void* buffer, size_t buffer_size)
{
    // Fix possible unalignment
    size_t remainder = (uintptr_t)buffer & (sizeof(uint32_t)-1);
    if (remainder != 0) {
        uint32_t rand = gp_random(state);
        memcpy(buffer, &rand, gp_min(remainder, buffer_size));
        if (remainder >= buffer_size)
            return;

        buffer = (uint8_t*)buffer + remainder;
        buffer_size -= remainder;
    }

    size_t data_length = buffer_size/sizeof(uint32_t);
    size_t i = 0;
    for (; i < data_length; ++i) {
        uint32_t rand = gp_random(state);
        memcpy((uint32_t*)buffer + i, &rand, sizeof rand);
    }

    remainder = buffer_size & (sizeof(uint32_t)-1);
    if (remainder != 0) {
        uint32_t rand = gp_random(state);
        memcpy((uint8_t*)buffer + i*sizeof(uint32_t), &rand, remainder);
    }
}
