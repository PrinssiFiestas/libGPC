// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file memory.h
 * @brief Memory management and allocators
 */

#ifndef GP_MEMORY_INCLUDED
#define GP_MEMORY_INCLUDED

#include <gpc/attributes.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Aligment of all pointers returned by any valid allocators
#ifndef GP_UTILS_INCLUDED
#if __STDC_VERSION__ >= 201112L && !defined(_MSC_VER)
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

// ----------------------------------------------------------------------------
// Scope allocator

// Create thread local scope.
GPAllocator* gp_begin(size_t size) GP_NONNULL_RETURN GP_NODISCARD;

// End scope and any inner scopes that have not been ended.
void gp_end(GPAllocator* optional_scope);

// Deferred functions are called in Last In First Out order in gp_end().
GP_NONNULL_ARGS(1, 2)
void gp_scope_defer(GPAllocator* scope, void (*f)(void* arg), void* arg);

// like scope_defer() but can take also take functions with non-void pointer
// arguments like fclose. Also argument to f will be type checked.
#define gp_defer(scope, f, arg) do { \
    if (0) (f)(arg); \
    gp_scope_defer(scope, (void(*)(void*))(f), arg); \
} while(0)

// Get lastly created scope in callbacks. You should prefer to just pass scopes
// as arguments when possible.
GPAllocator* gp_last_scope(const GPAllocator* return_this_if_no_scopes);

// Feel free to define your own values for these. If C11 atomics are available,
// the scope allocator estimates optimal init size during runtime. This init
// size determines minimum arena size.
#ifndef GP_SCOPE_DEFAULT_INIT_SIZE
#define GP_SCOPE_DEFAULT_INIT_SIZE 256
#endif
#ifndef GP_SCOPE_DEFAULT_MAX_SIZE
#define GP_SCOPE_DEFAULT_MAX_SIZE (1 << 15) // 32 KB
#endif
#ifndef GP_SCOPE_DEFAULT_GROWTH_COEFFICIENT
#define GP_SCOPE_DEFAULT_GROWTH_COEFFICIENT 2.0
#endif

// ----------------------------------------------------------------------------
// Arena allocator

// Arena that does not run out of memory. A new arena is created when old gets
// full.
typedef struct gp_arena
{
    GPAllocator allocator;
    double growth_coefficient;
    size_t max_size;
    size_t alignment;
    struct gp_arena_node* head; // private
} GPArena;

// Basic fast arena
GPArena gp_arena_new(size_t capacity) GP_NODISCARD;

// Arena with mutex alloc(). dealloc() is also thread safe, but delete() and
// rewind() is not!
GPArena* gp_arena_new_shared(size_t capacity) GP_NODISCARD;

// Use this to free everything after to_this_position including
// to_this_position. Pass the first allocated object to clear the whole arena.
void gp_arena_rewind(GPArena*, void* to_this_position) GP_NONNULL_ARGS();

void gp_arena_delete(GPArena* optional);

// ----------------------------------------------------------------------------
// Thread local scratch arena

// Use this for temporary memory. Rewind when you are done, but do NOT delete
// the arena. Scratch arenas get deleted automatically when threads exit.
GPArena* gp_scratch_arena(void) GP_NODISCARD;

// Feel free to define your own values for these. 256 is extremely conservative,
// you probably want much larger scratch arenas.
#ifndef GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE
#define GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE 256
#endif
#ifndef GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE
#define GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE SIZE_MAX
#endif
#ifndef GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT
#define GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT 2.0
#endif

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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_MEMORY_INCLUDED
