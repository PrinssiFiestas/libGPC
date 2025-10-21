// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/memory.h>
#include <gpc/utils.h>
#include <gpc/thread.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !_WIN32
#ifndef __USE_MISC // this should not be used, but _GNU_SOURCE had too many
#define __USE_MISC // portability related problems. Used for MAP_ANONYMOUS
#define GP_USE_MISC_DEFINED
#endif
#include <sys/mman.h>
#include <errno.h>
#else
#include <windows.h>
#endif

static void* gp_s_global_heap_alloc(GPAllocator* unused, size_t block_size, size_t alignment)
{
    (void)unused;

    #if _WIN32
    void* mem = _aligned_malloc(block_size, alignment);
    #elif __STDC_VERSION__ >= 201112L
    // Size has to be a multiple of alignment, which is wasteful, so we only round here.
    void* mem = aligned_alloc(alignment, gp_round_to_aligned(block_size, alignment));
    #elif _POSIX_C_SOURCE >= 200112L
    void* mem = NULL;
    posix_memalign(&mem, alignment, block_size);
    #else
    void* mem_start = malloc(block_size + gp_round_to_aligned(sizeof(void*), alignment));
    if (mem_start == NULL)
        abort();
    void* mem = (uint8_t*)gp_round_to_aligned((uintptr_t)((void**)mem_start + 1), alignment);
    memcpy((void**)mem - 1, &mem_start, sizeof mem_start);
    #endif
    if (mem == NULL && errno == ENOMEM)
        abort();
    // Arguments should already be validated, so this assertion should not fail,
    // but double check for good measure.
    gp_assert(mem != NULL, "malloc() failed!");
    return mem;
}

static void gp_s_global_heap_dealloc(GPAllocator* unused, void* block)
{
    (void)unused;
    #if _WIN32
    _aligned_free(block);
    #elif __STDC_VERSION__ >= 201112L || _POSIX_C_SOURCE >= 200112L
    free(block);
    #else
    memcpy(&block, (void**)block - 1, sizeof block);
    free(block);
    #endif
}

static GPAllocator gp_s_mallocator = {
    .alloc   = gp_s_global_heap_alloc,
    .dealloc = gp_s_global_heap_dealloc
};
GPAllocator* gp_global_heap = &gp_s_mallocator;

// ----------------------------------------------------------------------------

// Note: we probably want to keep the size of this a multiple of
// GP_ALLOC_ALIGNMENT since allocations are anyway rounded to this boundary. If
// you need to change this, add explicit padding.
typedef struct gp_arena_node
{
    struct gp_arena_node* tail;
    void*   position;
    size_t  capacity;
    void*   allocation;
    uint8_t memory[];
} GPArenaNode;

#if GP_HAS_SANITIZER
#define GP_POISON_BOUNDARY_SIZE 8 // 8 is minimum required by libasan
#else
#define GP_POISON_BOUNDARY_SIZE 0
#endif

static void* gp_s_arena_node_new_alloc(
    GPAllocator* allocator,
    GPArenaNode** head,
    size_t new_cap,
    size_t size,
    size_t alignment)
{
    GPArenaNode* new_node = gp_mem_alloc(allocator,
        gp_round_to_aligned(sizeof(GPArenaNode), alignment)
        + gp_max(new_cap, size + GP_POISON_BOUNDARY_SIZE)
        + alignment - GP_ALLOC_ALIGNMENT
    );
    new_node->tail       = *head;
    new_node->capacity   = new_cap;
    new_node->allocation = new_node;

    void* block = new_node->position = (void*)gp_round_to_aligned(
        (uintptr_t)(new_node + 1), alignment);
    new_node->position = (uint8_t*)(new_node->position) + size + GP_POISON_BOUNDARY_SIZE;
    *head = new_node;
    if (new_node->capacity > size) // -8 for poison boundary
        GP_TRY_POISON_MEMORY_REGION((uint8_t*)new_node->position - 8, new_node->capacity - size);
    // Poison padding possibly caused by large alginments
    GP_TRY_POISON_MEMORY_REGION(new_node + 1,
        gp_round_to_aligned((uintptr_t)(new_node + 1), alignment) - (uintptr_t)(new_node + 1));

    return block;
}

