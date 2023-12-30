// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file hashmap.h
 * @brief Dictionary data structure
 */

#ifndef GPC_HASHMAP_INCLUDED
#define GPC_HASHMAP_INCLUDED

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

typedef struct gpc_StringMap
{
    const size_t element_size;
    const unsigned init_size;     // Should be power of 2
    const unsigned alloc_divider; // Should be power of 2
    const bool no_hashing;
    struct gpc_Slots* slots;      // Private!
} gpc_StringMap;

typedef struct gpc_IntegerMap
{
    const size_t element_size;
    const unsigned init_size;     // Should be power of 2
    const unsigned alloc_divider; // Should be power of 2
    const bool no_hashing;
    struct gpc_Slots* slots;      // Private!
} gpc_IntegerMap;

typedef struct gpc_String128Map
{
    const size_t element_size;
    const bool no_hashing;
    const unsigned init_size;     // Should be power of 2
    const unsigned alloc_divider; // Should be power of 2
    struct gpc_Slots128* slots;   // Private!
} gpc_String128Map;

typedef struct gpc_Integer128Map
{
    const size_t element_size;
    const bool no_hashing;
    const unsigned init_size;     // Should be power of 2
    const unsigned alloc_divider; // Should be power of 2
    struct gpc_Slots128* slots;   // Private!
} gpc_Integer128Map;

#if UINTPTR_MAX == UINT64_MAX
typedef double gpc_floatptr;
#else
typedef float gpc_floatptr;
#endif

typedef struct gpc_Uint128
{
    uint64_t lo;
    uint64_t hi;
} gpc_Uint128;

// ------------------

uint32_t    gpc_hash32 (const char* str);
uint64_t    gpc_hash64 (const char* str);
gpc_Uint128 gpc_hash128(const char* str);

// ------------------

void  gpc_smap_set(gpc_StringMap* map,      const char* key, void* value);
void  gpc_smap_setf(gpc_StringMap* map,     const char* key, gpc_floatptr value);
void* gpc_smap_get(const gpc_StringMap map, const char* key);
gpc_floatptr gpc_smap_getf(const gpc_StringMap map, const char* key);
void gpc_smap_delete(gpc_StringMap* map, const char* key);

// ------------------

void  gpc_umap_set(gpc_IntegerMap* map,      uint64_t key, void* value);
void  gpc_umap_setf(gpc_IntegerMap* map,     uint64_t key, gpc_floatptr value);
void* gpc_umap_get(const gpc_IntegerMap map, uint64_t key);
gpc_floatptr gpc_umap_getf(const gpc_IntegerMap map, uint64_t key);
void gpc_umap_delete(gpc_IntegerMap* map, uint64_t key);

// ------------------

void  gpc_s128map_set(
    gpc_String128Map* map,
    const char* key,
    void* value);

void  gpc_s128map_setf(
    gpc_String128Map* map,
    const char*,
    gpc_floatptr value);

void* gpc_s128map_get(
    const gpc_String128Map map,
    const char* key);

gpc_floatptr gpc_s128map_getf(
    const gpc_String128Map map,
    const char* key);

void gpc_s128map_delete(gpc_String128Map* map, const char* key);

// ------------------

void  gpc_u128map_set(
    gpc_Integer128Map* map,
    gpc_Uint128 key,
    void* value);

void  gpc_u128map_setf(
    gpc_Integer128Map* map,
    gpc_Uint128 key,
    gpc_floatptr value);

void* gpc_u128map_get(
    const gpc_Integer128Map map,
    gpc_Uint128 key);

gpc_floatptr gpc_u128map_getf(
    const gpc_Integer128Map map,
    gpc_Uint128 key);

void gpc_u128map_delete(gpc_Integer128Map* map, gpc_Uint128 key);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

//
typedef struct gpc_Slots
{
//    union
//    {
//        void* value;
//        gpc_floatptr fvalue;
//        struct gpc_Slots* next;
//    };
    union
    {
        uint64_t ukey;
        bool has_value;
    };
    void* value[];
} gpc_Slots;

typedef struct gpc_Slots128
{
//    union
//    {
//        void* value;
//        gpc_floatptr fvalue;
//        struct gpc_Slots* next;
//    };
    union
    {
        gpc_Uint128 key;
        bool has_value; // This makes empty string an invalid key when no hash!
    };
    void* value[];
} gpc_Slots128;

#endif // GPC_HASHMAP_INCLUDED
