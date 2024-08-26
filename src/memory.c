// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <gpc/utils.h>
#include "common.h"
#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef GP_TESTS
#include <gpc/assert.h>
#endif

extern inline void* gp_mem_alloc       (const GPAllocator*,size_t);
extern inline void* gp_mem_alloc_zeroes(const GPAllocator*,size_t);
extern inline void  gp_mem_dealloc     (const GPAllocator*,void*);

static void* gp_heap_alloc(const GPAllocator* unused, size_t block_size)
{
    (void)unused;
    void* mem = malloc(block_size);
    if (mem == NULL) {
        GP_BREAKPOINT;
        perror("malloc() failed");
        abort();
    }
    return mem;
}

static void gp_heap_dealloc(const GPAllocator* unused, void* block)
{
    (void)unused;
    free(block);
}

static const GPAllocator gp_mallocator = {
    .alloc   = gp_heap_alloc,
    .dealloc = gp_heap_dealloc
};
#ifdef NDEBUG
const GPAllocator*const gp_heap = &gp_mallocator;
#else
const GPAllocator*      gp_heap = &gp_mallocator;
#endif

// ----------------------------------------------------------------------------

// Instances of these live in the beginning of the arenas memory block so the
// first object is in &node + 1;
typedef struct gp_arena_node
{
    void* position;
    struct gp_arena_node* tail;
    size_t capacity;
    void* _padding; // to round size to aligment boundary and for future use
} GPArenaNode;

static void* gp_arena_alloc(const GPAllocator* allocator, const size_t _size)
{
    GPArena* arena = (GPArena*)allocator;
    const size_t size = gp_round_to_aligned(_size, arena->alignment);
    GPArenaNode* head = arena->head;

    void* block = head->position;
    if ((uint8_t*)block + size > (uint8_t*)(head + 1) + arena->head->capacity)
    { // out of memory, create new arena
        const size_t new_cap = gp_round_to_aligned(
            arena->growth_coefficient * arena->head->capacity, arena->alignment);
        GPArenaNode* new_node = gp_mem_alloc(gp_heap,
            sizeof(GPArenaNode) + gp_max(new_cap, size));
        new_node->tail     = head;
        new_node->capacity = new_cap;

        block = new_node->position = new_node + 1;
        new_node->position = (uint8_t*)(new_node->position) + size;
        arena->head = new_node;
    }
    else
    {
        head->position = (uint8_t*)block + size;
    }
    return block;
}

static void* gp_arena_shared_alloc(const GPAllocator* allocator, const size_t size)
{
    gp_mutex_lock((GPMutex*)((GPArena*)allocator + 1));
    void* block = gp_arena_alloc(allocator, size);
    gp_mutex_unlock((GPMutex*)((GPArena*)allocator + 1));
    return block;
}

GPArena gp_arena_new(const size_t capacity)
{
    const size_t cap  = capacity != 0 ?
        gp_round_to_aligned(capacity, GP_ALLOC_ALIGNMENT)
      : 256;
    GPArenaNode* node = gp_mem_alloc(gp_heap, sizeof(GPArenaNode) + cap);
    node->position = node + 1;
    node->tail     = NULL;
    node->capacity = cap;
    return (GPArena) {
        .allocator          = { gp_arena_alloc, gp_arena_dealloc },
        .growth_coefficient = 2.,
        .max_size           = 1 << 15,
        .alignment          = GP_ALLOC_ALIGNMENT,
        .head               = node,
    };
}

GPArena* gp_arena_new_shared(const size_t capacity)
{
    GPArena arena_data = gp_arena_new(capacity + sizeof arena_data + sizeof(GPMutex));
    GPArena*arena = gp_mem_alloc(gp_heap, sizeof*arena + sizeof(GPMutex));
    arena_data.allocator.alloc = gp_arena_shared_alloc;
    *arena = arena_data;
    GPMutex* mutex = (GPMutex*)(arena + 1);
    gp_mutex_init(mutex);
    return arena;
}

