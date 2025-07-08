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
#define __USE_MISC // portability related problems
#define GP_USE_MISC_DEFINED
#endif
#include <sys/mman.h>
#include <errno.h>
#else
#include <windows.h>
#endif

static GP_MAYBE_ATOMIC size_t gp_heap_allocation_count = 0;

size_t gp_heap_alloc_count(void)
{
    return gp_heap_allocation_count;
}

static void* gp_heap_alloc(GPAllocator* unused, size_t block_size, size_t alignment)
{
    (void)unused;
    ++gp_heap_allocation_count;

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
    if (mem == NULL) // Inputs should be validated by the user, so this
        abort();     // indicates something critical, don't try to 'handle' this!
    return mem;
}

static void gp_heap_dealloc(GPAllocator* unused, void* block)
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

static GPAllocator gp_mallocator = {
    .alloc   = gp_heap_alloc,
    .dealloc = gp_heap_dealloc
};
GPAllocator* gp_heap = &gp_mallocator;

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

static void* gp_arena_node_new_alloc(
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
        ASAN_POISON_MEMORY_REGION((uint8_t*)new_node->position - 8, new_node->capacity - size);
    // Poison padding possibly caused by large alginments
    ASAN_POISON_MEMORY_REGION(new_node + 1,
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
        size_t new_cap = arena->growth_coefficient * arena->head->capacity;
        block = gp_arena_node_new_alloc(arena->backing, &arena->head, new_cap, size, alignment);
    }
    else {
        ASAN_UNPOISON_MEMORY_REGION(block, size);
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
      : gp_heap;

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
    ASAN_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);

    arena->base.alloc   = gp_arena_alloc;
    arena->base.dealloc = gp_arena_dealloc;
    arena->backing = init->backing_allocator != NULL ?
        init->backing_allocator
      : gp_heap;
    arena->growth_coefficient = init->growth_coefficient != 0. ?
        init->growth_coefficient
      : 2.;
    arena->max_size = init->max_size != 0 ?
        init->max_size
      : 1 << 15;

    return arena;
}

static bool gp_in_this_node(GPArenaNode* node, void* _pos)
{
    gp_db_assert(node != NULL,
        "gp_arena_rewind(): passed pointer was not allocated by the arena.");
    uint8_t* pos = _pos;
    uint8_t* block_start = (uint8_t*)(node + 1);
    return block_start <= pos && pos <= block_start + node->capacity;
}

static size_t gp_arena_node_delete(GPAllocator* allocator, GPArenaNode** head)
{
    GPArenaNode* old_head = *head;
    size_t old_capacity = old_head->capacity;
    *head = (*head)->tail;
    gp_mem_dealloc(allocator, old_head);
    return old_capacity;
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    while ( ! gp_in_this_node(arena->head, new_pos))
        gp_arena_node_delete(arena->backing, &arena->head);

    arena->head->position = new_pos;
    if ((uint8_t*)new_pos < (uint8_t*)arena->head->position + arena->head->capacity)
        ASAN_POISON_MEMORY_REGION(new_pos,
            arena->head->memory + arena->head->capacity - (uint8_t*)new_pos);
}

size_t gp_arena_reset(GPArena* arena)
{
    size_t total_capacity = 0;
    while (arena->head->tail != NULL)
        total_capacity += gp_arena_node_delete(arena->backing, &arena->head);

    arena->head->position = arena->head->memory;
    ASAN_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);
    return total_capacity + arena->head->capacity;
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;
    while (arena->head->tail != NULL)
        gp_arena_node_delete(arena->backing, &arena->head);
    gp_mem_dealloc(arena->backing, arena->head->allocation);
}

// ----------------------------------------------------------------------------
// Scratch arena

static GPThreadKey  gp_scratch_arena_key;
static GPThreadOnce gp_scratch_arena_key_once = GP_THREAD_ONCE_INIT;

// Make Valgrind shut up.
static void gp_delete_main_thread_scratch_arena(void)
{
    gp_arena_delete(gp_thread_local_get(gp_scratch_arena_key));
}

static void gp_make_scratch_arena_key(void)
{
    atexit(gp_delete_main_thread_scratch_arena);
    gp_thread_key_create(&gp_scratch_arena_key, (void(*)(void*))gp_arena_delete);
}

