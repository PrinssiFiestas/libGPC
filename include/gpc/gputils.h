// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_UTILS_INCLUDED
#define GP_UTILS_INCLUDED 1

#include <gpc/gpassert.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

GP_NOINLINE GP_OPTIMIZE_NONE
void gp_launder_noinline(void**); ///< @private

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup utils Other Utilities
/// @code
/// #include <gpc/gputils.h>
/// @endcode
/// @{

/** Static C array length.
 *
 * @return number of elements in a C array.
 */
#  define gp_countof(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))

/** Round 32-bit number up to the next power of two.
 *
 * @return number rounded up to the *next* power of two meaning that numbers
 * that are already powers of two will be raised to the next power too.
 */
GP_NODISCARD GP_INLINE
uint32_t gp_next_power_of_two_32(uint32_t x)
{
    #if __GNUC__ && INT_MAX == INT32_MAX // pedantic size check due to clzg() not always available
    return x == 0 ? 1 : 1u << (64 - __builtin_clz(x));
    #else
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
    #endif
}
/** Round 64-bit number up to the next power of two.
 *
 * @return number rounded up to the *next* power of two meaning that numbers
 * that are already powers of two will be raised to the next power too.
 */
GP_NODISCARD GP_INLINE
uint64_t gp_next_power_of_two_64(uint64_t x)
{
    #if __GNUC__ && LLONG_MAX == INT64_MAX // pedantic size check due to clzg() not always available
    return x == 0 ? 1 : 1llu << (64 - __builtin_clzll(x));
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
 *
 * @return number rounded up to the *next* power of two meaning that numbers
 * that are already powers of two will be raised to the next power too.
 */
GP_NODISCARD GP_INLINE
size_t gp_next_power_of_two(size_t x)
{
    return sizeof x == sizeof(uint32_t) ?
        gp_next_power_of_two_32(x) : gp_next_power_of_two_64(x);
}

/** Round number up to alignment boundary.
 *
 * @a boundary must be a power of 2.
 * @return @a x if already aligned.
 */
GP_NODISCARD GP_INLINE
uintptr_t gp_round_to_aligned(uintptr_t x, uintptr_t boundary)
{
    gp_assume((boundary & (boundary-1)) == 0, "Alignment boundary must be a power of 2.");
    return x + (boundary - 1) - ((x - 1) & (boundary - 1));
}

#ifndef __cplusplus // C++ uses templates instead of macros.
/** Type generic min.
 *
 * If one of the arguments is an unsigned integral type, the other one has to be
 * an unsigned integral type as well. If one of the arguments is a pointer, the
 * other one has to be a pointer as well. Any other combinations are ok. This
 * restriction prevents bugs like `gp_min(0, x - y)` always returning zero for
 * any unsigned x.
 *
 * Unlike common generic min macros in C, this only evaluates multiple times if
 * strict C99 (using `-std=c99 -DGP_PEDANTIC` flags), otherwise arguments are
 */
#define gp_min(A, B) gp_generic_min(A, B)

/** Type generic max.
 *
 * If one of the arguments is an unsigned integral type, the other one has to be
 * an unsigned integral type as well. If one of the arguments is a pointer, the
 * other one has to be a pointer as well. Any other combinations are ok. This
 * restriction prevents bugs like `gp_max(0, x - y)` always returning x for any
 * unsigned x.
 *
 * Unlike common generic max macros in C, this only evaluates multiple times if
 * strict C99 (using `-std=c99 -DGP_PEDANTIC` flags), otherwise arguments are
 * evaluated exactly once.
 */
#define gp_max(A, B) gp_generic_max(A, B)
#endif

/** Type generic signed min.
 *
 * Converts both integer arguments (signed or unsigned) to their signed
 * equivalents, sign extends if other one is larger, and then computes the min
 * of the processed arguments. This is most useful when the other argument is 0
 * and the other arguments has subtraction on unsigned values to prevent
 * wraparound. Accepts any combination of integers with different signedness.
 * For type safety (signedness matching) use @ref gp_min().
 */
#define gp_signed_min(A, B) gp_generic_signed_min(A, B)

/** Type generic signed max.
 *
 * Converts both integer arguments (signed or unsigned) to their signed
 * equivalents, sign extends if other one is larger, and then computes the max
 * of the processed arguments. This is most useful when the other argument is 0
 * and the other arguments has subtraction on unsigned values to prevent
 * wraparound. Accepts any combination of integers with different signedness.
 * For type safety (signedness matching) use @ref gp_max().
 */
#define gp_signed_max(A, B) gp_generic_signed_max(A, B)

/** Float comparison.
 *
 * Use this instead of `==` to accommodate for floating point precision issues.
 */
GP_NODISCARD GP_INLINE
bool gp_approx(double a, double b, double max_relative_diff) {
    a = fabs(a);
    b = fabs(b);
    return fabs(a - b) <= max_relative_diff * fmax(a, b);
}
/** @copydoc gp_approx */
GP_NODISCARD GP_INLINE
bool gp_approxf(float a, float b, float max_relative_diff) {
    a = fabsf(a);
    b = fabsf(b);
    return fabsf(a - b) <= max_relative_diff * fmaxf(a, b);
}
#if defined(GP_HAS_LONG_DOUBLE) || defined(GP_DOXYGEN)
/** @copydoc gp_approx */
GP_NODISCARD GP_INLINE
bool gp_approxl(long double a, long double b, long double max_rel_diff) {
    a = fabsl(a);
    b = fabsl(b);
    return fabsl(a - b) <= max_rel_diff * fmaxl(a, b);
}
#endif

/** Number of trailing zero bits of a 32-bit number.
 *
 * If @a u is zero, the result is undefined.
 */
GP_NODISCARD GP_INLINE
size_t gp_trailing_zeros_32(uint32_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // NOTE: C23 stdc_trailing_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned)); // be pedantic and paranoid
    return __builtin_ctz(u); // note: generic ctz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    u &= -u;
    // u==0 is undefined with ctz(), we know it's not 0 anyway
    // size_t c = 32;
    // if (u) c--;
    size_t c = 31;
    if (u & 0x0000FFFF0000FFFF) c -= 16;
    if (u & 0x00FF00FF00FF00FF) c -=  8;
    if (u & 0x0F0F0F0F0F0F0F0F) c -=  4;
    if (u & 0x3333333333333333) c -=  2;
    if (u & 0x5555555555555555) c -=  1;
    return c;
    #endif
}

/** Number of trailing zero bits of a 64-bit number.
 *
 * If @a u is zero, the result is undefined.
 */
GP_NODISCARD GP_INLINE
size_t gp_trailing_zeros_64(uint64_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // NOTE: C23 stdc_trailing_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned long long)); // be pedantic and paranoid
    return __builtin_ctzll(u); // note: generic ctz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    u &= -u;
    // u==0 is undefined with ctz(), we know it's not 0 anyway
    // size_t c = 64;
    // if (u) c--;
    size_t c = 63;
    if (u & 0x00000000FFFFFFFF) c -= 32;
    if (u & 0x0000FFFF0000FFFF) c -= 16;
    if (u & 0x00FF00FF00FF00FF) c -=  8;
    if (u & 0x0F0F0F0F0F0F0F0F) c -=  4;
    if (u & 0x3333333333333333) c -=  2;
    if (u & 0x5555555555555555) c -=  1;
    return c;
    #endif
}

/** Number of leading zero bits of a 32-bit number.
 *
 * If @a u is zero, the result is undefined.
 */
GP_NODISCARD GP_INLINE
size_t gp_leading_zeros_32(uint32_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // NOTE: C23 stdc_leading_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned)); // be pedantic and paranoid
    return __builtin_clz(u); // note: generic clz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    uint32_t v = u;
    uint32_t r;
    uint32_t shift;
    r     = (v > 0xFFFF    ) << 4; v >>= r;
    shift = (v > 0xFF      ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF       ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3       ) << 1; v >>= shift; r |= shift;
                                                r |= (v >> 1);
    return 31 - r;
    #endif
}

/** Number of leading zero bits of a 64-bit number.
 *
 * If @a u is zero, the result is undefined.
 */
GP_NODISCARD GP_INLINE
size_t gp_leading_zeros_64(uint64_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // Note: C23 stdc_leading_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned long long)); // be pedantic and paranoid
    return __builtin_clzll(u); // note: generic clz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    uint64_t v = u;
    uint64_t r;
    uint64_t shift;
    r =     (v > 0xFFFFFFFF) << 5; v >>= r;
    shift = (v > 0xFFFF    ) << 4; v >>= shift; r |= shift;
    shift = (v > 0xFF      ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF       ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3       ) << 1; v >>= shift; r |= shift;
                                                r |= (v >> 1);
    return 63 - r;
    #endif
}

/** Detach pointer from its origin.
 *
 * Remove any information about @a ptr as seen by the compiler. Most notably
 * this means that the compiler will not be able to do aliasing analysis, which
 * can be useful when debugging aliasing bugs. This can be also used for some
 * advanced optimizations that would be allowed by the CPU and the ABI, but are
 * undefined by the C implementation.
 *
 * If you don't know what you're doing, then don't use this for anything else
 * than debugging. Detaching pointer from its origins inhibits optimizations,
 * so using it wrongly just degrades performance. Furthermore, using it to fix
 * undefined behavior (like strict aliasing violations) doesn't inherently fix
 * anything; it just confuses the compiler to make the program look valid. Only
 * use this in production code if you are well familiar with your target
 * architecture and compiler.
 *
 * For GCC and Clang, this produces no code in optimized builds. Otherwise, this
 * has function call overhead.
 *
 * @return detached @a ptr.
 */
GP_ALWAYS_INLINE void* gp_launder(void* ptr)
{
    // TODO FilC has added some inline assembly support, test if this works and
    // remove the condition if it does.
    #if defined(__GNUC__) && !defined(__FILC__)
    __asm__ volatile("" : "+r"(ptr));
    // note: MSVC doesn't have inline assembly for x64 or ARM (why Microsoft, why??).
    #else // can't avoid function call overhead, sorry!
    gp_launder_noinline(&ptr); // no-op
    #endif
    return ptr;
}

/** Sometimes detach pointer from its origin.
 *
 * Like @ref gp_launder(), except only launders when @ref gp_launder() produces
 * no code, otherwise this just casts to `void*`. Laundering is anyway hacky,
 * but this is even more unsafe, so this is even less recommended to use than
 * @ref gp_launder().
 *
 * For compilers that don't produce code, laundering is based on an opaque
 * function call. This is redundant if anyway passing the pointer to an
 * opaque function, which is when this macro is useful.
 */
#if defined(__GNUC__) && !defined(__FILC__)
#  define GP_LAUNDER_CAST(PTR) gp_launder(PTR)
#else
#  define GP_LAUNDER_CAST(PTR) ((void*)(PTR))
#endif

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
/// @cond

GP_NODISCARD static inline char               gp_minc(char x,  char y)                              { return x < y ? x : y; }
GP_NODISCARD static inline signed char        gp_minhhi(signed char x, signed char y)               { return x < y ? x : y; }
GP_NODISCARD static inline short              gp_minhi(short x, short y)                            { return x < y ? x : y; }
GP_NODISCARD static inline int                gp_mini(int x, int y)                                 { return x < y ? x : y; }
GP_NODISCARD static inline long               gp_minli(long x, long y)                              { return x < y ? x : y; }
GP_NODISCARD static inline long long          gp_minlli(long long x, long long y)                   { return x < y ? x : y; }
GP_NODISCARD static inline unsigned char      gp_minhhu(unsigned char x, unsigned char y)           { return x < y ? x : y; }
GP_NODISCARD static inline unsigned short     gp_minhu(unsigned short x, unsigned short y)          { return x < y ? x : y; }
GP_NODISCARD static inline unsigned           gp_minu(unsigned x, unsigned y)                       { return x < y ? x : y; }
GP_NODISCARD static inline unsigned long      gp_minlu(unsigned long x, unsigned long y)            { return x < y ? x : y; }
GP_NODISCARD static inline unsigned long long gp_minllu(unsigned long long x, unsigned long long y) { return x < y ? x : y; }
GP_NODISCARD static inline float              gp_minf(float x, float y)                             { return x < y ? x : y; }
GP_NODISCARD static inline double             gp_mind(double x, double y)                           { return x < y ? x : y; }
#ifndef __cplusplus
GP_NODISCARD static inline void*              gp_minp(const void* x, const void* y)                 { return (void*)((char*)x < (char*)y ? x : y); }
#endif
#ifdef GP_HAS_LONG_DOUBLE
GP_NODISCARD static inline long double        gp_minld(long double x, long double y)                { return x < y ? x : y; }
#endif
#ifdef GP_INT128_INCLUDED
GP_NODISCARD static inline  GPInt128           gp_mini128(GPInt128  x, GPInt128  y) { return gp_i128_less_than(x, y) ? x : y; }
GP_NODISCARD static inline  GPUInt128          gp_minu128(GPUInt128 x, GPUInt128 y) { return gp_u128_less_than(x, y) ? x : y; }
#endif
GP_NODISCARD static inline  char               gp_maxc(char x, char y)                               { return x > y ? x : y; }
GP_NODISCARD static inline  signed char        gp_maxhhi(signed char x, signed char y)               { return x > y ? x : y; }
GP_NODISCARD static inline  short              gp_maxhi(short x, short y)                            { return x > y ? x : y; }
GP_NODISCARD static inline  int                gp_maxi(int x, int y)                                 { return x > y ? x : y; }
GP_NODISCARD static inline  long               gp_maxli(long x, long y)                              { return x > y ? x : y; }
GP_NODISCARD static inline  long long          gp_maxlli(long long x, long long y)                   { return x > y ? x : y; }
GP_NODISCARD static inline  unsigned char      gp_maxhhu(unsigned char x, unsigned char y)           { return x > y ? x : y; }
GP_NODISCARD static inline  unsigned short     gp_maxhu(unsigned short x, unsigned short y)          { return x > y ? x : y; }
GP_NODISCARD static inline  unsigned           gp_maxu(unsigned x, unsigned y)                       { return x > y ? x : y; }
GP_NODISCARD static inline  unsigned long      gp_maxlu(unsigned long x, unsigned long y)            { return x > y ? x : y; }
GP_NODISCARD static inline  unsigned long long gp_maxllu(unsigned long long x, unsigned long long y) { return x > y ? x : y; }
GP_NODISCARD static inline  float              gp_maxf(float x, float y)                             { return x > y ? x : y; }
GP_NODISCARD static inline  double             gp_maxd(double x, double y)                           { return x > y ? x : y; }
#ifndef __cplusplus
GP_NODISCARD static inline  void*              gp_maxp(const void* x, const void* y)                 { return (void*)((char*)x > (char*)y ? x : y); }
#endif
#ifdef GP_HAS_LONG_DOUBLE
GP_NODISCARD static inline  long double        gp_maxld(long double x, long double y)                { return x > y ? x : y; }
#endif
#ifdef GP_INT128_INCLUDED
GP_NODISCARD static inline  GPInt128           gp_maxi128(GPInt128  x, GPInt128  y) { return gp_i128_greater_than(x, y) ? x : y; }
GP_NODISCARD static inline  GPUInt128          gp_maxu128(GPUInt128 x, GPUInt128 y) { return gp_u128_greater_than(x, y) ? x : y; }
#endif

// gp_min() and gp_max() implementations
#if defined(__GNUC__) && __STDC_VERSION__ >= 201112L && !defined(GP_PEDANTIC) && !defined(GPC_IMPLEMENTATION)
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
#  define gp_generic_signed_min(X, Y) ({ \
    __typeof__(X) _gp_min_X = (X); __typeof__(Y) _gp_min_Y = (Y); \
    GP_AS_SIGNED(_gp_min_X) < GP_AS_SIGNED(_gp_min_Y) ? GP_AS_SIGNED(_gp_min_X) : GP_AS_SIGNED(_gp_min_Y); \
})
#  define gp_generic_signed_max(X, Y) ({ \
    __typeof__(X) _gp_max_X = (X); __typeof__(Y) _gp_max_Y = (Y); \
    GP_AS_SIGNED(_gp_max_X) > GP_AS_SIGNED(_gp_max_Y) ? GP_AS_SIGNED(_gp_max_X) : GP_AS_SIGNED(_gp_max_Y); \
})
#elif defined(GP_HAS_C11_GENERIC)
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

