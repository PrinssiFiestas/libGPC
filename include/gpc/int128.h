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

    #if __GNUC__ && defined(__SIZEOF_INT128__)
    __uint128_t u128;
    #endif
} GPUint128;

/** 128-bit signed integer.*/
typedef union gp_int128
{
    GPUint128 u128;

    #if __GNUC__ && defined(__SIZEOF_INT128__)
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
GP_NODISCARD static inline GPUint128 gp_u128(uint64_t hi_bits, uint64_t lo_bits)
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
GP_NODISCARD static inline GPInt128 gp_i128(uint64_t hi_bits, uint64_t lo_bits)
{
    GPInt128 i128;
    if (gp_is_big_endian()) {
        i128.u128.big_endian.hi = hi_bits;
        i128.u128.big_endian.lo = lo_bits;
    } else {
        i128.u128.little_endian.hi = hi_bits;
        i128.u128.little_endian.lo = lo_bits;
    }
    return i128;
}

/** Get low bits of 128-bit unsigned integer.*/
GP_NODISCARD static inline uint64_t gp_u128_lo(GPUint128 u)
{
    return gp_is_little_endian() ? u.little_endian.lo : u.big_endian.lo;
}

/** Get low bits of 128-bit signed integer.*/
GP_NODISCARD static inline uint64_t gp_i128_lo(GPInt128 i)
{
    return gp_is_little_endian() ? i.u128.little_endian.lo : i.u128.big_endian.lo;
}

/** Get high bits of 128-bit unsigned integer.*/
GP_NODISCARD static inline uint64_t gp_u128_hi(GPUint128 u)
{
    return gp_is_little_endian() ? u.little_endian.hi : u.big_endian.hi;
}

/** Get high bits of 128-bit signed integer.*/
GP_NODISCARD static inline uint64_t gp_i128_hi(GPInt128 i)
{
    return gp_is_little_endian() ? i.u128.little_endian.hi : i.u128.big_endian.hi;
}

/** Set low bits of 128-bit unsigned integer.*/
GP_NONNULL_ARGS() static inline void gp_u128_set_lo(GPUint128* u, uint64_t x)
{
    if (gp_is_little_endian())
        u->little_endian.lo = x;
    else
        u->big_endian.lo = x;
}

/** Set low bits of 128-bit signed integer.*/
GP_NONNULL_ARGS() static inline void gp_i128_set_lo(GPInt128* i, uint64_t x)
{
    if (gp_is_little_endian())
        i->u128.little_endian.lo = x;
    else
        i->u128.big_endian.lo = x;
}

/** Set high bits of 128-bit unsigned integer.*/
GP_NONNULL_ARGS() static inline void gp_u128_set_hi(GPUint128* u, uint64_t x)
{
    if (gp_is_little_endian())
        u->little_endian.hi = x;
    else
        u->big_endian.hi = x;
}

/** Set high bits of 128-bit signed integer.*/
GP_NONNULL_ARGS() static inline void gp_i128_set_hi(GPInt128* i, uint64_t x)
{
    if (gp_is_little_endian())
        i->u128.little_endian.hi = x;
    else
        i->u128.big_endian.hi = x;
}

// ----------------------------------------------------------------------------
// Arithmetic

#undef __SIZEOF_INT128__ // TODO TEMP

/** Add 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_u128_add(GPUint128 a, GPUint128 b)
{
    GPUint128 y;
    #if __GNUC__ && __SIZEOF_INT128__
    y.u128 = a.u128 + b.u128;
    #else
    gp_u128_set_lo(&y, gp_u128_lo(a) + gp_u128_lo(b));
    gp_u128_set_hi(&y, gp_u128_hi(a) + gp_u128_hi(b)
        + (gp_u128_lo(b) > UINT64_MAX - gp_u128_lo(a))); // carry
    #endif
    return y;
}

/** Add 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_i128_add(GPInt128 a, GPInt128 b)
{
    GPInt128 y;
    #if __GNUC__ && __SIZEOF_INT128__
    y.i128 = a.i128 + b.i128;
    #else
    gp_i128_set_lo(&y, gp_i128_lo(a) + gp_i128_lo(b));
    gp_i128_set_hi(&y, gp_i128_hi(a) + gp_i128_hi(b)
        + (gp_i128_lo(b) > UINT64_MAX - gp_i128_lo(a))); // carry
    #endif
    return y;
}

/** Subtract 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_u128_sub(GPUint128 a, GPUint128 b)
{
    GPUint128 y;
    #if __GNUC__ && __SIZEOF_INT128__
    y.u128 = a.u128 - b.u128;
    #else
    gp_u128_set_lo(&y, gp_u128_lo(a) - gp_u128_lo(b));
    gp_u128_set_hi(&y, gp_u128_hi(a) - gp_u128_hi(b)
        - (gp_u128_lo(b) > gp_u128_lo(a))); // borrow
    #endif
    return y;
}

/** Subtract 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_i128_sub(GPInt128 a, GPInt128 b)
{
    GPInt128 y;
    #if __GNUC__ && __SIZEOF_INT128__
    y.i128 = a.i128 - b.i128;
    #else
    gp_i128_set_lo(&y, gp_i128_lo(a) - gp_i128_lo(b));
    gp_i128_set_hi(&y, gp_i128_hi(a) - gp_i128_hi(b)
        - (gp_i128_lo(b) > gp_i128_lo(a))); // borrow
    #endif
    return y;
}

/** Multiply 64-bit unsigned integers to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_u128_mul64(uint64_t a, uint64_t b)
{
    #if __GNUC__ && __SIZEOF_INT128__
    GPUint128 y;
    y.u128 = (__uint128_t)a * b;
    return y;
    #elif _MSC_VER && _M_X64
    uint64_t lo, hi;
    lo = _umul128(a, b, &hi);
    return gp_u128(hi, lo);
    #else // very slow, but as portable as it gets
    GPUint128 gp_u128_long_mul64(uint64_t a, uint64_t b);
    return gp_u128_long_mul64(a, b);
    #endif
}

#endif // GP_INT128_INCLUDED
