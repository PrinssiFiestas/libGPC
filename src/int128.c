// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/int128.h>

GPUint128 gp_uint128_long_mul64(uint64_t a, uint64_t b)
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

// Yoinked from LLVM
// https://github.com/llvm-mirror/compiler-rt/blob/master/lib/builtins/udivmodti4.c
GPUint128 gp_uint128_divmod(GPUint128 a, GPUint128 b, GPUint128 *rem)
{
    const unsigned n_udword_bits = sizeof(uint64_t) * CHAR_BIT;
    const unsigned n_utword_bits = sizeof(GPUint128) * CHAR_BIT;
    GPUint128 n = a;
    GPUint128 d = b;
    GPUint128 q;
    GPUint128 r;
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
          // 0 0
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
          return gp_uint128(0, gp_uint128_hi(n) >> __builtin_ctzll(gp_uint128_hi(d)));
      }
      // K K
      // ---
      // K 0
      sr = __builtin_clzll(gp_uint128_hi(d)) - __builtin_clzll(gp_uint128_hi(n));
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
              sr = __builtin_ctzll(gp_uint128_lo(d));
              *gp_uint128_hi_addr(&q) = gp_uint128_hi(n) >> sr;
              *gp_uint128_lo_addr(&q) = (gp_uint128_hi(n) << (n_udword_bits - sr)) | (gp_uint128_lo(n) >> sr);
              return q;
          }
          // K X
          // ---
          // 0 K
          sr = 1 + n_udword_bits + __builtin_clzll(gp_uint128_lo(d)) -
               __builtin_clzll(gp_uint128_hi(n));
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
            sr = __builtin_clzll(gp_uint128_hi(d)) - __builtin_clzll(gp_uint128_hi(n));
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
    GPUint128 r;
    gp_uint128_divmod(gp_uint128_i128(a), gp_uint128_i128(b), &r);
    return gp_int128_sub(gp_int128_xor(gp_int128_u128(r), s), s); // negate if s == -1
}
