// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

#include <gpc/assert.h>
#include <gpc/utils.h>
#include "pcg_basic.h"
#include <string.h>

// Random stuff

//static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

GPRandomState gp_random_state(uint64_t seed)
{
    GPRandomState state = {0};
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
    gp_db_assert(max > min, "Invalid range.");
    union punner { uint32_t u; int32_t i; } r; // avoid implementation defined cast when u>INT32_MAX
    uint32_t bound = max - min;

    // Coin flip, which is very common, most calculations can be avoided by
    // caching bits.
    if (bound == 2) {
        if (state->coin_flip_cache_length == 0) {
            state->coin_flip_cache_bits   = pcg32_random_r((pcg32_random_t*)state);
            state->coin_flip_cache_length = 32;
        }
        r.u = (state->coin_flip_cache_bits & 1) + min;
        state->coin_flip_cache_bits  >>= 1;
        state->coin_flip_cache_length -= 1;
    }
    // Power of 2, also very common, avoid integer modulus in
    // pcg32_boundedrand_r().
    else if ((bound & (bound - 1)) == 0)
        r.u = (pcg32_random_r((pcg32_random_t*)state) & (bound - 1)) + min;
    else
        r.u = pcg32_boundedrand_r((pcg32_random_t*)state, bound) + min;

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
