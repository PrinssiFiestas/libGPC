// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file int128.h
 * Portable 128 bit integer
 */

// Note to everybody: Compiler Explorer is your friend.

#ifndef GP_INT128_INCLUDED
#define GP_INT128_INCLUDED 1

#include "attributes.h"
#include "overload.h"
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

#if _MSC_VER && _M_X64
// Note: MSVC sometimes produces much worse code using intrinsics, so only use
// them with some functions.
#include <intrin.h>
#endif

#if (__GNUC__ && defined(__SIZEOF_INT128__)) || __clang__ || GP_TEST_INT128
#define GP_HAS_TI_INT 1
typedef unsigned GPTIUint __attribute__((mode(TI)));
typedef int      GPTIInt  __attribute__((mode(TI)));
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


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

    #if GP_HAS_TI_INT || GP_TEST_INT128
    // Used internally for better performance, portable applications should not
    // use this.
    GPTIUint u128;
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

    #if GP_HAS_TI_INT || defined(GP_TEST_INT128)
    // Used internally for better performance, portable applications should not
    // use this.
    GPTIInt i128;
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
// Limits

#define GP_UINT128_MAX gp_uint128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
#define GP_INT128_MAX  gp_int128( 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
#define GP_INT128_MIN  gp_int128(INT64_MIN, 0)

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

#if GP_HAS_TI_INT || defined(GP_TEST_INT128)
GP_NODISCARD static inline GPUint128 gp_uint128_tiuint(GPTIUint _u)
{
    GPUint128 u;
    u.u128 = _u;
    return u;
}
GP_NODISCARD static inline GPInt128 gp_int128_tiint(GPTIInt _i)
{
    GPInt128 i;
    i.i128 = _i;
    return i;
}
#endif

#if __cplusplus // constructor overloads, available in C as macros
GP_NODISCARD static inline GPUint128 gp_u128(uint64_t hi, uint64_t lo) { return gp_uint128(hi, lo); }
GP_NODISCARD static inline GPUint128 gp_u128(uint64_t u)  { return gp_uint128(0, u); }
GP_NODISCARD static inline GPUint128 gp_u128(int64_t i)   { return gp_uint128(-(i<0), i); }
GP_NODISCARD static inline GPUint128 gp_u128(GPInt128 i)  { return gp_uint128_int128(i); }
GP_NODISCARD static inline GPUint128 gp_u128(GPUint128 u) { return u; } // may be useful for generics
GP_NODISCARD static inline GPInt128 gp_i128(int64_t hi, uint64_t lo) { return gp_int128(hi, lo); }
GP_NODISCARD static inline GPInt128 gp_i128(int64_t i)   { return gp_int128(-(i<0), i); }
GP_NODISCARD static inline GPInt128 gp_i128(uint64_t u)  { return gp_int128(0, u); }
GP_NODISCARD static inline GPInt128 gp_i128(GPUint128 u) { return gp_int128_uint128(u); }
GP_NODISCARD static inline GPInt128 gp_i128(GPInt128 i)  { return i; } // may be useful for generics
#  if GP_HAS_TI_INT
GP_NODISCARD static inline GPUint128 gp_u128(GPTIUint u) { return gp_uint128_tiuint(u); }
GP_NODISCARD static inline GPInt128  gp_i128(GPTIInt  i) { return gp_int128_tiint(i);  }
#  endif
#else
#define gp_u128(...) GP_OVERLOAD2(__VA_ARGS__, gp_uint128, GP_U128)(__VA_ARGS__)
#define gp_i128(...) GP_OVERLOAD2(__VA_ARGS__, gp_int128,  GP_I128)(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
// Bitwise operations

/** Unsigned bitwise NOT */
GP_NODISCARD static inline GPUint128 gp_uint128_not(GPUint128 a)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(~a.u128);
    #else
    return gp_uint128(~gp_uint128_hi(a), ~gp_uint128_lo(a));
    #endif
}
/** Signed bitwise NOT */
GP_NODISCARD static inline GPInt128 gp_int128_not(GPInt128 a)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(~a.i128);
    #else
    return gp_int128(~gp_int128_hi(a), ~gp_int128_lo(a));
    #endif
}

