// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/hashmap.h>
#include <gpc/utils.h>
#include <string.h>

extern inline uint64_t* gp_u128_lo(GPUint128*t);
extern inline uint64_t* gp_u128_hi(GPUint128*t);
#ifndef __GNUC__ // unused static function
static void gp_mult64to128(uint64_t u, uint64_t v, uint64_t* h, uint64_t* l)
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
#endif
static void gp_mult128(const GPUint128 N, const GPUint128 M, GPUint128*const Ans)
{
    #if __GNUC__
    Ans->u128 = N.u128 * M.u128;
    #else
    // gp_mult64to128(N.lo, M.lo, &Ans->hi, &Ans->lo);
    // Ans->hi += (N.hi * M.lo) + (N.lo * M.hi);
    //
    // TODO check that I got this right
    gp_mult64to128(*gp_u128_lo(&N), *gp_u128_lo(&M), gp_u128_hi(Ans), gp_u128_lo(Ans);
    *gp_u128_hi(Ans) += gp_u128_hi(&N) * gp_u128_lo(&M) + gp_u128_lo(&N * gp_u128_hi(&M);
    #endif
}

//static // TODO use this instead of __uint128_t
GPUint128 gp_bit_shift_r(const GPUint128 x, const size_t s)
{
    GPUint128 y;
    #if __GNUC__
    y.u128 = x.u128 >> s;
    #else
    *gp_u128_hi(&y) =  gp_u128_hi(&x) >> s;
    *gp_u128_lo(&y) = (gp_u128_lo(&x) >> s) | (gp_u128_hi(&x) << (64 - s));
    #endif
    return y;
}

uint32_t gp_bytes_hash32(const void* str, const size_t str_size)
{
    const uint32_t FNV_prime        = 0x01000193;
    const uint32_t FNV_offset_basis = 0x811c9dc5;
    const uint8_t* ustr = str;

    uint32_t hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; i++)
    {
        hash ^= ustr[i];
        hash *= FNV_prime;
    }
    return hash;
}

uint64_t gp_bytes_hash64(const void* str, const size_t str_size)
{
    const uint64_t FNV_prime        = 0x00000100000001B3;
    const uint64_t FNV_offset_basis = 0xcbf29ce484222325;
    const uint8_t* ustr = str;

    uint64_t hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; i++)
    {
        hash ^= ustr[i];
        hash *= FNV_prime;
    }
    return hash;
}

GPUint128 gp_bytes_hash128(const void* str, const size_t str_size)
{
    GPUint128 FNV_prime;
    GPUint128 FNV_offset_basis;
    *gp_u128_hi(&FNV_prime)        = 0x0000000001000000;
    *gp_u128_lo(&FNV_prime)        = 0x000000000000013B;
    *gp_u128_hi(&FNV_offset_basis) = 0x6c62272e07bb0142;
    *gp_u128_lo(&FNV_offset_basis) = 0x62b821756295c58d;
    const uint8_t* ustr = str;

    GPUint128 hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; i++)
    {
        *gp_u128_lo(&hash) ^= ustr[i];
        gp_mult128(hash, FNV_prime, &hash);
    }
    return hash;
}

// ----------------------------------------------------------------------------

struct gp_map
{
    const size_t length; // number of slots
    const size_t element_size; // if 0, elements is in GPSlot
    const GPAllocator*const allocator;
    void (*const destructor)(void* element); // may be NULL
};

struct gp_hash_map
{
    struct gp_map map;
};

#define GP_EMPTY  ((uintptr_t) 0)
#define GP_IN_USE ((uintptr_t)-1)
typedef struct gp_slot
{
    __uint128_t key;
    union {
        uintptr_t slot;
        void*     slots;
    };
    const void* element;
} GPSlot;

// GPMap in memory:
// |GPMap|Slot 0|Slot 1|...|Slot n|Element 0|Element 1|...|Element n|
//
// Subsequent slots in memory in case of collissions:
// |New slot 0|...|New slot n/2|New element 1|...|New element n/2|
// ^
// Slot i info points here where i is the index of the colliding slot.
//
// If GPMap.element_not_pointer, element is in Slot array, not element array.

static void gp_no_op_destructor(void*_) { (void)_; }

GPMap* gp_map_new(const GPAllocator* allocator, const GPMapInitializer*_init)
{
    static const size_t DEFAULT_CAP = 1 << 8; // somewhat arbitrary atm
    static const GPMapInitializer defaults = {
        .element_size = sizeof(void*),
        .capacity     = DEFAULT_CAP,
    };
    const GPMapInitializer* init = _init == NULL ? &defaults : _init;

    const size_t length = init->capacity == 0 ?
        DEFAULT_CAP
      : gp_next_power_of_2(init->capacity) >> 1;

    const GPMap init_map = {
        .length       = length,
        .element_size = init->element_size,
        .allocator    = allocator,
        .destructor   = init->destructor == NULL ?
            gp_no_op_destructor
          : init->destructor
    };
    GPMap* block = gp_mem_alloc_zeroes(allocator,
        sizeof init_map + length * sizeof(GPSlot) + length * init->element_size);
    return memcpy(block, &init_map, sizeof init_map);
}