void* gp_arena_alloc(GPAllocator* allocator, const size_t size, const size_t alignment)
{
    GPArena* arena = (GPArena*)allocator;
    GPArenaNode* head = arena->head;

    void* block = head->position = (void*)gp_round_to_aligned((uintptr_t)head->position, alignment);
    if ((uint8_t*)block + size + GP_POISON_BOUNDARY_SIZE > (uint8_t*)(head + 1) + arena->head->capacity)
    { // out of memory, create new arena
        size_t new_cap = arena->growth_factor * arena->head->capacity;
        block = gp_s_arena_node_new_alloc(arena->backing, &arena->head, new_cap, size, alignment);
    }
    else {
        GP_TRY_UNPOISON_MEMORY_REGION(block, size);
        head->position = (uint8_t*)block + size + GP_POISON_BOUNDARY_SIZE;
    }
    return block;
}

GPArena* gp_arena_new(const GPArenaInitializer* init, size_t capacity)
{
    GPArena* arena;
    GPArenaInitializer empty_init = {0};

    if (init == NULL)
        init = &empty_init;

    GPAllocator* allocator = init->backing_allocator ?
        init->backing_allocator
      : gp_global_heap;

    const size_t meta_size = init->meta_size != 0 ?
        init->meta_size
      : sizeof(GPArena);

    if (init->backing_buffer != NULL && capacity > meta_size + sizeof(GPArenaNode) + GP_POISON_BOUNDARY_SIZE)
    { // use backing buffer
        arena = init->backing_buffer;
        arena->head = (GPArenaNode*)((uint8_t*)arena + meta_size);
        arena->head->capacity = capacity - meta_size - sizeof(GPArenaNode) - GP_POISON_BOUNDARY_SIZE;
        arena->head->allocation = NULL; // don't free backing buffer on delete
    }
    else {
        capacity = capacity != 0 ? gp_round_to_aligned(capacity, GP_ALLOC_ALIGNMENT) : 256;
        capacity += GP_POISON_BOUNDARY_SIZE;
        arena = gp_mem_alloc(allocator, meta_size + sizeof(GPArenaNode) + capacity);
        arena->head = (GPArenaNode*)((uint8_t*)arena + meta_size);
        arena->head->capacity = capacity;
        arena->head->allocation = arena;
    }
    arena->head->position = arena->head->memory;
    arena->head->tail     = NULL;
    GP_TRY_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);

    arena->base.alloc   = gp_arena_alloc;
    arena->base.dealloc = gp_internal_arena_dealloc;
    arena->backing = init->backing_allocator != NULL ?
        init->backing_allocator
      : gp_global_heap;
    arena->growth_factor = init->growth_factor != 0. ?
        init->growth_factor
      : 2.;
    arena->max_size = init->max_size != 0 ?
        init->max_size
      : 1 << 15;

    return arena;
}

static bool gp_s_in_this_node(GPArenaNode* node, void* _pos)
{
    gp_db_assert(node != NULL,
        "gp_arena_rewind(): passed pointer was not allocated by the arena.");
    uint8_t* pos = _pos;
    uint8_t* block_start = (uint8_t*)(node + 1);
    return block_start <= pos && pos <= block_start + node->capacity;
}

static size_t gp_s_arena_node_delete(GPAllocator* allocator, GPArenaNode** head)
{
    GPArenaNode* old_head = *head;
    size_t old_capacity = old_head->capacity;
    *head = (*head)->tail;
    gp_mem_dealloc(allocator, old_head);
    return old_capacity;
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    while ( ! gp_s_in_this_node(arena->head, new_pos))
        gp_s_arena_node_delete(arena->backing, &arena->head);

    arena->head->position = new_pos;
    if ((uint8_t*)new_pos < (uint8_t*)arena->head->position + arena->head->capacity)
        GP_TRY_POISON_MEMORY_REGION(new_pos,
            arena->head->memory + arena->head->capacity - (uint8_t*)new_pos);
}

size_t gp_arena_reset(GPArena* arena)
{
    size_t total_capacity = 0;
    while (arena->head->tail != NULL)
        total_capacity += gp_s_arena_node_delete(arena->backing, &arena->head);

    arena->head->position = arena->head->memory;
    GP_TRY_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);
    return total_capacity + arena->head->capacity;
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;
    while (arena->head->tail != NULL)
        gp_s_arena_node_delete(arena->backing, &arena->head);
    gp_mem_dealloc(arena->backing, arena->head->allocation);
}

// ----------------------------------------------------------------------------
// Scratch arena

static GPThreadKey  gp_s_scratch_arena_key;
static GPThreadOnce gp_s_scratch_arena_key_once = GP_THREAD_ONCE_INIT;

