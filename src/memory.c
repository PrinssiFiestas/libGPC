// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"

#include "../include/gpc/memory.h"
#include "../include/gpc/utils.h"
#include <stdlib.h>
#include <stdint.h>

// The size of the hash table array storing all pointers
// Larger array: less collisions but more memory usage.
// Here are some measurements for a single dimensional array. Of course the
// amount of pointers without collisions changed each run so the value is
// roundabout. I only tried 3 runs for each table size.
//
// -----------------------------------------------------------------
// Table size (bits) | Table size (MB) | Pointers without collisions
// -----------------------------------------------------------------
//     1 << 8        |      .002       |               30
//     1 << 12       |      .032       |              800
//     1 << 16       |      .52        |            4'000
//     1 << 20       |      8.4        |           26'000
//     1 << 24       |      134        |          260'000
//     1 << 28       |     2147        |        4'000'000
//     1 << 32       |    34000        |       48'000'000
//
// So if I use an array with dynamic dimension that starts with 1 << 20 elements
// and grows by 1 << 8 elements for each added dimension I can easily cover the
// whole address space without collisions.

#define GPC_PTR_TABLE_SIZE (1 << 20)
#define GPC_PTR_INDEX_BITS (GPC_PTR_TABLE_SIZE - 1)

typedef struct gpc_AllocData
{
    void* ptr;
    const char* file;
    const char* func;
    const char* var_name;
    int line;
    int freed_line;
} gpc_AllocData;

typedef struct gpc_AllocDataArray
{
    size_t length;
    size_t capacity;
    gpc_AllocData data[];
} gpc_AllocDataArray;

// Array of allocation data arrays. Each entry is an array to avoid collisions.
static gpc_AllocDataArray** gpc_ptr_table;

static void gpc_init_ptr_table(void)
{
    gpc_ptr_table = calloc(GPC_PTR_TABLE_SIZE, sizeof(gpc_AllocDataArray*));
}

uint32_t gpc_hash(uint64_t x)
{
    uint64_t oldstate = x;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint64_t gpc_hash_seed = 0xe9c22514b81b5e45ULL;
uint32_t gpc_hash_range(uint64_t x, uint32_t bound)
{
    uint32_t threshold = -bound % bound;
    gpc_RandomState rng = { x, gpc_hash_seed };
    for (;;) {
        uint32_t r = (uint32_t)gpc_random_range(&rng, 0, GPC_PTR_TABLE_SIZE - 1);
        if (r >= threshold)
            return r % bound;
    }
}

// Simply use the the address given by malloc() as the index for the hash map.
static size_t gpc_ptr_table_index(void* ptr)
{
    // TODO check if a proper way is indeed better
    //return (size_t)gpc_hash_range(((uintptr_t)ptr >> 4), GPC_PTR_TABLE_SIZE - 1);

    uintptr_t p = (uintptr_t)ptr;
    p >>= 4; // malloc() on tested platforms don't use these bits
    return p & GPC_PTR_INDEX_BITS;
}

GPC_NODISCARD void* gpc_db_allocate(
    const char* file, int line, const char* func, const char* var_name,
    gpc_Owner* owner, size_t bytes)
{
    (void)owner;
    gpc_AllocData data = {
        .file = file, .func = func, .var_name = var_name, .line = line };

    void* p = malloc(bytes);

    data.ptr = p;

    // Find correct array
    gpc_AllocDataArray** data_arr = gpc_ptr_table + gpc_ptr_table_index(p);

    if (*data_arr == NULL)
    {
        *data_arr = calloc(sizeof(gpc_AllocDataArray) + 16 * sizeof(gpc_AllocData), 1);
        (*data_arr)->capacity = 16;
    }
    (*data_arr)->length++;

    // TODO bounds check and stuff
    //*data_arr = realloc(*data_arr,
    //    sizeof(gpc_AllocDataArray) + 16 * sizeof(gpc_AllocData));

    ((gpc_AllocData*)((*data_arr) + 1))[(*data_arr)->length - 1] = data;

    return p;
}

void gpc_db_deallocate(void* ptr)
{
    free(ptr);
}

static gpc_AllocData gpc_alloc_data(void* key_ptr)
{
    gpc_AllocDataArray** data_arr = gpc_ptr_table + gpc_ptr_table_index(key_ptr);
    return ((gpc_AllocData*)((*data_arr) + 1))[(*data_arr)->length - 1];
}

static gpc_AllocDataArray* gpc_alloc_data_arr(void* key_ptr)
{
    gpc_AllocDataArray** data_arr = gpc_ptr_table + gpc_ptr_table_index(key_ptr);
    return *data_arr;
}

void suppress(void)
{
    (void)gpc_alloc_data;
    (void)gpc_init_ptr_table;
    (void)gpc_alloc_data_arr;
}