static inline size_t gp_next_length(const size_t length)
{
    return length/2 < 4 ? 4 : length/2;
}

void gp_map_delete_elems(
    GPSlot*const slots,
    const size_t length,
    const size_t elem_size,
    void (*destructor)(void*))
{
    for (size_t i = 0; i < length; i++)
    {
        if (slots[i].slot == GP_IN_USE)
        {
            destructor(elem_size == 0 ?
                (void*)slots[i].element
              : (uint8_t*)(slots + length) + i * elem_size);
        }
        else if (slots[i].slot != GP_EMPTY)
        {
            gp_map_delete_elems(
                slots[i].slots, gp_next_length(length), elem_size, destructor);
        }
    }
}

void gp_map_delete(GPMap* map)
{
    gp_map_delete_elems(
        (GPSlot*)(map + 1),
        map->length,
        map->element_size,
        map->destructor);

    gp_mem_dealloc(map->allocator, map);
}

static void gp_map_set_elem(
    const GPAllocator*const allocator,
    GPSlot*const            slots,
    const size_t            length,
    const __uint128_t       key,
    const void*const        elem,
    const size_t            elem_size)
{
    uint8_t* values = (uint8_t*)(slots + length);
    const size_t i  = key & (length - 1);

    if (slots[i].slot == GP_EMPTY)
    {
        if (elem_size != 0)
            memcpy(values + i * elem_size, elem, elem_size);
        else
            slots[i].element = elem;
        slots[i].slot = GP_IN_USE;
        slots[i].key  = key;
        return;
    }
    const size_t next_length = gp_next_length(length);
    if (slots[i].slot == GP_IN_USE)
    {
        GPSlot* new_slots = gp_mem_alloc_zeroes(allocator,
            next_length * sizeof*new_slots + next_length * elem_size);

        gp_map_set_elem(
            allocator,
            new_slots,
            next_length,
            slots[i].key/next_length,
            values + i * elem_size,
            elem_size);

        slots[i].slots = new_slots;
    }
    gp_map_set_elem(
        allocator,
        slots[i].slots,
        next_length,
        key/next_length,
        elem,
        elem_size);
}

void gp_map_set(
    GPMap* map,
    GPUint128 key,
    const void* value)
{
    gp_map_set_elem(
        map->allocator,
        (GPSlot*)(map + 1),
        map->length,
        (union {__uint128_t key; GPUint128 _key; }) {._key = key }.key,
        value,
        map->element_size);
}

static void* gp_map_get_elem(
    const GPSlot*const slots,
    const size_t length,
    const __uint128_t key,
    const size_t elem_size)
{
    uint8_t* values = (uint8_t*)(slots + length);
    const size_t i  = key & (length - 1);
    if (slots[i].slot == GP_IN_USE)
        return elem_size != 0 ? values + i * elem_size : (void*)slots[i].element;
    else if (slots[i].slot == GP_EMPTY)
        return NULL;

    const size_t next_length = gp_next_length(length);
    return gp_map_get_elem(
        slots, next_length, key/next_length, elem_size);
}

void* gp_map_get(GPMap* map, GPUint128 key)
{
    return gp_map_get_elem(
        (GPSlot*)(map + 1),
        map->length,
        (union {__uint128_t key; GPUint128 _key; }) {._key = key }.key,
        map->element_size);
}

static bool gp_map_remove_elem(
    GPSlot*const slots,
    const size_t length,
    const __uint128_t key,
    const size_t elem_size,
    void (*const destructor)(void*))
{
    const size_t i  = key & (length - 1);
    if (slots[i].slot == GP_IN_USE) {
        slots[i].slot = GP_EMPTY;
        destructor(elem_size == 0 ?
            (void*)slots[i].element
          : (uint8_t*)(slots + length) + i * elem_size);
        return true;
    }
    else if (slots[i].slot == GP_EMPTY) {
        return false;
    }
    const size_t next_length = gp_next_length(length);
    return gp_map_remove_elem(
        slots, next_length, key/next_length, elem_size, destructor);
}

bool gp_map_remove(GPMap* map, GPUint128 key)
{
    return gp_map_remove_elem(
        (GPSlot*)(map + 1),
        map->length,
        (union {__uint128_t key; GPUint128 _key; }) {._key = key }.key,
        map->element_size,
        map->destructor);
}

GPHashMap* gp_hash_map_new(const GPAllocator* alc, const GPMapInitializer* init)
{
    return (GPHashMap*)gp_map_new(alc, init);
}

void gp_hash_map_delete(GPHashMap* map) { gp_map_delete((GPMap*)map); }

void gp_hash_map_set(
    GPHashMap*  map,
    const void* key,
    size_t      key_size,
    const void* value)
{
    gp_map_set((GPMap*)map, gp_bytes_hash128(key, key_size), value);
}

// Returns NULL if not found
void* gp_hash_map_get(
    GPHashMap*  map,
    const void* key,
    size_t      key_size)
{
    return gp_map_get((GPMap*)map, gp_bytes_hash128(key, key_size));
}

bool gp_hash_map_remove(
    GPHashMap*  map,
    const void* key,
    size_t      key_size)
{
    return gp_map_remove((GPMap*)map, gp_bytes_hash128(key, key_size));
}


