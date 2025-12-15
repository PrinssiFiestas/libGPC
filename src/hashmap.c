// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/hashmap.h>
#include <gpc/utils.h>
#include <string.h>

uint32_t gp_bytes_hash32(const void* str, const size_t str_size)
{
    const uint32_t FNV_prime        = 0x01000193;
    const uint32_t FNV_offset_basis = 0x811c9dc5;
    const uint8_t* ustr = str;

    uint32_t hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; ++i)
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
    for (size_t i = 0; i < str_size; ++i)
    {
        hash ^= ustr[i];
        hash *= FNV_prime;
    }
    return hash;
}

GPUInt128 gp_bytes_hash128(const void* str, const size_t str_size)
{
    const GPUInt128 FNV_prime        = gp_uint128(0x0000000001000000, 0x000000000000013B);
    const GPUInt128 FNV_offset_basis = gp_uint128(0x6c62272e07bb0142, 0x62b821756295c58d);
    const uint8_t* ustr = str;

    GPUInt128 hash = FNV_offset_basis;
    for (size_t i = 0; i < str_size; ++i)
    {
        *gp_uint128_lo_addr(&hash) ^= ustr[i];
        hash = gp_uint128_mul(hash, FNV_prime);
    }
    return hash;
}

// ----------------------------------------------------------------------------

struct gp_map
{
    GPAllocator* allocator;
    #if UINTPTR_MAX < UINT64_MAX
    uint32_t _alignment_pad;
    #endif
    uint32_t element_size;
    uint32_t size_shift; // 1 << size_shift == number_of_elements

    /* // Initial allocation:
    GPMapBucket initial_buckets[(1 << size_shift) + 1];
    T           initial_elements[1 << size_shift];
    */
};

typedef struct gp_map_bucket
{
    uint64_t              hash;
    struct gp_map_bucket* children;
} GPMapBucket;

GP_STATIC_ASSERT((sizeof(struct gp_map) & 0xF) == 0, "16 bytes of alignment required.");

GPMap gp_map_new(
    size_t       element_size,
    GPAllocator* allocator,
    size_t       capacity)
{
    gp_assert(element_size < UINT32_MAX);
    capacity = gp_max(16lu,     capacity);
    capacity = gp_min(0x4000lu, capacity);
    size_t size_shift = 63 - __builtin_clzll(capacity); // TODO portability!
    if (size_shift & 1) // even power makes things easier later
        size_shift++;
    capacity = 1 << size_shift;

    void* map_mem = gp_mem_alloc_aligned(
        allocator,
        sizeof(struct gp_map)
            + (capacity + 1) * sizeof(GPMapBucket) // extra one for terminator
            + (capacity + 0) * element_size,
        16); // low 4 bits of children pointer used to detect sentinel and to
             // store size shift of previous level.

    GPMap map = map_mem;
    *map = (struct gp_map){
        .allocator    = allocator,
        .element_size = element_size,
        .size_shift   = size_shift
    };
    GPMapBucket* buckets = (GPMapBucket*)(map + 1);
    memset(buckets, 0, capacity * sizeof buckets[0]);
    buckets[capacity].children = (GPMapBucket*)-1; // final (root) terminator
    return map;
}

void gp_s_bucket_delete(
    GPAllocator* allocator, GPMapBucket buckets[], size_t size_shift)
{
    size_t size = 1 << size_shift;
    for (size_t i = 0; i < size; ++i) {
        if (buckets[i].children != NULL)
            gp_s_bucket_delete(
                allocator, buckets[i].children, size_shift >> (2 * (size_shift>4)));
    }
    gp_mem_dealloc(allocator, buckets);
}

void gp_map_delete(GPMap map)
{
    if (map == NULL)
        return;

    // Root node lives in different allocation, which is why we need to
    // duplicate this loop.
    GPMapBucket* buckets = (void*)(map + 1);
    size_t size = 1 << map->size_shift;
    for (size_t i = 0; i < size; ++i) {
        if (buckets[i].children != NULL)
            gp_s_bucket_delete(
                map->allocator,
                buckets[i].children,
                map->size_shift >> (2 * (map->size_shift>4)));
    }
    gp_mem_dealloc(map->allocator, map);
}