static bool gp_in_this_node(GPArenaNode* node, void* _pos)
{
    uint8_t* pos = _pos;
    uint8_t* block_start = (uint8_t*)(node + 1);
    return block_start <= pos && pos <= block_start + node->capacity;
}

static void gp_arena_node_delete(GPArena* arena)
{
    GPArenaNode* old_head = arena->head;
    arena->head = arena->head->tail;
    gp_mem_dealloc(gp_heap, old_head);
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    while ( ! gp_in_this_node(arena->head, new_pos))
        gp_arena_node_delete(arena);
    arena->head->position = new_pos;
}

// With -03 GCC inlined bunch of functions and ignored the last if statement in
// gp_arena_delete() giving a false positive for -Wfree-nonheap-object and
// refusing to compile with -Werror so this pointless wrapper is needed.
#if __GNUC__ && ! __clang__
__attribute__((noinline))
#endif
static void gp_arena_shared_heap_dealloc(GPArena* arena)
{
    gp_mem_dealloc(gp_heap, arena);
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;
    while (arena->head != NULL) {
        GPArenaNode* old_head = arena->head;
        arena->head = arena->head->tail;
        gp_mem_dealloc(gp_heap, old_head);
    }
    if (arena->allocator.alloc == gp_arena_shared_alloc)
        gp_arena_shared_heap_dealloc(arena);
}

// ----------------------------------------------------------------------------
// Scratch arena

static GPThreadKey  gp_scratch_arena_key;
static GPThreadOnce gp_scratch_arena_key_once = GP_THREAD_ONCE_INIT;
#ifndef GP_NO_THREAD_LOCALS // Avoid unnecessary heap allocation
static GP_MAYBE_THREAD_LOCAL GPArena gp_scratch_allocator = {0};
#endif

static void gp_delete_scratch_arena(void* arena)
{
    gp_arena_delete(arena);
    #ifdef GP_NO_THREAD_LOCALS
    gp_mem_dealloc(gp_heap, arena);
    #endif
}

// Make Valgrind shut up.
static void gp_delete_main_thread_scratch_arena(void)
{
    GPArena* arena = gp_thread_local_get(gp_scratch_arena_key);
    if (arena != NULL)
        gp_delete_scratch_arena(arena);
}

static void gp_make_scratch_arena_key(void)
{
    atexit(gp_delete_main_thread_scratch_arena);
    gp_thread_key_create(&gp_scratch_arena_key, gp_delete_scratch_arena);
}

static GPArena* gp_new_scratch_arena(void)
{
    #ifdef GP_NO_THREAD_LOCALS
    GPArena _arena = gp_arena_new(GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE);
    GPArena* arena = gp_mem_alloc(gp_heap, sizeof*arena);
    *arena = _arena;
    #else
    gp_scratch_allocator = gp_arena_new(GP_SCOPE_DEFAULT_INIT_SIZE);
    GPArena* arena       = &gp_scratch_allocator;
    #endif
    arena->max_size           = GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE;
    arena->growth_coefficient = GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT;
    gp_thread_local_set(gp_scratch_arena_key, arena);
    return arena;
}

GPArena* gp_scratch_arena(void)
{
    gp_thread_once(&gp_scratch_arena_key_once, gp_make_scratch_arena_key);

    GPArena* arena = gp_thread_local_get(gp_scratch_arena_key);
    if (GP_UNLIKELY(arena == NULL))
        arena = gp_new_scratch_arena();
    return arena;
}

// ----------------------------------------------------------------------------

