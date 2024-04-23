// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file memory.h
 * @brief Memory management and allocators
 */

#ifndef GP_MEMORY_INCLUDED
#define GP_MEMORY_INCLUDED

#include "attributes.h"
#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/** Memory allocator. */
typedef struct gp_allocator
{
    void* (*const alloc)  (const struct gp_allocator*, size_t block_size);
    void  (*const dealloc)(const struct gp_allocator*, void* block);
} GPAllocator;

GP_NONNULL_ARGS() GP_NODISCARD
inline void* gp_mem_alloc(
    const GPAllocator* a,
    size_t size)
{
    return a->alloc(a, size);
}

GP_NONNULL_ARGS(1)
inline void gp_mem_dealloc(
    const GPAllocator* allocator,
    void* block)
{
    allocator->dealloc(allocator, block);
}

#define gp_alloc(allocator, type, count) \
    gpmem_alloc((GPAllocator*)(allocator), (count) * sizeof(type))
#define gp_dealloc(allocator, block) \
    gpmem_dealloc((GPAllocator*)(allocator), (block))

// ----------------------------------------------------------------------------

/** malloc() based allocator. */
extern const GPAllocator gp_heap;

// ----------------------------------------------------------------------------

typedef struct GPArena GPArena;

GPArena* gp_mem_arena(size_t capacity);
void gp_mem_free_arena(GPArena*);

#endif // GP_MEMORY_INCLUDED
