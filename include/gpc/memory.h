// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file memory.h
 * @brief Memory management and allocators
 */

#ifndef GPMEMORY_INCLUDED
#define GPMEMORY_INCLUDED

#include "attributes.h"
#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

/** Memory allocator. */
typedef struct GPAllocator
{
    void* (*alloc)  (const struct GPAllocator*, size_t block_size);
    void  (*dealloc)(const struct GPAllocator*, void* block);
} GPAllocator;

GP_NODISCARD inline void* gpmem_alloc(const GPAllocator a[GP_NONNULL], size_t size)
{
    return a->alloc(a, size);
}

inline void
gpmem_dealloc(const GPAllocator allocator[GP_NONNULL], void* block)
{
    allocator->dealloc(allocator, block);
}

#define gp_alloc(allocator, type) \
    gpmem_alloc((GPAllocator*)(allocator), sizeof(type))
#define gp_dealloc(allocator, block) \
    gpmem_dealloc((GPAllocator*)(allocator), block)

// ----------------------------------------------------------------------------

/** malloc() based allocator. */
extern const GPAllocator gpmem_std_allocator;

// ----------------------------------------------------------------------------

inline void* gpmem_null_alloc(const GPAllocator* unused, size_t unused1)
{
    (void)unused, (void)unused1;
    return NULL;
}

inline void
gpmem_null_dealloc(const GPAllocator* unused, void* unused1)
{
    (void)unused; (void)unused1;
}

/** Always returns NULL on allocations. */
extern const GPAllocator gpmem_null_allocator;

// ----------------------------------------------------------------------------

typedef struct GPArena GPArena;

GPArena* gpmem_arena(size_t capacity);
void gpmem_free_arena(GPArena*);

#endif // GPMEMORY_INCLUDED
