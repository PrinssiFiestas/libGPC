// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <gpc/utils.h>
#include "thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef GP_TESTS
#include <gpc/assert.h>
#endif

extern inline void* gp_mem_alloc       (const GPAllocator*,size_t);
extern inline void* gp_mem_alloc_zeroes(const GPAllocator*,size_t);
extern inline void* gp_mem_dealloc     (const GPAllocator*,void*);
extern inline void* gp_mem_realloc     (const GPAllocator*,void*,size_t,size_t);

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

static void* gp_heap_dealloc(const GPAllocator* unused, void* block)
{
    (void)unused;
    free(block);
    return NULL;
}

#ifdef NDEBUG
const
#endif
GPAllocator gp_heap = {
    .alloc   = gp_heap_alloc,
    .dealloc = gp_heap_dealloc
};

#ifdef __GNUC__
__attribute__((always_inline))
#endif
static inline void* gp_crashing_alloc(const GPAllocator* unused, size_t unused_size)
{
    (void)unused; (void)unused_size;
    GP_BREAKPOINT;
    abort();
    return "";
}

static void* gp_no_op_dealloc(const GPAllocator* unused, void* block)
{
    (void)unused;
    return block;
}

const GPAllocator gp_crash_on_alloc = {
    .alloc   = gp_crashing_alloc,
    .dealloc = gp_no_op_dealloc
};

// ----------------------------------------------------------------------------

// Instances of these live in the beginning of the arenas memory block so the
// first object is in &node + 1;
typedef struct gp_arena_node
{
    size_t capacity;
    void* position;
    struct gp_arena_node* tail;
} GPArenaNode;

