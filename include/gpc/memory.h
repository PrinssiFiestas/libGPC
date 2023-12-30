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
    void* (*alloc)  (struct GPAllocator*, size_t block_size);
    void  (*dealloc)(struct GPAllocator*, void* block, size_t block_size);
} GPAllocator;

inline void* gpmem_alloc(GPAllocator allocator[GPC_NONNULL], size_t size)
{
    return allocator->alloc(allocator, size);
}

inline void
gpmem_dealloc(GPAllocator allocator[GPC_NONNULL], void* block, size_t size)
{
    allocator->dealloc(allocator, block, size);
}

// Any-other-than-std-allocator encouragers
#define gp_alloc(allocator, size) gpmem_alloc((GPAllocator*)allocator, size)
#define gp_dealloc(allocator, block, block_size) \
    gpmem_dealloc((GPAllocator*)allocator, block, block_size)

/** Calls malloc(@p block_size). */
void* gpmem_std_alloc(GPAllocator* unused, size_t block_size);
/** Calls free(@p block). */
void  gpmem_std_dealloc(GPAllocator* unused, void* block, size_t unused1);

/** malloc() based allocator. */
const GPAllocator gpmem_std_allocator =
{
    gpmem_std_alloc,
    gpmem_std_dealloc
};

inline void* gpmem_null_alloc(GPAllocator* unused, size_t unused1)
{
    (void)unused, (void)unused1;
    return NULL;
}

inline void
gpmem_null_dealloc(GPAllocator* unused, void* unused1, size_t unused2)
{
    (void)unused; (void)unused1; (void)unused2;
}

/** Used for error handling. */
const GPAllocator gpmem_null_allocator =
{
    gpmem_null_alloc,
    gpmem_null_dealloc
};

// ----------------------------------------------------------------------------

typedef struct GPArena GPArena;

GPArena* gpmem_arena(size_t capacity);
void gpmem_free_arena(GPArena*);

#endif // GPMEMORY_INCLUDED
