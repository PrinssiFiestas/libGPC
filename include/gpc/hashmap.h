// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file hashmap.h
 * Hashing and hash maps
 */

#ifndef GP_HASHMAP_INCLUDED
#define GP_HASHMAP_INCLUDED 1

#include "memory.h"
#include "attributes.h"
#include "int128.h"
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


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// ------------------
// Hash map

/** Hash map using 128-bit keys.
 * Internally a tree of arrays. Simply uses lowest n bits from the key to index
 * to an array of size 2^n. In case of collisions, a new array of size 2^(n - 1)
 * is created and the last slot is set to point to the new array. Then the next
 * lowest n - 1 bits from the key are used to index to the new array.
 */
typedef struct gp_map GPMap;

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
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
GPHashMap* gp_hash_map_new(
    GPAllocator*,
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
GP_NONNULL_ARGS() GP_NODISCARD
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
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
GPMap* gp_map_new(
    GPAllocator*,
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
    const void* value);

/** Find element.
 * @return pointer to element if found, NULL otherwise.
 */
GP_NONNULL_ARGS() GP_NODISCARD
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
uint32_t  gp_bytes_hash32 (const void* key, size_t key_size) GP_NONNULL_ARGS() GP_NODISCARD;
uint64_t  gp_bytes_hash64 (const void* key, size_t key_size) GP_NONNULL_ARGS() GP_NODISCARD;
GPUint128 gp_bytes_hash128(const void* key, size_t key_size) GP_NONNULL_ARGS() GP_NODISCARD;


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

#endif // GP_HASHMAP_INCLUDED