static GPArena* gp_new_scratch_arena(void)
{
    GPArenaInitializer init = {
        .max_size           = GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE,
        .growth_coefficient = GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT,
    };
    GPArena* arena = gp_arena_new(&init, GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE);
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
    GPAllocator* allocator,
    void* old_block,
    size_t old_size,
    size_t new_size)
{
    gp_db_assert(old_size < SIZE_MAX/2, "Impossible size, no allocator accepts this.");
    gp_db_assert(new_size < SIZE_MAX/2, "Possibly negative allocation detected.");

    if (new_size <= old_size)
        return old_block;

    GPVirtualArena* varena = (GPVirtualArena*)allocator;
    if (allocator->dealloc == gp_virtual_dealloc && old_block != NULL &&
        (char*)old_block + old_size == (char*)varena->position)
    { // extend block instead of reallocating and copying
        varena->position = old_block;
        return gp_varena_alloc(varena, new_size, GP_ALLOC_ALIGNMENT);
    }

    GPArena* arena = (GPArena*)allocator;
    GPScope* scope = (GPScope*)allocator;
    GPArenaNode** head = allocator->alloc == gp_arena_alloc ? &arena->head : &scope->head;

    if (allocator->dealloc == gp_arena_dealloc && old_block != NULL &&
        (uint8_t*)old_block + old_size + GP_POISON_BOUNDARY_SIZE == (uint8_t*)(*head)->position)
    { // extend block instead of reallocating and copying
        (*head)->position = old_block;
        void* new_block = gp_mem_alloc(allocator, new_size);
        if (new_block != old_block) { // arena ran out of space and reallocated
            memcpy(new_block, old_block, old_size);
            ASAN_POISON_MEMORY_REGION(old_block, old_size);
        }
        return new_block;
    }
    void* new_block = gp_mem_alloc(allocator, new_size);
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

void* gp_scope_alloc(GPAllocator* allocator, const size_t size, const size_t alignment)
{
    GPScope* arena = (GPScope*)allocator;
    GPArenaNode* head = arena->head;

    void* block = head->position = (void*)gp_round_to_aligned((uintptr_t)head->position, alignment);
    if ((uint8_t*)block + size + GP_POISON_BOUNDARY_SIZE > (uint8_t*)(head + 1) + arena->head->capacity)
    { // out of memory, create new arena
        block = gp_arena_node_new_alloc(
            gp_heap, &arena->head, 2*arena->head->capacity, size, alignment);
    }
    else {
        ASAN_UNPOISON_MEMORY_REGION(block, size);
        head->position = (uint8_t*)block + size + GP_POISON_BOUNDARY_SIZE;
    }
    return block;
}

static GPThreadKey  gp_scope_list_key;
static GPThreadOnce gp_scope_list_key_once = GP_THREAD_ONCE_INIT;

GPScope* gp_last_scope(void)
{
    return gp_thread_local_get(gp_scope_list_key);
}

static void gp_scope_execute_defers(GPScope* scope)
{
    if (scope->defer_stack != NULL) {
        for (size_t i = scope->defer_stack->length - 1; i != (size_t)-1; --i) {
            scope->defer_stack->stack[i].f(scope->defer_stack->stack[i].arg);
        }
    }
}

static void gp_delete_thread_scopes(void*_scopes)
{
    GPScope* scope = _scopes;
    while (scope != NULL) {
        gp_scope_execute_defers(scope);
        GPScope* parent = scope->parent;

        while (scope->head->tail != NULL)
            gp_arena_node_delete(gp_heap, &scope->head);
        gp_mem_dealloc(gp_heap, scope);

        scope = parent;
    }
}

static void gp_delete_main_thread_scopes(void)
{
    gp_delete_thread_scopes(gp_thread_local_get(gp_scope_list_key));
}

static void gp_make_scope_list_key(void)
{
    atexit(gp_delete_main_thread_scopes);
    gp_thread_key_create(&gp_scope_list_key, gp_delete_thread_scopes);
}

static GPScope* gp_scope_new(size_t capacity)
{
    GPScope* arena;

    capacity = capacity != 0 ? gp_round_to_aligned(capacity, GP_ALLOC_ALIGNMENT) : 256;
    capacity += GP_POISON_BOUNDARY_SIZE;
    arena = gp_mem_alloc(gp_heap, sizeof*arena + sizeof(GPArenaNode) + capacity);
    arena->head = (GPArenaNode*)(arena + 1);
    arena->head->capacity = capacity;

    arena->head->position = arena->head->memory;
    arena->head->tail     = NULL;
    ASAN_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);

    arena->base.alloc   = gp_scope_alloc;
    arena->base.dealloc = gp_arena_dealloc;

    return arena;
}

GPScope* gp_begin(const size_t _size)
{
    gp_thread_once(&gp_scope_list_key_once, gp_make_scope_list_key);

    const size_t size = _size == 0 ?
        (size_t)GP_SCOPE_DEFAULT_INIT_SIZE
      : _size;

    GPScope* scope = gp_scope_new(size/* + sizeof*scope*/);
    scope->defer_stack = NULL;
    scope->parent = gp_thread_local_get(gp_scope_list_key);
    gp_thread_local_set(gp_scope_list_key, scope);

    return scope;
}

