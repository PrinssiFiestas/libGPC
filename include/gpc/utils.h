// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file utils.h
 * Misc utilities
 */

#ifndef GP_UTILS_INCLUDED
#define GP_UTILS_INCLUDED 1

#include "overload.h"
#include "attributes.h"
#include "assert.h"
#include <math.h>
#include <stdint.h>
#include <limits.h>
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


/** Round 32-bit number up to the next power of 2.
 * Always rounds up so 0 -> 1, 1 -> 2, 2 -> 4, etc.
 */
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
uint32_t gp_next_power_of_2_32(uint32_t x)
{
    #if __GNUC__ && INT_MAX == INT32_MAX // pedantic size check due to clzg() not always available
    return x == 0 ? 1 : 1 << (64 - __builtin_clz(x));
    #else
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
    #endif
}
/** Round 64-bit number up to the next power of 2.
 * Always rounds up so 0 -> 1, 1 -> 2, 2 -> 4, etc.
 */
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
uint64_t gp_next_power_of_2_64(uint64_t x)
{
    #if __GNUC__ && LLONG_MAX == INT64_MAX // pedantic size check due to clzg() not always available
    return x == 0 ? 1 : 1 << (64 - __builtin_clzll(x));
    #else
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
    #endif
}
/** Round number of type size_t up to the next power of 2.
 * Always rounds up so 0 -> 1, 1 -> 2, 2 -> 4, etc.
 */
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
size_t gp_next_power_of_2(size_t x)
{
    return sizeof x == sizeof(uint32_t) ?
        gp_next_power_of_2_32(x) : gp_next_power_of_2_64(x);
}

/** Round number up to alignment boundary.
 * @p boundary must be a power of 2.
 * @return @p x if already aligned.
 */
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
uintptr_t gp_round_to_aligned(uintptr_t x, uintptr_t boundary)
{
    #if !__cplusplus // constexpr prevents using gp_assert()
    gp_db_assert((boundary & (boundary-1)) == 0, "Alignment boundary must be a power of 2.");
    #endif
    return x + (boundary - 1) - ((x - 1) & (boundary - 1));
}

#if !__cplusplus // C++ uses templates instead of macros.
/** Type generic min().
 * If one of the arguments is an unsigned integral type, the other one has to be
 * an unsigned integral type as well. If one of the arguments is a pointer, the
 * other one has to be a pointer as well. Any other combinations are ok. This
 * restriction prevents bugs like gp_min(0, x) always returning zero for any
 * unsigned x.
 */
#define gp_min(A, B) gp_generic_min(A, B)

/** Type generic max().
 * If one of the arguments is an unsigned integral type, the other one has to be
 * an unsigned integral type as well. If one of the arguments is a pointer, the
 * other one has to be a pointer as well. Any other combinations are ok. This
 * restriction prevents bugs like gp_max(0, x) always returning x for any
 * unsigned x.
 */
#define gp_max(A, B) gp_generic_max(A, B)
#endif

/** Type generic signed min().
 * Converts both integer arguments (signed or unsigned) to their signed
 * equivalents, sign extends if other one is larger, and then computes the min()
 * of the processed arguments. This is most useful when the other argument is 0
 * and the other arguments has subtraction on unsigned values to prevent
 * wraparound. Accepts any combination of integers with different signedness.
 * For type safety (signedness matching) use @ref gp_min().
 */
#define gp_imin(A, B) gp_generic_imin(A, B)

/** Type generic signed max().
 * Converts both integer arguments (signed or unsigned) to their signed
 * equivalents, sign extends if other one is larger, and then computes the max()
 * of the processed arguments. This is most useful when the other argument is 0
 * and the other arguments has subtraction on unsigned values to prevent
 * wraparound. Accepts any combination of integers with different signedness.
 * For type safety (signedness matching) use @ref gp_max().
 */
#define gp_imax(A, B) gp_generic_imax(A, B)