// Make Valgrind shut up.
static void gp_s_delete_main_thread_scratch_arena(void)
{
    gp_arena_delete(gp_thread_local_get(gp_s_scratch_arena_key));
}

static void gp_s_make_scratch_arena_key(void)
{
    atexit(gp_s_delete_main_thread_scratch_arena);
    gp_thread_key_create(&gp_s_scratch_arena_key, (void(*)(void*))gp_arena_delete);
}

static GPArena* gp_s_new_scratch_arena(void)
{
    GPArenaInitializer init = {
        .max_size           = GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE,
        .growth_factor = GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT,
    };
    GPArena* arena = gp_arena_new(&init, GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE);
    gp_thread_local_set(gp_s_scratch_arena_key, arena);
    return arena;
}

GPArena* gp_scratch_arena(void)
{
    gp_thread_once(&gp_s_scratch_arena_key_once, gp_s_make_scratch_arena_key);

    GPArena* arena = gp_thread_local_get(gp_s_scratch_arena_key);
    if (GP_UNLIKELY(arena == NULL))
        arena = gp_s_new_scratch_arena();
    return arena;
}

// ----------------------------------------------------------------------------

// TODO use this to implement GPArray(AlignedT)
void* gp_mem_realloc_aligned(
    GPAllocator* allocator,
    void* old_block,
    size_t old_size,
    size_t new_size,
    size_t alignment)
{
    gp_db_assert(old_size <= GP_MAX_ALLOC_SIZE, "Impossible size, no allocator accepts this.");
    gp_db_assert(new_size <= GP_MAX_ALLOC_SIZE, "Maximum allocation size exceeded.");

    GPContiguousArena* carena = (GPContiguousArena*)allocator;
    if (allocator->dealloc == gp_internal_carena_dealloc && old_block != NULL &&
        (char*)old_block + old_size == (char*)carena->position)
    { // extend block instead of reallocating and copying
        carena->position = old_block;
        return gp_carena_alloc(carena, new_size, GP_ALLOC_ALIGNMENT);
    }

    GPArena* arena = (GPArena*)allocator;
    GPScope* scope = (GPScope*)allocator;
    GPArenaNode** head = allocator->alloc == gp_arena_alloc ? &arena->head : &scope->head;

    if (allocator->dealloc == gp_internal_arena_dealloc && old_block != NULL &&
        (uint8_t*)old_block + old_size + GP_POISON_BOUNDARY_SIZE == (uint8_t*)(*head)->position)
    { // extend block instead of reallocating and copying
        (*head)->position = old_block;
        void* new_block = gp_mem_alloc_aligned(allocator, new_size, alignment);
        if (new_block != old_block) { // arena ran out of space and reallocated
            memcpy(new_block, old_block, old_size);
            GP_TRY_POISON_MEMORY_REGION(old_block, old_size);
        }
        return new_block;
    }
    void* new_block = gp_mem_alloc_aligned(allocator, new_size, alignment);
    if (old_block != NULL)
        memcpy(new_block, old_block, old_size);
    gp_mem_dealloc(allocator, old_block);
    return new_block;
}

// ----------------------------------------------------------------------------
// Scope Allocator

#ifndef GP_SCOPE_DEFAULT_INIT_SIZE
#define GP_SCOPE_DEFAULT_INIT_SIZE 256
#endif
#ifndef GP_SCOPE_DEFAULT_MAX_SIZE
#define GP_SCOPE_DEFAULT_MAX_SIZE (1 << 15) // 32 KB
#endif
#ifndef GP_SCOPE_DEFAULT_GROWTH_COEFFICIENT
#define GP_SCOPE_DEFAULT_GROWTH_COEFFICIENT 2.0
#endif

typedef struct gp_defer_stack
{
    GPDefer* stack;
    uint32_t length;
    uint32_t capacity;
} GPDeferStack;

void* gp_scope_alloc(GPAllocator* allocator, const size_t size, const size_t alignment)
{
    GPScope* arena = (GPScope*)allocator;
    GPArenaNode* head = arena->head;

    void* block = head->position = (void*)gp_round_to_aligned((uintptr_t)head->position, alignment);
    if ((uint8_t*)block + size + GP_POISON_BOUNDARY_SIZE > (uint8_t*)(head + 1) + arena->head->capacity)
    { // out of memory, create new arena
        block = gp_s_arena_node_new_alloc(
            gp_global_heap, &arena->head, 2*arena->head->capacity, size, alignment);
    }
    else {
        GP_TRY_UNPOISON_MEMORY_REGION(block, size);
        head->position = (uint8_t*)block + size + GP_POISON_BOUNDARY_SIZE;
    }
    return block;
}