/** Unsigned bitwise AND */
GP_NODISCARD static inline GPUint128 gp_uint128_and(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 & b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) & gp_uint128_hi(b), gp_uint128_lo(a) & gp_uint128_lo(b));
    #endif
}
/** Signed bitwise AND */
GP_NODISCARD static inline GPInt128 gp_int128_and(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 & b.i128);
    #else
    return gp_int128(gp_int128_hi(a) & gp_int128_hi(b), gp_int128_lo(a) & gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise OR */
GP_NODISCARD static inline GPUint128 gp_uint128_or(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 | b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) | gp_uint128_hi(b), gp_uint128_lo(a) | gp_uint128_lo(b));
    #endif
}
/** Signed bitwise OR */
GP_NODISCARD static inline GPInt128 gp_int128_or(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 | b.i128);
    #else
    return gp_int128(gp_int128_hi(a) | gp_int128_hi(b), gp_int128_lo(a) | gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise XOR */
GP_NODISCARD static inline GPUint128 gp_uint128_xor(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 ^ b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) ^ gp_uint128_hi(b), gp_uint128_lo(a) ^ gp_uint128_lo(b));
    #endif
}
/** Signed bitwise XOR */
GP_NODISCARD static inline GPInt128 gp_int128_xor(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 ^ b.i128);
    #else
    return gp_int128(gp_int128_hi(a) ^ gp_int128_hi(b), gp_int128_lo(a) ^ gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD static inline GPUint128 gp_uint128_shift_left(GPUint128 a, uint8_t b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 << b);
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
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 << b);
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
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 >> b);
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
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 >> b);
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
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 + b.u128);
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
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 + b.i128);
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
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 - b.u128);
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
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 - b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) - gp_int128_hi(b)
            - (gp_int128_lo(b) > gp_int128_lo(a)), // borrow
        gp_int128_lo(a) - gp_int128_lo(b));
    #endif
}

/** Negate 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_uint128_negate(GPUint128 a)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(-a.u128);
    #else
    return gp_uint128(~a.hi + !a.lo, ~a.lo + 1);
    #endif
}
/** Negate 128-bit signed integer.*/
GP_NODISCARD static inline GPInt128 gp_int128_negate(GPInt128 a)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(-a.i128);
    #else
    return gp_int128_uint128(gp_uint128_negate(gp_uint128_int128(a)));
    #endif
}

/** Multiply 64-bit unsigned integers to 128-bit unsigned integer.*/
GP_NODISCARD static inline GPUint128 gp_uint128_mul64(uint64_t a, uint64_t b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint((GPTIUint)a * b);
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
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 * b.u128);
    #else
    GPUint128 y = gp_uint128_mul64(gp_uint128_lo(a), gp_uint128_lo(b));
    *gp_uint128_hi_addr(&y) += gp_uint128_hi(a)*gp_uint128_lo(b) + gp_uint128_lo(a)*gp_uint128_hi(b);
    return y;
    #endif
}
/** Multiply 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_mul(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 * b.i128);
    #else
    return gp_int128_uint128(
        gp_uint128_mul(
            gp_uint128_int128(a),
            gp_uint128_int128(b)));
    #endif
}

/** Divide 128-bit unsigned integers.*/
GP_NODISCARD static inline GPUint128 gp_uint128_div(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 / b.u128);
    #else
    GPUint128 gp_uint128_divmod(GPUint128 a, GPUint128 b, GPUint128* optional_remainder);
    return gp_uint128_divmod(a, b, NULL);
    #endif
}
/** Divide 128-bit signed integers.*/
GP_NODISCARD static inline GPInt128 gp_int128_div(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 / b.i128);
    #else
    GPInt128 gp_int128_idiv(GPInt128 a, GPInt128 b);
    return gp_int128_idiv(a, b);
    #endif
}

/** 128-bit unsigned integer modulus.*/
GP_NODISCARD static inline GPUint128 gp_uint128_mod(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return gp_uint128_tiuint(a.u128 % b.u128);
    #else
    GPUint128 gp_uint128_divmod(GPUint128 a, GPUint128 b, GPUint128* optional_remainder);
    GPUint128 remainder;
    gp_uint128_divmod(a, b, &remainder);
    return remainder;
    #endif
}
/** 128-bit signed integer modulus.*/
GP_NODISCARD static inline GPInt128 gp_int128_mod(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return gp_int128_tiint(a.i128 % b.i128);
    #else
    GPInt128 gp_int128_imod(GPInt128 a, GPInt128 b);
    return gp_int128_imod(a, b);
    #endif
}

