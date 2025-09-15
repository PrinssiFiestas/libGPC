// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file int128.h
 * Portable 128 bit integer
 */

// Note to everybody: Compiler Explorer is your friend.

// TODO generic macros and operator overloads are not well tested, TEST THEM!

#ifndef GP_INT128_INCLUDED
#define GP_INT128_INCLUDED 1

#include "attributes.h"
#include "overload.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#if _MSC_VER && defined(_M_X64)
#include <intrin.h>
#pragma intrinsic (__shiftleft128, __shiftright128, _umul128, _mul128)
#endif

#if __cplusplus
#include <type_traits>
#endif

// Can be defined by user to enable some features
#if __STDC_VERSION__ >= 201112L && !defined(GP_HAS_ANONYMOUS_STRUCT)
#define GP_HAS_ANONYMOUS_STRUCT 1
#endif

#if (__GNUC__ && defined(__SIZEOF_INT128__)) || __clang__ || GP_TEST_INT128
#  ifndef GP_TEST_INT128
#    define GP_HAS_TETRA_INT 1
#  else
#    include <limits.h>
#  endif
/** __uint128_t but more portable with Clang */
typedef unsigned gp_tetra_uint_t __attribute__((mode(TI)));
/** __int128_t but more portable with Clang */
typedef int      gp_tetra_int_t  __attribute__((mode(TI)));
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Endianness

#define GP_ENDIAN_LITTLE 1
#define GP_ENDIAN_BIG    2

// Preprocessor endianness check from RapidJSON with added check for C23
// standard endianness macros. If detected, GP_ENDIAN is defined to
// GP_ENDIAN_LITTLE, GP_ENDIAN_BIG, or nothing in case of mixed endianness.
// Undetected endianness leaves GP_ENDIAN undefined. GP_ENDIAN can be user
// defined to GP_ENDIAN_LITTLE or GP_ENDIAN_BIG. Unlike RapidJSON, undetected
// endianness will not #error since endianness can still be detected at runtime
// with gp_is_big_endian() and gp_is_little_endian().
#ifndef GP_ENDIAN
// Detect with C23. stdbit.h is missing during time of writing even with
// -std=c23. We can still check the macro, but do NOT include the header, even
// if the header pops up to support older libc versions.
#  ifdef __STDC_ENDIAN_NATIVE__
#    if __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__
#      define GP_ENDIAN GP_ENDIAN_BIG
#    elif
#      define GP_ENDIAN // mixed
#    endif // __STDC_ENDIAN_NATIVE
// Detect with GCC 4.6's macro
#  elif defined(__BYTE_ORDER__)
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define GP_ENDIAN GP_ENDIAN_BIG
#    endif // __BYTE_ORDER__
// Detect with GLIBC's endian.h
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define GP_ENDIAN GP_ENDIAN_BIG
#    else
#      define GP_ENDIAN // mixed
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define GP_ENDIAN GP_ENDIAN_BIG
#  elif defined(_LITTLE_ENDIAN) && defined(_BIG_ENDIAN)
#    define GP_ENDIAN // mixed
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define GP_ENDIAN GP_ENDIAN_BIG
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  endif
#endif // GP_ENDIAN

