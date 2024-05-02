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
#include <signal.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Aligment of all pointers returned by any valid allocators
#ifndef GP_UTILS_INCLUDED
#if __STDC_VERSION__ >= 201112L
#define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#else
#define GP_ALLOC_ALIGNMENT (sizeof(long double))
#endif
#endif

//
typedef struct gp_allocator
{
    void* (*alloc)  (const struct gp_allocator*, size_t block_size);
    void* (*dealloc)(const struct gp_allocator*, void*  block);
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
        return allocator->dealloc(allocator, block);
    return NULL;
}

GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
inline void* gp_mem_realloc(
    const GPAllocator* allocator,
    void*  old_block,
    size_t old_size,
    size_t new_size)
{
    void* new_block = gp_mem_alloc(allocator, new_size);
    void* memcpy(void*, const void*, size_t);
    if (old_block != NULL)
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
    (void*) #block" deallocated at "__FILE__" line "GP_MEM_STRFY(__LINE__) \
)

#define gp_realloc(allocator, block, old_capacity, new_capacity) \
    gp_mem_realloc( \
        allocator, \
        block, \
        old_capacity, \
        new_capacity)

// ----------------------------------------------------------------------------

#ifdef NDEBUG
const
#endif // else heap allocator can be overridden for debugging
/** malloc() based allocator. */
extern GPAllocator gp_heap;

/** Tries to set breakpoint and crashes on allocations. */
extern const GPAllocator gp_crash_on_alloc;

// ----------------------------------------------------------------------------

typedef struct gp_arena
{
    GPAllocator allocator;
    double growth_coefficient;
    struct gp_arena_node* head;
} GPArena;

GPArena gp_arena_new(size_t capacity, double growth_coefficient) GP_NODISCARD;
void gp_arena_rewind(GPArena*, void* to_this_position) GP_NONNULL_ARGS(1);
void gp_arena_delete(GPArena*);

// ----------------------------------------------------------------------------

extern GP_THREAD_LOCAL GPAllocator* gp_scope;
extern GP_THREAD_LOCAL GPAllocator* gp_scope_back;
void* gp_begin(void) GP_NODISCARD;
void  gp_end(void* return_value_of_gp_begin);

extern sig_atomic_t gp_scope_init_capacity;

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#define GP_MEM_STRFY(A) #A

#endif // GP_MEMORY_INCLUDED
