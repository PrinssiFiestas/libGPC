// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/hashmap.h>
#include <gpc/utils.h>
#include <string.h>

const union gp_endianness_detector GP_INTEGER = {.u16 = 1 };

#if !(defined(__COMPCERT__) && defined(GPC_IMPLEMENTATION))
extern inline GPUint128 gp_u128(const uint64_t hi_bits, const uint64_t lo_bits);
extern inline uint64_t* gp_u128_lo(const GPUint128*t);
extern inline uint64_t* gp_u128_hi(const GPUint128*t);
#endif

#if !(__GNUC__ && __SIZEOF_INT128__) // unused static function
static void gp_mult64to128(
    uint64_t u, uint64_t v, uint64_t* h, uint64_t* l)
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
    #if __GNUC__ && __SIZEOF_INT128__
    Ans->u128 = N.u128 * M.u128;
    #else
    gp_mult64to128(*gp_u128_lo(&N), *gp_u128_lo(&M), gp_u128_hi(Ans), gp_u128_lo(Ans));
    *gp_u128_hi(Ans) += *gp_u128_hi(&N) * *gp_u128_lo(&M) + *gp_u128_lo(&N) * *gp_u128_hi(&M);
    #endif
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
    GPUint128 FNV_prime        = gp_u128(0x0000000001000000, 0x000000000000013B);
    GPUint128 FNV_offset_basis = gp_u128(0x6c62272e07bb0142, 0x62b821756295c58d);
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

#define GP_EMPTY  ((size_t) 0)
#define GP_IN_USE ((size_t)-1)
typedef struct gp_slot
{
    GPUint128 key;
    union {
        uintptr_t index;
        void*     children;
    } slot;
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
    #define GP_DEFAULT_MAP_CAP (1 << 8) // somewhat arbitrary atm
    static const GPMapInitializer defaults = { .capacity = GP_DEFAULT_MAP_CAP };
    const GPMapInitializer* init = _init == NULL ? &defaults : _init;

    const size_t length = init->capacity == 0 ?
        GP_DEFAULT_MAP_CAP
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
static inline GPUint128 gp_shift_key(const GPUint128 key, const size_t length)
{
    #if __GNUC__ && __SIZEOF_INT128__
    if      (sizeof length == sizeof(unsigned))
        return (GPUint128){.u128 =
            key.u128 >> (sizeof(int)  * CHAR_BIT -__builtin_clz  (length) - 1)};
    else if (sizeof length == sizeof(long))
        return (GPUint128){.u128 =
            key.u128 >> (sizeof(long) * CHAR_BIT -__builtin_clzl (length) - 1)};
    return
        (GPUint128){.u128 =
        key.u128 >> (sizeof(long long)* CHAR_BIT -__builtin_clzll(length) - 1)};
    #else

    // Find bit width of length which is assumed to be a power of 2.
    // https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
    static const uint64_t b[] = {
        0xAAAAAAAAAAAAAAAA, 0xCCCCCCCCCCCCCCCC, 0xF0F0F0F0F0F0F0F0,
        0xFF00FF00FF00FF00, 0xFFFF0000FFFF0000, 0xFFFFFFFF00000000
    };
    uint64_t
    bitw  =  ((uint64_t)length & b[0]) != 0;
    bitw |= (((uint64_t)length & b[5]) != 0) << 5;
    bitw |= (((uint64_t)length & b[4]) != 0) << 4;
    bitw |= (((uint64_t)length & b[3]) != 0) << 3;
    bitw |= (((uint64_t)length & b[2]) != 0) << 2;
    bitw |= (((uint64_t)length & b[1]) != 0) << 1;

    // 128-bit bit shift right
    GPUint128 new_key = gp_u128(
        *gp_u128_hi(&key) >> bitw,
       (*gp_u128_lo(&key) >> bitw) | (*gp_u128_hi(&key)<<(64-bitw)));

    return new_key;
    #endif
}

// ----------------------------------------------------------------------------

void gp_map_delete_elems(
    GPMap*const  map,
    GPSlot*const slots,
    const size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (slots[i].slot.index == GP_IN_USE)
        {
            if (slots[i].element == NULL)
                continue;

            map->destructor((void*)slots[i].element);
        }
        else if (slots[i].slot.index != GP_EMPTY)
        {
            if (slots[i].element != NULL)
                map->destructor((void*)slots[i].element);
            gp_map_delete_elems(map, slots[i].slot.children, gp_next_length(length));
        }
    }
    if (slots != (GPSlot*)(map + 1))
        gp_mem_dealloc(map->allocator, slots);
    else
        gp_mem_dealloc(map->allocator, map);
}

