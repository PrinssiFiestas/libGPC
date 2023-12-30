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
#include <stdint.h>
#include <stdbool.h>
#include <tgmath.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Signed size_t
typedef ptrdiff_t gpc_ssize;

// Raw byte
typedef unsigned char gpc_byte;

/// Round number up to the next power of 2
/** Used to compute array sizes on reallocations, thus size_t. */
size_t gpc_next_power_of_2(size_t x);

/// Compare raw memory
/**
 * @return true if the first @p count bytes of @p lhs and @p rhs are equal.
 */
bool gpc_mem_eq(const void* lhs, const void* rhs, size_t count);

// Random functions. Note that these only work for 64 bit platforms for now.

// Random functions with global state

// ALWAYS seed!
void gpc_g_random_seed(uint64_t seed);
uint32_t gpc_g_random(void);
double gpc_g_frandom(void);
int32_t gpc_g_random_range(int32_t min, int32_t max);

// Random functions with local state

typedef struct gpc_RandomState gpc_RandomState;

// ALWAYS seed!
void gpc_random_seed(gpc_RandomState*, uint64_t seed);
uint32_t gpc_random(gpc_RandomState*);
double gpc_frandom(gpc_RandomState*);
int32_t gpc_random_range(gpc_RandomState*, int32_t min, int32_t max);

#if defined(__GNUC__) || __STDC_VERSION__ >= 201112L

#define gpc_min(x, y) gpc_generic_min(x, y)
#define gpc_max(x, y) gpc_generic_max(x, y)

#endif

int                gpc_imin(int x, int y);
long               gpc_lmin(long x, long y);
long long          gpc_llmin(long long x, long long y);
unsigned           gpc_umin(unsigned x, unsigned y);
unsigned long      gpc_lumin(unsigned long x, unsigned long y);
unsigned long long gpc_llumin(unsigned long long x, unsigned long long y);

int                gpc_imax(int x, int y);
long               gpc_lmax(long x, long y);
long long          gpc_llmax(long long x, long long y);
unsigned           gpc_umax(unsigned x, unsigned y);
unsigned long      gpc_lumax(unsigned long x, unsigned long y);
unsigned long long gpc_llumax(unsigned long long x, unsigned long long y);

// Generic approx for floats, doubles and long doubles.
#define gpc_approx(x, y, tolerance) (fabs((x) - (y)) <= (tolerance))

bool gpc_fapproxf(float x, float y, float tolerance);
bool gpc_fapprox(double x, double y, double tolerance);
bool gpc_fapproxl(long double x, long double y, long double tolerance);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

struct gpc_RandomState
{
    uint64_t state;
    uint64_t inc;
};

#if defined(__GNUC__) // gpc_min() and gpc_max() implementations

#define gpc_generic_min(x, y) \
({ typeof(x) _##x = (x); typeof(y) _##y = (y); _##x < _##y ? _##x : _##y; })

#define gpc_generic_max(x, y) \
({ typeof(x) _##x = (x); typeof(y) _##y = (y); _##x > _##y ? _##x : _##y; })

#elif __STDC_VERSION__ >= 201112L

#define gpc_generic_min(x, y) \
_Generic(x, \
int:                gpc_imin(x, y),   \
long:               gpc_lmin(x, y),   \
long long:          gpc_llmin(x, y),  \
unsigned:           gpc_umin(x, y),   \
unsigned long:      gpc_lumin(x, y),  \
unsigned long long: gpc_llumin(x, y), \
float:              gpc_fminf(x, y),  \
double:             gpc_fmin(x, y),   \
long double:        gpc_fminl(x, y))

#define gpc_generic_max(x, y) \
_Generic(x, \
int:                gpc_imax(x, y),   \
long:               gpc_lmax(x, y),   \
long long:          gpc_llmax(x, y),  \
unsigned:           gpc_umax(x, y),   \
unsigned long:      gpc_lumax(x, y),  \
unsigned long long: gpc_llumax(x, y), \
float:              gpc_fmaxf(x, y),  \
double:             gpc_fmax(x, y),   \
long double:        gpc_fmaxl(x, y))

#endif // gpc_min() and gpc_max() implementations

#endif // GPC_UTILS_INCLUDED
