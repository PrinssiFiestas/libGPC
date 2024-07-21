// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

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

#ifdef __cplusplus
extern "C" {
#endif

struct gp_random_state
{
    uint64_t state;
    uint64_t inc;
};


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// Aligment of all pointers returned by any valid allocators
#ifndef GP_MEMORY_INCLUDED
#if __STDC_VERSION__ >= 201112L && !defined(_MSC_VER)
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
 * Always rounds up so gp_next_power_of_2(1 << n) == 1 << (n + 1).
 */
size_t   gp_next_power_of_2   (size_t);
uint32_t gp_next_power_of_2_32(uint32_t);
uint64_t gp_next_power_of_2_64(uint64_t);

/** Round number up to alignment boundary.
 * @p boundary must be a power of 2.
 * @return @p x if already aligned.
 */
inline uintptr_t gp_round_to_aligned(const uintptr_t x,const uintptr_t boundary)
{
    return x + (boundary - 1) - ((x - 1) & (boundary - 1));
}

#if !__cplusplus
#define gp_min(x, y) gp_generic_min(x, y)
#define gp_max(x, y) gp_generic_max(x, y)
#endif

inline bool gp_fapproxf(float x, float y, float max_relative_diff) {
    return fabsf(x - y) <= max_relative_diff * fmaxf(x, y);
}
inline bool gp_fapprox(double x, double y, double max_relative_diff) {
    return fabs(x - y) <= max_relative_diff * fmax(x, y);
}
inline bool gp_fapproxl(long double x, long double y, long double max_rel_diff){
    return fabsl(x - y) <= max_rel_diff * fmaxl(x, y);
}

#define GP_BREAKPOINT GP_BREAKPOINT_INTERNAL

// ----------------------------------------------------------------------------
// Random functions

typedef struct gp_random_state GPRandomState;

GPRandomState gp_new_random_state(uint64_t seed);
uint32_t gp_random      (GPRandomState*) GP_NONNULL_ARGS();
double   gp_frandom     (GPRandomState*) GP_NONNULL_ARGS();
int32_t  gp_random_range(GPRandomState*, int32_t min, int32_t max) GP_NONNULL_ARGS();


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


inline int                gp_imin(int x, int y)                                 { return x < y ? x : y; }
inline long               gp_lmin(long x, long y)                               { return x < y ? x : y; }
inline long long          gp_llmin(long long x, long long y)                    { return x < y ? x : y; }
inline unsigned           gp_umin(unsigned x, unsigned y)                       { return x < y ? x : y; }
inline unsigned long      gp_lumin(unsigned long x, unsigned long y)            { return x < y ? x : y; }
inline unsigned long long gp_llumin(unsigned long long x, unsigned long long y) { return x < y ? x : y; }

inline int                gp_imax(int x, int y)                                 { return x > y ? x : y; }
inline long               gp_lmax(long x, long y)                               { return x > y ? x : y; }
inline long long          gp_llmax(long long x, long long y)                    { return x > y ? x : y; }
inline unsigned           gp_umax(unsigned x, unsigned y)                       { return x > y ? x : y; }
inline unsigned long      gp_lumax(unsigned long x, unsigned long y)            { return x > y ? x : y; }
inline unsigned long long gp_llumax(unsigned long long x, unsigned long long y) { return x > y ? x : y; }

// gp_min() and gp_max() implementations
#if __GNUC__
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
#define gp_generic_min(X, Y) ((X) < (Y) ? (X) : (Y))
#define gp_generic_max(X, Y) ((X) > (Y) ? (X) : (Y))
#endif // gp_min() and gp_max() implementations

// Set breakpoint
#ifdef _WIN32
#define GP_BREAKPOINT_INTERNAL __debugbreak()
#elif defined(SIGTRAP)
#define GP_BREAKPOINT_INTERNAL raise(SIGTRAP)
#elif (__GNUC__ && __i386__) || (__GNUC__ && __x86_64__)
#define GP_BREAKPOINT_INTERNAL __asm__("int $3")
#else // no breakpoints for you
#define GP_BREAKPOINT_INTERNAL
#endif // breakpoint

#ifdef __cplusplus
} // extern "C"

template <typename T>
static inline T gp_max(T a, T b)
{
    return a > b ? a : b;
}
template <typename T>
static inline T gp_min(T a, T b)
{
    return a < b ? a : b;
}
#endif

#endif // GP_UTILS_INCLUDED
