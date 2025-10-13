// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/int128.h>
#include <gpc/assert.h>
#include <stdarg.h>
#include "common.h"

GPUInt128 gp_u128_c99_ctor(size_t size, ...)
{
    GPUInt128 u;
    va_list args;
    va_start(args, size);
    switch (size) {
    case sizeof(GPInt128): u =               va_arg(args, GPUInt128); break;
    case sizeof(uint64_t): u = gp_uint128(0, va_arg(args, gp_promoted_arg_uint64_t)); break;
    case sizeof(uint32_t): u = gp_uint128(0, va_arg(args, gp_promoted_arg_uint32_t)); break;
    case sizeof(uint16_t): u = gp_uint128(0, va_arg(args, gp_promoted_arg_uint16_t)); break;
    case sizeof(uint8_t) : u = gp_uint128(0, va_arg(args, gp_promoted_arg_uint8_t )); break;
    default: GP_UNREACHABLE("gp_u128(): invalid argument.");
    }
    va_end(args);
    return u;
}
GPInt128 gp_i128_c99_ctor(size_t size, ...)
{
    int64_t lo;
    va_list args;
    va_start(args, size);
    switch (size) {
    case sizeof(GPInt128):; GPInt128 i = va_arg(args, GPInt128);
        va_end(args);
        return i;
    case sizeof(int64_t): lo = va_arg(args, gp_promoted_arg_int64_t); break;
    case sizeof(int32_t): lo = va_arg(args, gp_promoted_arg_int32_t); break;
    case sizeof(int16_t): lo = va_arg(args, gp_promoted_arg_int16_t); break;
    case sizeof(int8_t ): lo = va_arg(args, gp_promoted_arg_int8_t ); break;
    default: GP_UNREACHABLE("gp_i128(): invalid argument.");
    }
    va_end(args);
    return gp_int128(lo<0, lo);
}
GPUInt128 gp_uint128_long_mul64(uint64_t a, uint64_t b)
{
    if ((a | b) <= UINT32_MAX)
        return gp_uint128(0, a*b);
    // Optimizing for other edge cases is probably not worth the extra branches
    // and implementation complexity. Or is it? Todo: benchmark on 32-bit ARM.

    // Slice to 32-bit components
    uint64_t ah = a >> 32, al = a & 0xFFFFFFFF;
    uint64_t bh = b >> 32, bl = b & 0xFFFFFFFF;

    // Multiply all pairs of components
    uint64_t albl = al*bl, albh = al*bh, ahbl = ah*bl, ahbh = ah*bh;

    // Multiplication carries and sum carry crossing 64 bit boundary
    uint64_t cahbl = ahbl >> 32, calbh = albh >> 32;
    uint64_t c = ((ahbl&0xFFFFFFFF) + (albh&0xFFFFFFFF) + (albl>>32)) >> 32;

    return gp_uint128(
        ahbh + cahbl + calbh + c,
        (ahbl << 32) + (albh << 32) + albl);
}

