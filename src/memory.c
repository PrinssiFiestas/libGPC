// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdlib.h>

extern inline void* gp_mem_alloc       (const GPAllocator*,size_t);
extern inline void* gp_mem_alloc_zeroes(const GPAllocator*,size_t);
extern inline void* gp_mem_dealloc     (const GPAllocator*,void*);
extern inline void* gp_mem_realloc     (const GPAllocator*,void*,size_t,size_t);

static void* gp_heap_alloc(const GPAllocator* unused, size_t block_size)
{
    (void)unused;
    void* mem = malloc(block_size);
    if (mem == NULL) {
        GP_BREAKPOINT;
        abort();
    }
    return mem;
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

#ifdef __GNUC__
__attribute__((always_inline))
#endif
static inline void* gp_crashing_alloc(const GPAllocator* unused, size_t unused_size)
{
    (void)unused; (void)unused_size;
    GP_BREAKPOINT;
    abort();
    return "";
}

static void gp_no_op_dealloc(const GPAllocator* unused, void* unused_block)
{
    (void)unused; (void)unused_block;
}

const GPAllocator gp_crash_on_alloc = {
    .alloc   = gp_crashing_alloc,
    .dealloc = gp_no_op_dealloc
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
