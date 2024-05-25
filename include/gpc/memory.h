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
    void  (*dealloc)(const struct gp_allocator*, void*  block);
} GPAllocator;

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_MALLOC_SIZE(2)
inline void* gp_mem_alloc(
    const GPAllocator* allocator,
    size_t size)
{
    return allocator->alloc(allocator, size);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_MALLOC_SIZE(2)
inline void* gp_mem_alloc_zeroes(
    const GPAllocator* allocator,
    size_t size)
{
    void* memset(void*, int, size_t);
    return memset(gp_mem_alloc(allocator, size), 0, size);
}

GP_NONNULL_ARGS(1)
inline void gp_mem_dealloc(
    const GPAllocator* allocator,
    void* block)
{
    if (block != NULL)
        allocator->dealloc(allocator, block);
}

GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
void* gp_mem_realloc(
    const GPAllocator* allocator,
    void*  optional_old_block,
    size_t old_size,
    size_t new_size);

#define gp_alloc(allocator, type, count) \
    gp_mem_alloc((GPAllocator*)(allocator), (count) * sizeof(type))

#define gp_alloc_zeroes(allocator, type, count) \
    gp_mem_alloc_zeroes((GPAllocator*)(allocator), (count) * sizeof(type))

#define gp_dealloc(allocator, optional_block) \
    gp_mem_dealloc((GPAllocator*)(allocator), (optional_block))

#define gp_realloc(allocator, optional_block, old_capacity, new_capacity) \
    gp_mem_realloc( \
        (GPAllocator*)(allocator), \
        optional_block, \
        old_capacity, \
        new_capacity)

// ----------------------------------------------------------------------------
// Scope allocator

// Create thread local scope.
GPAllocator* gp_begin(size_t size) GP_NONNULL_RETURN GP_NODISCARD;

// End scope and any inner scopes that have not been ended.
void gp_end(GPAllocator* optional_scope);

// Deferred functions are called in Last In First Out order in gp_end().
void gp_defer(GPAllocator* scope, void (*f)(void* arg), void* arg)
    GP_NONNULL_ARGS(1, 2);

// Get lastly created scope in callbacks. You should prefer to just pass scopes
// as arguments when possible.
GPAllocator* gp_last_scope(GPAllocator* return_this_if_no_scopes);

// ----------------------------------------------------------------------------
// Arena allocator

// Arena that does not run out of memory. This is achieved by creating new
// arenas when old one gets full.
typedef struct gp_arena GPArena;

// growth_coefficient determines how large each subsequent arena in arena list
// is relative to previous arena when the previous arena gets full.
GPArena gp_arena_new(size_t capacity, double growth_coefficient) GP_NODISCARD;
void gp_arena_delete(GPArena* optional);
void gp_arena_rewind(GPArena*, void* to_this_position) GP_NONNULL_ARGS();

// ----------------------------------------------------------------------------
// Heap allocator

#ifdef NDEBUG
/** malloc() based allocator. */
extern const GPAllocator*const gp_heap;
#else // heap allocator can be overridden for debugging
/** malloc() based allocator. */
extern const GPAllocator* gp_heap;
#endif


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

//
struct gp_arena
{
    GPAllocator allocator;
    double growth_coefficient;
    struct gp_arena_node* head; // also contains arenas memory block
};

#define GP_MEM_STRFY(A) #A

#endif // GP_MEMORY_INCLUDED