static void* gp_arena_alloc(const GPAllocator* allocator, const size_t _size)
{
    GPArena* arena = (GPArena*)allocator;
    const size_t size = gp_round_to_aligned(_size);
    GPArenaNode* head = arena->head;

    void* block = head->position;
    if ((uint8_t*)block + size > (uint8_t*)(head + 1) + head->capacity)
    { // out of memory, create new arena
        const size_t new_cap = arena->growth_coefficient * head->capacity;
        GPArenaNode* new_node = gp_mem_alloc(&gp_heap,
            sizeof(GPArenaNode) + gp_max(new_cap, size));
        new_node->capacity = new_cap;
        new_node->tail     = head;

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

GPArena gp_arena_new(const size_t capacity, const double growth_coefficient)
{
    const size_t cap = gp_round_to_aligned(capacity);
    GPArenaNode* node = gp_mem_alloc(&gp_heap, sizeof(GPArenaNode) + cap);
    node->capacity = cap;
    node->position = node + 1;
    node->tail     = NULL;
    GPArena arena = {
        { gp_arena_alloc, gp_no_op_dealloc },
        growth_coefficient,
        node
    };
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
    gp_mem_dealloc(&gp_heap, old_head);
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    if (new_pos == NULL) { // clear arena
        while (arena->head->tail != NULL)
            gp_arena_node_delete(arena);
        arena->head->position = arena->head + 1;
        return;
    }
    while ( ! gp_in_this_node(arena->head, new_pos))
        gp_arena_node_delete(arena);
    arena->head->position = new_pos;
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;
    while (arena->head != NULL) {
        GPArenaNode* old_head = arena->head;
        arena->head = arena->head->tail;
        gp_mem_dealloc(&gp_heap, old_head);
    }
}

// ----------------------------------------------------------------------------
// Scope allocator

typedef struct gp_defer_object
{
    void (*f)(void*);
    void* arg;
} GPDeferObject;

typedef struct gp_defer_stack
{
    GPDeferObject* stack;
    uint32_t length;
    uint32_t capacity;
} GPDeferStack;

typedef struct gp_scope
{
    GPArena arena;
    struct gp_scope* next; // nested scopes
    GPDeferStack* defer_stack;
} GPScope;

static GPThreadKey  gp_scope_factory_key;
static GPThreadOnce gp_scope_factory_key_once = GP_THREAD_ONCE_INIT;

static void gp_delete_scope_factory(void*_factory)
{
    GPArena* factory = _factory;
    GPScope* first_scope = (GPScope*)factory + 1;
    if (first_scope != (GPScope*)factory->head->position) {
        for (GPScope* s = first_scope; s != NULL; s = s->next) {
            gp_arena_delete((GPArena*)s);
        }
    }
    gp_mem_dealloc(&gp_heap, factory->head);
}
static void gp_make_scope_factory_key(void)
{
    gp_thread_key_create(&gp_scope_factory_key, gp_delete_scope_factory);
}

static sig_atomic_t gp_total_scope_sizes = 0;
static sig_atomic_t gp_total_scope_count = 0;
static sig_atomic_t gp_max_scope_depth   = 0;

static void* gp_scope_alloc(const GPAllocator* scope, size_t size)
{
    if ((uint64_t)gp_total_scope_sizes + size <= (uint64_t)SIG_ATOMIC_MAX)
        gp_total_scope_sizes += size;
    else
        gp_total_scope_sizes = SIG_ATOMIC_MAX;

    return gp_arena_alloc(scope, size);
}

// Many allocations were optimized to single allocations where a single block
// hold multiple objects and many loops were eliminated by doing some pointer
// arithmetic so there is a lot of incomprehensible pointer arithmetic.
// TODO: Document this insanity.

#ifdef __GNUC__
#define GP_UNLIKELY(COND) __builtin_expect(!!(COND), 0)
#else
#define GP_UNLIKELY(COND) (COND)
#endif

GPAllocator* gp_begin(const size_t _size)
{
    gp_thread_once(&gp_scope_factory_key_once, gp_make_scope_factory_key);

    // scope_factory should only allocate GPScope sized objects!
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    if (GP_UNLIKELY(scope_factory == NULL)) // initialize scope factory
    {
        GPArena scope_factory_data = gp_arena_new(
            (64/*TODO better default*/+ 1/*self*/) * sizeof(GPScope), 1.);

        // Extend lifetime
        GPArena* scope_factory_mem = gp_arena_alloc(
            (GPAllocator*)&scope_factory_data, sizeof(GPScope));
        *scope_factory_mem = scope_factory_data;

        scope_factory = scope_factory_mem;
        gp_thread_local_set(gp_scope_factory_key, scope_factory);
    }
    if (gp_total_scope_sizes != SIG_ATOMIC_MAX)
        gp_total_scope_count++;
    const size_t average_scope_size = gp_total_scope_sizes/gp_total_scope_count;
    const size_t size = _size == 0 ?
        gp_max(2 * average_scope_size, (size_t)1024) : _size;

    GPScope* previous = (GPScope*) ((uint8_t*)(scope_factory->head->position) -
        gp_round_to_aligned(sizeof(GPScope)));
    if (previous == (GPScope*)scope_factory) // no previous scopes
        previous = NULL;

    GPScope* scope = gp_arena_alloc((GPAllocator*)scope_factory, sizeof*scope);
    *(GPArena*)scope = gp_arena_new(size, 1.0);
    scope->arena.allocator.alloc = gp_scope_alloc;
    scope->next        = NULL;
    scope->defer_stack = NULL;

    if (previous != NULL)
        previous->next = scope;
    return (GPAllocator*)scope;
}

void gp_end(GPAllocator*_scope)
{
    GPScope* scope = (GPScope*)_scope;
    GPArena* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    const size_t depth =
        (((uintptr_t)(scope_factory->head->position) -
          (uintptr_t)(scope_factory->head + 1      ))
         / gp_round_to_aligned(sizeof(GPScope))      ) - 1/*factory*/;
    gp_max_scope_depth = gp_max((size_t)gp_max_scope_depth, depth);

    if (scope->defer_stack != NULL) {
        for (size_t i = scope->defer_stack->length - 1; i != (size_t)-1; i--) {
            scope->defer_stack->stack[i].f(scope->defer_stack->stack[i].arg);
        }
    }
    for (GPScope* s = scope; s != NULL; s = s->next)
        gp_arena_delete((GPArena*)s);
    gp_arena_rewind(scope_factory, scope);
}

void gp_defer(GPAllocator*_scope, void (*f)(void*), void* arg)
{
    GPScope* scope = (GPScope*)_scope;
    if (scope->defer_stack == NULL)
    {
        const size_t init_cap = 8;
        scope->defer_stack = gp_arena_alloc((GPAllocator*)scope,
            sizeof*(scope->defer_stack) + init_cap * sizeof(GPDeferObject));

        scope->defer_stack->length   = 0;
        scope->defer_stack->capacity = init_cap;
        scope->defer_stack->stack    = (GPDeferObject*)(scope->defer_stack + 1);
    }
    else if (scope->defer_stack->length == scope->defer_stack->capacity)
    {
        GPDeferStack* old_stack = scope->defer_stack;
        scope->defer_stack = gp_arena_alloc((GPAllocator*)scope,
            scope->defer_stack->capacity * 2 * sizeof(GPDeferObject));
        memcpy(scope->defer_stack, old_stack, scope->defer_stack->length);
        scope->defer_stack->capacity *= 2;
    }
    scope->defer_stack->stack[scope->defer_stack->length].f   = f;
    scope->defer_stack->stack[scope->defer_stack->length].arg = arg;
    scope->defer_stack->length++;
}