/** Float comparison.
 * Use this instead of == to accommodate for floating point precision issues.
 */
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
bool gp_approx(double a, double b, double max_relative_diff) {
    a = fabs(a); b = fabs(b);
    return fabs(a - b) <= max_relative_diff * fmax(a, b);
}
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
bool gp_approxf(float a, float b, float max_relative_diff) {
    a = fabsf(a); b = fabsf(b);
    return fabsf(a - b) <= max_relative_diff * fmaxf(a, b);
}
#if GP_HAS_LONG_DOUBLE
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
bool gp_approxl(long double a, long double b, long double max_rel_diff) {
    a = fabsl(a); b = fabsl(b);
    return fabsl(a - b) <= max_rel_diff * fmaxl(a, b);
}
#endif

/** Cast integer to it's equivalent signed type.
 * Most useful for casting unsigned types to signed types for comparisons close
 * to zero. This improves type safety compared to cast by converting to an
 * integer of inferred size making the conversion typedef agnostic. It also
 * guarantees that `gp_as_signed(a - b) < 0` will always return 1 for all b > a.
 * If C99, the return value cannot be inferred and will be long long, but the
 * comparison property will still hold.
 */
#define gp_as_signed(X) GP_AS_SIGNED(X)

// ----------------------------------------------------------------------------
// Random number generator

/** Random Number Generator.
 * A PGC https://www.pcg-random.org/ based RNG. Superior to most RNGs in almost
 * every way (e.g. performance, statistical quality, size). Must be initialized
 * with @ref gp_random_state() or @ref gp_random_state_seed().
 */
typedef struct gp_random_state
{
    uint64_t state;                  /** @private */
    uint64_t inc;                    /** @private */
    uint32_t coin_flip_cache_bits;   /** @private */
    uint32_t coin_flip_cache_length; /** @private */
} GPRandomState;

/** Create a Random Number Generator.
 * The RNG will be seeded with /dev/random on Unices, rand_s() on Windows,
 * or some unspecified values on other systems. If you need deterministic
 * random number sequence or need to avoid initialization costs, use
 * @ref gp_random_state_seed() instead.
 */
GPRandomState gp_random_state(void) GP_NODISCARD;

/** Create a seeded Random Number Generator.
 * For this generator, there are 2^63 possible sequences of pseudorandom
 * numbers. Each sequence is entirely distinct and has a period of 2^64. The low
 * 63 bits of @p stream_id selects which stream you will use.
 * @p init_state specifies where you are in that 2^64 period. Calling this with
 * the same arguments produces the same output.
 *     For the most non-deterministic output, use @ref gp_random_state()
 * instead. One quick and dirty option to initialize unique RNGs is to pass
 * current time and the address of the RNG like so:
 * `GPRandomState rng = gp_random_state_seed(time(NULL), (uintptr_t)&rng)`
 */
GP_NODISCARD
GPRandomState gp_random_state_seed(uint64_t init_state, uint64_t stream_id);

/** Generate random integer.
 * Generates a pseudorandom uniformly distributed 32-bit unsigned integer. Do
 * not use modulus operator (%) for bounded random numbers, this is wrong with
 * all RNGs and will cause a bias. Use @ref gp_random_bound() or
 * @ref gp_random_range() instead.
 */
uint32_t gp_random(GPRandomState*) GP_NONNULL_ARGS() GP_NODISCARD;

/** Generate random float.
 * Generates a pseudorandom float f where 0.0 <= f < 1.0.
 */
double gp_frandom(GPRandomState*) GP_NONNULL_ARGS() GP_NODISCARD;

/** Generate bounded random integer.
 * Generates uniformly distributed bounded random integer, unlike using the
 * modulus operator, which would give bias.
 */
GP_NONNULL_ARGS() GP_NODISCARD
uint32_t gp_random_bound(GPRandomState*, uint32_t bound);

/** Generate random integer in given range.
 * Generates uniformly distributed bounded random integer i where
 * @p min <= i < @p max_non_inclusive.
 */
GP_NONNULL_ARGS() GP_NODISCARD
int32_t gp_random_range(GPRandomState*, int32_t min, int32_t max_non_inclusive);

/** Generate random bytes.
 * Fills @p buffer with @p buffer_size random bytes.
 */