static GPThreadKey  gp_s_scope_list_key;
static GPThreadOnce gp_s_scope_list_key_once = GP_THREAD_ONCE_INIT;

GPScope* gp_last_scope(void)
{
    return gp_thread_local_get(gp_s_scope_list_key);
}

static void gp_s_scope_execute_defers(GPScope* scope)
{
    if (scope->defer_stack != NULL) {
        for (size_t i = scope->defer_stack->length - 1; i != (size_t)-1; --i) {
            scope->defer_stack->stack[i].func(scope->defer_stack->stack[i].arg);
        }
    }
}

static void gp_s_delete_thread_scopes(void*_scopes)
{
    GPScope* scope = _scopes;
    while (scope != NULL) {
        gp_s_scope_execute_defers(scope);
        GPScope* parent = scope->parent;

        while (scope->head->tail != NULL)
            gp_s_arena_node_delete(gp_global_heap, &scope->head);
        gp_mem_dealloc(gp_global_heap, scope);

        scope = parent;
    }
}

static void gp_s_delete_main_thread_scopes(void)
{
    gp_s_delete_thread_scopes(gp_thread_local_get(gp_s_scope_list_key));
}

static void gp_s_make_scope_list_key(void)
{
    atexit(gp_s_delete_main_thread_scopes);
    gp_thread_key_create(&gp_s_scope_list_key, gp_s_delete_thread_scopes);
}

static GPScope* gp_s_scope_new(size_t capacity)
{
    GPScope* arena;

    capacity = capacity != 0 ? gp_round_to_aligned(capacity, GP_ALLOC_ALIGNMENT) : 256;
    capacity += GP_POISON_BOUNDARY_SIZE;
    arena = gp_mem_alloc(gp_global_heap, sizeof*arena + sizeof(GPArenaNode) + capacity);
    arena->head = (GPArenaNode*)(arena + 1);
    arena->head->capacity = capacity;

    arena->head->position = arena->head->memory;
    arena->head->tail     = NULL;
    GP_TRY_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);

    arena->base.alloc   = gp_scope_alloc;
    arena->base.dealloc = gp_internal_arena_dealloc;

    return arena;
}

GPScope* gp_begin(const size_t _size)
{
    gp_thread_once(&gp_s_scope_list_key_once, gp_s_make_scope_list_key);

    const size_t size = _size == 0 ?
        (size_t)GP_SCOPE_DEFAULT_INIT_SIZE
      : _size;

    GPScope* scope = gp_s_scope_new(size);
    scope->defer_stack = NULL;
    scope->parent = gp_thread_local_get(gp_s_scope_list_key);
    gp_thread_local_set(gp_s_scope_list_key, scope);

    return scope;
}

size_t gp_end(GPScope* scope)
{
    if (scope == NULL)
        return 0;

    GPScope* child = gp_thread_local_get(gp_s_scope_list_key);

    // If gp_end() is called in thread destructor twice (e.g. in case of skipped
    // GP_END), child will be NULL and scope has already been freed.
    if (child == NULL)
        return 0;

    while (child != scope) {
        GPScope* parent = child->parent;
        gp_s_scope_execute_defers(child);

         while (child->head->tail != NULL)
             gp_s_arena_node_delete(gp_global_heap, &child->head);
        gp_mem_dealloc(gp_global_heap, child);

        child = parent;
    }
    gp_s_scope_execute_defers(child);
    GPScope* parent = scope->parent;

    size_t scope_size = 0;
    while (scope->head->tail != NULL)
        scope_size += gp_s_arena_node_delete(gp_global_heap, &scope->head);

    gp_mem_dealloc(gp_global_heap, scope);
    gp_thread_local_set(gp_s_scope_list_key, parent);
    return scope_size;
}