#if GP_ENDIAN == GP_ENDIAN_LITTLE
#  define gp_is_big_endian()    0
#  define gp_is_little_endian() 1
#elif GP_ENDIAN == GP_ENDIAN_BIG
#  define gp_is_big_endian()    1
#  define gp_is_little_endian() 0
#elif defined(GP_ENDIAN) // mixed endianness
#  define gp_is_big_endian()    0
#  define gp_is_little_endian() 0
#else
/** Run-time check if system is big endian.*/
GP_NODISCARD GP_CONSTEXPR_FUNCTION
static inline bool gp_is_big_endian(void)
{
    union Endianness {
        uint16_t u16;
        struct { uint8_t is_little; uint8_t is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_big;
}
/** Run-time check if system is little endian.*/
GP_NODISCARD GP_CONSTEXPR_FUNCTION
static inline bool gp_is_little_endian(void)
{
    union Endianness {
        uint16_t u16;
        struct { uint8_t is_little; uint8_t is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_little;
}
#endif // GP_ENDIAN

// ----------------------------------------------------------------------------
// 128-bit Integer Types

/** 128-bit unsigned integer.*/
typedef union gp_uint128
{
    #if GP_HAS_ANONYMOUS_STRUCT && GP_ENDIAN == GP_ENDIAN_LITTLE
    struct {
        uint64_t lo;
        uint64_t hi;
    };
    #elif GP_HAS_ANONYMOUS_STRUCT && GP_ENDIAN == GP_ENDIAN_BIG
    struct {
        uint64_t hi;
        uint64_t lo;
    };
    #endif

    struct {
        uint64_t lo;
        uint64_t hi;
    } little_endian;

    struct {
        uint64_t hi;
        uint64_t lo;
    } big_endian;

    #if GP_HAS_TETRA_INT || GP_TEST_INT128
    gp_tetra_uint_t u128;
    #endif
} GPUInt128;

/** 128-bit signed integer.
 * Overflow is undefined.
 */
typedef union gp_int128
{
    #if GP_HAS_ANONYMOUS_STRUCT && GP_ENDIAN == GP_ENDIAN_LITTLE
    struct {
        uint64_t lo;
        int64_t  hi;
    };
    #elif GP_HAS_ANONYMOUS_STRUCT && GP_ENDIAN == GP_ENDIAN_BIG
    struct {
        int64_t  hi;
        uint64_t lo;
    };
    #endif

    struct {
        uint64_t lo;
        int64_t  hi;
    } little_endian;

    struct {
        int64_t  hi;
        uint64_t lo;
    } big_endian;

    #if GP_HAS_TETRA_INT || defined(GP_TEST_INT128)
    gp_tetra_int_t i128;
    #endif
} GPInt128;

// ----------------------------------------------------------------------------
// Limits

#define GP_UINT128_MAX gp_uint128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
#define GP_INT128_MAX  gp_int128(0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
#define GP_INT128_MIN  gp_int128(INT64_MIN, 0)

#define GP_TETRA_UINT_MAX ((gp_tetra_uint_t)-1)
#define GP_TETRA_INT_MAX  ((gp_tetra_int_t)((gp_tetra_uint_t)-1 >> 1))
#define GP_TETRA_INT_MIN  ((gp_tetra_int_t)-1 << 127)

// ----------------------------------------------------------------------------
// Constructors and Accessors

/** Create 128-bit unsigned integer.*/
GP_NODISCARD GP_CONSTEXPR_FUNCTION
static inline GPUInt128 gp_uint128(uint64_t hi_bits, uint64_t lo_bits)
{
    GPUInt128 u128;
    if (gp_is_big_endian()) {
        u128.big_endian.hi = hi_bits;
        u128.big_endian.lo = lo_bits;
    } else {
        u128.little_endian.hi = hi_bits;
        u128.little_endian.lo = lo_bits;
    }
    return u128;
}
/** Create 128-bit signed integer.*/
GP_NODISCARD GP_CONSTEXPR_FUNCTION
static inline GPInt128 gp_int128(int64_t hi_bits, uint64_t lo_bits)
{
    GPInt128 i128;
    if (gp_is_big_endian()) {
        i128.big_endian.hi = hi_bits;
        i128.big_endian.lo = lo_bits;
    } else {
        i128.little_endian.hi = hi_bits;
        i128.little_endian.lo = lo_bits;
    }
    return i128;
}

/** Convert 128-bit signed integer to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_i128(GPInt128 i)
{
    GPUInt128 u;
    memcpy(&u, &i, sizeof u);
    return u;
}
/** Convert 128-bit unsigned integer to 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_u128(GPUInt128 u)
{
    GPInt128 i;
    memcpy(&i, &u, sizeof i);
    return i;
}

/** Convert 64-bit unsigned integer to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_u64(uint64_t u) { return gp_uint128(0, u)     ; }
/** Convert sign extended 64-bit signed integer to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_i64(int64_t i)  { return gp_uint128(-(i<0), i); }
/** Convert 64-bit unsigned integer to 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_u64(uint64_t u)   { return gp_int128(0, u)      ; }
/** Convert sign extended 64-bit signed integer to 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_i64(int64_t i)    { return gp_int128(-(i<0), i) ; }

/** Get low bits of 128-bit unsigned integer.*/
GP_NODISCARD static inline uint64_t gp_uint128_lo(GPUInt128 u)
{
    return gp_is_little_endian() ? u.little_endian.lo : u.big_endian.lo;
}
/** Get low bits of 128-bit signed integer.*/
GP_NODISCARD static inline uint64_t gp_int128_lo(GPInt128 i)
{
    return gp_is_little_endian() ? i.little_endian.lo : i.big_endian.lo;
}

/** Get high bits of 128-bit unsigned integer.*/
GP_NODISCARD static inline uint64_t gp_uint128_hi(GPUInt128 u)
{
    return gp_is_little_endian() ? u.little_endian.hi : u.big_endian.hi;
}
/** Get signed high bits of 128-bit signed integer.*/
GP_NODISCARD static inline int64_t gp_int128_hi(GPInt128 i)
{
    return gp_is_little_endian() ? i.little_endian.hi : i.big_endian.hi;
}

/** Get address of low bits of 128-bit unsigned integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
static inline uint64_t* gp_uint128_lo_addr(GPUInt128* u)
{
    return gp_is_little_endian() ? &u->little_endian.lo : &u->big_endian.lo;
}
/** Get address of low bits of 128-bit signed integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
static inline uint64_t* gp_int128_lo_addr(GPInt128* i)
{
    return gp_is_little_endian() ? &i->little_endian.lo : &i->big_endian.lo;
}

/** Get address of high bits of 128-bit unsigned integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
static inline uint64_t* gp_uint128_hi_addr(GPUInt128* u)
{
    return gp_is_little_endian() ? &u->little_endian.hi : &u->big_endian.hi;
}
/** Get address of signed high bits of 128-bit signed integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
static inline int64_t* gp_int128_hi_addr(GPInt128* i)
{
    return gp_is_little_endian() ? &i->little_endian.hi : &i->big_endian.hi;
}

#if GP_HAS_TETRA_INT || defined(GP_TEST_INT128)
GP_NODISCARD static inline GPUInt128 gp_uint128_tetra_uint(gp_tetra_uint_t _u)
{
    GPUInt128 u;
    u.u128 = _u;
    return u;
}
GP_NODISCARD static inline GPInt128 gp_int128_tetra_int(gp_tetra_int_t _i)
{
    GPInt128 i;
    i.i128 = _i;
    return i;
}
#endif

GP_NODISCARD static inline GPUInt128 gp_uint128_f64(double d)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint((gp_tetra_uint_t)d);
    #else
    GPUInt128 gp_uint128_convert_f64(double);
    return gp_uint128_convert_f64(d);
    #endif
}
GP_NODISCARD static inline GPInt128 gp_int128_f64(double d)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int((gp_tetra_uint_t)d);
    #else
    GPInt128 gp_int128_convert_f64(double);
    return gp_int128_convert_f64(d);
    #endif
}
GP_NODISCARD static inline GPUInt128 gp_uint128_f32(float f)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint((gp_tetra_uint_t)f);
    #else
    GPUInt128 gp_uint128_convert_f32(float);
    return gp_uint128_convert_f32(f);
    #endif
}
GP_NODISCARD static inline GPInt128 gp_int128_f32(float f)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int((gp_tetra_uint_t)f);
    #else
    GPInt128 gp_int128_convert_f32(float);
    return gp_int128_convert_f32(f);
    #endif
}

GP_NODISCARD static inline double gp_f64_uint128(GPUInt128 u)
{
    #if GP_HAS_TETRA_INT
    return u.u128;
    #else
    double gp_f64_convert_uint128(GPUInt128);
    return gp_f64_convert_uint128(u);
    #endif
}
GP_NODISCARD static inline double gp_f64_int128(GPInt128 i)
{
    #if GP_HAS_TETRA_INT
    return i.i128;
    #else
    double gp_f64_convert_int128(GPInt128);
    return gp_f64_convert_int128(i);
    #endif
}
GP_NODISCARD static inline float gp_f32_uint128(GPUInt128 u)
{
    #if GP_HAS_TETRA_INT
    return u.u128;
    #else
    float gp_f32_convert_uint128(GPUInt128);
    return gp_f32_convert_uint128(u);
    #endif
}
GP_NODISCARD static inline float gp_f32_int128(GPInt128 i)
{
    #if GP_HAS_TETRA_INT
    return i.i128;
    #else
    float gp_f32_convert_int128(GPInt128);
    return gp_f32_convert_int128(i);
    #endif
}

#if __cplusplus // concise constructors, available in C as macros
GP_NODISCARD static inline GPUInt128 gp_u128(uint64_t hi, uint64_t lo) { return gp_uint128(hi, lo); }
GP_NODISCARD static inline GPUInt128 gp_u128(GPInt128 i)  { return gp_uint128_i128(i); }
GP_NODISCARD static inline GPUInt128 gp_u128(double f)    { return gp_uint128_f64(f);  } // unavailable in C99
GP_NODISCARD static inline GPUInt128 gp_u128(float f)     { return gp_uint128_f32(f);  } // unavailable in C99
GP_NODISCARD static inline GPUInt128 gp_u128(GPUInt128 u) { return u; } // useful for generics

/** Convert primitive integers to 128-bit unsigned integer.
 * Will sign extend if @p i is a negative signed integer. If C99, @p i is
 * expected to be a positive value and will not be sign extended.
 */
template <typename T> GP_NODISCARD static inline
typename std::enable_if<std::is_integral<T>::value, GPUInt128>::type
gp_u128(T i) { return gp_uint128(-(i<0), i); }

GP_NODISCARD static inline GPInt128 gp_i128(int64_t hi, uint64_t lo) { return gp_int128(hi, lo); }
GP_NODISCARD static inline GPInt128 gp_i128(GPUInt128 u) { return gp_int128_u128(u); }
GP_NODISCARD static inline GPInt128 gp_u128(double f)    { return gp_int128_f64(f);  } // unavailable in C99
GP_NODISCARD static inline GPInt128 gp_u128(float f)     { return gp_int128_f32(f);  } // unavailable in C99
GP_NODISCARD static inline GPInt128 gp_i128(GPInt128 i)  { return i; } // useful for generics

/** Convert primitive integers to 128-bit signed integer.
 * Will sign extend if @p i is a negative signed integer. If C99, @p i will
 * always be interpreted as signed, so even unsigned @p i will sign extend.
 */
template <typename T> GP_NODISCARD static inline
typename std::enable_if<std::is_integral<T>::value, GPInt128>::type
gp_i128(T i) { return gp_int128(-(i<0), i); }

#  if GP_HAS_TETRA_INT
GP_NODISCARD static inline GPUInt128 gp_u128(gp_tetra_uint_t u) { return gp_uint128_tetra_uint(u); }
GP_NODISCARD static inline GPInt128  gp_i128(gp_tetra_int_t  i) { return gp_int128_tetra_int(i);   }
#  endif

#else // C
#define gp_u128(...) GP_OVERLOAD2(__VA_ARGS__, gp_uint128, GP_U128_CTOR)(__VA_ARGS__)
#define gp_i128(...) GP_OVERLOAD2(__VA_ARGS__, gp_int128,  GP_I128_CTOR)(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
// Type-generic Macros

// Macros with more concise name (gp_uint128_xxx -> gp_u128_xxx) that take
// any combination of integers as arguments and return GPUInt128 or GPInt128.

#define gp_u128_add(A, B)                gp_uint128_add(gp_u128(A), gp_u128(B))
#define gp_i128_add(A, B)                gp_int128_add(gp_i128(A), gp_i128(B))
#define gp_u128_sub(A, B)                gp_uint128_sub(gp_u128(A), gp_u128(B))
#define gp_i128_sub(A, B)                gp_int128_sub(gp_i128(A), gp_i128(B))
#define gp_u128_mul(A, B)                gp_uint128_mul(gp_u128(A), gp_u128(B))
#define gp_i128_mul(A, B)                gp_int128_mul(gp_i128(A), gp_i128(B))
#define gp_u128_div(A, B)                gp_uint128_div(gp_u128(A), gp_u128(B))
#define gp_i128_div(A, B)                gp_int128_div(gp_i128(A), gp_i128(B))
#define gp_u128_mod(A, B)                gp_uint128_mod(gp_u128(A), gp_u128(B))
#define gp_i128_mod(A, B)                gp_int128_mod(gp_i128(A), gp_i128(B))
#define gp_u128_negate(A)                gp_uint128_negate(gp_u128(A))
#define gp_i128_negate(A)                gp_int128_negate(gp_i128(A))
#define gp_u128_not(A)                   gp_uint128_not(gp_u128(A))
#define gp_i128_not(A)                   gp_int128_not(gp_i128(A))
#define gp_u128_and(A, B)                gp_uint128_and(gp_u128(A), gp_u128(B))
#define gp_i128_and(A, B)                gp_int128_and(gp_i128(A), gp_i128(B))
#define gp_u128_or(A, B)                 gp_uint128_or(gp_u128(A),  gp_u128(B))
#define gp_i128_or(A, B)                 gp_int128_or(gp_i128(A),  gp_i128(B))
#define gp_u128_xor(A, B)                gp_uint128_xor(gp_u128(A), gp_u128(B))
#define gp_i128_xor(A, B)                gp_int128_xor(gp_i128(A), gp_i128(B))
#define gp_u128_shift_left(A, B)         gp_uint128_shift_left(gp_u128(A), B)
#define gp_i128_shift_left(A, B)         gp_int128_shift_left(gp_i128(A), B)
#define gp_u128_shift_right(A, B)        gp_uint128_shift_right(gp_u128(A), B)
#define gp_i128_shift_right(A, B)        gp_int128_shift_right(gp_i128(A), B)
#define gp_u128_equal(A, B)              gp_uint128_equal(gp_u128(A), gp_u128(B))
#define gp_i128_equal(A, B)              gp_int128_equal(gp_i128(A), gp_i128(B))
#define gp_u128_not_equal(A, B)          gp_uint128_not_equal(gp_u128(A), gp_u128(B))
#define gp_i128_not_equal(A, B)          gp_int128_not_equal(gp_i128(A), gp_i128(B))
#define gp_u128_greater_than(A, B)       gp_uint128_greater_than(gp_u128(A), gp_u128(B))
#define gp_i128_greater_than(A, B)       gp_int128_greater_than(gp_i128(A), gp_i128(B))
#define gp_u128_less_than(A, B)          gp_uint128_less_than(gp_u128(A), gp_u128(B))
#define gp_i128_less_than(A, B)          gp_int128_less_than(gp_i128(A), gp_i128(B))
#define gp_u128_greater_than_equal(A, B) gp_uint128_greater_than_equal(gp_u128(A), gp_u128(B))
#define gp_i128_greater_than_equal(A, B) gp_int128_greater_than_equal(gp_i128(A), gp_i128(B))
#define gp_u128_less_than_equal(A, B)    gp_uint128_less_than_equal(gp_u128(A), gp_u128(B))
#define gp_i128_less_than_equal(A, B)    gp_int128_less_than_equal(gp_i128(A), gp_i128(B))

// ----------------------------------------------------------------------------
// Bitwise Operations

/** Unsigned bitwise NOT */
GP_NODISCARD static inline GPUInt128 gp_uint128_not(GPUInt128 a)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(~a.u128);
    #else
    return gp_uint128(~gp_uint128_hi(a), ~gp_uint128_lo(a));
    #endif
}
/** Signed bitwise NOT */
GP_NODISCARD static inline GPInt128 gp_int128_not(GPInt128 a)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(~a.i128);
    #else
    return gp_int128(~gp_int128_hi(a), ~gp_int128_lo(a));
    #endif
}

/** Unsigned bitwise AND */
GP_NODISCARD static inline GPUInt128 gp_uint128_and(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 & b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) & gp_uint128_hi(b), gp_uint128_lo(a) & gp_uint128_lo(b));
    #endif
}
/** Signed bitwise AND */
GP_NODISCARD static inline GPInt128 gp_int128_and(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 & b.i128);
    #else
    return gp_int128(gp_int128_hi(a) & gp_int128_hi(b), gp_int128_lo(a) & gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise OR */
GP_NODISCARD static inline GPUInt128 gp_uint128_or(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 | b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) | gp_uint128_hi(b), gp_uint128_lo(a) | gp_uint128_lo(b));
    #endif
}
/** Signed bitwise OR */
GP_NODISCARD static inline GPInt128 gp_int128_or(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 | b.i128);
    #else
    return gp_int128(gp_int128_hi(a) | gp_int128_hi(b), gp_int128_lo(a) | gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise XOR */
GP_NODISCARD static inline GPUInt128 gp_uint128_xor(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 ^ b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) ^ gp_uint128_hi(b), gp_uint128_lo(a) ^ gp_uint128_lo(b));
    #endif
}
/** Signed bitwise XOR */
GP_NODISCARD static inline GPInt128 gp_int128_xor(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 ^ b.i128);
    #else
    return gp_int128(gp_int128_hi(a) ^ gp_int128_hi(b), gp_int128_lo(a) ^ gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPUInt128 gp_uint128_shift_left(GPUInt128 a, uint8_t b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 << b);
    #else
    if (b == 0) // avoid UB in `<< (64-b)`
        return a;
    if (b >= 64)
        return gp_uint128(gp_uint128_lo(a) << (b-64), 0);
    #  if _MSC_VER && defined(_M_X64)
    uint64_t hi = __shiftleft128(gp_uint128_lo(a), gp_uint128_hi(a), b);
    return gp_uint128(hi, gp_uint128_lo(a) << b);
    #  else
    return gp_uint128(
        (gp_uint128_hi(a) << b) | (gp_uint128_lo(a) >> (64-b)),
         gp_uint128_lo(a) << b);
    #  endif
    #endif
}
/** Signed bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPInt128 gp_int128_shift_left(GPInt128 a, uint8_t b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 << b);
    #else
    if (b == 0) // avoid UB in `<< (64-b)`
        return a;
    if (b >= 64)
        return gp_int128(gp_int128_lo(a) << (b-64), 0);
    return gp_int128(
        (gp_int128_hi(a) << b) | (gp_int128_lo(a) >> (64-b)),
         gp_int128_lo(a) << b);
    #endif
}

/** Unsigned bitwise right shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPUInt128 gp_uint128_shift_right(GPUInt128 a, uint8_t b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 >> b);
    #else
    if (b == 0) // avoid UB in `>> (64-b)`
        return a;
    if (b >= 64)
        return gp_uint128(0, gp_uint128_hi(a) >> (b-64));
    #  if _MSC_VER && defined(_M_X64)
    uint64_t lo = __shiftright128(gp_uint128_lo(a), gp_uint128_hi(a), b);
    return gp_uint128(gp_uint128_hi(a) >> b, lo);
    #  else
    return gp_uint128(
         gp_uint128_hi(a) >> b,
        (gp_uint128_lo(a) >> b) | (gp_uint128_hi(a) << (64-b)));
    #  endif
    #endif
}
/** Signed bitwise right shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPInt128 gp_int128_shift_right(GPInt128 a, uint8_t b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 >> b);
    #else
    if (b == 0) // avoid UB in `>> (64-b)`
        return a;
    if (b >= 64)
        return gp_int128(gp_int128_hi(a) >> 63/*sign extend*/, gp_int128_hi(a) >> (b-64));
    return gp_int128(
         gp_int128_hi(a) >> b,
        (gp_int128_lo(a) >> b) | ((uint64_t)gp_int128_hi(a) << (64-b)));
    #endif
}

