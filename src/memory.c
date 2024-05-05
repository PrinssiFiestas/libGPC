// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#include <threads.h>
// #else
#include <pthread.h>
// #endif

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

//
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

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    if (new_pos == NULL) { // clear arena
        while (arena->head->tail != NULL) {
            GPArenaNode* old_head = arena->head;
            arena->head = arena->head->tail;
            gp_mem_dealloc(&gp_heap, old_head);
        }
        arena->head->position = arena->head + 1;
        return;
    }

    while ( ! gp_in_this_node(arena->head, new_pos)) {
        GPArenaNode* old_head = arena->head;
        arena->head = arena->head->tail;
        gp_mem_dealloc(&gp_heap, old_head);
    }
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
    *arena = (GPArena){0};
}

// ----------------------------------------------------------------------------
// Scope allocator

static GP_THREAD_LOCAL GPArena gp_scope_factory = {0};
static pthread_key_t  gp_scope_factory_key;
static pthread_once_t gp_scope_factory_key_once = PTHREAD_ONCE_INIT;
static void gp_arena_delete_void(void* arena) { gp_arena_delete(arena); }
static void gp_make_scope_factory_key(void) {
    pthread_key_create(&gp_scope_factory_key, gp_arena_delete_void);
}

// TODO make these sig_atomic_t
static GP_THREAD_LOCAL uint64_t gp_total_scope_sizes =  0 ;
static GP_THREAD_LOCAL size_t   gp_total_scope_count =  0 ;

static void* gp_scope_alloc(const GPAllocator* scope, size_t size)
{
    gp_total_scope_sizes += size;
    return gp_arena_alloc(scope, size);
}

#ifdef __GNUC__
#define GP_UNLIKELY(COND) __builtin_expect(!!(COND), 0)
#else
#define GP_UNLIKELY(COND) (COND)
#endif

#ifndef GP_MAX_SCOPE_DEPTH // exceeding this will cause a memory leak
#define GP_MAX_SCOPE_DEPTH 1024 // * sizeof(GPArena) == 32 KB per thread
#endif

GPAllocator* gp_begin(const size_t _size)
{
    // if (GP_UNLIKELY(gp_scope_factory.head == NULL))
    //    gp_scope_factory = gp_arena_new(GP_MAX_SCOPE_DEPTH * sizeof(GPArena), 1.0);
    pthread_once(&gp_scope_factory_key_once, gp_make_scope_factory_key);
    GPArena* scope_factory = pthread_getspecific(gp_scope_factory_key);
    if (GP_UNLIKELY(scope_factory == NULL)) {
        gp_scope_factory = gp_arena_new(GP_MAX_SCOPE_DEPTH * sizeof(GPArena), 1.);
        pthread_setspecific(gp_scope_factory_key, &gp_scope_factory);
    }
    gp_total_scope_count++;
    const size_t average_scope_size = gp_total_scope_sizes/gp_total_scope_count;
    const size_t size = _size == 0 ?
        gp_max(2 * average_scope_size, (size_t)1024) : _size;

    GPArena* scope = gp_mem_alloc((GPAllocator*)&gp_scope_factory, sizeof*scope);
    *scope = gp_arena_new(size, 1.0);
    scope->allocator.alloc = gp_scope_alloc;
    return (GPAllocator*)scope;
}

static sig_atomic_t gp_max_scope_depth = 0;

void gp_end(GPAllocator* scope)
{
    const size_t depth =
        ((uintptr_t)(gp_scope_factory.head->position) -
         (uintptr_t)(gp_scope_factory.head + 1      )  ) / sizeof(GPArena);
    gp_max_scope_depth = gp_max((size_t)gp_max_scope_depth, depth);

    for (GPArena* unallocd_scope = (GPArena*)scope;
        unallocd_scope < (GPArena*)(gp_scope_factory.head->position); unallocd_scope++) {
        gp_arena_delete(unallocd_scope);
    }
    gp_arena_rewind(&gp_scope_factory, scope);
}

size_t gp_get_max_scope_depth(void)
{
    return gp_max_scope_depth;
}