void* gp_mem_realloc(
    const GPAllocator* allocator,
    void* old_block,
    size_t old_size,
    size_t new_size)
{
    GPArena* arena = (GPArena*)allocator;
    if (allocator->dealloc == gp_arena_dealloc && old_block != NULL &&
        (char*)old_block + gp_round_to_aligned(old_size, arena->alignment)
          == (char*)arena->head->position)
    { // extend block instead of reallocating and copying
        arena->head->position = old_block;
        void* new_block = gp_arena_alloc(allocator, new_size);
        if (new_block != old_block) // arena ran out of space and reallocated
            memcpy(new_block, old_block, old_size);
        return new_block;
    }
    void* new_block = gp_mem_alloc(allocator, new_size);
    if (old_block != NULL)
        memcpy(new_block, old_block, old_size);
    gp_mem_dealloc(allocator, old_block);
    return new_block;
}

// ----------------------------------------------------------------------------
// Scope allocator

typedef struct gp_defer
{
    void (*f)(void* arg);
    void* arg;
} GPDefer;

typedef struct gp_defer_stack
{
    GPDefer* stack;
    uint32_t length;
    uint32_t capacity;
} GPDeferStack;

typedef struct gp_scope
{
    GPArena          arena;
    struct gp_scope* parent;
    GPDeferStack*    defer_stack;
} GPScope;

static GPThreadKey  gp_scope_factory_key;
static GPThreadOnce gp_scope_factory_key_once = GP_THREAD_ONCE_INIT;

GP_NO_FUNCTION_POINTER_SANITIZE
static void gp_end_scopes(GPScope* scope, GPScope*const last_to_be_ended)
{
    if (scope->defer_stack != NULL) {
        for (size_t i = scope->defer_stack->length - 1; i != (size_t)-1; i--) {
            scope->defer_stack->stack[i].f(scope->defer_stack->stack[i].arg);
        }
    }
    GPScope* previous = scope->parent;
    gp_arena_delete((GPArena*)scope);
    if (previous != NULL && scope != last_to_be_ended)
        gp_end_scopes(previous, last_to_be_ended);
}

// scope_factory lives in it's own arena so returns &scope_factory if there is
// no scopes.
static GPScope* gp_last_scope_of(GPArena* scope_factory)
{
    return (GPScope*) ((uint8_t*)(scope_factory->head->position) -
       gp_round_to_aligned(sizeof(GPScope), GP_ALLOC_ALIGNMENT));
}

GPAllocator* gp_last_scope(const GPAllocator* fallback)
{
    GPArena* factory = gp_thread_local_get(gp_scope_factory_key);
    GPScope* scope = NULL;
    if (factory == NULL || (scope = gp_last_scope_of(factory)) == (GPScope*)factory)
        return (GPAllocator*)fallback;
    return (GPAllocator*)scope;
}

static void gp_delete_scope_factory(void*_factory)
{
    GPArena* factory = _factory;
    GPScope* remaining = gp_last_scope_of(factory);
    if (remaining != (GPScope*)factory)
        gp_end_scopes(remaining, NULL);

    gp_mem_dealloc(gp_heap, factory->head);
}

// Make Valgrind shut up.
static void gp_delete_main_thread_scope_factory(void)
{
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    if (scope_factory != NULL)
        gp_delete_scope_factory(scope_factory);
}
static void gp_make_scope_factory_key(void)
{
    atexit(gp_delete_main_thread_scope_factory);
    gp_thread_key_create(&gp_scope_factory_key, gp_delete_scope_factory);
}

#if __STDC_VERSION__ >= 201112L  && \
    !defined(__STDC_NO_ATOMICS__) && \
    ATOMIC_LLONG_LOCK_FREE == 2 // always lock-free
// Keeping track of average scope size allows scope allocator to estimate
// optimal scope arena size when creating scopes.
static _Atomic uint64_t gp_total_scope_sizes = 0;
static _Atomic size_t   gp_total_scope_count = 0;
#define GP_ATOMIC_OP(OP) OP
#else
#define GP_ATOMIC_OP(OP)
#endif

static size_t gp_scope_average_memory_usage(void)
{
    return GP_ATOMIC_OP(gp_total_scope_sizes/gp_total_scope_count) - 0;
}

