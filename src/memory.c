// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/memory.h"
#include <stdlib.h>
#include <stdint.h>

// The size of the hash table array storing all pointers
// Larger array: less collisions but more memory usage.
// Here are some measurements for a single dimensional array. Of course the
// amount of pointers without collisions changed each run so the value is
// roundabout. I only tried 3 runs for each table size.
// -----------------------------------------------------------------
// Table size (bits) | Table size (MB) | Pointers without collisions
// -----------------------------------------------------------------
//     1 << 8        |      .002       |               30
//     1 << 12       |      .032       |              800
//     1 << 16       |      .52        |            4'000
//     1 << 20       |      8.4        |           26'000
//     1 << 24       |      134        |          260'000
//     1 << 28       |     2147        |        4'000'000
#define GPC_PTR_TABLE_SIZE (1 << 8)
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

// Simply use the the address given by malloc() as the index for the hash map.
// Not very random but ridiculously fast! (If no collisions... shush...)
static size_t gpc_ptr_table_index(void* ptr)
{
    uintmax_t p = (uintmax_t)ptr;
    p >>= 4; // malloc() on test machine does not use these bits
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
