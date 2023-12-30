// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GPC_DEBUG_MEMORY
#include "../include/gpc/assert.h"
#include "../src/memory.c"

#include <stdio.h>
#include <time.h>

int main(void)
{
    return 0;

    gpc_init_ptr_table();
    gpc_g_random_seed((uint64_t)time(NULL));
    gpc_hash_seed = (uint64_t)time(NULL) ^ (uint64_t)(uintptr_t)printf;

    // void* p0 = gpc_allocate(NULL, 1); int p0_line = __LINE__;
    // void* p1 = gpc_allocate(NULL, 1); int p1_line = __LINE__;
    // void* p2 = gpc_allocate(NULL, 1); int p2_line = __LINE__;
    // gpc_expect(gpc_alloc_data(p0).line == p0_line);
    // gpc_expect(gpc_alloc_data(p1).line == p1_line);
    // gpc_expect(gpc_alloc_data(p2).line == p2_line);

    gpc_expect(0, (gpc_hash_range(5, 100)));
    gpc_expect(0, (gpc_hash_range(5, 100)));
    gpc_expect(0, (gpc_hash_range(5, 100)));
    gpc_expect(0, (gpc_hash_range(6, 100)));
    gpc_expect(0, (gpc_hash_range(6, 100)));
    gpc_expect(0, (gpc_hash_range(6, 100)));
    gpc_expect(0, (gpc_hash_range(7, 100)));
    gpc_expect(0, (gpc_hash_range(7, 100)));
    gpc_expect(0, (gpc_hash_range(7, 100)));
    gpc_expect(0, (gpc_hash_range(5, 100)));
    gpc_expect(0, (gpc_hash_range(5, 100)));
    gpc_expect(0, (gpc_hash_range(5, 100)));

    size_t i = 1;
    while (i++)
    {
        void* p = gpc_allocate(NULL, 1);
        gpc_assert(gpc_alloc_data_arr(p)->length < 2, (i));
    }

    for (size_t i = 0; i < 16; i++)
    {
        char* p = malloc(sizeof(*p));
        printf("%p\n", p);
    }

    //gpc_assert(0);
}
