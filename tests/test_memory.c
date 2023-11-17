// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GPC_DEBUG_MEMORY
#include "../include/gpc/assert.h"
#include "../src/memory.c"

int main(void)
{
    gpc_init_ptr_table();


    // void* p0 = gpc_allocate(NULL, 1); int p0_line = __LINE__;
    // void* p1 = gpc_allocate(NULL, 1); int p1_line = __LINE__;
    // void* p2 = gpc_allocate(NULL, 1); int p2_line = __LINE__;
    // gpc_expect(gpc_alloc_data(p0).line == p0_line);
    // gpc_expect(gpc_alloc_data(p1).line == p1_line);
    // gpc_expect(gpc_alloc_data(p2).line == p2_line);

    size_t i = 1;
    while (i++)
    {
        void* p = gpc_allocate(NULL, 1);
        gpc_assert(gpc_alloc_data_arr(p)->length < 2, (i));
    }

    gpc_assert(0);
}
