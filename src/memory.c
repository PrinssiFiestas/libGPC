// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <stdlib.h>

extern inline void* gp_mem_alloc(const GPAllocator*, size_t);
extern inline void  gp_mem_dealloc(const GPAllocator*, void*);

static void* gp_heap_alloc(const GPAllocator* unused, size_t block_size)
{
    (void)unused;
    return malloc(block_size);
}

static void gp_heap_dealloc(const GPAllocator* unused, void* block)
{
    (void)unused;
    free(block);
}

const GPAllocator gp_heap = {
    .alloc   = gp_heap_alloc,
    .dealloc = gp_heap_dealloc
};

// ----------------------------------------------------------------------------

struct GPArena
{
    GPAllocator allocator;
    size_t pos;
    size_t cap;
    unsigned char primary_mem[];
};

GPArena* gpmem_arena(size_t capacity);
void gpmem_free_arena(GPArena*);
