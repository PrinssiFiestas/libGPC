// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file hashmap.h
 * @brief Dictionary data structure
 */

#ifndef GPC_HASHMAP_INCLUDED
#define GPC_HASHMAP_INCLUDED

#include "memory.h"
#include "attributes.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

typedef struct gp_map      GPMap;
typedef struct gp_hash_map GPHashMap;

// Optional attributes
typedef struct gp_map_initializer
{
    // If 0, elements are assumed to be pointers.
    size_t element_size;

    // Should be a power of 2. Defaults to 256
    size_t capacity;

    // If element_size != 0, argument is pointer to the element, else argument
    // is the actual pointer. In the latter case an example of a valid
    // destructor is free().
    void (*destructor)(void* element);
} GPMapInitializer;

// ------------------
// 128-bit uint

union gp_endianness_detector
{
    uint16_t u16;
    struct { uint8_t is_little; uint8_t is_big; } endianness;
};
extern const union gp_endianness_detector GP_INTEGER; // = {.u16 = 1 }

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

    #if __GNUC__
    __uint128_t u128;
    #endif
} GPUint128;

GP_NONNULL_ARGS_AND_RETURN
inline uint64_t* gp_u128_lo(const GPUint128* u)
{
    return (uint64_t*)(GP_INTEGER.endianness.is_little ?
        &u->little_endian.lo : &u->big_endian.lo);
}
GP_NONNULL_ARGS_AND_RETURN
inline uint64_t* gp_u128_hi(const GPUint128* u)
{
    return (uint64_t*)(GP_INTEGER.endianness.is_little ?
        &u->little_endian.hi : &u->big_endian.hi);
}

// ------------------
// Hash map

GPHashMap* gp_hash_map_new(
    const GPAllocator*,
    const GPMapInitializer* optional) GP_NONNULL_ARGS(1) GP_NONNULL_RETURN;

void gp_hash_map_delete(GPHashMap*);

void gp_hash_map_set(
    GPHashMap*,
    const void* key,
    size_t      key_size,
    const void* value) GP_NONNULL_ARGS();

// Returns NULL if not found
void* gp_hash_map_get(
    GPHashMap*,
    const void* key,
    size_t      key_size) GP_NONNULL_ARGS();

bool gp_hash_map_remove(
    GPHashMap*,
    const void* key,
    size_t      key_size) GP_NONNULL_ARGS();

// ------------------
// Non-hashed map

GPMap* gp_map_new(
    const GPAllocator*,
    const GPMapInitializer* optional) GP_NONNULL_ARGS(1) GP_NONNULL_RETURN;

void gp_map_delete(GPMap*);

void gp_map_set(
    GPMap*,
    GPUint128   key,
    const void* value) GP_NONNULL_ARGS();

// Returns NULL if not found
void* gp_map_get(
    GPMap*,
    GPUint128 key) GP_NONNULL_ARGS();

// Returns false if not found
bool gp_map_remove(
    GPMap*,
    GPUint128 key) GP_NONNULL_ARGS();

// ------------------
// Hashing

uint32_t  gp_bytes_hash32 (const void* key, size_t key_size) GP_NONNULL_ARGS();
uint64_t  gp_bytes_hash64 (const void* key, size_t key_size) GP_NONNULL_ARGS();
GPUint128 gp_bytes_hash128(const void* key, size_t key_size) GP_NONNULL_ARGS();


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#endif // GPC_HASHMAP_INCLUDED
