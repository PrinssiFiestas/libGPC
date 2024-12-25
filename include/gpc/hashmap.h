// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file hashmap.h
 * Hashing and hash maps
 */

#ifndef GPC_HASHMAP_INCLUDED
#define GPC_HASHMAP_INCLUDED 1

#include "memory.h"
#include "attributes.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

union gp_endianness_detector
{
    uint16_t u16;
    struct { uint8_t is_little; uint8_t is_big; } endianness;
};


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// ------------------
// 128-bit uint

/** Endianness detection.
 * Use GP_INTEGER.endianness.is_little or GP_INTEGER.endianness.is_big to check
 * the endianness of your system.
 */
extern const union gp_endianness_detector GP_INTEGER; // = {.u16 = 1 }

/** 128-bit unsigned integer.
 * No arithmetic operations implemented, these are only used as keys for GPMap.
 */
typedef union gp_uint128
{
    struct {
        #if __STDC_VERSION__ >= 201112L
        alignas(16)
        #endif
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

/** Create 128-bit unsigned integer.*/
inline GPUint128 gp_u128(const uint64_t hi_bits, const uint64_t lo_bits)
{
    GPUint128 u128;
    if (GP_INTEGER.endianness.is_big) {
        u128.big_endian.hi = hi_bits;
        u128.big_endian.lo = lo_bits;
    } else {
        u128.little_endian.hi = hi_bits;
        u128.little_endian.lo = lo_bits;
    }
    return u128;
}

/** Access low bits of 128-bit unsigned integer.
 * @return pointer to low bits.
 */
GP_NONNULL_ARGS_AND_RETURN
inline uint64_t* gp_u128_lo(const GPUint128* u)
{
    return (uint64_t*)(GP_INTEGER.endianness.is_little ?
        &u->little_endian.lo : &u->big_endian.lo);
}

/** Access high bits of 128-bit unsigned integer.
 * @return pointer to high bits.
 */
GP_NONNULL_ARGS_AND_RETURN
inline uint64_t* gp_u128_hi(const GPUint128* u)
{
    return (uint64_t*)(GP_INTEGER.endianness.is_little ?
        &u->little_endian.hi : &u->big_endian.hi);
}

// ------------------
// Hash map

/** Hash map using 128-bit keys.
 * Internally a tree of arrays. Simply uses lowest n bits from the key to index
 * to an array of size 2^n. In case of collisions, a new array of size 2^(n - 1)
 * is created and the last slot is set to point to the new array. Then the next
 * lowest n - 1 bits from the key are used to index to the new array.
 */
typedef struct gp_map      GPMap;

/** Hash map using any bytes as keys.
 * Internally based on GPMap.
 * Keys are hashed to 128-bit keys with fast, but non-cryptographic, FNV hashing
 * function.
 */
typedef struct gp_hash_map GPHashMap;

/** Optional hash map attributes.*/
typedef struct gp_map_initializer
{
    /** 0 for pointers, else elements stored in map memory.*/
    size_t element_size;

    /** Initial capacity.
     * Should be a power of 2. Defaults to 256.
     */
    size_t capacity;

    /** Element destructor.
     * If element_size != 0, argument is pointer to the element, else argument
     * is the actual pointer. In the latter case an example of a valid
     * destructor is free().
     */
    void (*destructor)(void* element);
} GPMapInitializer;

/** Create hash map that takes any bytes as keys.*/
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN
GPHashMap* gp_hash_map_new(
    const GPAllocator*,
    const GPMapInitializer* optional);

/** Deallocate memory.*/
void gp_hash_map_delete(GPHashMap*);

/** Put element to hash table.
 * @return pointer to the element put in the table.
 */
GP_NONNULL_ARGS(1, 2) GP_NONNULL_RETURN
void* gp_hash_map_put(
    GPHashMap*,
    const void* key,
    size_t      key_size,
    const void* optional_value);

/** Find element.
 * @return pointer to element if found, NULL otherwise.
 */
GP_NONNULL_ARGS()
void* gp_hash_map_get(
    GPHashMap*,
    const void* key,
    size_t      key_size);

/** Remove element.
 * @return `true` if element found and removed, `false` otherwise.
 */
GP_NONNULL_ARGS()
bool gp_hash_map_remove(
    GPHashMap*,
    const void* key,
    size_t      key_size);

// ------------------
// Non-hashed map

/** Create hash map that takes 128-bit keys.*/
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN
GPMap* gp_map_new(
    const GPAllocator*,
    const GPMapInitializer* optional);

/** Deallocate memory.*/
void gp_map_delete(GPMap* optional);

/** Put element to the table.
 * @return pointer to the element put in the table.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN
void* gp_map_put(
    GPMap*,
    GPUint128   key,
    const void* optional_value);

/** Find element.
 * @return pointer to element if found, NULL otherwise.
 */
GP_NONNULL_ARGS()
void* gp_map_get(
    GPMap*,
    GPUint128 key);

/** Remove element.
 * @return `true` if element found and removed, `false` otherwise.
 */
GP_NONNULL_ARGS()
bool gp_map_remove(
    GPMap*,
    GPUint128 key);

// ------------------
// Hashing

/** Hashing functions based on non-cryptographic FNV function.*/
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


#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPC_HASHMAP_INCLUDED