void gp_scope_defer(GPScope* scope, void (*f)(void*), void* arg)
{
    if (scope->defer_stack == NULL)
    {
        const size_t init_cap = 4;
        scope->defer_stack = gp_scope_alloc(&scope->base,
            sizeof*(scope->defer_stack) + init_cap * sizeof(GPDefer), GP_ALLOC_ALIGNMENT);

        scope->defer_stack->length   = 0;
        scope->defer_stack->capacity = init_cap;
        scope->defer_stack->stack    = (GPDefer*)(scope->defer_stack + 1);
    }
    else if (scope->defer_stack->length == scope->defer_stack->capacity)
    {
        GPDefer* old_stack = scope->defer_stack->stack;
        scope->defer_stack->stack = gp_scope_alloc(&scope->base,
            scope->defer_stack->capacity * 2 * sizeof(GPDefer), GP_ALLOC_ALIGNMENT);
        memcpy(scope->defer_stack->stack, old_stack,
            scope->defer_stack->length * sizeof(GPDefer));
        scope->defer_stack->capacity *= 2;
    }
    scope->defer_stack->stack[scope->defer_stack->length].func = f;
    scope->defer_stack->stack[scope->defer_stack->length].arg  = arg;
    scope->defer_stack->length++;
}

// ----------------------------------------------------------------------------
// Contiguous Arena

size_t gp_page_size(void)
{
    #ifdef _SC_PAGE_SIZE
    return sysconf(_SC_PAGE_SIZE);
    #elif defined(_WIN32)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize;
    #else
    return 4096;
    #endif
}

GPContiguousArena* gp_carena_new(size_t size)
{
    gp_db_assert(size != 0, "%zu", size);
    gp_db_assert(size <= GP_MAX_ALLOC_SIZE - sizeof(GPContiguousArena));
    gp_db_expect(size >= 4096, "%zu", size,
        "Contiguous arenas are supposed to be HUGE. "
        "Are you sure you are allocating enough?");

    size = gp_round_to_aligned(size, gp_page_size());

    #if _WIN32
    GPContiguousArena* arena = VirtualAlloc(
        NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    gp_db_expect(arena != NULL, "VirtualAlloc():", "%lu", GetLastError());
    #else
    GPContiguousArena* arena = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        // MAP_NORESERVE: don't reserve swap memory. Arenas tend to be HUGE, we
        // don't want to waste swap memory especially for virtual machines. If
        // we run out of swap memory, we might segfault or get killed by OOM,
        // but at that point the user would deserve to be killed anyway.
        //
        // Note: MAP_HUGETLB (huge pages) would improve performance, but raises
        // SIGBUS if Linux machine not configured appropriately, so we'll leave
        // it out in interest of portability.
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1, 0);
    gp_db_expect(arena != NULL && arena != (void*)-1, "mmap():", "%s", strerror(errno));
    #endif

    if (arena == NULL || arena == (void*)-1)
        return arena = NULL;

    arena->base.alloc   = (void*(*)(GPAllocator*,size_t,size_t))gp_carena_alloc;
    arena->base.dealloc = gp_internal_carena_dealloc;
    arena->position     = arena->memory;
    arena->capacity     = size - sizeof*arena;
    return arena;
}

void gp_carena_reset(GPContiguousArena* arena)
{
    arena->position = arena->memory;
    size_t page_size = gp_page_size();
    if (arena->capacity < page_size)
        return;

    #if _WIN32
    VirtualAlloc(
        (uint8_t*)arena + page_size,
        arena->capacity - page_size,
        MEM_RESET,
        PAGE_READWRITE);
    #else
    madvise(
        (uint8_t*)arena + page_size,
        gp_round_to_aligned(arena->capacity, page_size),
        MADV_DONTNEED);
    #endif
}

void gp_carena_delete(GPContiguousArena* arena)
{
    if (arena == NULL)
        return;

    #if _WIN32
    BOOL VirtualFree_result = VirtualFree(arena, 0, MEM_RELEASE);
    gp_db_expect(VirtualFree_result != 0, "%lu", GetLastError());
    #else // TODO why are we aligning when munmap() and mmap() allows unaligned sizes?
    int munmap_result = munmap(arena, gp_round_to_aligned(arena->capacity, gp_page_size()));
    gp_db_expect(munmap_result != -1, "%s", strerror(errno));
    #endif
}

// ----------------------------------------------------------------------------
// C99 Auto Scope Defer

#ifndef GP_AUTO_SCOPE_DEFERS_SIZE
#define GP_AUTO_SCOPE_DEFERS_SIZE ((size_t)1024*1024)
#endif

static GPThreadKey  gp_s_auto_scope_key;
static GPThreadOnce gp_s_auto_scope_key_once = GP_THREAD_ONCE_INIT;

GPAutoScope99* gp_internal_thread_local_auto_scope(void);

