// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/int128.h>

GPUint128 gp_u128_long_mul64(uint64_t a, uint64_t b)
{
    // Slice to 32-bit components
    uint64_t ah = a >> 32, al = a & 0xFFFFFFFF;
    uint64_t bh = b >> 32, bl = b & 0xFFFFFFFF;

    // Multiply all pairs of components
    uint64_t albl = al*bl, albh = al*bh, ahbl = ah*bl, ahbh = ah*bh;

    // Multiplication carries and sum carry crossing 64 bit boundary
    uint64_t cahbl = ahbl >> 32, calbh = albh >> 32;
    uint64_t carry = ((ahbl&0xFFFFFFFF) + (albh&0xFFFFFFFF) + (albl>>32)) >> 32;

    return gp_u128(
        ahbh + cahbl + calbh + carry,
        (ahbl << 32) + (albh << 32) + albl);
}
