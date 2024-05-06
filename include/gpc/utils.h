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

#ifndef GP_UTILS_INCLUDED
#define GP_UTILS_INCLUDED 1

#include "attributes.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <signal.h>

#ifdef _WIN32
#include <intrin.h>
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Aligment of all pointers returned by any valid allocators
#ifndef GP_MEMORY_INCLUDED
#if __STDC_VERSION__ >= 201112L
#define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#else
#define GP_ALLOC_ALIGNMENT (sizeof(long double))
#endif
#endif

/** Check and limit upper and lower bounds at once.
 * @p end will be limited to @p limit and @p start will be limited to @p end and
 * @p limit.
 * @return true if arguments are in bounds and @p end > @p start.
 */
bool gp_check_bounds(
    size_t* optional_start_non_inclusive,
    size_t* optional_end_inclusive,
    size_t  limit);

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

inline uintptr_t gp_round_to_aligned(const uintptr_t x)
{
    return x + (GP_ALLOC_ALIGNMENT - 1) - ((x - 1) % GP_ALLOC_ALIGNMENT);
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

/** Compare raw memory. */
inline bool gp_mem_equal(
    const void* lhs,
    const void* rhs,
    size_t lhs_size,
    size_t rhs_size)
{
    if (lhs_size != rhs_size)
        return false;
    int memcmp(const void*, const void*, size_t);
    return memcmp(lhs, rhs, lhs_size) == 0;
}

#define GP_BREAKPOINT            GP_BREAKPOINT_INTERNAL

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

// gp_min() and gp_max() implementations
#if defined(__GNUC__)

#define gp_generic_min(X, Y) ({ \
    typeof(X) _gp_min_X = (X); typeof(Y) _gp_min_Y = (Y); \
    _gp_min_X < _gp_min_Y ? _gp_min_X : _gp_min_Y; \
})

#define gp_generic_max(X, Y) ({ \
    typeof(X) _gp_max_X = (X); typeof(Y) _gp_max_Y = (Y); \
    _gp_max_X > _gp_max_Y ? _gp_max_X : _gp_max_Y; \
})

#elif __STDC_VERSION__ >= 201112L

#define gp_generic_min(X, Y) \
_Generic(X, \
    int:                gp_imin  (X, Y), \
    long:               gp_lmin  (X, Y), \
    long long:          gp_llmin (X, Y), \
    unsigned:           gp_umin  (X, Y), \
    unsigned long:      gp_lumin (X, Y), \
    unsigned long long: gp_llumin(X, Y), \
    float:              gp_fminf (X, Y), \
    double:             gp_fmin  (X, Y), \
    long double:        gp_fminl (X, Y))

#define gp_generic_max(X, Y) \
_Generic(X, \
    int:                gp_imax  (X, Y), \
    long:               gp_lmax  (X, Y), \
    long long:          gp_llmax (X, Y), \
    unsigned:           gp_umax  (X, Y), \
    unsigned long:      gp_lumax (X, Y), \
    unsigned long long: gp_llumax(X, Y), \
    float:              gp_fmaxf (X, Y), \
    double:             gp_fmax  (X, Y), \
    long double:        gp_fmaxl (X, Y))

#else // Non-GNU C99

// Not ideal but does the job
#define gp_generic_min(x, y) gp_fmin(x, y)
#define gp_generic_max(x, y) gp_fmax(x, y)

#endif // gp_min() and gp_max() implementations

// Set breakpoint
#ifdef _WIN32
#define GP_BREAKPOINT_INTERNAL __debugbreak()
#elif defined(SIGTRAP)
#define GP_BREAKPOINT_INTERNAL raise(SIGTRAP)
#else // no breakpoints for you
#define GP_BREAKPOINT_INTERNAL
#endif // breakpoint

#endif // GP_UTILS_INCLUDED