// ----------------------------------------------------------------------------
// Arithmetic

/** Add 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_add(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 + b.u128);
    #else
    return gp_uint128(
        gp_uint128_hi(a) + gp_uint128_hi(b)
            + (gp_uint128_lo(b) > UINT64_MAX - gp_uint128_lo(a)), // carry
        gp_uint128_lo(a) + gp_uint128_lo(b));
    #endif
}
/** Add 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_add(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 + b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) + gp_int128_hi(b)
            + (gp_int128_lo(b) > UINT64_MAX - gp_int128_lo(a)), // carry
        gp_int128_lo(a) + gp_int128_lo(b));
    #endif
}

/** Subtract 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_sub(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 - b.u128);
    #else
    return gp_uint128(
        gp_uint128_hi(a) - gp_uint128_hi(b)
            - (gp_uint128_lo(b) > gp_uint128_lo(a)), // borrow
        gp_uint128_lo(a) - gp_uint128_lo(b));
    #endif
}
/** Subtract 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_sub(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 - b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) - gp_int128_hi(b)
            - (gp_int128_lo(b) > gp_int128_lo(a)), // borrow
        gp_int128_lo(a) - gp_int128_lo(b));
    #endif
}

/** Negate 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_negate(GPUInt128 a)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(-a.u128);
    #else
    return gp_uint128(~gp_uint128_hi(a) + !gp_uint128_lo(a), ~gp_uint128_lo(a) + 1);
    #endif
}
/** Negate 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_negate(GPInt128 a)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(-a.i128);
    #else
    return gp_int128_u128(gp_uint128_negate(gp_uint128_i128(a)));
    #endif
}

/** Multiply 64-bit unsigned integers to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_mul64(uint64_t a, uint64_t b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint((gp_tetra_uint_t)a * b);
    #elif _MSC_VER && defined(_M_X64)
    uint64_t lo, hi;
    lo = _umul128(a, b, &hi);
    return gp_uint128(hi, lo);
    #else
    GPUInt128 gp_uint128_long_mul64(uint64_t a, uint64_t b);
    return gp_uint128_long_mul64(a, b);
    #endif
}
/** Multiply 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_mul(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 * b.u128);
    #else
    GPUInt128 y = gp_uint128_mul64(gp_uint128_lo(a), gp_uint128_lo(b));
    *gp_uint128_hi_addr(&y) += gp_uint128_hi(a)*gp_uint128_lo(b) + gp_uint128_lo(a)*gp_uint128_hi(b);
    return y;
    #endif
}
/** Multiply 64-bit signed integers to 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_mul64(int64_t a, int64_t b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int((gp_tetra_int_t)a * b);
    #elif _MSC_VER && defined(_M_X64)
    __int64 lo, hi;
    lo = _mul128(a, b, &hi);
    return gp_int128(hi, lo);
    #else
    return gp_int128_u128(gp_uint128_mul(gp_uint128(-(a<0), a), gp_uint128(-(b<0), b)));
    #endif
}

/** Multiply 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_mul(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 * b.i128);
    #else
    return gp_int128_u128(
        gp_uint128_mul(
            gp_uint128_i128(a),
            gp_uint128_i128(b)));
    #endif
}

/** Divide 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_div(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 / b.u128);
    #else
    GPUInt128 gp_uint128_divmod(GPUInt128 a, GPUInt128 b, GPUInt128* optional_remainder);
    return gp_uint128_divmod(a, b, NULL);
    #endif
}
/** Divide 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_div(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 / b.i128);
    #else
    GPInt128 gp_int128_idiv(GPInt128 a, GPInt128 b);
    return gp_int128_idiv(a, b);
    #endif
}

/** 128-bit unsigned integer modulus.*/
GP_NODISCARD static inline GPUInt128 gp_uint128_mod(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 % b.u128);
    #else
    GPUInt128 gp_uint128_divmod(GPUInt128 a, GPUInt128 b, GPUInt128* optional_remainder);
    GPUInt128 remainder;
    gp_uint128_divmod(a, b, &remainder);
    return remainder;
    #endif
}
/** 128-bit signed integer modulus.*/
GP_NODISCARD static inline GPInt128 gp_int128_mod(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return gp_int128_tetra_int(a.i128 % b.i128);
    #else
    GPInt128 gp_int128_imod(GPInt128 a, GPInt128 b);
    return gp_int128_imod(a, b);
    #endif
}

