// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file memory.h
 * Memory management and allocators
 */

#ifndef GP_MEMORY_INCLUDED
#define GP_MEMORY_INCLUDED 1

#include <gpc/attributes.h>
#include <gpc/assert.h>
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


// No valid GPAllocator will return NULL in any circumstance. This is because
// malloc() and aligned_alloc() only return NULL on invalid inputs or if there
// was insufficient memory available. In the latter case, malloc() probably
// already crashed and if not, the system is in a critical state and should be
// abort()ed anyway. In the first case, the inputs should be validated before
// calling the allocators. This massively simplifies NULL handling and makes
// error handling more explicit.

/** Polymorphic Allocator.*/
typedef struct gp_allocator
{
    void* (*alloc)  (struct gp_allocator*, size_t size, size_t alignment);
    void  (*dealloc)(struct gp_allocator*, void*  block);
} GPAllocator;

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline void* gp_mem_alloc(
    GPAllocator* allocator,
    size_t size)
{
    gp_db_assert(size < SIZE_MAX/2, "Possibly negative allocation detected.");
    return allocator->alloc(allocator, size, GP_ALLOC_ALIGNMENT);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_ALLOC_ALIGN(3)
static inline void* gp_mem_alloc_aligned(
    GPAllocator* allocator,
    size_t size,
    size_t alignment)
{
    gp_db_assert(size < SIZE_MAX/2, "Possibly negative allocation detected.");
    gp_db_assert((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2.");
    return allocator->alloc(allocator, size, alignment);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline void* gp_mem_alloc_zeroes(
    GPAllocator* allocator,
    size_t size)
{
    gp_db_assert(size < SIZE_MAX/2, "Possibly negative allocation detected.");
    return memset(gp_mem_alloc(allocator, size), 0, size);
}

GP_NONNULL_ARGS(1)
static inline void gp_mem_dealloc(
    GPAllocator* allocator,
    void* block)
{
    if (block != NULL)
        allocator->dealloc(allocator, block);
}

/** Maybe reallocate block.
 * If @p new_size <= @p old_size, no reallocation happens. Also, if @p allocator
 * is a GPArena, the arena extends @p old_block without reallocating if
 * @p old_block is the last object allocated by the arena.
 * @p old_block may be NULL if @p old_size is zero.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
void* gp_mem_realloc(
    GPAllocator* allocator,
    void*  optional_old_block,
    size_t old_size,
    size_t new_size);

// ----------------------------------------------------------------------------
// Scope Allocator

// TODO better docs

/** Create scope arena.*/
GPAllocator* gp_begin(size_t size) GP_NONNULL_RETURN GP_NODISCARD;

/** Free scope arena.
 * Also frees any inner scopes in the current thread that have not been ended.
 * Calls deferred functions.
 * @return combined size of all internal buffers. This may be useful to
 * determine appropriate size for gp_begin().
 */
size_t gp_end(GPAllocator* optional_scope);

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
GP_NODISCARD
GPAllocator* gp_last_scope(void);

#if __GNUC__ || _MSC_VER
#define GP_BEGIN { GP_SCOPE_BEGIN
#define GP_END     GP_SCOPE_END }
#endif

// ----------------------------------------------------------------------------
// Arena Allocator

/** Arena that does not run out of memory.
 * If address sanitizer is used, unused memory, freed memory, and allocation
 * boundaries are poisoned. The allocated memory cannot be assumed to be
 * contiguous due to boundary poisoning and linked list based backing buffers.
 */
typedef struct gp_arena GPArena;

typedef struct gp_arena_initializer
{
    /** Determine where arena gets it's memory from.
     * Default is gp_heap. If backing buffer is provided, then this will only
     * determine how additional buffers are allocated.
     */
    GPAllocator* backing_allocator;

    /** Determines initial arena memory.
     * Useful for recycling large buffers or using static memory. If not
     * provided, backing allocator will allocate the inital block instead.
     * If provided, capacity argument of gp_arena_new() must match buffer size.
     * If the buffer cannot fit arena meta data, it will not be used.
     */
    void* backing_buffer;

    /** Limit the arena size.
     * Arenas will not grow past this value. Useful when
     * growth_coefficient > 1.0.
     */
    size_t max_size;

    /** Determine how new arenas grow.
     * Use this to determine the size of new arena node when old gets full. A
     * value larger than 1.0 is useful for arenas that have small initial size.
     * This allows the arena to estimate an optimal size for itself during
     * runtime. A value smaller than 1.0 is useful for arenas that start out
     * huge to not waste memory.
     */
    double growth_coefficient;

    /** Size of the structure.
     * Default is sizeof(GPArena). When inheriting from GPArena, this must be
     * set to sizeof(YourArena) to allocate enough memory for the structure.
     */
     size_t meta_size;
} GPArenaInitializer;

/** Create arena.*/
GP_NONNULL_RETURN
GPArena* gp_arena_new(const GPArenaInitializer* optional, size_t capacity);

/** Deallocate some memory.
 * Use this to free everything allocated after @p to_this_position including
 * @p to_this_position. The passed pointer must be a pointer returned by the
 * arena.
 */
void gp_arena_rewind(GPArena*, void* to_this_position) GP_NONNULL_ARGS();

/** Deallocate all memory excluding the arena itself.
 * Fully rewinds the arena pointer to the beginning of the arena.
 * @return combined size of all internal buffers. This may be useful to
 * determine appropriate size for gp_arena_init().
 */
size_t gp_arena_reset(GPArena*) GP_NONNULL_ARGS();

/** Deallocate all arena memory including the arena itself.
 */
void gp_arena_delete(GPArena* optional);

// ----------------------------------------------------------------------------
// Thread Local Scratch Arena

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
// Heap Allocator

/** malloc() based allocator.*/
extern GPAllocator* gp_heap;

/** Allocation count to help optimizations.*/
GP_NODISCARD
size_t gp_heap_alloc_count(void);

// ----------------------------------------------------------------------------
// Virtual Allocator

/** Contiguous fast huge arena allocator.
 * Arena that uses contiguous possibly huge memory blocks for it's backing
 * buffer. Allocated memory is contiguous ignoring possible padding bytes to
 * satisfy alignment requirements. The assumption is that the backing buffer is
 * large enough to not run out of memory, thus bounds are only checked if
 * NDEBUG is not defined or GP_VIRTUAL_ALWAYS_BOUNDS_CHECK is defined.
 */
typedef struct gp_virtual_arena
{
    GPAllocator base;
    void* start;     // of the memory block
    void* position;  // arena pointer
    size_t capacity; // of arena
} GPVirtualArena;

/** Allocate and initialize.
 * @p capacity will be rounded up to page size boundary. It is recommended to
 * pass huge (at least hundreds of megs depending on your application and
 * underlying system) @p capacity to prevent out of memory bugs. Physical memory
 * will only be used on writes to the arena.
 * @return pointer to arena casted to GPAllocator* or NULL if virtual memory
 * allocation fails. In case of failures, you may want to try again with smaller
 * capacity.
 */
GPAllocator* gp_virtual_init(GPVirtualArena*, size_t capacity) GP_NONNULL_ARGS();

/** Deallocate some memory.
 * Use this to free everything allocated after @p to_this_position including
 * @p to_this_position. Physical memory remains untouched.
 */
GP_NONNULL_ARGS()
static inline void gp_virtual_rewind(GPVirtualArena* arena, void* to_this_position)
{
    uint8_t* pointer = arena->position = to_this_position;
    gp_db_assert(pointer < (uint8_t*)arena->start + arena->capacity, "Pointer points outside the arena.");
    gp_db_assert(pointer >= (uint8_t*)arena->start, "Pointer points outside the arena.");
}

/** Deallocate all memory excluding the arena itself.
 * Fully rewinds the arena pointer to the beginning of the arena. Physical
 * memory will be deallocated, but virtual address space remains untouched.
 */
void gp_virtual_reset(GPVirtualArena*) GP_NONNULL_ARGS();

/** Deallocate all arena memory including the arena itself.*/
void gp_virtual_delete(GPVirtualArena* optional);

/** Allocate memory from virtual arena.
 * gp_mem_alloc() is meant to be polymorphic, use this directly to maximize
 * performance.
 */
GP_NONNULL_ARGS_AND_RETURN GP_ALLOC_ALIGN(3)
static inline void* gp_virtual_alloc(
    GPVirtualArena* allocator, const size_t size, const size_t alignment)
{
    gp_db_assert(size < SIZE_MAX/2, "Possibly negative allocation detected.");
    gp_db_assert((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2.");

    GPVirtualArena* arena = (GPVirtualArena*)allocator;
    void* block = arena->position;
    arena->position = (void*)(gp_round_to_aligned((uintptr_t)arena->position, alignment) + size);

    #if !defined(NDEBUG) || /*user*/defined(GP_VIRTUAL_ALWAYS_BOUNDS_CHECK)
    gp_assert((uint8_t*)arena->position <= (uint8_t*)arena->start + arena->capacity, "Virtual allocator out of memory.");
    #endif
    return block;
}

// ----------------------------------------------------------------------------
// Mutex Allocator

typedef struct gp_mutex_allocator
{
    GPAllocator  base;
    GPAllocator* backing;
    void*        mutex; // TODO make GPThread public
} GPMutexAllocator;


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


struct gp_arena
{
    GPAllocator base;
    GPAllocator* backing;
    double growth_coefficient;
    size_t max_size; // TODO this should be asserted, not saturated! Saturation is unnecessary due to virtual memory. Assertion allows compile time virtual/generic arena.
    void* is_shared; // TODO get rid of this, replace with mutex allocator
    struct gp_arena_node* head;
};

// GP_SCOPE_BEGIN and GP_SCOPE_END
#if __GNUC__
typedef struct gp_auto_scope_data
{ // TODO defer stack size as well
    GPAllocator* scope;
    size_t* size;
} GPAutoScopeData;

static inline void gp_auto_scope_clean(GPAutoScopeData* scope)
{
    size_t scope_size = gp_end(scope->scope);
    *scope->size = (scope_size >> 1) + (*scope->size >> 1); // IIR smoothing
}
#define GP_SCOPE_BEGIN \
    static __thread size_t _gp_auto_scope_size; \
    GPAllocator* scope = gp_begin(_gp_auto_scope_size); \
    __attribute__((cleanup(gp_auto_scope_clean))) GPAutoScopeData _gp_auto_scope_data = \
        { scope, &_gp_auto_scope_size };
    // user code
#define GP_SCOPE_END

#elif _MSC_VER

#define GP_SCOPE_BEGIN \
    static __declspec(thread) size_t _gp_auto_scope_size; \
    GPAllocator* scope = gp_begin(_gp_auto_scope_size); \
    __try {
        // user code
#define GP_SCOPE_END } __finally { \
    _gp_auto_scope_size = (gp_end(scope) >> 1) + (_gp_auto_scope_size >> 1); \
}

#endif // GP_SCOPE_BEGIN and GP_SCOPE_END

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_MEMORY_INCLUDED
