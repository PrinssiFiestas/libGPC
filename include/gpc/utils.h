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

// Raw byte
typedef unsigned char gp_byte;

/** Round number up to the next power of 2.
 * Used to compute array sizes on reallocations, thus size_t.
 */
size_t gp_next_power_of_2(size_t x);

/** Compare raw memory.
 * @return true if the first @p count bytes of @p lhs and @p rhs are equal.
 */
bool gp_mem_eq(const void* lhs, const void* rhs, size_t count);

// Random functions. Note that these only work for 64 bit platforms for now.

// Random functions with global state

// ALWAYS seed!
void gp_g_random_seed(uint64_t seed);
uint32_t gp_g_random(void);
double gp_g_frandom(void);
int32_t gp_g_random_range(int32_t min, int32_t max);

// Random functions with local state

typedef struct gp_RandomState gp_RandomState;

// ALWAYS seed!
void gp_random_seed(gp_RandomState*, uint64_t seed);
uint32_t gp_random(gp_RandomState*);
double gp_frandom(gp_RandomState*);
int32_t gp_random_range(gp_RandomState*, int32_t min, int32_t max);

#if defined(__GNUC__) || __STDC_VERSION__ >= 201112L

#define gp_min(x, y) gp_generic_min(x, y)
#define gp_max(x, y) gp_generic_max(x, y)

#endif

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

// Generic approx for floats, doubles and long doubles.
#define gp_approx(x, y, tolerance) (fabs((x) - (y)) <= (tolerance))

bool gp_fapproxf(float x, float y, float tolerance);
bool gp_fapprox(double x, double y, double tolerance);
bool gp_fapproxl(long double x, long double y, long double tolerance);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

struct gp_RandomState
{
    uint64_t state;
    uint64_t inc;
};

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

#endif // gp_min() and gp_max() implementations

#endif // GPC_UTILS_INCLUDED
