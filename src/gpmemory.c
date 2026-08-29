// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpmemory.h>

#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#endif

#if !defined(_WIN32)
#ifndef __USE_MISC // this should not be used, but _GNU_SOURCE had too many
#define __USE_MISC // portability related problems. Used for MAP_ANONYMOUS
#define GP_USE_MISC_DEFINED
#endif
#include <sys/mman.h>
#else
#include <windows.h>
#endif

static void* gp_s_global_heap_alloc(
    struct GPAllocator* unused,
    void*   optional_old_block,
    size_t  optional_old_block_size,
    size_t  block_size,
    size_t  alignment,
    bool    uninitialized,
    size_t* actual_size)
{
    (void)unused;
    (void)optional_old_block_size;

    // Standard aligned_alloc() and posix_memalign() require size to be a
    // multiple of alignment, but _aligned_realloc() and our manual alignment
    // implementation does not. Requiring size to be a multiple of alignment
    // makes this much harder to use: it should be allowed to allocate say
    // `char[3]` with alignment of GP_ALLOC_ALIGNMENT. Therefore, we'll just
    // round up the size for aligned_alloc() and posix_memalign().

    #ifdef GP_TARGET_OS_WINDOWS // aligned_alloc() not available even in C11
    void* mem = _aligned_realloc(optional_old_block, block_size, alignment);
    #elif __STDC_VERSION__ >= 201112L
    void* mem;
    block_size = gp_round_to_aligned(block_size, alignment);

    // aligned_alloc() doesn't play well with realloc(), so have to handle
    // separately. Prefer realloc(), because realloc() can sometimes just resize
    // the current allocation instead of actually reallocating.
    if (alignment <= alignof(max_align_t))
        mem = realloc(optional_old_block, block_size);
    else if (optional_old_block == NULL)
        mem = aligned_alloc(alignment, block_size);
    else if (block_size <= optional_old_block_size) {
        mem = optional_old_block;
        block_size = optional_old_block_size; // in case user uses actual_size for dealloc
    } else {
        mem = aligned_alloc(alignment, block_size);
        if (mem != NULL) {
            memcpy(mem, optional_old_block, optional_old_block_size);
            #if __STDC_VERSION__ >= 202311L
            free_aligned_sized(optional_old_block, alignment, optional_old_block_size);
            #else
            free(optional_old_block);
            #endif
        }
    }
    #elif _POSIX_C_SOURCE >= 200112L
    // No `alignof(max_align_t)`. It might be relatively safe to assume
    // `2*sizeof(size_t)`, but we'll play it even safer and just not use realloc()
    // for now. Might want to optimize later for known targets though.
    void* mem = NULL;
    if (alignment < sizeof(void*))
        alignment = sizeof(void*);
    block_size = gp_round_to_aligned(block_size, alignment);

    posix_memalign(&mem, alignment, block_size);
    if (optional_old_block != NULL && mem != NULL) {
        memcpy(mem, optional_old_block, optional_old_block_size);
        free(optional_old_block);
    }
    #else // have to align manually.
    void* mem = NULL;
    void* mem_start = realloc(
        optional_old_block, block_size + gp_round_to_aligned(sizeof(void*), alignment));
    if (mem_start != NULL) {
        mem = (char*)gp_round_to_aligned((uintptr_t)((void**)mem_start + 1), alignment);
        memcpy((void**)mem - 1, &mem_start, sizeof mem_start);
    }
    #endif

    // TODO: Better would be to use calloc() when possible, it might skip
    // zeroing for big mmapped blocks, which would be a huge win, but the code
    // above is already crazy enough considering that this is supposed to be a
    // simple malloc() wrapper, so we'll do it later once more important stuff
    // are implemented.
    if ( ! uninitialized && mem != NULL) {
        if (optional_old_block == NULL)
            memset(mem, 0x00, block_size);
        else
            memset(
                (char*)mem + optional_old_block_size,
                0x00,
                block_size - optional_old_block_size);
    }
    *actual_size = block_size;
    return mem;
}

static void gp_s_global_heap_dealloc(
    struct GPAllocator* unused,
    void* block,
    size_t size,
    size_t alignment)
{
    (void)unused;
    (void)size;
    (void)alignment;
    #ifdef GP_TARGET_OS_WINDOWS
    _aligned_free(block);
    #elif __STDC_VERSION__ >= 202311L
    if (alignment <= alignof(max_align_t))
        free_sized(block, size);
    else
        free_aligned_sized(block, alignment, size);
    #elif __STDC_VERSION__ >= 201112L || _POSIX_C_SOURCE >= 200112L
    free(block);
    #else
    memcpy(&block, (void**)block - 1, sizeof block);
    free(block);
    #endif
}

GPAllocator gp_mallocator =
{
    .alloc   = gp_s_global_heap_alloc,
    .dealloc = gp_s_global_heap_dealloc
};

GPAllocator* gp_heap = &gp_mallocator;

//-------------------------------------
// Arena

typedef struct GPArenaNode
{
    struct GPArenaNode* next;
    size_t capacity;
    unsigned char memory[];
} GPArenaNode;