GP_NONNULL_ARGS()
void gp_random_bytes(GPRandomState*, void* buffer, size_t buffer_size);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION char               gp_minc(char x,  char y)                              { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION signed char        gp_minhhi(signed char x, signed char y)               { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION short              gp_minhi(short x, short y)                            { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION int                gp_mini(int x, int y)                                 { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION long               gp_minli(long x, long y)                              { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION long long          gp_minlli(long long x, long long y)                   { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned char      gp_minhhu(unsigned char x, unsigned char y)           { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned short     gp_minhu(unsigned short x, unsigned short y)          { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned           gp_minu(unsigned x, unsigned y)                       { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned long      gp_minlu(unsigned long x, unsigned long y)            { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned long long gp_minllu(unsigned long long x, unsigned long long y) { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION float              gp_minf(float x, float y)                             { return x < y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION double             gp_mind(double x, double y)                           { return x < y ? x : y; }
#if !__cplusplus
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION void*              gp_minp(const void* x, const void* y)                 { return (void*)((char*)x < (char*)y ? x : y); }
#endif
#ifndef __COMPCERT__
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION long double        gp_minld(long double x, long double y)                { return x < y ? x : y; }
#endif
#if GP_INT128_INCLUDED
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION GPInt128           gp_mini128(GPInt128  x, GPInt128  y) { return gp_i128_less_than(x, y) ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION GPUInt128          gp_minu128(GPUInt128 x, GPUInt128 y) { return gp_u128_less_than(x, y) ? x : y; }
#endif
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION char               gp_maxc(char x, char y)                               { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION signed char        gp_maxhhi(signed char x, signed char y)               { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION short              gp_maxhi(short x, short y)                            { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION int                gp_maxi(int x, int y)                                 { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION long               gp_maxli(long x, long y)                              { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION long long          gp_maxlli(long long x, long long y)                   { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned char      gp_maxhhu(unsigned char x, unsigned char y)           { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned short     gp_maxhu(unsigned short x, unsigned short y)          { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned           gp_maxu(unsigned x, unsigned y)                       { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned long      gp_maxlu(unsigned long x, unsigned long y)            { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION unsigned long long gp_maxllu(unsigned long long x, unsigned long long y) { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION float              gp_maxf(float x, float y)                             { return x > y ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION double             gp_maxd(double x, double y)                           { return x > y ? x : y; }
#if !__cplusplus
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION void*              gp_maxp(const void* x, const void* y)                 { return (void*)((char*)x > (char*)y ? x : y); }
#endif
#ifndef __COMPCERT__
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION long double        gp_maxld(long double x, long double y)                { return x > y ? x : y; }
#endif
#if GP_INT128_INCLUDED
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION GPInt128           gp_maxi128(GPInt128  x, GPInt128  y) { return gp_i128_greater_than(x, y) ? x : y; }
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION GPUInt128          gp_maxu128(GPUInt128 x, GPUInt128 y) { return gp_u128_greater_than(x, y) ? x : y; }
#endif

// gp_min() and gp_max() implementations
#if __GNUC__ && __STDC_VERSION__ >= 201112L && !defined(GP_PEDANTIC) && !defined(GPC_IMPLEMENTATION)
#  define gp_generic_min(X, Y) ({ \
    _Static_assert(GP_IS_UNSIGNED(X) == GP_IS_UNSIGNED(Y), \
        "Signedness of gp_min() arguments must match."); \
    __typeof__(X) _gp_min_X = (X); __typeof__(Y) _gp_min_Y = (Y); \
    _gp_min_X < _gp_min_Y ? _gp_min_X : _gp_min_Y; \
})
#  define gp_generic_max(X, Y) ({ \
    _Static_assert(GP_IS_UNSIGNED(X) == GP_IS_UNSIGNED(Y), \
        "Signedness of gp_min() arguments must match."); \
    __typeof__(X) _gp_max_X = (X); __typeof__(Y) _gp_max_Y = (Y); \
    _gp_max_X > _gp_max_Y ? _gp_max_X : _gp_max_Y; \
})
#  define gp_generic_imin(X, Y) ({ \
    __typeof__(X) _gp_min_X = (X); __typeof__(Y) _gp_min_Y = (Y); \
    GP_AS_SIGNED(_gp_min_X) < GP_AS_SIGNED(_gp_min_Y) ? GP_AS_SIGNED(_gp_min_X) : GP_AS_SIGNED(_gp_min_Y); \
})
#  define gp_generic_imax(X, Y) ({ \
    __typeof__(X) _gp_max_X = (X); __typeof__(Y) _gp_max_Y = (Y); \
    GP_AS_SIGNED(_gp_max_X) > GP_AS_SIGNED(_gp_max_Y) ? GP_AS_SIGNED(_gp_max_X) : GP_AS_SIGNED(_gp_max_Y); \
})
#elif GP_HAS_C11_GENERIC
#  define gp_generic_min(A, B) \
( \
    !sizeof(bool[1-2*(GP_IS_UNSIGNED(A)!=GP_IS_UNSIGNED(B))]) ? (A)+(B) : \
    _Generic((A)+(B), \
        GP_LONG_DOUBLE_SELECTION(gp_minld,) \
        GP_DOUBLE_SELECTION(gp_mind,)       \
        GP_TETRA_UINT_SELECTION(gp_mintu,)  \
        GP_TETRA_INT_SELECTION(gp_minti,)   \
        GP_CHAR_SELECTION(gp_minc,)         \
        signed char: gp_minhhi, unsigned char     : gp_minhhu, \
        short      : gp_minhi , unsigned short    : gp_minhu , \
        int        : gp_mini  , unsigned int      : gp_minu  , \
        long       : gp_minli , unsigned long     : gp_minlu , \
        long long  : gp_minlli, unsigned long long: gp_minllu, \
        float      : gp_minf)(A, B) \
)
#  define gp_generic_max(A, B) \
( \
    !sizeof(bool[1-2*(GP_IS_UNSIGNED(A)!=GP_IS_UNSIGNED(B))]) ? (A)+(B) : \
    _Generic((A)+(B), \
        GP_LONG_DOUBLE_SELECTION(gp_maxld,) \
        GP_DOUBLE_SELECTION(gp_maxd,)       \
        GP_TETRA_UINT_SELECTION(gp_maxtu,)  \
        GP_TETRA_INT_SELECTION(gp_maxti,)   \
        GP_CHAR_SELECTION(gp_maxc,)         \
        signed char: gp_maxhhi, unsigned char     : gp_maxhhu, \
        short      : gp_maxhi , unsigned short    : gp_maxhu , \
        int        : gp_maxi  , unsigned int      : gp_maxu  , \
        long       : gp_maxli , unsigned long     : gp_maxlu , \
        long long  : gp_maxlli, unsigned long long: gp_maxllu, \
        float      : gp_maxf)(A, B) \
)

#  define gp_generic_imin(X, Y) _Generic(GP_AS_SIGNED((X) + (Y)),         \
    signed char: gp_minhhi, short: gp_minhi, int: gp_mini, long: gp_minli, \
    long long: gp_minlli)(GP_AS_SIGNED(X), GP_AS_SIGNED(Y))

#  define gp_generic_imax(X, Y) _Generic(GP_AS_SIGNED((X) + (Y)),         \
    signed char: gp_maxhhi, short: gp_maxhi, int: gp_maxi, long: gp_maxli, \
    long long: gp_maxlli)(GP_AS_SIGNED(X), GP_AS_SIGNED(Y))
#else
// Use assert() to detect multiple evaluation bugs e.g. pass i++. Other side
// effects cannot be detected, so beware!
#  define gp_generic_min(X, Y) \
( \
    gp_assert((X)==(X) && (Y)==(Y), "gp_min() must not have side effects."), \
    (X) < (Y) ? (X) : (Y) \
)
#  define gp_generic_max(X, Y) \
( \
    gp_assert((X)==(X) && (Y)==(Y), "gp_max() must not have side effects."), \
    (X) > (Y) ? (X) : (Y) \
)
#  define gp_generic_imin(X, Y) \
( \
    gp_assert((X)==(X) && (Y)==(Y), "gp_min() must not have side effects."), \
    GP_AS_SIGNED(X) < GP_AS_SIGNED(Y) ? GP_AS_SIGNED(X) : GP_AS_SIGNED(Y) \
)
#  define gp_generic_imax(X, Y) \
( \
    gp_assert((X)==(X) && (Y)==(Y), "gp_max() must not have side effects."), \
    GP_AS_SIGNED(X) > GP_AS_SIGNED(Y) ? GP_AS_SIGNED(X) : GP_AS_SIGNED(Y) \
)
#endif

#if GP_HAS_C11_GENERIC

#define GP_AS_SIGNED(X) _Generic(X,                         \
    GP_CHAR_SELECTION((signed char)(X),)                    \
    GP_TETRA_UINT_SELECTION((gp_tetra_int_t)(X),)           \
    GP_TETRA_INT_SELECTION((X),)                            \
    unsigned char     : (signed char)(X), signed char: (X), \
    unsigned short    : (short)(X)      , short      : (X), \
    unsigned int      : (int)(X)        , int        : (X), \
    unsigned long     : (long)(X)       , long       : (X), \
    unsigned long long: (long long)(X)  , long long  : (X))

#elif !__cplusplus

#define GP_AS_SIGNED(X) \
( \
    sizeof(X) == 1 ? (int8_t )(X) : \
    sizeof(X) == 2 ? (int16_t)(X) : \
    sizeof(X) == 4 ? (int32_t)(X) : \
    sizeof(X) == 8 ? (int64_t)(X) : \
    (intmax_t)(X)                   \
)

#else // __cplusplus
} // extern "C"

template <typename T0, typename T1>
GP_NODISCARD static inline constexpr
typename std::enable_if<
    std::is_unsigned<T0>::value == std::is_unsigned<T1>::value,
    decltype(T0{0} + T1{0})>::type
gp_min(T0 a, T1 b) {return a < b ? a : b; }

template <typename T0, typename T1>
GP_NODISCARD static inline constexpr
typename std::enable_if<
    std::is_unsigned<T0>::value == std::is_unsigned<T1>::value,
    decltype(T0{0} + T1{0})>::type
gp_max(T0 a, T1 b) {return a > b ? a : b; }

template <typename T> GP_NODISCARD static inline constexpr T* gp_minp(T* a, T* b) { return a < b ? a : b; }
template <typename T> GP_NODISCARD static inline constexpr T* gp_maxp(T* a, T* b) { return a > b ? a : b; }

static inline constexpr signed char    GP_AS_SIGNED(unsigned char      x) { return x; }
static inline constexpr short          GP_AS_SIGNED(unsigned short     x) { return x; }
static inline constexpr int            GP_AS_SIGNED(unsigned int       x) { return x; }
static inline constexpr long           GP_AS_SIGNED(unsigned long      x) { return x; }
static inline constexpr long long      GP_AS_SIGNED(unsigned long long x) { return x; }
#  if GP_HAS_TETRA_INT
static inline constexpr gp_tetra_int_t GP_AS_SIGNED(gp_tetra_uint_t    x) { return x; }
#  endif

template <typename T0, typename T1> constexpr
decltype(GP_AS_SIGNED(T0{0} + T1{0})) gp_imin(T0 a, T1 b) { GP_AS_SIGNED(a) < GP_AS_SIGNED(b) ? GP_AS_SIGNED(a) : GP_AS_SIGNED(b); }

template <typename T0, typename T1> constexpr
decltype(GP_AS_SIGNED(T0{0} + T1{0})) gp_imax(T0 a, T1 b) { GP_AS_SIGNED(a) > GP_AS_SIGNED(b) ? GP_AS_SIGNED(a) : GP_AS_SIGNED(b); }

#endif // GP_HAS_C11_GENERIC and extern "C"

#endif // GP_UTILS_INCLUDED