void gp_map_ptr_delete(GPMap* map)
{
    if (map == NULL || *map == NULL)
        return;
    gp_map_delete(*map);
    *map = NULL;
}

static void* gp_s_map_put(
    GPMap        map,
    size_t       size_shift,
    GPMapBucket  buckets[],
    uint64_t     hash,
    const void*  value)
{
    size_t size = 1 << size_shift;
    size_t mask = size - 1;
    size_t i    = hash & mask;

    if (buckets[i].hash == 0) {
        buckets[i].hash = hash;
        void* ptr = (unsigned char*)(buckets + size + 1) + i * map->element_size;
        return memcpy(ptr, value, map->element_size);
    }

    uint64_t hash1 = (hash << (64 - size_shift)) | (hash >> size_shift);
    size_t size_shift1 = gp_max(size_shift - 2, 4u);

    if (buckets[i].children == NULL) {
        size_t size1 = gp_max(size >> 2,  1u << 4);
        size_t mask1 = gp_max(mask >> 2, (1u << 4) - 1);
        size_t i1    = hash1 & mask1;

        buckets[i].children = gp_mem_alloc_aligned(
            map->allocator,
            (size1 + 1) * sizeof(GPMapBucket) + size1 * map->element_size,
            16);
        memset(buckets[i].children, 0, size1 *  sizeof(GPMapBucket));
        buckets[i].children[i1].hash = hash1;

        // Store information for the iterator on how to get back. Low 4 bits of
        // all children pointers are 0, except for this terminator node, so the
        // iterator just checks the low 4 bits to detect end of level.
        buckets[i].children[size1].hash = i;
        buckets[i].children[size1].children = (void*)(0
            | (uintptr_t)buckets // aligned to 16 bytes, so we have 4 bits free
            | (size_shift-1) // max shift = 0x10, must offset -1 to fit in 4 bits
        );

        void* ptr = (unsigned char*)(buckets[i].children + size1 + 1)
            + i1 * map->element_size;
        return memcpy(ptr, value, map->element_size);
    }

    return gp_s_map_put(map, size_shift1, buckets[i].children, hash1, value);
}

void* gp_map_put(
    GPMap*      map,
    const void* key,
    uint64_t    hash,
    const void* value)
{
    if (key != NULL)
        hash = gp_bytes_hash(key, hash);
    else
        gp_assert(hash != 0, "Invalid hash.");

    return gp_s_map_put(
        *map,
        (*map)->size_shift,
        (GPMapBucket*)(*map + 1),
        hash,
        value);
}

static void* gp_s_map_get(
    size_t       size_shift,
    GPMapBucket  buckets[],
    uint64_t     hash,
    const size_t value_size)
{
    size_t size = 1 << size_shift;
    size_t mask = size - 1;
    size_t i    = hash & mask;

    if (buckets[i].hash == hash)
        return (unsigned char*)(buckets + size + 1) + i * value_size;

    if (buckets[i].children != NULL) {
        hash = (hash << (64 - size_shift)) | (hash >> size_shift);
        return gp_s_map_get(
            size_shift - 2 * (size_shift>4), buckets[i].children, hash, value_size);
    }
    return NULL;
}

void* gp_map_get(
    GPMap       map,
    const void* key,
    uint64_t    hash)
{
    if (key != NULL)
        hash = gp_bytes_hash(key, hash);
    else
        gp_assert(hash != 0, "Invalid hash.");

    return gp_s_map_get(
        map->size_shift,
        (GPMapBucket*)(map + 1),
        hash,
        map->element_size);
}