// ----------------------------------------------------------------------------
// Comparisons

//
GP_NODISCARD static inline bool gp_uint128_equal(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return a.u128 == b.u128;
    #else
    return a.little_endian.lo == b.little_endian.lo && a.little_endian.hi == b.little_endian.hi;
    #endif
}
GP_NODISCARD static inline bool gp_int128_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return a.i128 == b.i128;
    #else
    return a.little_endian.lo == b.little_endian.lo && a.little_endian.hi == b.little_endian.hi;
    #endif
}

GP_NODISCARD static inline bool gp_uint128_not_equal(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return a.u128 != b.u128;
    #else
    return a.little_endian.lo != b.little_endian.lo || a.little_endian.hi != b.little_endian.hi;
    #endif
}
GP_NODISCARD static inline bool gp_int128_not_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return a.i128 != b.i128;
    #else
    return a.little_endian.lo != b.little_endian.lo || a.little_endian.hi != b.little_endian.hi;
    #endif
}

GP_NODISCARD static inline bool gp_uint128_greater_than(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return a.u128 > b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_lo(b))
        return gp_uint128_lo(a) > gp_uint128_lo(b);
    return gp_uint128_hi(a) > gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_greater_than(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return a.i128 > b.i128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_lo(b))
        return gp_uint128_lo(a) > gp_uint128_lo(b);
    return gp_uint128_hi(a) > gp_uint128_hi(b);
    #endif
}

GP_NODISCARD static inline bool gp_uint128_less_than(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return a.u128 < b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) < gp_uint128_lo(b);
    return gp_uint128_hi(a) < gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_less_than(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return a.i128 < b.i128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) < gp_uint128_lo(b);
    return gp_uint128_hi(a) < gp_uint128_hi(b);
    #endif
}

GP_NODISCARD static inline bool gp_uint128_greater_than_equal(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return a.u128 >= b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_lo(b))
        return gp_uint128_lo(a) >= gp_uint128_lo(b);
    return gp_uint128_hi(a) >= gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_greater_than_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return a.i128 >= b.i128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_lo(b))
        return gp_uint128_lo(a) >= gp_uint128_lo(b);
    return gp_uint128_hi(a) >= gp_uint128_hi(b);
    #endif
}

GP_NODISCARD static inline bool gp_uint128_less_than_equal(GPUint128 a, GPUint128 b)
{
    #if GP_HAS_TI_INT
    return a.u128 <= b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_lo(b))
        return gp_uint128_lo(a) <= gp_uint128_lo(b);
    return gp_uint128_hi(a) <= gp_uint128_hi(b);
    #endif
}
GP_NODISCARD static inline bool gp_int128_less_than_equal(GPInt128 a, GPInt128 b)
{
    #if GP_HAS_TI_INT
    return a.i128 <= b.i128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_lo(b))
        return gp_uint128_lo(a) <= gp_uint128_lo(b);
    return gp_uint128_hi(a) <= gp_uint128_hi(b);
    #endif
}

#if __cplusplus // operator overloads
// Note: the provided set of overloads may seem limited, but a lot of type
// combinations produce ambiguities, are bug prone, or both, so only overload
// carefully selected combinations of types.

// Bitwise operators
GP_NODISCARD static inline GPUint128 operator ~(GPUint128  a) { return gp_uint128_not(a); }
GP_NODISCARD static inline GPInt128  operator ~(GPInt128   a)  { return gp_int128_not(a);  }
GP_NODISCARD static inline GPUint128 operator &(GPUint128  a, GPUint128 b) { return gp_uint128_and(a, b); }
GP_NODISCARD static inline GPInt128  operator &(GPInt128   a, GPInt128 b)  { return gp_int128_and(a, b) ; }
GP_NODISCARD static inline GPUint128 operator |(GPUint128  a, GPUint128 b) { return gp_uint128_or(a, b) ; }
GP_NODISCARD static inline GPInt128  operator |(GPInt128   a, GPInt128 b)  { return gp_int128_or(a, b)  ; }
GP_NODISCARD static inline GPUint128 operator ^(GPUint128  a, GPUint128 b) { return gp_uint128_xor(a, b); }
GP_NODISCARD static inline GPInt128  operator ^(GPInt128   a, GPInt128 b)  { return gp_int128_xor(a, b) ; }
GP_NODISCARD static inline GPUint128 operator <<(GPUint128 a, uint8_t b)   { return gp_uint128_shift_left(a, b) ; }
GP_NODISCARD static inline GPInt128  operator <<(GPInt128  a, uint8_t b)   { return gp_int128_shift_left(a, b)  ; }
GP_NODISCARD static inline GPUint128 operator >>(GPUint128 a, uint8_t b)   { return gp_uint128_shift_rigth(a, b); }
GP_NODISCARD static inline GPInt128  operator >>(GPInt128  a, uint8_t b)   { return gp_int128_shift_right(a, b) ; }
#endif // __cplusplus // operator overloads


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#ifdef GP_INT128_SELECTION
#undef GP_INT128_SELECTION
#undef GP_UINT128_SELECTION
#endif
#define GP_INT128_SELECTION(...)  GPInt128:  __VA_ARGS__
#define GP_UINT128_SELECTION(...) GPUint128: __VA_ARGS__