size_t gp_end(GPScope* scope)
{
    if (scope == NULL)
        return 0;

    GPScope* child = gp_thread_local_get(gp_scope_list_key);
    while (child != scope) {
        GPScope* parent = child->parent;
        gp_scope_execute_defers(child);

         while (child->head->tail != NULL)
             gp_arena_node_delete(gp_heap, &child->head);
        gp_mem_dealloc(gp_heap, child);

        child = parent;
    }
    GPScope* parent = scope->parent;
    gp_scope_execute_defers(child);

    size_t scope_size = 0;
    while (scope->head->tail != NULL)
        scope_size += gp_arena_node_delete(gp_heap, &scope->head);

    gp_mem_dealloc(gp_heap, scope);
    gp_thread_local_set(gp_scope_list_key, parent);
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
        GPDefer* old_stack  = scope->defer_stack->stack;
        scope->defer_stack->stack = gp_scope_alloc(&scope->base,
            scope->defer_stack->capacity * 2 * sizeof(GPDefer), GP_ALLOC_ALIGNMENT);
        memcpy(scope->defer_stack->stack, old_stack,
            scope->defer_stack->length * sizeof(GPDefer));
        scope->defer_stack->capacity *= 2;
    }
    scope->defer_stack->stack[scope->defer_stack->length].f   = f;
    scope->defer_stack->stack[scope->defer_stack->length].arg = arg;
    scope->defer_stack->length++;
}

// ----------------------------------------------------------------------------
// Virtual Arena

GPAllocator* gp_varena_init(GPVirtualArena* alc, size_t size)
{
    gp_db_assert(size != 0, "%zu", size);
    gp_db_assert(size < SIZE_MAX/2, "%zu", size, "Possibly negative size detected.");
    gp_db_expect(size >= 4096, "%zu", size,
        "Virtual allocator is supposed to be used with HUGE arenas. "
        "Are you sure you are allocating enough?");

    #ifdef _SC_PAGE_SIZE
    const size_t page_size = sysconf(_SC_PAGE_SIZE);
    #elif defined(_WIN32)
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    const size_t page_size = sys_info.dwPageSize;
    #else
    const size_t page_size = 4096;
    #endif
    size = gp_round_to_aligned(size, page_size);

    #if _WIN32
    alc->start = alc->position = VirtualAlloc(
        NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    gp_db_expect(alc->start != NULL, "VirtualAlloc():", "%lu", GetLastError());
    #else
    alc->start = alc->position = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1, 0);
    gp_db_expect(alc->start != NULL && alc->start != (void*)-1, "mmap():", "%s", strerror(errno));
    #endif

    if (alc->start == NULL || alc->start == (void*)-1)
        return alc->start = alc->position = NULL;

    alc->base.alloc   = (void*(*)(GPAllocator*,size_t,size_t))gp_varena_alloc;
    alc->base.dealloc = gp_virtual_dealloc;
    alc->capacity = size;
    return (GPAllocator*)alc;
}

void gp_varena_reset(GPVirtualArena* arena)
{
    arena->position = arena->start;

    #if _WIN32
    VirtualAlloc(arena->start, 0, MEM_RESET, PAGE_READWRITE);
    #else
    madvise(arena->start, arena->capacity, MADV_DONTNEED);
    #endif
}

void gp_varena_delete(GPVirtualArena* arena)
{
    if (arena == NULL)
        return;

    #if _WIN32
    BOOL VirtualFree_result = VirtualFree(arena->start, 0, MEM_RELEASE);
    gp_db_expect(VirtualFree_result != 0, "%lu", GetLastError());
    #else
    int munmap_result = munmap(arena->start, arena->capacity);
    gp_db_expect(munmap_result != -1, "%s", strerror(errno));
    #endif

    arena->start = arena->position = NULL;
    arena->capacity = 0;
}

// ----------------------------------------------------------------------------
// Mutex Allocator

static void* gp_mutex_alloc(GPAllocator*_alc, size_t size, size_t alignment)
{
    GPMutexAllocator* alc = (GPMutexAllocator*)_alc;
    gp_mutex_lock(&alc->mutex);
    void* ptr = gp_mem_alloc_aligned(alc->backing, size, alignment);
    gp_mutex_unlock(&alc->mutex);
    return ptr;
}

static void gp_mutex_dealloc(GPAllocator*_alc, void* ptr)
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
    alc->base.alloc   = gp_mutex_alloc;
    alc->base.dealloc = gp_mutex_dealloc;
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