typedef struct GPArenaDefer
{
    struct GPArenaDefer* next;
    void (*func)(void* arg);
    void* arg;
} GPArenaDefer;

static bool gp_s_arena_node_new(
    GPArena* arena,
    size_t new_cap)
{
    if (arena->_cache != NULL && new_cap <= arena->_cache->capacity) {
        arena->_head = arena->_cache;
        arena->_cache = NULL;
        arena->_position = arena->_head->memory;
        gp_asan_poison(arena->_head->memory, arena->_head->capacity);
        return true;
    }

    size_t full_size;
    size_t new_node_size = sizeof(GPArenaNode) + new_cap;

    GPArenaNode* new_node = arena->backing_allocator->alloc(
        arena->backing_allocator,
        NULL, 0,
        new_node_size,
        GP_ALLOC_ALIGNMENT,
        true,
        &full_size);
    if (new_node == NULL)
        return false;

    new_cap += full_size - new_node_size;
    new_node->next = arena->_head;
    new_node->capacity = new_cap;
    arena->_head = new_node;
    arena->_position = new_node->memory;
    gp_asan_poison(new_node->memory, new_cap);
    return true;
}

static void* gp_s_arena_realloc(
    GPArena* arena,
    void*    old_block,
    size_t   old_block_size,
    size_t   size,
    size_t   alignment,
    bool     uninitialized,
    size_t*  actual_size);

void* gp_arena_alloc(
    GPAllocator* me,
    void*   old_block,
    size_t  old_block_size,
    size_t  size,
    size_t  alignment,
    bool    uninitialized,
    size_t* actual_size)
{
    // In our docs we recommend that user allocates using gp_mem_alloc(), which
    // passes GP_ALLOC_ALIGNMENT as alignment. Therefore, it is reasonable to
    // predict that the next allocation will have that alignment, so round size
    // up so we can return a more accurate actual_size.
    size = gp_round_to_aligned(size, GP_ALLOC_ALIGNMENT);
    GPArena* arena = (GPArena*)me;

    if (old_block != NULL)
        return gp_s_arena_realloc(
            arena, old_block, old_block_size, size, alignment, uninitialized, actual_size);

    // Use uintptr_t to avoid out of bounds pointer arithmetic UB.
    uintptr_t block_start = gp_round_to_aligned((uintptr_t)arena->_position, alignment);
    uintptr_t block_end = block_start + size + GP_POISON_BOUNDARY_SIZE;
    uintptr_t node_end = (uintptr_t)arena->_head->memory + arena->_head->capacity;

    if (block_end > node_end) { // out of memory, create new node
        size_t new_cap = gp_min(
            (double)arena->max_size,
            arena->growth_factor * arena->_head->capacity);

        new_cap = gp_max(
            new_cap,
            size
                + GP_POISON_BOUNDARY_SIZE
                + gp_signed_max(0, alignment - GP_ALLOC_ALIGNMENT));

        if ( ! gp_s_arena_node_new(arena, new_cap))
            return NULL;
    }

    void* block = arena->_position = (void*)gp_round_to_aligned(
        (uintptr_t)arena->_position, alignment);

    gp_asan_unpoison(block, size);
    if ( ! uninitialized)
        memset(block, 0, size);

    *actual_size = size;
    return block;
}

static void* gp_s_arena_realloc(
    GPArena* arena,
    void*    old_block,
    size_t   old_block_size,
    size_t   size,
    size_t   alignment,
    bool     uninitialized,
    size_t*  actual_size)
{
    old_block_size = gp_round_to_aligned(old_block_size, alignment);

    if (size <= old_block_size) {
        if ((char*)old_block + old_block_size == arena->_position)
            arena->_position = (char*)old_block + size;
        gp_asan_poison(arena->_position, old_block_size - size);
        *actual_size = size;
        return old_block;
    }
    if ((char*)old_block + old_block_size == arena->_position)
        arena->_position = old_block;

    void* block = gp_arena_alloc(&arena->base, NULL, 0, size, alignment, true, actual_size);
    if (block == NULL)
        return NULL;
    if (block != old_block)
        memcpy(block, old_block, old_block_size);
    if ( ! uninitialized)
        memset((char*)block + old_block_size, 0, size - old_block_size);

    return block;
}

void gp_arena_dealloc(
    struct GPAllocator* me,
    void* block,
    size_t size,
    size_t alignment)
{
    GPArena* arena = (GPArena*)me;
    size = gp_round_to_aligned(size, alignment);
    if ((char*)block + size == arena->_position) {
        arena->_position = block;
        gp_asan_poison(block, size);
    }
}