static void gp_s_delete_auto_scope(void*_auto_scope)
{
    if (_auto_scope == NULL)
        return;

    GPAutoScope99* auto_scope = _auto_scope;

    GPDefer* defers = auto_scope->defers;
    for ( ; auto_scope->defers_length > 0; auto_scope->defers_length--)
        defers[auto_scope->defers_length - 1].func(defers[auto_scope->defers_length - 1].arg);

    #if _WIN32
    BOOL VirtualFree_result = VirtualFree(defers, 0, MEM_RELEASE);
    gp_db_expect(VirtualFree_result != 0, "%lu", GetLastError());
    #else
    int munmap_result = munmap(defers, GP_AUTO_SCOPE_DEFERS_SIZE);
    gp_db_expect(munmap_result != -1, "%s", strerror(errno));
    #endif

    gp_carena_delete(auto_scope->arena);
}

static void gp_s_delete_main_thread_auto_scope(void)
{
    gp_s_delete_auto_scope(gp_internal_thread_local_auto_scope());
}

static void gp_s_make_auto_scope_key(void)
{
    atexit(gp_s_delete_main_thread_auto_scope);
    gp_thread_key_create(&gp_s_auto_scope_key, gp_s_delete_auto_scope);
}

static GPAutoScope99* gp_s_new_auto_scope(void)
{
    GPAutoScope99 auto_scope_data = {.arena = gp_carena_new(GP_AUTO_SCOPE_DEFERS_SIZE)};
    gp_assert(
        auto_scope_data.arena != NULL,
        "Failed allocating memory for static defers. Try recompiling libGPC "
        "with smaller GP_AUTO_SCOPE_DEFERS_SIZE. Current size:",
        "%zu", GP_AUTO_SCOPE_DEFERS_SIZE);

    // Extend lifetime
    GPAutoScope99* auto_scope = auto_scope_data.arena->position;
    auto_scope_data.arena->position = auto_scope + 1;
    *auto_scope = auto_scope_data;

    #if _WIN32
    auto_scope->defers = VirtualAlloc(
        NULL, GP_AUTO_SCOPE_DEFERS_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    gp_db_assert(auto_scope->defers != NULL, "VirtualAlloc():", "%lu", GetLastError());
    #else
    auto_scope->defers = mmap(
        NULL,
        GP_AUTO_SCOPE_DEFERS_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1, 0);
    gp_db_assert(auto_scope->defers != NULL && auto_scope->defers != (void*)-1,
        "mmap():", "%s", strerror(errno));
    #endif

    gp_thread_local_set(gp_s_auto_scope_key, auto_scope);
    return auto_scope;
}

GPAutoScope99* gp_internal_thread_local_auto_scope(void)
{
    gp_thread_once(&gp_s_auto_scope_key_once, gp_s_make_auto_scope_key);

    GPAutoScope99* auto_scope = gp_thread_local_get(gp_s_auto_scope_key);
    if (GP_UNLIKELY(auto_scope == NULL))
        auto_scope = gp_s_new_auto_scope();
    return auto_scope;
}

// ----------------------------------------------------------------------------
// Mutex Allocator

static void* gp_s_mutex_alloc(GPAllocator*_alc, size_t size, size_t alignment)
{
    GPMutexAllocator* alc = (GPMutexAllocator*)_alc;
    gp_mutex_lock(&alc->mutex);
    void* ptr = gp_mem_alloc_aligned(alc->backing, size, alignment);
    gp_mutex_unlock(&alc->mutex);
    return ptr;
}

static void gp_s_mutex_dealloc(GPAllocator*_alc, void* ptr)
{
    GPMutexAllocator* alc = (GPMutexAllocator*)_alc;
    gp_mutex_lock(&alc->mutex);
    gp_mem_dealloc(alc->backing, ptr);
    gp_mutex_unlock(&alc->mutex);
}

GPAllocator* gp_mutex_allocator_init(GPMutexAllocator* alc, GPAllocator* backing)
{
    if ( ! gp_mutex_init(&alc->mutex))
        return NULL;
    alc->base.alloc   = gp_s_mutex_alloc;
    alc->base.dealloc = gp_s_mutex_dealloc;
    alc->backing      = backing;
    return (GPAllocator*)alc;
}

void gp_mutex_allocator_destroy(GPMutexAllocator* alc)
{
    if (alc == NULL)
        return;
    gp_mutex_destroy(&alc->mutex);
}

#ifdef GP_USE_MISC_DEFINED
#undef __USE_MISC
#undef GP_USE_MISC_DEFINED
#endif