void* gp_s_map_remove(
    const size_t element_size,
    size_t       size_shift,
    GPMapBucket  buckets[],
    uint64_t     hash)
{
    size_t size = 1 << size_shift;
    size_t mask = size - 1;
    size_t i    = hash & mask;

    if (buckets[i].hash == hash) {
        buckets[i].hash = 0;
        return (unsigned char*)(buckets + size + 1) + i * element_size;
    }

    if (buckets[i].children != NULL) {
        hash = (hash << (64 - size_shift)) | (hash >> size_shift);
        return gp_s_map_remove(
            element_size, size_shift - 2 * (size_shift>4), buckets[i].children, hash);
    }
    return NULL;
}

void* gp_map_remove(
    GPMap*      map,
    const void* key,
    uint64_t    hash)
{
    if (key != NULL)
        hash = gp_bytes_hash(key, hash);
    else
        gp_assert(hash != 0, "Invalid hash.");

    return gp_s_map_remove(
        (*map)->element_size,
        (*map)->size_shift,
        (GPMapBucket*)(*map + 1),
        hash);
}

GPMapIterator gp_map_begin(GPMap map)
{
    size_t       shift   = map->size_shift;
    size_t       size    = 1 << shift;
    GPMapBucket* buckets = (void*)(map + 1);
    GPMapBucket* b       = buckets;

    find_first:
    while (((uintptr_t)b->children & 0xF) == 0) {
        if (b->children != NULL) { // go down
            b = buckets = b->children;
            shift = gp_max(shift - 2, 4u);
            size  = 1 << shift;
        }
        else if (b->hash != 0)
            return (GPMapIterator){
                .value        = (unsigned char*)(buckets + size + 1)
                                    + (b - buckets) * map->element_size,
                .element_size = map->element_size,
                ._bs          = buckets,
                ._i           = b - buckets,
                ._shift       = shift};
        else
            b++;
    }
    if (b->children == (void*)-1)
        return (GPMapIterator){0};
    // else go up

    shift = ((uintptr_t)b->children & 0xF)
        + 1; // compensate for the -1 in gp_map_put()
    size = 1 << shift;
    buckets = (void*)((uintptr_t)b->children &~ 0xF);
    b = buckets + b->hash;

    if (b->hash == 0) {
        b++;
        goto find_first;
    }
    return (GPMapIterator){
        .value        = (unsigned char*)(buckets + size + 1)
                            + (b - buckets) * map->element_size,
        .element_size = map->element_size,
        ._bs          = buckets,
        ._i           = b - buckets,
        ._shift       = shift
    };
}

GPMapIterator gp_map_next(GPMapIterator it)
{
    size_t       shift   = it._shift;
    size_t       size    = 1 << shift;
    GPMapBucket* buckets = it._bs;
    GPMapBucket* b       = buckets + it._i + 1;

    find_next:
    while (((uintptr_t)b->children & 0xF) == 0) {
        if (b->children != NULL) { // go down
            b = buckets = b->children;
            shift = gp_max(shift - 2, 4u);
            size  = 1 << shift;
        }
        else if (b->hash != 0)
            return (GPMapIterator){
                .value        = (unsigned char*)(buckets + size + 1)
                    + (b - buckets) * it.element_size,
                .element_size = it.element_size,
                ._bs          = buckets,
                ._i           = b - buckets,
                ._shift       = shift};
        else
            b++;
    }
    if (b->children == (void*)-1)
        return (GPMapIterator){0};
    // else go up

    shift = ((uintptr_t)b->children & 0xF)
        + 1; // compensate for the -1 in gp_map_put()
    size = 1 << shift;
    buckets = (void*)((uintptr_t)b->children &~ 0xF);
    b = buckets + b->hash;

    if (b->hash == 0)  {
        b++;
        goto find_next;
    }
    return (GPMapIterator){
        .value        = (unsigned char*)(buckets + size + 1)
                            + (b - buckets) * it.element_size,
        .element_size = it.element_size,
        ._bs          = buckets,
        ._i           = b - buckets,
        ._shift       = shift
    };
}