// ----------------------------------------------------------------------------
// Comparisons

//
GP_NODISCARD static inline bool gp_uint128_equal(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.u128 == b.u128;
    #else
    return a.little_endian.lo == b.little_endian.lo && a.little_endian.hi == b.little_endian.hi;
    #endif
}
GP_NODISCARD static inline bool gp_int128_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.i128 == b.i128;
    #else
    return a.little_endian.lo == b.little_endian.lo && a.little_endian.hi == b.little_endian.hi;
    #endif
}

GP_NODISCARD static inline bool gp_uint128_not_equal(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.u128 != b.u128;
    #else
    return a.little_endian.lo != b.little_endian.lo || a.little_endian.hi != b.little_endian.hi;
    #endif
}
GP_NODISCARD static inline bool gp_int128_not_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.i128 != b.i128;
    #else
    return a.little_endian.lo != b.little_endian.lo || a.little_endian.hi != b.little_endian.hi;
    #endif
}

GP_NODISCARD static inline bool gp_uint128_greater_than(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.u128 > b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) > gp_uint128_lo(b);
    return gp_uint128_hi(a) > gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_greater_than(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.i128 > b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) > gp_int128_lo(b);
    return gp_int128_hi(a) > gp_int128_hi(b);
    #endif
}

GP_NODISCARD static inline bool gp_uint128_less_than(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.u128 < b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) < gp_uint128_lo(b);
    return gp_uint128_hi(a) < gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_less_than(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.i128 < b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) < gp_int128_lo(b);
    return gp_int128_hi(a) < gp_int128_hi(b);
    #endif
}

