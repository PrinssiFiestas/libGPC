// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file utils.h
 * Misc utilities
 */

#ifndef GP_UTILS_INCLUDED
#define GP_UTILS_INCLUDED 1

#include "attributes.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#ifdef _WIN32
#include <intrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


/** Round number up to the next power of 2.
 * Always rounds up so gp_next_power_of_2(1 << n) == 1 << (n + 1).
 */
size_t   gp_next_power_of_2   (size_t)   GP_NODISCARD;
uint32_t gp_next_power_of_2_32(uint32_t) GP_NODISCARD;
uint64_t gp_next_power_of_2_64(uint64_t) GP_NODISCARD;

/** Round number up to alignment boundary.
 * @p boundary must be a power of 2.
 * @return @p x if already aligned.
 */
GP_NODISCARD inline
uintptr_t gp_round_to_aligned(uintptr_t x, uintptr_t boundary)
{
    return x + (boundary - 1) - ((x - 1) & (boundary - 1));
}

#if !__cplusplus // C++ uses templates instead of macros.
#define gp_min(x, y) gp_generic_min(x, y)
#define gp_max(x, y) gp_generic_max(x, y)
#endif

/** Float comparison.
 * Use this instead of == to accommodate for floating point precision issues.
 */
GP_NODISCARD
static inline bool gp_approx(double a, double b, double max_relative_diff) {
    a = fabs(a); b = fabs(b);
    return fabs(a - b) <= max_relative_diff * fmax(a, b);
}
GP_NODISCARD
static inline bool gp_approxf(float a, float b, float max_relative_diff) {
    a = fabsf(a); b = fabsf(b);
    return fabsf(a - b) <= max_relative_diff * fmaxf(a, b);
}
#ifndef __COMPCERT__
GP_NODISCARD
static inline bool gp_approxl(long double a, long double b, long double max_rel_diff){
    a = fabsl(a); b = fabsl(b);
    return fabsl(a - b) <= max_rel_diff * fmaxl(a, b);
}
#endif

// ----------------------------------------------------------------------------
// Random number generator
// https://www.pcg-random.org/

/** PCG based random number generator.
 * Create a RNG object with gp_random_state() which you can use to generate
 * high quality random numbers with great performance.
 */
typedef struct gp_random_state
{
    uint64_t state;
    uint64_t inc;
} GPRandomState;

GPRandomState gp_random_state(uint64_t seed) GP_NODISCARD;
uint32_t gp_random      (GPRandomState*) GP_NONNULL_ARGS() GP_NODISCARD;
double   gp_frandom     (GPRandomState*) GP_NONNULL_ARGS() GP_NODISCARD;
int32_t  gp_random_range(GPRandomState*, int32_t min, int32_t max_non_inclusive) GP_NONNULL_ARGS() GP_NODISCARD;
void     gp_random_bytes(GPRandomState*, void* buffer, size_t buffer_size) GP_NONNULL_ARGS();


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


GP_NODISCARD static inline int                gp_imin(int x, int y)                                 { return x < y ? x : y; }
GP_NODISCARD static inline long               gp_lmin(long x, long y)                               { return x < y ? x : y; }
GP_NODISCARD static inline long long          gp_llmin(long long x, long long y)                    { return x < y ? x : y; }
GP_NODISCARD static inline unsigned           gp_umin(unsigned x, unsigned y)                       { return x < y ? x : y; }
GP_NODISCARD static inline unsigned long      gp_lumin(unsigned long x, unsigned long y)            { return x < y ? x : y; }
GP_NODISCARD static inline unsigned long long gp_llumin(unsigned long long x, unsigned long long y) { return x < y ? x : y; }
GP_NODISCARD static inline float              gp_fminf(float x, float y)                            { return x < y ? x : y; }
GP_NODISCARD static inline double             gp_fmin(double x, double y)                           { return x < y ? x : y; }
#ifndef __COMPCERT__
GP_NODISCARD static inline long double        gp_fminl(long double x, long double y)                { return x < y ? x : y; }
#endif
GP_NODISCARD
GP_NODISCARD static inline int                gp_imax(int x, int y)                                 { return x > y ? x : y; }
GP_NODISCARD static inline long               gp_lmax(long x, long y)                               { return x > y ? x : y; }
GP_NODISCARD static inline long long          gp_llmax(long long x, long long y)                    { return x > y ? x : y; }
GP_NODISCARD static inline unsigned           gp_umax(unsigned x, unsigned y)                       { return x > y ? x : y; }
GP_NODISCARD static inline unsigned long      gp_lumax(unsigned long x, unsigned long y)            { return x > y ? x : y; }
GP_NODISCARD static inline unsigned long long gp_llumax(unsigned long long x, unsigned long long y) { return x > y ? x : y; }
GP_NODISCARD static inline float              gp_fmaxf(float x, float y)                            { return x > y ? x : y; }
GP_NODISCARD static inline double             gp_fmax(double x, double y)                           { return x > y ? x : y; }
#ifndef __COMPCERT__
GP_NODISCARD static inline long double        gp_fmaxl(long double x, long double y)                { return x > y ? x : y; }
#endif

// gp_min() and gp_max() implementations
#if __GNUC__ && !defined(GP_PEDANTIC) && !defined(GPC_IMPLEMENTATION)
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
#elif defined(__COMPCERT__) // long double not supported
#define gp_generic_min(X, Y) \
_Generic(X, \
    int:                gp_imin  (X, Y), \
    long:               gp_lmin  (X, Y), \
    long long:          gp_llmin (X, Y), \
    unsigned:           gp_umin  (X, Y), \
    unsigned long:      gp_lumin (X, Y), \
    unsigned long long: gp_llumin(X, Y), \
    float:              gp_fminf (X, Y), \
    double:             gp_fmin  (X, Y))
#define gp_generic_max(X, Y) \
_Generic(X, \
    int:                gp_imax  (X, Y), \
    long:               gp_lmax  (X, Y), \
    long long:          gp_llmax (X, Y), \
    unsigned:           gp_umax  (X, Y), \
    unsigned long:      gp_lumax (X, Y), \
    unsigned long long: gp_llumax(X, Y), \
    float:              gp_fmaxf (X, Y), \
    double:             gp_fmax  (X, Y))
#else // Non-GNU C99
// Use assert() to detect multiple evaluation bugs e.g. pass i++. Other side
// effects cannot be detected, so beware! May also require #inclusion of
// gpc/assert.h, we cannot include here due to cyclical #include.
#define gp_generic_min(X, Y) \
( \
    gp_assert((X)==(X) && (Y)==(Y), "gp_min() must not have side effects."), \
    (X) < (Y) ? (X) : (Y) \
)
#define gp_generic_max(X, Y) \
( \
    gp_assert((X)==(X) && (Y)==(Y), "gp_max() must not have side effects."), \
    (X) > (Y) ? (X) : (Y) \
)
#endif // gp_min() and gp_max() implementations

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