static size_t gp_trailing_zeros_u64(uint64_t u)
{
    gp_db_assert(u != 0, "Invalid argument.");

    // Note: C23 stdc_trailing_zeros() breaks builds, don't use it!
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

static size_t gp_leading_zeros_u64(uint64_t u)
{
    gp_db_assert(u != 0, "Invalid argument.");

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

// Yoinked from LLVM
// https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/udivmodti4.c
GPUInt128 gp_uint128_divmod(GPUInt128 a, GPUInt128 b, GPUInt128 *rem)
{
    gp_db_assert(gp_uint128_not_equal(b, gp_uint128(0, 0)), "Division by zero.");

    const unsigned n_udword_bits = sizeof(uint64_t)  * CHAR_BIT;
    const unsigned n_utword_bits = sizeof(GPUInt128) * CHAR_BIT;
    GPUInt128 n = a;
    GPUInt128 d = b;
    GPUInt128 q;
    GPUInt128 r;
    unsigned sr;
    // special cases, X is unknown, K != 0
    if (gp_uint128_hi(n) == 0) {
        if (gp_uint128_hi(d) == 0) {
            // 0 X
            // ---
            // 0 X
            if (rem)
                *rem = gp_uint128(0, gp_uint128_lo(n) % gp_uint128_lo(d));
            return gp_uint128(0, gp_uint128_lo(n) / gp_uint128_lo(d));
        }
        // 0 X
        // ---
        // K X
        if (rem)
            *rem = gp_uint128(0, gp_uint128_lo(n));
        return gp_uint128(0, 0);
    }
    // gp_uint128_hi(n) != 0
    if (gp_uint128_lo(d) == 0) {
      if (gp_uint128_hi(d) == 0) {
          // K X
          // ---
          // 0 0 // wait, we're handling zero division??
          if (rem)
              *rem = gp_uint128(0, gp_uint128_hi(n) % gp_uint128_lo(d));
          return gp_uint128(0, gp_uint128_hi(n) / gp_uint128_lo(d));
      }
      // gp_uint128_hi(d) != 0
      if (gp_uint128_lo(n) == 0) {
          // K 0
          // ---
          // K 0
          if (rem) {
              *gp_uint128_hi_addr(&r) = gp_uint128_hi(n) % gp_uint128_hi(d);
              *gp_uint128_lo_addr(&r) = 0;
              *rem = r;
          }
          return gp_uint128(0, gp_uint128_hi(n) / gp_uint128_hi(d));
      }
      // K K
      // ---
      // K 0
      if ((gp_uint128_hi(d) & (gp_uint128_hi(d) - 1)) == 0) /* if d is a power of 2 */ {
          if (rem) {
              *gp_uint128_lo_addr(&r) = gp_uint128_lo(n);
              *gp_uint128_hi_addr(&r) = gp_uint128_hi(n) & (gp_uint128_hi(d) - 1);
              *rem = r;
          }
          return gp_uint128(0, gp_uint128_hi(n) >> gp_trailing_zeros_u64(gp_uint128_hi(d)));
      }
      // K K
      // ---
      // K 0
      sr = gp_leading_zeros_u64(gp_uint128_hi(d)) - gp_leading_zeros_u64(gp_uint128_hi(n));
      // 0 <= sr <= n_udword_bits - 2 or sr large
      if (sr > n_udword_bits - 2) {
          if (rem)
              *rem = n;
          return gp_uint128(0, 0);
      }
      ++sr;
      // 1 <= sr <= n_udword_bits - 1
      // q.all = n.all << (n_utword_bits - sr);
      *gp_uint128_lo_addr(&q) = 0;
      *gp_uint128_hi_addr(&q) = gp_uint128_lo(n) << (n_udword_bits - sr);
      // r.all = n.all >> sr;
      *gp_uint128_hi_addr(&r) = gp_uint128_hi(n) >> sr;
      *gp_uint128_lo_addr(&r) = (gp_uint128_hi(n) << (n_udword_bits - sr)) | (gp_uint128_lo(n) >> sr);
    } else /* gp_uint128_lo(d) != 0 */ {
        if (gp_uint128_hi(d) == 0) {
          // K X
          // ---
          // 0 K
          if ((gp_uint128_lo(d) & (gp_uint128_lo(d) - 1)) == 0) /* if d is a power of 2 */ {
              if (rem)
                  *rem = gp_uint128(0, gp_uint128_lo(n) & (gp_uint128_lo(d) - 1));
              if (gp_uint128_lo(d) == 1)
                  return n;
              sr = gp_trailing_zeros_u64(gp_uint128_lo(d));
              *gp_uint128_hi_addr(&q) = gp_uint128_hi(n) >> sr;
              *gp_uint128_lo_addr(&q) = (gp_uint128_hi(n) << (n_udword_bits - sr)) | (gp_uint128_lo(n) >> sr);
              return q;
          }
          // K X
          // ---
          // 0 K
          sr = 1 + n_udword_bits + gp_leading_zeros_u64(gp_uint128_lo(d)) -
               gp_leading_zeros_u64(gp_uint128_hi(n));
          // 2 <= sr <= n_utword_bits - 1
          // q = n << (n_utword_bits - sr);
          // r = n >> sr;
          if (sr == n_udword_bits) {
              *gp_uint128_lo_addr(&q) = 0;
              *gp_uint128_hi_addr(&q) = gp_uint128_lo(n);
              *gp_uint128_hi_addr(&r) = 0;
              *gp_uint128_lo_addr(&r) = gp_uint128_hi(n);
          } else if (sr < n_udword_bits) /* 2 <= sr <= n_udword_bits - 1 */ {
              *gp_uint128_lo_addr(&q) = 0;
              *gp_uint128_hi_addr(&q) = gp_uint128_lo(n) << (n_udword_bits - sr);
              *gp_uint128_hi_addr(&r) = gp_uint128_hi(n) >> sr;
              *gp_uint128_lo_addr(&r) = (gp_uint128_hi(n) << (n_udword_bits - sr)) | (gp_uint128_lo(n) >> sr);
          } else /* n_udword_bits + 1 <= sr <= n_utword_bits - 1 */ {
              *gp_uint128_lo_addr(&q) = gp_uint128_lo(n) << (n_utword_bits - sr);
              *gp_uint128_hi_addr(&q) = (gp_uint128_hi(n) << (n_utword_bits - sr)) |
                         (gp_uint128_lo(n) >> (sr - n_udword_bits));
              *gp_uint128_hi_addr(&r) = 0;
              *gp_uint128_lo_addr(&r) = gp_uint128_hi(n) >> (sr - n_udword_bits);
          }
        } else {
            // K X
            // ---
            // K K
            sr = gp_leading_zeros_u64(gp_uint128_hi(d)) - gp_leading_zeros_u64(gp_uint128_hi(n));
            // 0 <= sr <= n_udword_bits - 1 or sr large
            if (sr > n_udword_bits - 1) {
                if (rem)
                    *rem = n;
                return gp_uint128(0, 0);
            }
            ++sr;
            // 1 <= sr <= n_udword_bits
            // q = n << (n_utword_bits - sr);
            // r = n >> sr;
            *gp_uint128_lo_addr(&q) = 0;
            if (sr == n_udword_bits) {
                *gp_uint128_hi_addr(&q) = gp_uint128_lo(n);
                *gp_uint128_hi_addr(&r) = 0;
                *gp_uint128_lo_addr(&r) = gp_uint128_hi(n);
            } else {
                *gp_uint128_hi_addr(&r) = gp_uint128_hi(n) >> sr;
                *gp_uint128_lo_addr(&r) = (gp_uint128_hi(n) << (n_udword_bits - sr)) | (gp_uint128_lo(n) >> sr);
                *gp_uint128_hi_addr(&q) = gp_uint128_lo(n) << (n_udword_bits - sr);
            }
        }
    }
    // Not a special case
    // q and r are initialized with:
    // q = n << (n_utword_bits - sr);
    // r = n >> sr;
    // 1 <= sr <= n_utword_bits - 1
    unsigned carry = 0;
    for (; sr > 0; --sr) {
        // r:q = ((r:q)  << 1) | carry
        *gp_uint128_hi_addr(&r) = (gp_uint128_hi(r) << 1) | (gp_uint128_lo(r) >> (n_udword_bits - 1));
        *gp_uint128_lo_addr(&r) = (gp_uint128_lo(r) << 1) | (gp_uint128_hi(q) >> (n_udword_bits - 1));
        *gp_uint128_hi_addr(&q) = (gp_uint128_hi(q) << 1) | (gp_uint128_lo(q) >> (n_udword_bits - 1));
        *gp_uint128_lo_addr(&q) = (gp_uint128_lo(q) << 1) | carry;
        // carry = 0;
        // if (r >= d)
        // {
        //     r -= d;
        //      carry = 1;
        // }
        const GPInt128 s = gp_int128_shift_right(
            gp_int128_u128(gp_uint128_sub(
                gp_uint128_sub(d, r),
                gp_uint128(0, 1))),
            n_utword_bits - 1);
        carry = gp_int128_lo(s) & 1;
        r = gp_uint128_sub(r, gp_uint128_and(d, gp_uint128_i128(s)));
    }
    q = gp_uint128_or(gp_uint128_shift_left(q, 1), gp_uint128(0, carry));
    if (rem)
        *rem = r;
    return q;
}

// Yoinked from LLVM
// https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/divti3.c
GPInt128 gp_int128_idiv(GPInt128 a, GPInt128 b)
{
    const int bits_in_tword_m1 = (int)(sizeof(GPInt128) * CHAR_BIT) - 1;
    GPInt128 s_a = gp_int128_shift_right(a, bits_in_tword_m1); // s_a = a < 0 ? -1 : 0
    GPInt128 s_b = gp_int128_shift_right(b, bits_in_tword_m1); // s_b = b < 0 ? -1 : 0
    a = gp_int128_sub(gp_int128_xor(a, s_a), s_a);             // negate if s_a == -1
    b = gp_int128_sub(gp_int128_xor(b, s_b), s_b);             // negate if s_b == -1
    s_a = gp_int128_xor(s_a, s_b);                             // sign of quotient
    return gp_int128_sub(
        gp_int128_xor(
            gp_int128_u128(gp_uint128_divmod(
                gp_uint128_i128(a),
                gp_uint128_i128(b),
                NULL)),
            s_a),
        s_a); // negate if s_a == -1
}

// Yoinked from LLVM
// https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/modti3.c
GPInt128 gp_int128_imod(GPInt128 a, GPInt128 b)
{
    const int bits_in_tword_m1 = (int)(sizeof(GPInt128) * CHAR_BIT) - 1;
    GPInt128 s = gp_int128_shift_right(b, bits_in_tword_m1); // s = b < 0 ? -1 : 0
    b = gp_int128_sub(gp_int128_xor(b, s), s);               // negate if s == -1
    s = gp_int128_shift_right(a, bits_in_tword_m1);          // s = a < 0 ? -1 : 0
    a = gp_int128_sub(gp_int128_xor(a, s), s);               // negate if s == -1
    GPUInt128 r;
    gp_uint128_divmod(gp_uint128_i128(a), gp_uint128_i128(b), &r);
    return gp_int128_sub(gp_int128_xor(gp_int128_u128(r), s), s); // negate if s == -1
}

// ----------------------------------------------------------------------------
// Floating Point Conversions

// TODO some of the assertions are a bit harsh. LLVM saturates, which is what
// user may expect. It might be a good idea to provide 'safe' alternatives that
// return boolean if saturated and only assert NAN.

GPUInt128 gp_uint128_convert_f64(double a)
{
    // TODO these assertions are only conditionally compiled due to CompCert
    // failing to compile. So isinf() and isnan() is not portable, we need to
    // provide our own.
    #if __GNUC__ || _MSC_VER
    gp_db_assert( ! isnan(a), "Nan cannot be represented as an integral type.");
    gp_db_assert( ! isinf(a), "Inf cannot be represented as an integral type.");
    #endif
    gp_db_assert(a >= 0., "Negative value cannot be represented as an unsigned type.");

    uint64_t a_bits;
    memcpy(&a_bits, &a, sizeof a);
    int exponent = (a_bits >> 52) - 1023;
    if (exponent < 0)
        return gp_uint128(0, 0);
    gp_db_assert(exponent < 128 + 52, // failing this cause UB anyway
        "Value too big to be represented as an 128-bit unsigned int.");

    uint64_t fraction_bits = (a_bits & ((1llu << 52) - 1)) | (1llu << 52); // = fraction bits | implicit 1
    if (exponent > 52)
        return gp_uint128_shift_left(gp_uint128(0, fraction_bits), exponent - 52);
    return gp_uint128(0, fraction_bits >> (52 - exponent));
}
GPInt128 gp_int128_convert_f64(double a)
{
    #if __GNUC__ || _MSC_VER
    gp_db_assert( ! isnan(a), "Nan cannot be represented as an integral type.");
    gp_db_assert( ! isinf(a), "Inf cannot be represented as an integral type.");
    #endif

    uint64_t a_bits;
    memcpy(&a_bits, &a, sizeof a);
    uint64_t sign_bit = a_bits & (1llu << 63);
    uint64_t abs_a = a_bits & (-1llu >> 1);
    int exponent = (abs_a >> 52) - 1023;
    if (exponent <= 0)
        return gp_int128(0, 0);
    gp_db_assert(exponent < 127 + 52, // failing this cause UB anyway
        "Value too big to be represented as an 128-bit signed int.");

    uint64_t fraction_bits = (abs_a & ((1llu << 52) - 1)) | (1llu << 52); // = fraction bits | implicit 1
    GPUInt128 u = exponent > 52 ?
        gp_uint128_shift_left(gp_uint128(0, fraction_bits), exponent - 52)
      : gp_uint128(0, fraction_bits >> (52 - exponent));
    if (sign_bit)
        return gp_int128_u128(gp_uint128_negate(u));
    return gp_int128_u128(u);
}
GPUInt128 gp_uint128_convert_f32(float a)
{
    #if __GNUC__ || _MSC_VER
    gp_db_assert( ! isnan(a), "Nan cannot be represented as an integral type.");
    gp_db_assert( ! isinf(a), "Inf cannot be represented as an integral type.");
    #endif
    gp_db_assert(a >= 0.f, "Negative value cannot be represented as an unsigned type.");

    uint32_t a_bits;
    memcpy(&a_bits, &a, sizeof a);
    int exponent = (a_bits >> 23) - 127;
    if (exponent < 0)
        return gp_uint128(0, 0);
    // always fits, no need to assert it

    uint32_t fraction_bits = (a_bits & ((1u << 23) - 1)) | (1u << 23); // = fraction bits | implicit 1
    if (exponent > 23)
        return gp_uint128_shift_left(gp_uint128(0, fraction_bits), exponent - 23);
    return gp_uint128(0, fraction_bits >> (23 - exponent));
}
GPInt128 gp_int128_convert_f32(float a)
{
    #if __GNUC__ || _MSC_VER
    gp_db_assert( ! isnan(a), "Nan cannot be represented as an integral type.");
    gp_db_assert( ! isinf(a), "Inf cannot be represented as an integral type.");
    #endif

    uint32_t a_bits;
    memcpy(&a_bits, &a, sizeof a);
    uint32_t sign_bit = a_bits & (1u << 31);
    uint32_t abs_a = a_bits & (-1u >> 1);
    int exponent = (abs_a >> 23) - 127;
    if (exponent <= 0)
        return gp_int128(0, 0);
    // always fits, no need to assert it

    uint32_t fraction_bits = (abs_a & ((1llu << 23) - 1)) | (1llu << 23); // = fraction bits | implicit 1
    GPUInt128 u = exponent > 23 ?
        gp_uint128_shift_left(gp_uint128(0, fraction_bits), exponent - 23)
      : gp_uint128(0, fraction_bits >> (23 - exponent));
    if (sign_bit)
        return gp_int128_u128(gp_uint128_negate(u));
    return gp_int128_u128(u);
}

// From Mara's blog post
// https://blog.m-ou.se/floats/
double gp_f64_convert_uint128(GPUInt128 x)
{
    if (gp_uint128_hi(x) == 0)
        return gp_uint128_lo(x);

    #if __SIZEOF_INT128__ // optimize for 64-bit targets if __uint128_t available
    // https://github.com/m-ou-se/floatconv/blob/main/src/special.rs
    union punner { double f; uint64_t u; } A, B, C, D, l, h;
    A.u = 0x4330000000000000; // (double)(__uint128_t)1 << 52
    B.u = 0x4670000000000000; // (double)(__uint128_t)1 << 104
    C.u = 0x44b0000000000000; // (double)(__uint128_t)1 << 76
    D.u = 0x47f0000000000000; // (double)(__uint128_t)-1
    if (gp_uint128_hi(x) < 1llu << (104-64)) {
        l.u = A.u | (gp_uint128_lo(x) << 12) >> 12; l.f -= A.f;
        h.u = B.u | gp_uint128_lo(gp_uint128_shift_right(x, 52)); h.f -= B.f;
    } else {
        l.u = C.u | (gp_uint128_lo(gp_uint128_shift_right(x, 12)) >> 12) | (gp_uint128_lo(x) & 0xFFFFFF); l.f -= C.f;
        h.u = D.u | gp_uint128_lo(gp_uint128_shift_right(x, 76)); h.f -= D.f;
    }
    return l.f + h.f;
    #else
    // https://github.com/m-ou-se/floatconv/blob/main/src/soft.rs
    size_t n = gp_leading_zeros_u64(gp_uint128_hi(x));
    GPUInt128 y = gp_uint128_shift_left(x, n); // 0 handled, no need for wrapping_shl()
    uint64_t a = gp_uint128_lo(gp_uint128_shift_right(y, 75)); // Significant bits, with bit 53 still intact
    uint64_t b = gp_uint128_lo(gp_uint128_shift_right(y, 11)) | (gp_uint128_lo(y) & 0xFFFFFFFF); // Insignificant bits, only relevant for rounding.
    uint64_t m = a + ((b - (b >> 63 & !a)) >> 63); // Add one when we need to round up. Break ties to even.
    uint64_t e = 1149 - n; // Exponent plus 1023, minus one.
    union punner { double f; uint64_t u; } ret;
    ret.u = (e << 52) + m; // + not |, so the mantissa can overflow into the exponent.
    return ret.f;
    #endif
}
double gp_f64_convert_int128(GPInt128 x)
{
    if (gp_int128_hi(x) >= 0)
        return gp_f64_convert_uint128(gp_uint128_i128(x));
    return -gp_f64_convert_uint128(gp_uint128_negate(gp_uint128_i128(x)));
}
float gp_f32_convert_uint128(GPUInt128 x)
{
    if (gp_uint128_hi(x) == 0)
        return gp_uint128_lo(x);

    // https://github.com/m-ou-se/floatconv/blob/main/src/soft.rs
    size_t n = gp_leading_zeros_u64(gp_uint128_hi(x));
    GPUInt128 y = gp_uint128_shift_left(x, n); // 0 handled, no need for wrapping_shl()
    uint32_t a = gp_uint128_lo(gp_uint128_shift_right(y, 104)); // Significant bits, with bit 24 still intact
    uint32_t b = (uint32_t)gp_uint128_lo(gp_uint128_shift_right(y, 72))
        | (uint32_t)!!gp_uint128_lo(gp_uint128_shift_right(
            gp_uint128_shift_left(y, 32),
            32)); // Insignificant bits, only relevant for rounding.
    uint32_t m = a + ((b - (b >> 31 & !a)) >> 31); // Add one when we need to round up. Break ties to even.
    uint32_t e = 253 - n; // Exponent plus 127, minus one.
    union punner { float f; uint32_t u; } ret;
    ret.u = (e << 23) + m; // + not |, so the mantissa can overflow into the exponent.
    return ret.f;
}
float gp_f32_convert_int128(GPInt128 x)
{
    if (gp_int128_hi(x) >= 0)
        return gp_f32_convert_uint128(gp_uint128_i128(x));
    return -gp_f32_convert_uint128(gp_uint128_negate(gp_uint128_i128(x)));
}
