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


// ------------------------------------
// Hash map

/** Hash map */
typedef struct gp_map* GPMap;

/** Hash map iterator */
typedef struct gp_map_iterator
{
    void*  value; /**< Pointer to the element. */
    size_t element_size;

    struct gp_map_bucket* _bs;    /**< @private */
    uint16_t              _i;     /**< @private */
    uint16_t              _shift; /**< @private */
} GPMapIterator;

/** Create hash map.*/
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
GPMap gp_map_new(
    size_t       element_size,
    GPAllocator* allocator,
    size_t       init_capacity);

/** Deallocate hash map.*/
void gp_map_delete(GPMap optional);

/** Deallocate hash map trough pointer.*/
void gp_map_ptr_delete(GPMap* optional_ptr);

/** Put element to the table.
 * @return pointer to the element put in the table.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN
void* gp_map_put(
    GPMap*      map_addr,
    const void* optional_key,
    uint64_t    key_size_or_hash,
    const void* value);

/** Find element.
 * @return pointer to element if found, NULL otherwise.
 */
GP_NONNULL_ARGS(1) GP_NODISCARD
void* gp_map_get(
    GPMap,
    const void* optional_key,
    uint64_t    key_size_or_hash);

/** Remove element.
 * @return pointer to removed element, which is valid until the next operation
 * on passed map, or NULL if no element found. The return value is mostly used
 * as a boolean to detect if the value was actually there. If the value is used,
 * (and not NULL of course) it is recommended to copy it immediately to another
 * variable.
 */
GP_NONNULL_ARGS(1)
void* gp_map_remove(
    GPMap*,
    const void* optional_key,
    uint64_t    key_size_or_hash);

/** Create a hash map iterator.
 * If the map is empty, then the value pointer of the iterator will be NULL.
 * Otherwise it will point to the first unordered element.
 */
GP_NODISCARD
GPMapIterator gp_map_begin(GPMap);

/** Iterate over hash map.*/
GP_NODISCARD
GPMapIterator gp_map_next(GPMapIterator);

// ------------------------------------
// Hashing

/** 32-bit non-cryptographic FNV hash.*/
uint32_t gp_bytes_hash32(const void* key, size_t key_size) GP_NONNULL_ARGS() GP_NODISCARD;
/** 64-bit non-cryptographic FNV hash.*/
uint64_t gp_bytes_hash64(const void* key, size_t key_size) GP_NONNULL_ARGS() GP_NODISCARD;
/** 128-bit non-cryptographic FNV hash.*/
GPUInt128 gp_bytes_hash128(const void* key, size_t key_size) GP_NONNULL_ARGS() GP_NODISCARD;

/** Default hash.
 * @ref GPMap uses this internally. This can be used for caching hashes to avoid
 * repeated hashing.
 */
GP_NONNULL_ARGS() GP_NODISCARD
static inline uint64_t gp_bytes_hash(const void* key, size_t key_size)
{
    return gp_bytes_hash64(key, key_size);
}


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
