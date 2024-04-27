// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// License for random functions
// (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

/**
 * @file string.h
 * @brief General purpose utilities
 */

#ifndef GPC_UTILS_INCLUDED
#define GPC_UTILS_INCLUDED 1

#include "attributes.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/** Round number up to the next power of 2.
 * Used to compute array sizes on reallocations, thus size_t.
 */
inline size_t gp_next_power_of_2(size_t x)
{
    if (x >= SIZE_MAX/2) // prevent overflow
        return SIZE_MAX;
    size_t result = 1;
    while (result <= x)
        result *= 2;
    return result;
}

/** Compare raw memory.
 * @return true if the first @p count bytes of @p lhs and @p rhs are equal.
 */
inline bool gp_mem_equal(const void* lhs, const void* rhs, size_t count) {
    int memcmp(const void*, const void*, size_t);
    return memcmp(lhs, rhs, count) == 0;
}

#define gp_min(x, y) gp_generic_min(x, y)
#define gp_max(x, y) gp_generic_max(x, y)

inline bool gp_fapproxf(float x, float y, float max_relative_diff) {
    return fabsf(x - y) <= max_relative_diff * fmaxf(x, y);
}
inline bool gp_fapprox(double x, double y, double max_relative_diff) {
    return fabs(x - y) <= max_relative_diff * fmax(x, y);
}
inline bool gp_fapproxl(long double x, long double y, long double max_rel_diff){
    return fabsl(x - y) <= max_rel_diff * fmaxl(x, y);
}

bool gp_check_bounds(
    size_t* optional_start_non_inclusive,
    size_t* optional_end_inclusive,
    size_t  limit);

// ----------------------------------------------------------------------------
// Random functions. Only work for 64 bit platforms for now. // TODO

typedef struct gp_random_state GPRandomState;

GPRandomState gp_new_random_state(uint64_t seed);
uint32_t gp_random      (GPRandomState*);
double   gp_frandom     (GPRandomState*);
int32_t  gp_random_range(GPRandomState*, int32_t min, int32_t max);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

struct gp_random_state
{
    uint64_t private_state;
    uint64_t private_inc;
};

int                gp_imin(int x, int y);
long               gp_lmin(long x, long y);
long long          gp_llmin(long long x, long long y);
unsigned           gp_umin(unsigned x, unsigned y);
unsigned long      gp_lumin(unsigned long x, unsigned long y);
unsigned long long gp_llumin(unsigned long long x, unsigned long long y);

int                gp_imax(int x, int y);
long               gp_lmax(long x, long y);
long long          gp_llmax(long long x, long long y);
unsigned           gp_umax(unsigned x, unsigned y);
unsigned long      gp_lumax(unsigned long x, unsigned long y);
unsigned long long gp_llumax(unsigned long long x, unsigned long long y);

#if defined(__GNUC__) // gp_min() and gp_max() implementations

#define gp_generic_min(x, y) \
    ({ typeof(x) _##x = (x); typeof(y) _##y = (y); _##x < _##y ? _##x : _##y; })

#define gp_generic_max(x, y) \
    ({ typeof(x) _##x = (x); typeof(y) _##y = (y); _##x > _##y ? _##x : _##y; })

#elif __STDC_VERSION__ >= 201112L

#define gp_generic_min(x, y) \
_Generic(x, \
    int:                gp_imin(x, y),   \
    long:               gp_lmin(x, y),   \
    long long:          gp_llmin(x, y),  \
    unsigned:           gp_umin(x, y),   \
    unsigned long:      gp_lumin(x, y),  \
    unsigned long long: gp_llumin(x, y), \
    float:              gp_fminf(x, y),  \
    double:             gp_fmin(x, y),   \
    long double:        gp_fminl(x, y))

#define gp_generic_max(x, y) \
_Generic(x, \
    int:                gp_imax(x, y),   \
    long:               gp_lmax(x, y),   \
    long long:          gp_llmax(x, y),  \
    unsigned:           gp_umax(x, y),   \
    unsigned long:      gp_lumax(x, y),  \
    unsigned long long: gp_llumax(x, y), \
    float:              gp_fmaxf(x, y),  \
    double:             gp_fmax(x, y),   \
    long double:        gp_fmaxl(x, y))

#else // Non-GNU C99

// Not ideal but does the job
#define gp_generic_min(x, y) gp_fmin(x, y)
#define gp_generic_max(x, y) gp_fmax(x, y)

#endif // gp_min() and gp_max() implementations

#endif // GPC_UTILS_INCLUDED
