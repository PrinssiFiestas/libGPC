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
    uint64_t u64[2];

    struct {
        uint64_t lo;
        uint64_t hi;
    } little_endian;

    struct {
        uint64_t hi;
        uint64_t lo;
    } big_endian;

    #if (__GNUC__ && __SIZEOF_INT128__) || defined(GP_TEST_INT128)
    __uint128_t u128;
    #endif
} GPUint128;

/** 128-bit signed integer.
 * Overflow is undefined.
 */
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
    __int128_t i128;
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

/** Convert 128-bit signed integer to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_uint128_int128(GPInt128 i)
{
    GPUint128 u;
    memcpy(&u, &i, sizeof u);
    return u;
}
/** Convert 128-bit unsigned integer to 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_uint128(GPUint128 u)
{
    GPInt128 i;
    memcpy(&i, &u, sizeof i);
    return i;
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

#if (__GNUC__ && __SIZEOF_INT128__) || GP_TEST_INT128
GP_NODISCARD static inline GPUint128 gp_uint128_u128(__uint128_t _u)
{
    GPUint128 u;
    u.u128 = _u;
    return u;
}
GP_NODISCARD static inline GPInt128 gp_int128_i128(__int128_t _i)
{
    GPInt128 i;
    i.i128 = _i;
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
    return gp_uint128_u128(~a.u128);
    #else
    return gp_uint128(~gp_uint128_hi(a), ~gp_uint128_lo(a));
    #endif
}
/** Signed bitwise NOT */
GP_NODISCARD static inline GPInt128 gp_int128_not(GPInt128 a)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(~a.i128);
    #else
    return gp_int128(~gp_int128_hi(a), ~gp_int128_lo(a));
    #endif
}

/** Unsigned bitwise AND */
GP_NODISCARD static inline GPUint128 gp_uint128_and(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 & b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) & gp_uint128_hi(b), gp_uint128_lo(a) & gp_uint128_lo(b));
    #endif
}
/** Signed bitwise AND */
GP_NODISCARD static inline GPInt128 gp_int128_and(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 & b.i128);
    #else
    return gp_int128(gp_int128_hi(a) & gp_int128_hi(b), gp_int128_lo(a) & gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise OR */
GP_NODISCARD static inline GPUint128 gp_uint128_or(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 | b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) | gp_uint128_hi(b), gp_uint128_lo(a) | gp_uint128_lo(b));
    #endif
}
/** Signed bitwise OR */
GP_NODISCARD static inline GPInt128 gp_int128_or(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 | b.i128);
    #else
    return gp_int128(gp_int128_hi(a) | gp_int128_hi(b), gp_int128_lo(a) | gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise XOR */
GP_NODISCARD static inline GPUint128 gp_uint128_xor(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 ^ b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) ^ gp_uint128_hi(b), gp_uint128_lo(a) ^ gp_uint128_lo(b));
    #endif
}
/** Signed bitwise XOR */
GP_NODISCARD static inline GPInt128 gp_int128_xor(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 ^ b.i128);
    #else
    return gp_int128(gp_int128_hi(a) ^ gp_int128_hi(b), gp_int128_lo(a) ^ gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPUint128 gp_uint128_shift_left(GPUint128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 << b);
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
/** Signed bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPInt128 gp_int128_shift_left(GPInt128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 << b);
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

/** Unsigned bitwise right shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPUint128 gp_uint128_shift_right(GPUint128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 >> b);
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
/** Signed bitwise right shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPInt128 gp_int128_shift_right(GPInt128 a, uint8_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 >> b);
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
    return gp_uint128_u128(a.u128 + b.u128);
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
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 + b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) + gp_int128_hi(b)
            + (gp_int128_lo(b) > UINT64_MAX - gp_int128_lo(a)), // carry
        gp_int128_lo(a) + gp_int128_lo(b));
    #endif
}

/** Subtract 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_uint128_sub(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 - b.u128);
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
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 - b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) - gp_int128_hi(b)
            - (gp_int128_lo(b) > gp_int128_lo(a)), // borrow
        gp_int128_lo(a) - gp_int128_lo(b));
    #endif
}

/** Multiply 64-bit unsigned integers to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_uint128_mul64(uint64_t a, uint64_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128((__uint128_t)a * b);
    #elif _MSC_VER && _M_X64
    uint64_t lo, hi;
    lo = _umul128(a, b, &hi);
    return gp_uint128(hi, lo);
    #else
    GPUint128 gp_uint128_long_mul64(uint64_t a, uint64_t b);
    return gp_uint128_long_mul64(a, b);
    #endif
}

/** Multiply 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_uint128_mul(GPUint128 a, GPUint128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_uint128_u128(a.u128 * b.u128);
    #else
    GPUint128 y = gp_uint128_mul64(gp_uint128_lo(a), gp_uint128_lo(b));
    *gp_uint128_hi_addr(&y) += gp_uint128_hi(a)*gp_uint128_lo(b) + gp_uint128_lo(a)*gp_uint128_hi(b);
    return y;
    #endif
}

/** Multiply 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_mul(GPInt128 a, GPInt128 b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    return gp_int128_i128(a.i128 * b.i128);
    #else
    return gp_int128_uint128(
        gp_uint128_mul(
            gp_uint128_int128(a),
            gp_uint128_int128(b)));
    #endif
}

/** Divide 128-bit unsigned integers with remainder.*/
GPUint128 gp_uint128_divmod(GPUint128 a, GPUint128 b, GPUint128* optional_remainder);

/** Divide 128-bit signed integers with remainder.*/
GPInt128 gp_int128_idiv(GPInt128 a, GPInt128 b);
GPInt128 gp_int128_imod(GPInt128 a, GPInt128 b);

#endif // GP_INT128_INCLUDED