#if __STDC_VERSION__ >= 201112L

GP_NODISCARD static inline GPUint128 gp_uint128_u64(uint64_t u) { return gp_uint128(0, u);      }
GP_NODISCARD static inline GPUint128 gp_uint128_i64(int64_t  i) { return gp_uint128(-(i<0), i); }
GP_NODISCARD static inline GPUint128 gp_uint128_uint128(GPUint128 u) { return u;                }
GP_NODISCARD static inline GPInt128  gp_int128_u64(uint64_t  u) { return gp_int128(0, u);       }
GP_NODISCARD static inline GPInt128  gp_int128_i64(int64_t   i) { return gp_int128(-(i<0), i);  }
GP_NODISCARD static inline GPInt128  gp_int128_int128(GPInt128 i) { return i;                   }
#  if GP_HAS_TI_INT // use implicit integer conversions
#    define GP_U128(A) _Generic(A, GPUint128: gp_uint128_uint128, GPInt128: gp_uint128_int128, default: gp_uint128_tiuint)(A)
#    define GP_I128(A) _Generic(A, GPUint128: gp_int128_uint128,  GPInt128: gp_int128_int128,  default: gp_int128_tiint)(A)
#  else
#    define GP_U128(A) _Generic(A, \
         GP_C11_GENERIC_SIGNED_INTEGER(gp_uint128_i64), \
         GP_C11_GENERIC_UNSIGNED_INTEGER(gp_uint128_u64), \
         GPUint128: gp_uint128_uint128, GPInt128: gp_uint128_int128)(A)
#    define GP_I128(A) _Generic(A, \
         GP_C11_GENERIC_SIGNED_INTEGER(gp_int128_i64), \
         GP_C11_GENERIC_UNSIGNED_INTEGER(gp_int128_u64), \
         GPUint128: gp_int128_uint128, GPInt128: gp_int128_int128)(A)
#  endif
#else // C99, no type safety

#include <stdarg.h>
#define GP_U128(A) gp_u128_raw_ctor(sizeof(A), ((A)<0), A)
#define GP_I128(A) gp_i128_raw_ctor(sizeof(A), ((A)<0), A)

GP_NODISCARD static GPUint128 gp_u128_raw_ctor(size_t size, int negative, ...)
{
    GPUint128 u;
    va_list args;
    va_start(args, negative);
    switch (size) {
        case sizeof(GPInt128): u =                       va_arg(args, GPUint128); break;
        case sizeof(uint64_t): u = gp_uint128(-negative, va_arg(args, uint64_t)); break;
        case sizeof(uint32_t): u = gp_uint128(-negative, va_arg(args, uint32_t)); break;
        // default: invoke UB by returning uninitialzied u, no need for GP_UNREACHABLE
    }
    va_end(args);
    return u;
}
GP_NODISCARD static GPInt128 gp_i128_raw_ctor(size_t size, int negative, ...)
{
    GPInt128 i;
    va_list args;
    va_start(args, negative);
    if (size == sizeof(GPInt128))
        i = va_arg(args, GPInt128);
    else {
        int64_t i64 = size == sizeof(int64_t) ?
            va_arg(args, int64_t)
          : va_arg(args, int32_t);
        i = gp_int128(-negative, i64);
    }
    va_end(args);
    return i;
}
#endif // __STDC_VERSION__ >= 201112L

#endif // GP_INT128_INCLUDED
