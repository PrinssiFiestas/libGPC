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
#include <gpc/thread.h>
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
//     Allocators inerit from GPAllocator by having GPAllocator as the first
// member of the struct. Pointers to allocators can safely be upcasted to
// GPAllocator*, although taking the address of the base allocator (the first
// member, usually also named as 'base') is more type safe.

/** Polymorphic abstract allocator.*/
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
    gp_db_assert(size <= PTRDIFF_MAX, "Possibly negative allocation detected.");
    return allocator->alloc(allocator, size, GP_ALLOC_ALIGNMENT);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_ATTRIB_ALLOC_ALIGN(3)
static inline void* gp_mem_alloc_aligned(
    GPAllocator* allocator,
    size_t size,
    size_t alignment)
{
    gp_db_assert(size <= PTRDIFF_MAX, "Possibly negative allocation detected.");
    gp_db_assert((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2.");
    return allocator->alloc(allocator, size, alignment);
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline void* gp_mem_alloc_zeroes(
    GPAllocator* allocator,
    size_t size)
{
    gp_db_assert(size <= PTRDIFF_MAX, "Possibly negative allocation detected.");
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

/** Reallocate aligned block.
 * Free @p old_block, allocate a new block and copy the memory from
 * @p old_block to the new block.
 * If @p allocator is a builtin arena, the arena extends @p old_block without
 * reallocating if @p old_block is the last object allocated by the arena.
 * @p old_block may be NULL if @p old_size is zero.
 * @return newly allocated memory or @p old_block if no reallocation happened.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
void* gp_mem_realloc_aligned(
    GPAllocator* allocator,
    void* old_block,
    size_t old_size,
    size_t new_size,
    size_t alignment);

/** Reallocate block.
 * Free @p old_block, allocate a new block and copy the memory from
 * @p old_block to the new block.
 * If @p allocator is a builtin arena, the arena extends @p old_block without
 * reallocating if @p old_block is the last object allocated by the arena.
 * @p old_block may be NULL if @p old_size is zero.
 * @return newly allocated memory or @p old_block if no reallocation happened.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
static inline void* gp_mem_realloc(
    GPAllocator* allocator,
    void* old_block,
    size_t old_size,
    size_t new_size)
{
    return gp_mem_realloc_aligned(
        allocator, old_block, old_size, new_size, GP_ALLOC_ALIGNMENT);
}

/** Maybe reallocate aligned block.
 * Possibly free @p old_block, allocate a new block and copy the memory from
 * @p old_block to the new block.
 * If @p new_size <= @p old_size, no reallocation happens. Also, if @p allocator
 * is a builtin arena, the arena extends @p old_block without reallocating if
 * @p old_block is the last object allocated by the arena. @p old_block may be
 * NULL if @p old_size is zero.
 * @return newly allocated memory or @p old_block if no reallocation happened.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
static inline void* gp_mem_reserve_aligned(
    GPAllocator* allocator,
    void*  optional_old_block,
    size_t old_size,
    size_t new_size,
    size_t alignment)
{
    if (new_size <= old_size)
        return optional_old_block;
    return gp_mem_realloc_aligned(
        allocator, optional_old_block, old_size, new_size, alignment);
}

/** Maybe reallocate block.
 * Possibly free @p old_block, allocate a new block and copy the memory from
 * @p old_block to the new block.
 * If @p new_size <= @p old_size, no reallocation happens. Also, if @p allocator
 * is a builtin arena, the arena extends @p old_block without reallocating if
 * @p old_block is the last object allocated by the arena. @p old_block may be
 * NULL if @p old_size is zero.
 * @return newly allocated memory or @p old_block if no reallocation happened.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD
static inline void* gp_mem_reserve(
    GPAllocator* allocator,
    void*  optional_old_block,
    size_t old_size,
    size_t new_size)
{
    if (new_size <= old_size)
        return optional_old_block;
    return gp_mem_reserve_aligned(
        allocator, optional_old_block, old_size, new_size, GP_ALLOC_ALIGNMENT);
}

// ----------------------------------------------------------------------------
// Arena Allocator

/** Arena that does not run out of memory.
 * If address sanitizer is used, unused memory, freed memory, and allocation
 * boundaries are poisoned. The allocated memory cannot be assumed to be
 * contiguous due to boundary poisoning and linked list based backing buffers.
 */
typedef struct gp_arena
{
    GPAllocator base;

    /** Determine where arena gets it's memory from.
     * Default is gp_heap. If backing buffer is provided, then this will only
     * determine how additional buffers are allocated.
     */
    GPAllocator* backing;

    /** Determine how new arenas grow.
     * Use this to determine the size of new arena node when old gets full. A
     * value larger than 1.0 is useful for arenas that have small initial size.
     * This allows the arena to estimate an optimal size for itself during
     * runtime. A value smaller than 1.0 is useful for arenas that start out
     * huge to not waste memory.
     */
    double growth_factor;

    /** Limit the arena size.
     * Arenas will not grow past this value. Useful when
     * growth_factor > 1.0.
     */
    size_t max_size; // TODO this should be asserted, not saturated! Saturation is unnecessary due to virtual memory. Assertion allows compile time virtual/generic arena.

    /** @private */
    struct gp_arena_node* head;
} GPArena;

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
     * growth_factor > 1.0.
     */
    size_t max_size;

    /** Determine how new arenas grow.
     * Use this to determine the size of new arena node when old gets full. A
     * value larger than 1.0 is useful for arenas that have small initial size.
     * This allows the arena to estimate an optimal size for itself during
     * runtime. A value smaller than 1.0 is useful for arenas that start out
     * huge to not waste memory.
     */
    double growth_factor;

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
 * @return combined size of all internal buffers. This may be useful for
 * optimizing appropriate size for gp_arena_new().
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

// Feel free to define your own values for these.
#ifndef GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE
#define GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE (8192 - sizeof(GPArena) - 4*sizeof(void*))
#endif
#ifndef GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE
#define GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE SIZE_MAX
#endif
#ifndef GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT
#define GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT 1.0
#endif

// ----------------------------------------------------------------------------
// Heap Allocator

/** malloc() based allocator.*/
extern GPAllocator* gp_heap;

/** Allocation count to help optimizations.*/
GP_NODISCARD
size_t gp_heap_alloc_count(void);

// ----------------------------------------------------------------------------
// Contiguous Arena Allocator

/** Contiguous fast huge arena allocator.
 * Arena that uses contiguous possibly huge memory blocks for it's backing
 * buffer. Allocated memory is contiguous ignoring possible padding bytes to
 * satisfy alignment requirements. The assumption is that the backing buffer is
 * large enough to not run out of memory, thus bounds are only checked if
 * NDEBUG is not defined or GP_VIRTUAL_ALWAYS_BOUNDS_CHECK is defined.
 */
typedef struct gp_contiguous_arena
{
    GPAllocator base;
    void*   position; // arena pointer
    size_t  capacity; // size of memory
    uint8_t memory[];
} GPContiguousArena;

/** Get page size. */
size_t gp_page_size(void);

/** Create contiguous arena.
 * @p capacity will be rounded up to page size  - sizeof(GPContiguousArena). It
 * is recommended to pass huge (at least hundreds of megs depending on your
 * application and underlying system) @p capacity to prevent out-of-memory bugs.
 * Physical memory will only be used on writes to the arena.
 * @return pointer to arena or NULL if virtual memory allocation fails. In case
 * of failures, you may want to try again with smaller capacity.
 */
GPContiguousArena* gp_carena_new(size_t capacity);

/** Deallocate some memory.
 * Use this to free everything allocated after @p to_this_position including
 * @p to_this_position. Physical memory remains untouched.
 */
GP_NONNULL_ARGS()
static inline void gp_carena_rewind(GPContiguousArena* arena, void* to_this_position)
{
    arena->position = to_this_position;
    uint8_t* pointer = (uint8_t*)to_this_position;
    gp_db_assert(pointer < (uint8_t*)arena->memory + arena->capacity, "Pointer points outside the arena.");
    gp_db_assert(pointer >= (uint8_t*)arena->memory, "Pointer points outside the arena.");
}

/** Deallocate all memory excluding the arena itself.
 * Fully rewinds the arena pointer to the beginning of the arena. Physical
 * memory will be deallocated, but virtual address space remains untouched.
 */
void gp_carena_reset(GPContiguousArena*) GP_NONNULL_ARGS();

/** Deallocate all arena memory including the arena itself.*/
void gp_carena_delete(GPContiguousArena* optional);

/** Allocate memory from contiguous arena.
 * gp_mem_alloc() is meant to be polymorphic, use this directly to maximize
 * performance.
 */
GP_NONNULL_ARGS_AND_RETURN GP_ATTRIB_ALLOC_ALIGN(3)
static inline void* gp_carena_alloc(
    GPContiguousArena* allocator, const size_t size, const size_t alignment)
{
    gp_db_assert(size <= PTRDIFF_MAX, "Possibly negative allocation detected.");
    gp_db_assert((alignment & (alignment - 1)) == 0, "Alignment must be a power of 2.");

    GPContiguousArena* arena = (GPContiguousArena*)allocator;
    void* block = arena->position;
    arena->position = (void*)(gp_round_to_aligned((uintptr_t)arena->position, alignment) + size);

    #if !defined(NDEBUG) || /*user*/defined(GP_VIRTUAL_ALWAYS_BOUNDS_CHECK)
    gp_assert((uint8_t*)arena->position <= (uint8_t*)arena->memory + arena->capacity, "Virtual allocator out of memory.");
    #endif
    return block;
}

// ----------------------------------------------------------------------------
// Scope Allocator

// TODO better docs

typedef struct gp_scope
{
    GPAllocator base;

    /** @private */
    struct gp_arena_node* head;
    /** @private */
    struct gp_scope* parent;
    /** @private */
    struct gp_defer_stack* defer_stack;
} GPScope;

/** Create scope arena.*/
GPScope* gp_begin(size_t size) GP_NONNULL_RETURN GP_NODISCARD;

/** Free scope arena.
 * Also frees any inner scopes in the current thread that have not been ended.
 * Calls deferred functions.
 * @return combined size of all internal buffers. This may be useful to
 * determine appropriate size for gp_begin().
 */
size_t gp_end(GPScope* optional_scope);

/** Free scope arena.
 * Like gp_end(), but appropriate return type for destructors or deferring.
 */
static inline void gp_end_scope(GPScope* optional_scope)
{
    gp_end(optional_scope);
}

/** Set cleanup routines to be executed on gp_end().
 * Deferred functions are called in Last In First Out order in gp_end().
 * Deferring should not be used for gp_str_delete() or gp_arr_delete() due
 * to possibility of reallocating which would cause double free. It is not
 * needed either, since using the scope allocator makes freeing redundant.
 * Deferring is meant to clean other than memory resources like file pointers.
 */
GP_NONNULL_ARGS(1, 2)
void gp_scope_defer(GPScope* scope, void (*f)(void* arg), void* arg);

/** Set cleanup routines to be executed on gp_end().
 * like gp_scope_defer() but with type checking and can also take functions
 * with non-void pointer arguments like gp_file_close().
 */
#define gp_defer(scope, f, arg) do { \
    if (0) (f)(arg);\
    gp_scope_defer(scope, gp_defer_func_cast(f), arg); \
} while(0)

/** Get lastly created scope in the current thread.
 * You should prefer to just pass scopes as arguments when possible. This exists
 * only to be able to access the current scope allocator in callbacks.
 */
GP_NODISCARD
GPScope* gp_last_scope(void);

// ----------------------------------------------------------------------------
// Deferring

// TODO docs!

#define GP_BEGIN(...) { GP_DEFER_BEGIN(__VA_ARGS__)
#define GP_END          GP_DEFER_END }

#define GP_AUTO_MEM    ( gp_auto_mem_clean, _gp_auto_mem, \
    static GP_AUTO_MEM_THREAD size_t _gp_auto_mem_size; \
    GPAutoMem* _gp_auto_mem = gp_defer_new(GPAutoMem, gp_begin(_gp_auto_mem_size), &_gp_auto_mem_size); \
    GPScope* scope = _gp_auto_mem->scope; )

#define gp_defer_alloc(/* size_t n_bytes */...)             GP_DEFER_ALLOC(__VA_ARGS__)
#define gp_defer_new(/* T type, optional_init_values */...) GP_DEFER_NEW(__VA_ARGS__)

// ----------------------------------------------------------------------------
// Mutex Allocator

/** Shared allocator wrapper.
 * Uses backing allocator to do allocations and deallocations, which are mutex
 * protected. This can be used to make any allocator thread safe.
 */
typedef struct gp_mutex_allocator
{
    GPAllocator  base;
    GPAllocator* backing;
    GPMutex      mutex;
} GPMutexAllocator;

/** Initialize mutex allocator.
 * @return pointer to allocator casted to GPAllocator* or NULL if mutex creation
 * fails.
 */
GP_NONNULL_ARGS()
GPAllocator* gp_mutex_allocator_init(
    GPMutexAllocator*,
    GPAllocator* backing_allocator);

/** Destroy mutex allocator mutex.*/
void gp_mutex_allocator_destroy(GPMutexAllocator* optional);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


static inline void gp_allocator_type_check(GPAllocator*_) { (void)_; }

#if !__cplusplus && __STDC_VERSION__ < 202311L
// Unlike cast to void(*)(void*), this checks that return type matches.
static inline void(*gp_defer_func_cast(void(*f)(/* accept any arg type */)))(void*)
{
    return (void(*)(void*))f;
}
#else // no return type checking available
#define gp_defer_func_cast(...) ((void(*)(void*))(__VA_ARGS__))
#endif

// ----------------------------------------------------------------------------
// GP_DEFER_BEGIN, GP_DEFER_END

// Note: (void)_gp_auto_scope_defers is used for compiler errors when using
// macros that are only supposed to be used in auto scopes.

typedef struct gp_auto_mem
{
    GPScope* scope;
    size_t*  size;
} GPAutoMem;

static inline void gp_auto_mem_clean(void*_arena)
{
    GPAutoMem* arena = (GPAutoMem*)_arena;
    *arena->size = (gp_end(arena->scope) >> 1) + (*arena->size >> 1);
}
#define GP_DEFER_DECLARATION(DESTRUCTOR, DESTRUCTOR_ARGUMENT,/* declarations */...) \
    __VA_ARGS__; \
    if (0) (DESTRUCTOR)(DESTRUCTOR_ARGUMENT); \
    _gp_auto_scope_defers[_gp_auto_scope_defers_length].func = gp_defer_func_cast(DESTRUCTOR); \
    _gp_auto_scope_defers[_gp_auto_scope_defers_length].arg = (DESTRUCTOR_ARGUMENT); \
    ++_gp_auto_scope_defers_length;

#define GP_DEFER_DECLARE(...) GP_DEFER_DECLARATION __VA_ARGS__

typedef struct gp_defer
{
    void (*func)(void* arg);
    void* arg;
} GPDefer;

#define GP_DEFER_NEW(...) GP_OVERLOAD64(__VA_ARGS__, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, GP_DEFER_NEW_INIT, \
    GP_DEFER_NEW_ZERO_INIT)(__VA_ARGS__)

typedef struct gp_auto_scope99
{
    GPContiguousArena* arena; // for gp_defer_alloc()
    size_t defers_length;
    GPDefer* defers;
} GPAutoScope99;

#if __GNUC__ && !defined(__MINGW32__) // Note: __thread is broken in MinGW // TODO just conditionally define GP_AUTO_DEFER_THREAD to __thread or _Atomic

#define GP_AUTO_MEM_THREAD __thread

#define GP_DEFER_ALLOC(...) __builtin_alloca((void)_gp_auto_scope_defers, (__VA_ARGS__))

// Note: alloca.h not available in many platforms, so use builtin.
#ifndef __cplusplus
#define GP_DEFER_NEW_ZERO_INIT(T) ((void)_gp_auto_scope_defers, &(T){0})
#define GP_DEFER_NEW_INIT(T, ...) ((void)_gp_auto_scope_defers, &(T){__VA_ARGS__})
#endif

#define GP_ALLOCA(...) __builtin_alloca(__VA_ARGS__)

typedef const struct gp_auto_scope
{
    GPDefer* defers;
    size_t defers_length;
} GPAutoScope;

__attribute__((always_inline))
static inline void gp_auto_scope_clean(const GPAutoScope* scope)
{
    for (size_t i = scope->defers_length - 1; i != (size_t)-1; --i)
        scope->defers[i].func(scope->defers[i].arg);
}
#define GP_DEFER_BEGIN(...) \
    GPDefer _gp_auto_scope_defers[GP_COUNT_ARGS(__VA_ARGS__)]; \
    size_t _gp_auto_scope_defers_length = 0; \
    GP_PROCESS_ALL_ARGS(GP_DEFER_DECLARE, GP_SEMICOLON, __VA_ARGS__); \
    __attribute__((cleanup(gp_auto_scope_clean))) GPAutoScope _gp_auto_scope = { \
        _gp_auto_scope_defers, \
        GP_COUNT_ARGS(__VA_ARGS__) \
    };
    // user code
#define GP_DEFER_END // cleanup attribute handles cleanup, nothing to do here

#elif _MSC_VER

#include <malloc.h>
// _alloca() is deprecated, but we have no way of free() the returned pointer,
// so cannot use _malloca() either. We expect Microsoft keeping _alloca() around
// for a long time due to backwards compatibility. Even they use it themselves
// to implement _malloca().
#define GP_DEFER_ALLOC(...) _alloca((void)_gp_auto_scope_defers, (__VA_ARGS__))
#define GP_ALLOCA(...) _alloca(__VA_ARGS__)

#define GP_AUTO_MEM_THREAD __declspec(thread)

#ifndef __cplusplus
#define GP_DEFER_NEW_ZERO_INIT(T) ((void)_gp_auto_scope_defers, &(T){0})
#define GP_DEFER_NEW_INIT(T, ...) ((void)_gp_auto_scope_defers, &(T){__VA_ARGS__})
#endif

#define GP_DEFER_BEGIN(...) \
    GPDefer _gp_auto_scope_defers[GP_COUNT_ARGS(__VA_ARGS__)]; \
    size_t _gp_auto_scope_defers_length = 0; \
    GP_PROCESS_ALL_ARGS(GP_DEFER_DECLARE, GP_SEMICOLON, __VA_ARGS__); \
    __try {
        //user code
#define GP_DEFER_END } __finally { \
    for (size_t i = _gp_auto_scope_defers_length - 1; i != (size_t)-1; --i) \
        _gp_auto_scope_defers[i].func(_gp_auto_scope_defers[i].arg); \
    }

#else

// Note: size estimation is not thread local due to not being able to have
// globals in these macros, which is required by pthread thread locals. It also
// saves some performance. Reads and writes to uint32_t assumed atomic. IIR
// smoothing expression is not atomic, but has negligible effect on program
// correctness.

#define GP_AUTO_MEM_THREAD

#ifndef __cplusplus
#define GP_DEFER_NEW_ALLOC(T) (T*)gp_carena_alloc(_gp_auto_scope->arena, sizeof(T), GP_ALLOC_ALIGNMENT) // TODO use GP_PTR_TO()
#define GP_DEFER_NEW_ZERO_INIT(T) memset(GP_DEFER_NEW_ALLOC(T), 0, sizeof(T))
#define GP_DEFER_NEW_INIT(T, ...) memcpy(GP_DEFER_NEW_ALLOC(T), &(T){__VA_ARGS__}, sizeof (T){__VA_ARGS__})
#define GP_DEFER_ALLOC(...) gp_carena_alloc(&_gp_auto_scope.arena, __VA_ARGS__, GP_ALLOC_ALIGNMENT)
#endif

GPAutoScope99* gp_thread_local_auto_scope(void);
#define GP_DEFER_BEGIN(...) \
    GPAutoScope99* _gp_auto_scope = gp_thread_local_auto_scope(); \
    void* _gp_auto_scope_arena_position = _gp_auto_scope->arena->position; \
    size_t _gp_auto_scope_defers_old_length = _gp_auto_scope->defers_length; \
    GPDefer* _gp_auto_scope_defers = _gp_auto_scope->defers + _gp_auto_scope->defers_length; \
    _gp_auto_scope->defers_length += GP_COUNT_ARGS(__VA_ARGS__); \
    size_t _gp_auto_scope_defers_length = 0; \
    GP_PROCESS_ALL_ARGS(GP_DEFER_DECLARE, GP_SEMICOLON, __VA_ARGS__);
    // user code
#define GP_DEFER_END \
    _gp_auto_scope->arena->position = _gp_auto_scope_arena_position; \
    for ( ; _gp_auto_scope->defers_length > _gp_auto_scope_defers_old_length; --_gp_auto_scope->defers_length) \
        _gp_auto_scope->defers[_gp_auto_scope->defers_length-1] \
            .func(_gp_auto_scope->defers[_gp_auto_scope->defers_length-1].arg);

#endif // GP_DEFER_BEGIN, and GP_DEFER_END
// ----------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"

// Note: we could use RAII for GP_BEGIN(), but if GP_ALLOCA() is not available,
// then we need a thread local GPVirtualArena anyway for gp_scope_alloc(), so
// we'll settle to C99 implementation for now. We probably should later use RAII
// for performance though.

template <typename T>
#if __GNUC__ // allow using alloca() in function arguments. Note: on MSVC it's okay anyway.
__attribute__((always_inline))
#endif
static inline T* gp_cpp_ptr_init(T* ptr, const T& value)
{
    *ptr = value;
    return ptr;
}

#define GP_DEFER_NEW_ALLOC(T) ((T*)GP_DEFER_ALLOC(sizeof(T))) // TODO T!
#define GP_DEFER_NEW_ZERO_INIT(T) gp_cpp_ptr_init(GP_DEFER_NEW_ALLOC(T), T{0})
#define GP_DEFER_NEW_INIT(T, ...) gp_cpp_ptr_init(GP_DEFER_NEW_ALLOC(T), T{__VA_ARGS__})

#endif

#endif // GP_MEMORY_INCLUDED