#  define gp_generic_signed_min(X, Y) _Generic(GP_AS_SIGNED((X) + (Y)),         \
    signed char: gp_minhhi, short: gp_minhi, int: gp_mini, long: gp_minli, \
    long long: gp_minlli)(GP_AS_SIGNED(X), GP_AS_SIGNED(Y))

#  define gp_generic_signed_max(X, Y) _Generic(GP_AS_SIGNED((X) + (Y)),         \
    signed char: gp_maxhhi, short: gp_maxhi, int: gp_maxi, long: gp_maxli, \
    long long: gp_maxlli)(GP_AS_SIGNED(X), GP_AS_SIGNED(Y))
#else // C99
// Use assume() to detect multiple evaluation bugs e.g. pass i++. Other side
// effects cannot be detected, so beware!
#  define gp_generic_min(X, Y) \
( \
    gp_assume((X)==(X) && (Y)==(Y), "gp_min() must not have side effects."), \
    (X) < (Y) ? (X) : (Y) \
)
#  define gp_generic_max(X, Y) \
( \
    gp_assume((X)==(X) && (Y)==(Y), "gp_max() must not have side effects."), \
    (X) > (Y) ? (X) : (Y) \
)
#  define gp_generic_signed_min(X, Y) \
( \
    gp_assume((X)==(X) && (Y)==(Y), "gp_min() must not have side effects."), \
    GP_AS_SIGNED(X) < GP_AS_SIGNED(Y) ? GP_AS_SIGNED(X) : GP_AS_SIGNED(Y) \
)
#  define gp_generic_signed_max(X, Y) \
( \
    gp_assume((X)==(X) && (Y)==(Y), "gp_max() must not have side effects."), \
    GP_AS_SIGNED(X) > GP_AS_SIGNED(Y) ? GP_AS_SIGNED(X) : GP_AS_SIGNED(Y) \
)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

/// @endcond
#endif // GP_UTILS_INCLUDED
