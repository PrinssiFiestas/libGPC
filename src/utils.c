// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define _CRT_RAND_S
#include <gpc/assert.h>
#include <gpc/utils.h>
#include <gpc/thread.h>
#include <stdio.h>
#include <string.h>

// Random stuff

GPRandomState gp_random_state(void)
{
    uint64_t init_state;
    uint64_t stream_id;
    unsigned char seed[sizeof init_state + sizeof stream_id];
    unsigned error = 0;

    #if _WIN32
    for (size_t i = 0; i < sizeof seed_bits; i += sizeof(unsigned))
        error |= rand_s((unsigned*)(seed_bits + i));
    #else
    FILE* dev_random;
    error = 0
        || (dev_random = fopen("/dev/random", "rb")) == NULL
        || fread(seed, 1, sizeof seed, dev_random) != sizeof seed;
    if (dev_random != NULL)
        fclose(dev_random);
    #endif
    if (error == 0) {
        memcpy(&init_state, seed, sizeof init_state);
        memcpy(&stream_id,  seed + sizeof init_state, sizeof stream_id);
        return gp_random_state_seed(init_state, stream_id);
    }

    static GP_MAYBE_ATOMIC size_t counter = 0;
    return gp_random_state_seed(time(NULL), counter++);
}

GPRandomState gp_random_state_seed(uint64_t init_state, uint64_t stream_id)
{
    GPRandomState rng = {.inc = (stream_id << 1u) | 1u };
    (uint32_t){0} = gp_random(&rng);
    rng.state += init_state;
    (uint32_t){0} = gp_random(&rng);
    return rng;
}

uint32_t gp_random(GPRandomState* rng)
{
    #ifndef NDEBUG
    gp_assert(rng->inc & 1,
        "GPRandomState must be created with gp_random_state() or "
        "gp_random_state_seed() and it's internals must not be modified.");
    #endif
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((0-rot) & 31));
}

double gp_frandom(GPRandomState* rng)
{
    return ldexp(gp_random(rng), -32);
}

uint32_t gp_random_bound(GPRandomState* rng, uint32_t bound)
{
    // Coin flip, which is very common, most calculations can be avoided by
    // caching bits.
    if (bound == 2) {
        if (rng->coin_flip_cache_length == 0) {
            rng->coin_flip_cache_bits   = gp_random(rng);
            rng->coin_flip_cache_length = 32;
        }
        uint32_t bit = rng->coin_flip_cache_bits & 1;
        rng->coin_flip_cache_bits  >>= 1;
        rng->coin_flip_cache_length -= 1;
        return bit;
    }
    // Power of 2, also very common, avoid integer modulus.
    else if ((bound & (bound - 1)) == 0)
        return gp_random(rng) & (bound - 1);

    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = (0-bound) % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = gp_random(rng);
        if (r >= threshold)
            return r % bound;
    }
}

int32_t gp_random_range(GPRandomState* rng, int32_t min, int32_t max)
{
    gp_db_assert(max > min, "Invalid range.");
    union punner { uint32_t u; int32_t i; } r; // avoid implementation defined cast when u>INT32_MAX
    r.u = gp_random_bound(rng, (uint32_t)max - min) + min;

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

// Having this in a dedicated translation unit should be enough to confuse most
// compilers. The attributes prevent LTO.
GP_GNU_ATTRIB(noinline) GP_OPTIMIZE_NONE
void gp_launder_noinline(void**_)
{
    (void)_;
}
