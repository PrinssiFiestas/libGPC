// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file int128.h
 * Portable 128 bit integer
 */

#ifndef GP_INT128_INCLUDED
#define GP_INT128_INCLUDED 1

#include "attributes.h"
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#if _MSC_VER && _M_X64
// Note: MSVC sometimes produces much worse code using intrinsics, so only use
// them with some functions.
#include <intrin.h>
#endif

/** 128-bit unsigned integer.*/
typedef union gp_uint128
{
    struct {
        uint64_t lo;
        uint64_t hi;
    } little_endian;

    struct {
        uint64_t hi;
        uint64_t lo;
    } big_endian;

    #if (__GNUC__ && __SIZEOF_INT128__) || defined(GP_TEST_INT128)
    __uint128_t gnu;
    #endif
} GPUint128;

/** 128-bit signed integer.*/
typedef union gp_int128
{
    struct {
        uint64_t lo;
        int64_t  hi;
    } little_endian;

    struct {
        int64_t  hi;
        uint64_t lo;
    } big_endian;


    #if (__GNUC__ && defined(__SIZEOF_INT128__)) || defined(GP_TEST_INT128)
    __int128_t gnu;
    #endif
} GPInt128;

// ----------------------------------------------------------------------------
// Endianness

/** Check if system is big endian.*/
GP_NODISCARD
static inline bool gp_is_big_endian(void)
{
    union Endianness {
        uint16_t u16;
        struct { uint8_t is_little; uint8_t is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_big;
}
/** Check if system is little endian.*/
static inline bool gp_is_little_endian(void)
{
    union Endianness {
        uint16_t u16;
        struct { uint8_t is_little; uint8_t is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_little;
}

// ----------------------------------------------------------------------------
// Constructors and Accessors

/** Create 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_uint128(uint64_t hi_bits, uint64_t lo_bits)
{
    GPUint128 u128;
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
GP_NODISCARD static inline GPInt128 gp_int128(int64_t hi_bits, uint64_t lo_bits)
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

/** Get low bits of 128-bit unsigned integer.*/
GP_NODISCARD static inline uint64_t gp_uint128_lo(GPUint128 u)
{
    return gp_is_little_endian() ? u.little_endian.lo : u.big_endian.lo;
}
/** Get low bits of 128-bit signed integer.*/
GP_NODISCARD static inline uint64_t gp_int128_lo(GPInt128 i)
{
    return gp_is_little_endian() ? i.little_endian.lo : i.big_endian.lo;
}

/** Get high bits of 128-bit unsigned integer.*/
GP_NODISCARD static inline uint64_t gp_uint128_hi(GPUint128 u)
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
static inline uint64_t* gp_uint128_lo_addr(GPUint128* u)
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
static inline uint64_t* gp_uint128_hi_addr(GPUint128* u)
{
    return gp_is_little_endian() ? &u->little_endian.hi : &u->big_endian.hi;
}
/** Get address of signed high bits of 128-bit signed integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
static inline int64_t* gp_int128_hi_addr(GPInt128* i)
{
    return gp_is_little_endian() ? &i->little_endian.hi : &i->big_endian.hi;
}

#if __GNUC__ && __SIZEOF_INT128__
GP_NODISCARD static inline GPUint128 gp_uint128_gnu(__uint128_t gnu_u)
{
    GPUint128 u;
    u.gnu = gnu_u;
    return u;
}
GP_NODISCARD static inline GPInt128 gp_int128_gnu(__int128_t gnu_i)
{
    GPInt128 i;
    i.gnu = gnu_i;
    return i;
}
#endif

// ----------------------------------------------------------------------------
// Limits

#define GP_UINT128_MAX gp_uint128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
#define GP_INT128_MAX  gp_int128( 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
#define GP_INT128_MIN  gp_int128(INT64_MIN, 0)

// ----------------------------------------------------------------------------
// Bitwise operations

/** Unsigned bitwise NOT */
GP_NODISCARD static inline GPUint128 gp_uint128_not(GPUint128 a)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(~a.gnu);
    #else
    return gp_uint128(~gp_uint128_hi(a), ~gp_uint128_lo(a));
    #endif
}
/** Signed bitwise NOT */
GP_NODISCARD static inline GPInt128 gp_int128_not(GPInt128 a)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(~a.gnu);
    #else
    return gp_int128(~gp_int128_hi(a), ~gp_int128_lo(a));
    #endif
}

/** Unsigned bitwise AND */
GP_NODISCARD static inline GPUint128 gp_uint128_and(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu & b.gnu);
    #else
    return gp_uint128(gp_uint128_hi(a) & gp_uint128_hi(b), gp_uint128_lo(a) & gp_uint128_lo(b));
    #endif
}
/** Signed bitwise AND */
GP_NODISCARD static inline GPInt128 gp_int128_and(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu & b.gnu);
    #else
    return gp_int128(gp_int128_hi(a) & gp_int128_hi(b), gp_int128_lo(a) & gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise OR */
GP_NODISCARD static inline GPUint128 gp_uint128_or(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu | b.gnu);
    #else
    return gp_uint128(gp_uint128_hi(a) | gp_uint128_hi(b), gp_uint128_lo(a) | gp_uint128_lo(b));
    #endif
}
/** Signed bitwise OR */
GP_NODISCARD static inline GPInt128 gp_int128_or(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu | b.gnu);
    #else
    return gp_int128(gp_int128_hi(a) | gp_int128_hi(b), gp_int128_lo(a) | gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise XOR */
GP_NODISCARD static inline GPUint128 gp_uint128_xor(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu ^ b.gnu);
    #else
    return gp_uint128(gp_uint128_hi(a) ^ gp_uint128_hi(b), gp_uint128_lo(a) ^ gp_uint128_lo(b));
    #endif
}
/** Signed bitwise XOR */
GP_NODISCARD static inline GPInt128 gp_int128_xor(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu ^ b.gnu);
    #else
    return gp_int128(gp_int128_hi(a) ^ gp_int128_hi(b), gp_int128_lo(a) ^ gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise left shift */
GP_NODISCARD static inline GPUint128 gp_uint128_shift_left(GPUint128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu << b);
    #else
    if (b == 0) // avoid UB in `>> (64-b)`
        return a;
    if (b >= 64)
        return gp_uint128(gp_uint128_lo(a) << (b-64), 0);
    return gp_uint128(
        (gp_uint128_hi(a) << b) | (gp_uint128_lo(a) >> (64-b)),
         gp_uint128_lo(a) << b);
    #endif
}
/** Signed bitwise left shift */
GP_NODISCARD static inline GPInt128 gp_int128_shift_left(GPInt128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu << b);
    #else
    if (b == 0) // avoid UB in `>> (64-b)`
        return a;
    if (b >= 64)
        return gp_int128(gp_int128_lo(a) << (b-64), 0);
    return gp_int128(
        (gp_int128_hi(a) << b) | (gp_int128_lo(a) >> (64-b)),
         gp_int128_lo(a) << b);
    #endif
}

