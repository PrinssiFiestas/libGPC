// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/hashmap.h"

static void gpc_mult64to128(uint64_t u, uint64_t v, uint64_t* h, uint64_t* l)
{
    uint64_t u1 = (u & 0xffffffff);
    uint64_t v1 = (v & 0xffffffff);
    uint64_t t = (u1 * v1);
    uint64_t w3 = (t & 0xffffffff);
    uint64_t k = (t >> 32);

    u >>= 32;
    t = (u * v1) + k;
    k = (t & 0xffffffff);
    uint64_t w1 = (t >> 32);

    v >>= 32;
    t = (u1 * v) + k;
    k = (t >> 32);

    *h = (u * v) + w1 + k;
    *l = (t << 32) + w3;
}

static void gpc_mult128(gpc_Uint128 N, gpc_Uint128 M, gpc_Uint128* Ans)
{
    gpc_mult64to128(N.lo, M.lo, &Ans->hi, &Ans->lo);
    Ans->hi += (N.hi * M.lo) + (N.lo * M.hi);
}

uint32_t gpc_hash32(const char* str)
{
    const uint32_t FNV_prime = 0x01000193;
    const uint32_t FNV_offset_basis = 0x811c9dc5;

    uint32_t hash = FNV_offset_basis;
    for (; *str; str++)
    {
        hash ^= *str;
        hash *= FNV_prime;
    }
    return hash;
}

uint64_t gpc_hash64(const char* str)
{
    const uint64_t FNV_prime = 0x00000100000001B3;
    const uint64_t FNV_offset_basis = 0xcbf29ce484222325;

    uint64_t hash = FNV_offset_basis;
    for (; *str; str++)
    {
        hash ^= *str;
        hash *= FNV_prime;
    }
    return hash;
}

gpc_Uint128 gpc_hash128(const char* str)
{
    const gpc_Uint128 FNV_prime = {	0x000000000000013B, 0x0000000001000000};
    const gpc_Uint128 FNV_offset_basis = { 0x62b821756295c58d, 0x6c62272e07bb0142 };

    gpc_Uint128 hash = FNV_offset_basis;
    for (; *str; str++)
    {
        hash.lo ^= *str;
        gpc_mult128(hash, FNV_prime, &hash);
    }

    return hash;
}