static void* gp_scope_alloc(const GPAllocator* scope, size_t _size)
{
    const size_t size = gp_round_to_aligned(_size, ((GPScope*)scope)->arena.alignment);
    GP_ATOMIC_OP(gp_total_scope_sizes += size);
    return gp_arena_alloc(scope, size);
}

GPArena* gp_new_scope_factory(void)
{
    const size_t nested_scopes = 64; // before reallocation
    GPArena scope_factory_arena = gp_arena_new(
        (nested_scopes + 1/*self*/) * gp_round_to_aligned(sizeof(GPScope), GP_ALLOC_ALIGNMENT));

    // Extend lifetime
    GPArena* scope_factory = gp_arena_alloc(
        (GPAllocator*)&scope_factory_arena,
        sizeof(GPScope)); // gets rounded in gp_arena_alloc()
    memset(scope_factory, 0, sizeof*scope_factory);
    memcpy(scope_factory, &scope_factory_arena, sizeof*scope_factory);

    gp_thread_local_set(gp_scope_factory_key, scope_factory);
    return scope_factory;
}

GPAllocator* gp_begin(const size_t _size)
{
    gp_thread_once(&gp_scope_factory_key_once, gp_make_scope_factory_key);

    // scope_factory should only allocate gp_round_to_aligned(sizeof(GPScope), GP_ALLOC_ALIGNMENT)
    // sized objects for consistent pointer arithmetic.
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    if (GP_UNLIKELY(scope_factory == NULL)) // initialize scope factory
        scope_factory = gp_new_scope_factory();

    GP_ATOMIC_OP(gp_total_scope_count++);
    const size_t size = _size == 0 ?
        gp_max(2 * gp_scope_average_memory_usage(), (size_t)GP_SCOPE_DEFAULT_INIT_SIZE)
      : _size;

    GPScope* previous = gp_last_scope_of(scope_factory);
    if (previous == (GPScope*)scope_factory)
        previous = NULL;

    GPScope* scope = gp_arena_alloc((GPAllocator*)scope_factory, sizeof*scope);
    *(GPArena*)scope = gp_arena_new(size);
    scope->arena.allocator.alloc    = gp_scope_alloc;
    scope->arena.max_size           = GP_SCOPE_DEFAULT_MAX_SIZE;
    scope->arena.growth_coefficient = GP_SCOPE_DEFAULT_GROWTH_COEFFICIENT;
    scope->parent = previous;
    scope->defer_stack = NULL;

    return (GPAllocator*)scope;
}

void gp_end(GPAllocator*_scope)
{
    if (_scope == NULL)
        return;
    GPScope* scope = (GPScope*)_scope;
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    gp_end_scopes(gp_last_scope_of(scope_factory), scope);
    gp_arena_rewind(scope_factory, scope);
}

void gp_scope_defer(GPAllocator*_scope, void (*f)(void*), void* arg)
{
    GPScope* scope = (GPScope*)_scope;
    if (scope->defer_stack == NULL)
    {
        const size_t init_cap = 4;
        scope->defer_stack = gp_arena_alloc((GPAllocator*)scope,
            sizeof*(scope->defer_stack) + init_cap * sizeof(GPDefer));

        scope->defer_stack->length   = 0;
        scope->defer_stack->capacity = init_cap;
        scope->defer_stack->stack    = (GPDefer*)(scope->defer_stack + 1);
    }
    else if (scope->defer_stack->length == scope->defer_stack->capacity)
    {
        GPDefer* old_stack  = scope->defer_stack->stack;
        scope->defer_stack->stack = gp_arena_alloc((GPAllocator*)scope,
            scope->defer_stack->capacity * 2 * sizeof(GPDefer));
        memcpy(scope->defer_stack->stack, old_stack,
            scope->defer_stack->length * sizeof(GPDefer));
        scope->defer_stack->capacity *= 2;
    }
    scope->defer_stack->stack[scope->defer_stack->length].f   = f;
    scope->defer_stack->stack[scope->defer_stack->length].arg = arg;
    scope->defer_stack->length++;
}