GPArena* gp_internal_arena_new(const GPArena* init, size_t capacity)
{
    GPArena* arena;

    GPAllocator* alc = init->backing_allocator;
    if (alc == NULL)
        alc = gp_heap;

    if (init->backing_buffer != NULL
        && capacity > sizeof *arena + sizeof(GPArenaNode) + GP_POISON_BOUNDARY_SIZE)
    {
        arena = init->backing_buffer;
        arena->_head = (GPArenaNode*)(arena + 1);
        arena->_head->capacity = capacity - sizeof *arena - sizeof(GPArenaNode);
        arena->_head->capacity -= GP_POISON_BOUNDARY_SIZE;
    }
    else {
        capacity = capacity != 0 ? gp_round_to_aligned(capacity, GP_ALLOC_ALIGNMENT) : 256;
        capacity += GP_POISON_BOUNDARY_SIZE;
        size_t size = sizeof *arena + sizeof(GPArenaNode) + capacity;
        size_t full_size;
        arena = alc->alloc(alc, NULL, 0, size, sizeof(double), true, &full_size);
        if (arena == NULL)
            return NULL;
        capacity += full_size - size;
        arena->_head = (GPArenaNode*)(arena + 1);
        arena->_head->capacity = capacity;
    }
    arena->_position = arena->_head->memory;
    arena->_head->next = NULL;
    gp_asan_poison(arena->_position, arena->_head->capacity);

    arena->_defers = NULL;
    arena->_cache = NULL;
    arena->base.alloc = gp_arena_alloc;
    arena->base.dealloc = gp_arena_dealloc;
    arena->backing_allocator = alc;
    arena->backing_buffer = init->backing_buffer;
    arena->growth_factor = init->growth_factor;
    if (arena->growth_factor == 0.0)
        arena->growth_factor = 1.5;
    arena->max_size = init->max_size;
    if (arena->max_size == 0)
        arena->max_size = 1 << 15;

    return arena;
}

static bool gp_s_in_this_node(GPArenaNode* node, void* _pos)
{
    gp_assume(node != NULL, "Pointer not allocated by arena.");
    uintptr_t pos = (uintptr_t)_pos;
    uintptr_t block_start = (uintptr_t)node->memory;
    return block_start <= pos && pos <= block_start + node->capacity;
}

static void gp_s_arena_deallocate_cache(GPArena* arena)
{
    if (arena->_cache != NULL)
        arena->backing_allocator->dealloc(
            arena->backing_allocator,
            arena->_cache,
            sizeof *arena->_cache + arena->_cache->capacity,
            sizeof(void*));
}

static size_t gp_s_arena_node_delete(GPArena* arena)
{
    GPArenaNode* old_head = arena->_head;
    while (arena->_defers != NULL && gp_s_in_this_node(old_head, arena->_defers)) {
        arena->_defers->func(arena->_defers->arg);
        arena->_defers = arena->_defers->next;
    }
    size_t old_capacity = old_head->capacity;
    arena->_head = arena->_head->next;
    gp_s_arena_deallocate_cache(arena);
    arena->_cache = old_head;
    return old_capacity;
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    while ( ! gp_s_in_this_node(arena->_head, new_pos))
        gp_s_arena_node_delete(arena);
    while (arena->_defers != NULL
            && gp_s_in_this_node(arena->_head, arena->_defers)
            && new_pos < (void*)arena->_defers)
    {
        arena->_defers->func(arena->_defers->arg);
        arena->_defers = arena->_defers->next;
    }
    arena->_position = new_pos;
    gp_asan_poison(
        new_pos,
        arena->_head->memory + arena->_head->capacity - (unsigned char*)new_pos);
}

size_t gp_arena_clear(GPArena* arena)
{
    size_t total_capacity = 0;
    while (arena->_head->next != NULL)
        total_capacity += gp_s_arena_node_delete(arena);
    gp_s_arena_deallocate_cache(arena);
    arena->_cache = NULL;
    while (arena->_defers != NULL) {
        arena->_defers->func(arena->_defers->arg);
        arena->_defers = arena->_defers->next;
    }
    arena->_position = arena->_head->memory;
    gp_asan_poison(arena->_head->memory, arena->_head->capacity);
    return total_capacity + arena->_head->capacity;
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;

    while (arena->_head->next != NULL)
        gp_s_arena_node_delete(arena);
    gp_s_arena_deallocate_cache(arena);
    while (arena->_defers != NULL) {
        arena->_defers->func(arena->_defers->arg);
        arena->_defers = arena->_defers->next;
    }

    if (arena->backing_buffer == NULL)
        arena->backing_allocator->dealloc(
            arena->backing_allocator,
            arena,
            sizeof *arena + sizeof(GPArenaNode) + arena->_head->capacity,
            sizeof(double));
}

void* gp_internal_arena_defer(GPArena* arena, void (*func)(void*), void* arg)
{
    size_t ignore_out_size;
    GPArenaDefer* node = gp_arena_alloc(
        &arena->base, NULL, 0, sizeof *node, GP_ALLOC_ALIGNMENT, true, &ignore_out_size);
    if (node == NULL)
        return NULL;

    node->func = func;
    node->arg  = arg;
    node->next = arena->_defers;
    arena->_defers = node;
    gp_asan_poison(node + 1, GP_ALLOC_ALIGNMENT - sizeof *node);

    return node;
}

// Undef for single header users
#ifdef GP_USE_MISC_DEFINED
#undef __USE_MISC
#undef GP_USE_MISC_DEFINED
#endif
