// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    return block_start <= pos && pos < block_start + node->capacity;
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

    // TODO in debug mode maybe it's a good idea to validate new_pos?

    while ( ! gp_in_this_node(arena->head, new_pos)) {
        GPArenaNode* old_head = arena->head;
        arena->head = arena->head->tail;
        gp_mem_dealloc(&gp_heap, old_head);
    }
    arena->head->position = new_pos;
}

void gp_arena_delete(GPArena* arena)
{
    while (arena->head != NULL) {
        GPArenaNode* old_head = arena->head;
        arena->head = arena->head->tail;
        gp_mem_dealloc(&gp_heap, old_head);
    }
    *arena = (GPArena){0};
}