void gp_map_delete(GPMap* map)
{
    gp_map_delete_elems(map, (GPSlot*)(map + 1), map->length);
}

static void* gp_map_put_elem(
    const GPAllocator*const allocator,
    GPSlot*const            slots,
    const size_t            length,
    const GPUint128         key,
    const void*const        elem,
    const size_t            elem_size)
{
    uint8_t* values = (uint8_t*)(slots + length);
    const size_t i  = *gp_u128_lo(&key) & (length - 1);

    if (slots[i].slot.index == GP_EMPTY)
    {
        if (elem_size != 0) {
            if (elem != NULL)
                memcpy(values + i * elem_size, elem, elem_size);
            slots[i].element = values + i * elem_size;
        } else {
            slots[i].element = elem;
        }
        slots[i].slot.index = GP_IN_USE;
        slots[i].key  = key;
        return (void*)slots[i].element;
    }
    const size_t next_length = gp_next_length(length);
    if (slots[i].slot.index == GP_IN_USE)
    {
        GPSlot* new_slots = gp_mem_alloc_zeroes(allocator,
            next_length * sizeof*new_slots + next_length * elem_size);
        slots[i].slot.children = new_slots;
    }
    return gp_map_put_elem(
        allocator,
        slots[i].slot.children,
        next_length,
        gp_shift_key(key, length),
        elem,
        elem_size);
}

void* gp_map_put(
    GPMap* map,
    GPUint128 key,
    const void* value)
{
    return gp_map_put_elem(
        map->allocator,
        (GPSlot*)(map + 1),
        map->length,
        key,
        value,
        map->element_size);
}

static void* gp_map_get_elem(
    const GPSlot*const slots,
    const size_t length,
    const GPUint128 key,
    const size_t elem_size)
{
    const size_t i = *gp_u128_lo(&key) & (length - 1);

    if (slots[i].slot.index == GP_EMPTY)
        return NULL;
    else if (slots[i].slot.index == GP_IN_USE || memcmp(&slots[i].key, &key, sizeof key) == 0)
        return (void*)slots[i].element;

    return gp_map_get_elem(
        slots[i].slot.children, gp_next_length(length), gp_shift_key(key, length), elem_size);
}

void* gp_map_get(GPMap* map, GPUint128 key)
{
    return gp_map_get_elem(
        (GPSlot*)(map + 1),
        map->length,
        key,
        map->element_size);
}

static bool gp_map_remove_elem(
    GPSlot*const slots,
    const size_t length,
    const GPUint128 key,
    const size_t elem_size,
    void (*const destructor)(void*))
{
    const size_t i  = *gp_u128_lo(&key) & (length - 1);
    if (slots[i].slot.index == GP_IN_USE) {
        slots[i].slot.index = GP_EMPTY;
        destructor((void*)slots[i].element);
        slots[i].element = NULL;
        return true;
    }
    else if (slots[i].slot.index == GP_EMPTY) {
        return false;
    }
    else if (memcmp(&slots[i].key, &key, sizeof key) == 0) {
        slots[i].key = gp_bytes_hash128(&key, sizeof key);
        destructor((void*)slots[i].element);
        slots[i].element = NULL;
        return true;
    }
    return gp_map_remove_elem(
        slots, gp_next_length(length), gp_shift_key(key, length), elem_size, destructor);
}

bool gp_map_remove(GPMap* map, GPUint128 key)
{
    return gp_map_remove_elem(
        (GPSlot*)(map + 1),
        map->length,
        key,
        map->element_size,
        map->destructor);
}

GPHashMap* gp_hash_map_new(const GPAllocator* alc, const GPMapInitializer* init)
{
    return (GPHashMap*)gp_map_new(alc, init);
}

void gp_hash_map_delete(GPHashMap* map) { gp_map_delete((GPMap*)map); }

void* gp_hash_map_put(
    GPHashMap*  map,
    const void* key,
    size_t      key_size,
    const void* value)
{
    return gp_map_put((GPMap*)map, gp_bytes_hash128(key, key_size), value);
}

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


