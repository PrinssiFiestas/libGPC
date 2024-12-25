// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file memory.h
 * Memory management and allocators
 */

#ifndef GP_MEMORY_INCLUDED
#define GP_MEMORY_INCLUDED 1

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
#if (__STDC_VERSION__ >= 201112L && !defined(_MSC_VER)) || defined(__COMPCERT__)
#define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#else
#define GP_ALLOC_ALIGNMENT (sizeof(long double))
#endif
#endif

/** Polymorphic allocator.*/
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

// The scope allocator is an allocator designed to make lifetimes trivial. Use
// gp_begin() to create a new arena based allocator. You can then encapsulate
// the allocator in GPString, GPArray, or manually allocate memory. When the
// allocator is passed to gp_end(), all memory is freed at once. This is much
// simpler and more performant than using malloc()-free() pairs. It can also
// handle mismatched gp_begin()-gp_end() pairs: if a scope misses it's gp_end()
// call, the next call to gp_end() will end all unended scopes making memory
// leaks and other memory bugs practically impossible.

/** Create scope arena.*/
GPAllocator* gp_begin(size_t size) GP_NONNULL_RETURN GP_NODISCARD;

/** Free scope arena.
 * Also frees any inner scopes in the current thread that have not been ended.
 * Calls deferred functions.
 */
void gp_end(GPAllocator* optional_scope);

/** Set cleanup routines to be executed when scope ends.
 * Deferred functions are called in Last In First Out order in gp_end().
 * Deferring should not be used for gp_str_delete() or gp_arr_delete() due
 * to possibility of reallocating which would cause double free. It is not
 * needed either, since using the scope allocator makes freeing redundant.
 * Deferring is meant to clean other than memory resources like file pointers.
 */
GP_NONNULL_ARGS(1, 2)
void gp_scope_defer(GPAllocator* scope, void (*f)(void* arg), void* arg);

/** Set cleanup routines to be executed when scope ends.
 * like gp_scope_defer() but with type checking and can also take functions
 * with non-void pointer arguments like gp_file_close().
 */
#define gp_defer(scope, f, arg) do { \
    if (0) (f)(arg); \
    gp_scope_defer(scope, (void(*)(void*))(f), arg); \
} while(0)

/** Get lastly created scope in the current thread.
 * You should prefer to just pass scopes as arguments when possible. This exists
 * only to be able to access the current scope allocator in callbacks.
 */
GPAllocator* gp_last_scope(const GPAllocator* return_this_if_no_scopes);

// ----------------------------------------------------------------------------
// Arena allocator

/** Arena that does not run out of memory.
 * If arena gets full, a new one is created in a linked list.
 */
typedef struct gp_arena
{
    /** @private */
    GPAllocator allocator;

    /** Determine how new arenas grow.
     * Use this to determine the size of new arena node when old gets full. A
     * value larger than 1.0 is useful for arenas that have small initial size.
     * This allows the arena to estimate an optimal size for itself during
     * runtime. A value smaller than 1.0 is useful for arenas that start out
     * huge to not waste memory.
     */
    double growth_coefficient;

    /** Limit the arena size.
     * Arenas will not grow past this value. Useful when
     * growth_coefficient > 1.0.
     */
    size_t max_size;

    /** Alignment requirement returned memory blocks.
     * Default is GP_ALLOC_ALIGNMENT. A larger requirement should be used if
     * arena is used for SIMD vectors. A smaller requirement can be used to save
     * memory and limit fragmentation if it is known that the arena is only used
     * to allocate objects with smaller alignment requirement e.g. strings.
     * However, note that GPString and GPArray assumes an alignment of
     * GP_ALLOC_ALIGNMENT, so it is recommended to not use GPString and GPArray
     * when alignment is not GP_ALLOC_ALIGNMENT.
     */
    size_t alignment;

    /** @private */
    struct gp_arena_node* head;
} GPArena;

/** Basic fast arena.*/
GPArena gp_arena_new(size_t capacity) GP_NODISCARD;

/** Mutex protected arena.
 * Arena with mutex alloc(). dealloc() is also thread safe, but delete() and
 * rewind() is not!
 */
GPArena* gp_arena_new_shared(size_t capacity) GP_NODISCARD;

/** Deallocate some memory.
 * Use this to free everything allocated after @p to_this_position including
 * @p to_this_position. Pass the first allocated object to clear the arena.
 */
void gp_arena_rewind(GPArena*, void* to_this_position) GP_NONNULL_ARGS();

/** Deallocate all arena memory including the arena itself.*/
void gp_arena_delete(GPArena* optional);

// ----------------------------------------------------------------------------
// Thread local scratch arena

/** Arena allocator for temporary memory.
 * Unlike the scope allocator, which creates a new arena for each scope, there
 * is only one scratch arena per thread. This is almost as fast as using stack
 * memory, but the downside is that you cannot safely use this for objects that
 * may reallocate.
 *     Rewind when you are done, but do NOT delete the arena. Scratch arenas get
 * deleted automatically when threads exit.
 */
GPArena* gp_scratch_arena(void) GP_NODISCARD;

// Feel free to define your own values for these. 256 is extremely conservative,
// you probably want much larger scratch arenas. Check above for the meanings of
// these.
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

/** malloc() based allocator.*/
extern const GPAllocator* gp_heap;


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