GP_NODISCARD static inline bool gp_uint128_greater_than_equal(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.u128 >= b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) >= gp_uint128_lo(b);
    return gp_uint128_hi(a) >= gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_greater_than_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.i128 >= b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) >= gp_int128_lo(b);
    return gp_int128_hi(a) >= gp_int128_hi(b);
    #endif
}

GP_NODISCARD static inline bool gp_uint128_less_than_equal(GPUInt128 a, GPUInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.u128 <= b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) <= gp_uint128_lo(b);
    return gp_uint128_hi(a) <= gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_less_than_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TETRA_INT
    return a.i128 <= b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) <= gp_int128_lo(b);
    return gp_int128_hi(a) <= gp_int128_hi(b);
    #endif
}

#if __cplusplus // operator overloads

// No implicit conversions provided for simplicity and to avoid pitfalls of C++
// template meta-programming and to avoid implicit conversion bugs.

// Arithmetic
GP_NODISCARD static inline GPUInt128 operator +(GPUInt128 a, GPUInt128 b) { return gp_uint128_add(a, b); }
GP_NODISCARD static inline GPInt128  operator +(GPInt128  a, GPInt128  b) { return gp_int128_add(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator -(GPUInt128 a, GPUInt128 b) { return gp_uint128_sub(a, b); }
GP_NODISCARD static inline GPInt128  operator -(GPInt128  a, GPInt128  b) { return gp_int128_sub(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator *(GPUInt128 a, GPUInt128 b) { return gp_uint128_mul(a, b); }
GP_NODISCARD static inline GPInt128  operator *(GPInt128  a, GPInt128  b) { return gp_int128_mul(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator /(GPUInt128 a, GPUInt128 b) { return gp_uint128_div(a, b); }
GP_NODISCARD static inline GPInt128  operator /(GPInt128  a, GPInt128  b) { return gp_int128_div(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator %(GPUInt128 a, GPUInt128 b) { return gp_uint128_mod(a, b); }
GP_NODISCARD static inline GPInt128  operator %(GPInt128  a, GPInt128  b) { return gp_int128_mod(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator -(GPUInt128 a) { return gp_uint128_negate(a); }
GP_NODISCARD static inline GPInt128  operator -(GPInt128  a) { return gp_int128_negate(a);  }

// Bitwise operators
GP_NODISCARD static inline GPUInt128 operator ~(GPUInt128  a) { return gp_uint128_not(a); }
GP_NODISCARD static inline GPInt128  operator ~(GPInt128   a) { return gp_int128_not(a) ; }
GP_NODISCARD static inline GPUInt128 operator &(GPUInt128  a, GPUInt128 b) { return gp_uint128_and(a, b); }
GP_NODISCARD static inline GPInt128  operator &(GPInt128   a, GPInt128  b) { return gp_int128_and(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator |(GPUInt128  a, GPUInt128 b) { return gp_uint128_or(a, b) ; }
GP_NODISCARD static inline GPInt128  operator |(GPInt128   a, GPInt128  b) { return gp_int128_or(a, b)  ; }
GP_NODISCARD static inline GPUInt128 operator ^(GPUInt128  a, GPUInt128 b) { return gp_uint128_xor(a, b); }
GP_NODISCARD static inline GPInt128  operator ^(GPInt128   a, GPInt128  b) { return gp_int128_xor(a, b) ; }
GP_NODISCARD static inline GPUInt128 operator <<(GPUInt128 a, uint8_t   b) { return gp_uint128_shift_left(a, b) ; }
GP_NODISCARD static inline GPInt128  operator <<(GPInt128  a, uint8_t   b) { return gp_int128_shift_left(a, b)  ; }
GP_NODISCARD static inline GPUInt128 operator >>(GPUInt128 a, uint8_t   b) { return gp_uint128_shift_right(a, b); }
GP_NODISCARD static inline GPInt128  operator >>(GPInt128  a, uint8_t   b) { return gp_int128_shift_right(a, b) ; }

// Assignments
static inline GPUInt128 operator +=(GPUInt128&  a, GPUInt128 b) { return a = a + b ; }
static inline GPInt128  operator +=(GPInt128&   a, GPInt128  b) { return a = a + b ; }
static inline GPUInt128 operator -=(GPUInt128&  a, GPUInt128 b) { return a = a - b ; }
static inline GPInt128  operator -=(GPInt128&   a, GPInt128  b) { return a = a - b ; }
static inline GPUInt128 operator *=(GPUInt128&  a, GPUInt128 b) { return a = a * b ; }
static inline GPInt128  operator *=(GPInt128&   a, GPInt128  b) { return a = a * b ; }
static inline GPUInt128 operator /=(GPUInt128&  a, GPUInt128 b) { return a = a / b ; }
static inline GPInt128  operator /=(GPInt128&   a, GPInt128  b) { return a = a / b ; }
static inline GPUInt128 operator %=(GPUInt128&  a, GPUInt128 b) { return a = a % b ; }
static inline GPInt128  operator %=(GPInt128&   a, GPInt128  b) { return a = a % b ; }
static inline GPUInt128 operator &=(GPUInt128&  a, GPUInt128 b) { return a = a & b ; }
static inline GPInt128  operator &=(GPInt128&   a, GPInt128  b) { return a = a & b ; }
static inline GPUInt128 operator |=(GPUInt128&  a, GPUInt128 b) { return a = a | b ; }
static inline GPInt128  operator |=(GPInt128&   a, GPInt128  b) { return a = a | b ; }
static inline GPUInt128 operator ^=(GPUInt128&  a, GPUInt128 b) { return a = a ^ b ; }
static inline GPInt128  operator ^=(GPInt128&   a, GPInt128  b) { return a = a ^ b ; }
static inline GPUInt128 operator <<=(GPUInt128& a, uint8_t   b) { return a = a << b; }
static inline GPInt128  operator <<=(GPInt128&  a, uint8_t   b) { return a = a << b; }
static inline GPUInt128 operator >>=(GPUInt128& a, uint8_t   b) { return a = a >> b; }
static inline GPInt128  operator >>=(GPInt128&  a, uint8_t   b) { return a = a >> b; }

// Increment/decrement
static inline GPUInt128& operator ++(GPUInt128& a) { return a = a + gp_u128(1); }
static inline GPInt128&  operator ++(GPInt128&  a) { return a = a + gp_i128(1); }
static inline GPUInt128& operator --(GPUInt128& a) { return a = a - gp_u128(1); }
static inline GPInt128&  operator --(GPInt128&  a) { return a = a - gp_i128(1); }
static inline GPUInt128  operator ++(GPUInt128& a, int _) { (void)_; return ++a - gp_u128(1); }
static inline GPInt128   operator ++(GPInt128&  a, int _) { (void)_; return ++a - gp_i128(1); }
static inline GPUInt128  operator --(GPUInt128& a, int _) { (void)_; return --a + gp_u128(1); }
static inline GPInt128   operator --(GPInt128&  a, int _) { (void)_; return --a + gp_i128(1); }

// Comparisons
GP_NODISCARD static inline bool operator ==(GPUInt128 a, GPUInt128 b) { return gp_uint128_equal(a, b)             ; }
GP_NODISCARD static inline bool operator ==(GPInt128  a, GPInt128  b) { return gp_int128_equal(a, b)              ; }
GP_NODISCARD static inline bool operator !=(GPUInt128 a, GPUInt128 b) { return gp_uint128_not_equal(a, b)         ; }
GP_NODISCARD static inline bool operator !=(GPInt128  a, GPInt128  b) { return gp_int128_not_equal(a, b)          ; }
GP_NODISCARD static inline bool operator  <(GPUInt128 a, GPUInt128 b) { return gp_uint128_less_than(a, b)         ; }
GP_NODISCARD static inline bool operator  <(GPInt128  a, GPInt128  b) { return gp_int128_less_than(a, b)          ; }
GP_NODISCARD static inline bool operator <=(GPUInt128 a, GPUInt128 b) { return gp_uint128_less_than_equal(a, b)   ; }
GP_NODISCARD static inline bool operator <=(GPInt128  a, GPInt128  b) { return gp_int128_less_than_equal(a, b)    ; }
GP_NODISCARD static inline bool operator  >(GPUInt128 a, GPUInt128 b) { return gp_uint128_greater_than(a, b)      ; }
GP_NODISCARD static inline bool operator  >(GPInt128  a, GPInt128  b) { return gp_int128_greater_than(a, b)       ; }
GP_NODISCARD static inline bool operator >=(GPUInt128 a, GPUInt128 b) { return gp_uint128_greater_than_equal(a, b); }
GP_NODISCARD static inline bool operator >=(GPInt128  a, GPInt128  b) { return gp_int128_greater_than_equal(a, b) ; }
#endif // __cplusplus // operator overloads


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#if __cplusplus
static inline GP_MAYBE_CONSTEXPR GPType GP_TYPE(GPUInt128 x) { (void)x; return GP_UINT128; }
static inline GP_MAYBE_CONSTEXPR GPType GP_TYPE(GPInt128  x) { (void)x; return GP_INT128;  }
#endif

#ifdef GP_INT128_SELECTION
#  undef GP_INT128_SELECTION
#  undef GP_UINT128_SELECTION
#endif
#define GP_INT128_SELECTION(...)  GPInt128:  __VA_ARGS__
#define GP_UINT128_SELECTION(...) GPUInt128: __VA_ARGS__
#if GP_HAS_TETRA_INT || defined(GP_TEST_INT128)
#  ifdef GP_TETRA_INT_SELECTION
#    undef GP_TETRA_INT_SELECTION
#    undef GP_TETRA_UINT_SELECTION
#  endif
#  define GP_TETRA_INT_SELECTION(...)  gp_tetra_int_t:  __VA_ARGS__
#  define GP_TETRA_UINT_SELECTION(...) gp_tetra_uint_t: __VA_ARGS__
#endif

#if __STDC_VERSION__ >= 201112L
GP_NODISCARD static inline GPUInt128 gp_uint128_uint128(GPUInt128 u) { return u; }
GP_NODISCARD static inline GPInt128  gp_int128_int128(GPInt128 i) { return i;    }
#  if GP_HAS_TETRA_INT || defined(GP_TEST_INT128) // use implicit integer conversions
#    define GP_U128_CTOR(A) _Generic(A, GPUInt128: gp_uint128_uint128, GPInt128: gp_uint128_i128, default: gp_uint128_tetra_uint)(A)
#    define GP_I128_CTOR(A) _Generic(A, GPUInt128: gp_int128_u128, GPInt128: gp_int128_int128, default: gp_int128_tetra_int)(A)
#  else
#    define GP_U128_CTOR(A) _Generic(A, \
         GP_C11_GENERIC_SIGNED_INTEGER(gp_uint128_i64), \
         GP_C11_GENERIC_UNSIGNED_INTEGER(gp_uint128_u64), \
         GPUInt128: gp_uint128_uint128, GPInt128: gp_uint128_i128)(A)
#    define GP_I128_CTOR(A) _Generic(A, \
         GP_C11_GENERIC_SIGNED_INTEGER(gp_int128_i64), \
         GP_C11_GENERIC_UNSIGNED_INTEGER(gp_int128_u64), \
         GPUInt128: gp_int128_u128, GPInt128: gp_int128_int128)(A)
#  endif
#elif !__cplusplus // C99, no type safety and performance hit from va_args
#  define GP_U128_CTOR(...) gp_u128_c99_ctor(sizeof(__VA_ARGS__), __VA_ARGS__)
#  define GP_I128_CTOR(...) gp_i128_c99_ctor(sizeof(__VA_ARGS__), __VA_ARGS__)
GP_NODISCARD GPUInt128 gp_u128_c99_ctor(size_t size, ...);
GP_NODISCARD GPInt128 gp_i128_c99_ctor(size_t size, ...);
#endif // __STDC_VERSION__ >= 201112L

#endif // GP_INT128_INCLUDED
