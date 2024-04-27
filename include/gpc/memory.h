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
    void  (*const dealloc)(const struct gp_allocator*, void*  block);
} GPAllocator;

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
inline void* gp_mem_alloc(
    const GPAllocator* allocator,
    size_t size)
{
    return allocator->alloc(allocator, size);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
inline void* gp_mem_alloc_zeroes(
    const GPAllocator* allocator,
    size_t size)
{
    void* memset(void*, int, size_t);
    return memset(gp_mem_alloc(allocator, size), 0, size);
}

GP_NONNULL_ARGS(1)
inline void* gp_mem_dealloc(
    const GPAllocator* allocator,
    void* block)
{
    if (block != NULL)
        allocator->dealloc(allocator, block);
    return NULL;
}

GP_NONNULL_ARGS(1) GP_NODISCARD
inline void* gp_mem_realloc(
    const GPAllocator* allocator,
    void*  old_block,
    size_t old_size,
    size_t new_size)
{
    void* new_block = gp_mem_alloc(allocator, new_size);
    void* memcpy(void*, const void*, size_t);
    memcpy(new_block, old_block, old_size);
    gp_mem_dealloc(allocator, old_block);
    return new_block;
}

#define gp_alloc(allocator, type, count) \
    gp_mem_alloc((GPAllocator*)(allocator), (count) * sizeof(type))

#define gp_alloc_zeroes(allocator, type, count) \
    gp_mem_alloc_zeroes((GPAllocator*)(allocator), (count) * sizeof(type))

#define gp_dealloc(allocator, block) ( \
    gp_mem_dealloc((GPAllocator*)(allocator), (block)), \
    (void*)"Deallocated at "__FILE__" line "GP_MEM_STRFY(__LINE__) \
)

#define gp_realloc(allocator, ptrptr, old_capacity, new_capacity) \
    (*(ptrptr) = gp_mem_realloc( \
        allocator, \
        (*ptrptr), \
        sizeof(**(ptrptr)) * (old_capacity), \
        sizeof(**(ptrptr)) * (new_capacity)))

// ----------------------------------------------------------------------------

/** malloc() based allocator. */
extern const GPAllocator gp_heap;

// ----------------------------------------------------------------------------

typedef struct GPArena GPArena;

GPArena* gp_mem_arena(size_t capacity);
void gp_mem_free_arena(GPArena*);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#define GP_MEM_STRFY(A) #A

#endif // GP_MEMORY_INCLUDED
