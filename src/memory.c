// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/memory.h"
#include <stdlib.h>

extern inline void* gpmem_alloc(const GPAllocator[GP_NONNULL], size_t);
extern inline void gpmem_dealloc(const GPAllocator[GP_NONNULL], void*);
extern inline void* gpmem_null_alloc(const GPAllocator*, size_t);
extern inline void gpmem_null_dealloc(const GPAllocator*, void*);

static void* gpmem_std_alloc(const GPAllocator* unused, size_t block_size)
{
    (void)unused;
    return malloc(block_size);
}

static void gpmem_std_dealloc(const GPAllocator* unused, void* block)
{
    (void)unused;
    free(block);
}

const GPAllocator gpmem_std_allocator =
{
    gpmem_std_alloc,
    gpmem_std_dealloc
};

const GPAllocator gpmem_null_allocator =
{
    gpmem_null_alloc,
    gpmem_null_dealloc
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