/** Unsigned bitwise right shift */
GP_NODISCARD static inline GPUint128 gp_uint128_shift_right(GPUint128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu >> b);
    #else
    if (b == 0) // avoid UB in `<< (64-b)`
        return a;
    if (b >= 64)
        return gp_uint128(0, gp_uint128_hi(a) >> (b-64));
    return gp_uint128(
         gp_uint128_hi(a) >> b,
        (gp_uint128_lo(a) >> b) | (gp_uint128_hi(a) << (64-b)));
    #endif
}
/** Signed bitwise right shift */
GP_NODISCARD static inline GPInt128 gp_int128_shift_right(GPInt128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu >> b);
    #else
    if (b == 0) // avoid UB in `<< (64-b)`
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
GP_NODISCARD static inline GPUint128 gp_uint128_add(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu + b.gnu);
    #else
    return gp_uint128(
        gp_uint128_lo(a) + gp_uint128_lo(b),
        gp_uint128_hi(a) + gp_uint128_hi(b)
            + (gp_uint128_lo(b) > UINT64_MAX - gp_uint128_lo(a))); // carry
    #endif
}

/** Add 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_add(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu + b.gnu);
    #else
    return gp_int128(
        gp_int128_lo(a) + gp_int128_lo(b),
        gp_int128_hi(a) + gp_int128_hi(b)
            + (gp_int128_lo(b) > UINT64_MAX - gp_int128_lo(a))); // carry
    #endif
}

/** Subtract 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_uint128_sub(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu - b.gnu);
    #else
    return gp_uint128(
        gp_uint128_lo(a) - gp_uint128_lo(b),
        gp_uint128_hi(a) - gp_uint128_hi(b)
            - (gp_uint128_lo(b) > gp_uint128_lo(a))); // borrow
    #endif
}

/** Subtract 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_sub(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu - b.gnu);
    #else
    return gp_int128(
        gp_int128_lo(a) - gp_int128_lo(b),
        gp_int128_hi(a) - gp_int128_hi(b)
            - (gp_int128_lo(b) > gp_int128_lo(a))); // borrow
    #endif
}

/** Multiply 64-bit unsigned integers to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_uint128_mul64(uint64_t a, uint64_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu((__uint128_t)a * b);
    #elif _MSC_VER && _M_X64
    uint64_t lo, hi;
    lo = _umul128(a, b, &hi);
    return gp_uint128(hi, lo);
    #else
    GPUint128 gp_uint128_long_mul64(uint64_t a, uint64_t b);
    return gp_uint128_long_mul64(a, b);
    #endif
}

GP_NODISCARD static inline GPUint128 gp_uint128_negate(GPUint128 a)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu * -1);
    #else
    (void)a;
    return gp_uint128(0xadf, 0xfeedbeef); // TODO bogus just to make sure that test work
    #endif
}

/** Multiply 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_uint128_mul(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_gnu(a.gnu * b.gnu);
    #else
    GPUint128 y = gp_uint128_mul64(gp_uint128_lo(a), gp_uint128_hi(b));
    *gp_uint128_hi_addr(&y) += gp_uint128_hi(a)*gp_uint128_lo(b) + gp_uint128_lo(a)*gp_uint128_hi(b);
    return y;
    #endif
}

/** Multiply 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_mul(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_gnu(a.gnu * b.gnu);
    #else
    GPUint128 abs_a;
    GPUint128 abs_b;
    memcpy(&abs_a, &a, sizeof a);
    memcpy(&abs_b, &b, sizeof b);

    bool neg_a = false;
    bool neg_b = false;

    if (gp_int128_hi(a) < 0) {
        *gp_uint128_hi_addr(&abs_a) = ~gp_uint128_hi(abs_a);
        *gp_uint128_lo_addr(&abs_a) = ~gp_uint128_lo(abs_a);
        abs_a = gp_uint128_add(abs_a, gp_uint128(0, 1));
        neg_a = true;
    }
    if (gp_int128_hi(b) < 0) {
        *gp_uint128_hi_addr(&abs_b) = ~gp_uint128_hi(abs_b);
        *gp_uint128_lo_addr(&abs_b) = ~gp_uint128_lo(abs_b);
        abs_b = gp_uint128_add(abs_b, gp_uint128(0, 1));
        neg_b = true;
    }

    GPUint128 prod = gp_uint128_mul(abs_a, abs_b);
    if (neg_a ^ neg_b) {
        *gp_uint128_hi_addr(&prod) = ~gp_uint128_hi(prod);
        *gp_uint128_lo_addr(&prod) = ~gp_uint128_lo(prod);
        prod = gp_uint128_add(prod, gp_uint128(0, 1));
    }

    GPInt128 y;
    memcpy(&y, &prod, sizeof y);
    return y;
    #endif
}

#endif // GP_INT128_INCLUDED
