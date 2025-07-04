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

// Instances of these live in the beginning of the arenas memory block so the
// first object is in &node + 1;
typedef struct gp_arena_node
{
    void* position;
    struct gp_arena_node* tail;
    size_t capacity;
    void* _padding; // to round size to aligment boundary and for future use
} GPArenaNode;

void* gp_arena_alloc(GPAllocator* allocator, const size_t size, const size_t alignment)
{
    GPArena* arena = (GPArena*)allocator;
    GPArenaNode* head = arena->head;
    const size_t size_with_poison = size + 8*GP_HAS_SANITIZER;

    void* block = head->position = (void*)gp_round_to_aligned((uintptr_t)head->position, alignment);
    if ((uint8_t*)block + size_with_poison > (uint8_t*)(head + 1) + arena->head->capacity)
    { // out of memory, create new arena
        size_t new_cap = arena->growth_coefficient * arena->head->capacity;
        new_cap = gp_min(new_cap, arena->max_size);
        GPArenaNode* new_node = gp_mem_alloc(arena->allocator,
            gp_round_to_aligned(sizeof(GPArenaNode), alignment)
            + gp_max(new_cap, size_with_poison)
            + alignment - GP_ALLOC_ALIGNMENT
        );
        new_node->tail     = head;
        new_node->capacity = new_cap;

        block = new_node->position = (void*)gp_round_to_aligned(
            (uintptr_t)(new_node + 1), alignment);
        new_node->position = (uint8_t*)(new_node->position) + size_with_poison;
        arena->head = new_node;
        if (new_node->capacity > size) // -8 for poison boundary
            ASAN_POISON_MEMORY_REGION((uint8_t*)new_node->position - 8, new_node->capacity - size);
        // Poison padding possibly caused by large alginments
        ASAN_POISON_MEMORY_REGION(new_node + 1,
            gp_round_to_aligned((uintptr_t)(new_node + 1), alignment) - (uintptr_t)(new_node + 1));
    }
    else {
        ASAN_UNPOISON_MEMORY_REGION(block, size);
        head->position = (uint8_t*)block + size_with_poison;
    }
    return block;
}

static void* gp_arena_shared_alloc(
    GPAllocator* allocator, const size_t size, const size_t alignment)
{
    gp_mutex_lock(((GPArena*)allocator)->is_shared);
    void* block = gp_arena_alloc(allocator, size, alignment);
    gp_mutex_unlock(((GPArena*)allocator)->is_shared);
    return block;
}

GPAllocator* gp_arena_init(GPArena* arena, const size_t capacity)
{
    const size_t cap = (capacity != 0 ?
        gp_round_to_aligned(capacity, GP_ALLOC_ALIGNMENT)
      : 256) + 8*GP_HAS_SANITIZER;
    if (arena->allocator == NULL)
        arena->allocator = gp_heap;
    GPArenaNode* node = gp_mem_alloc(arena->allocator, sizeof(GPArenaNode) + cap);
    node->position = node + 1;
    node->tail     = NULL;
    node->capacity = cap;
    ASAN_POISON_MEMORY_REGION(node->position, node->capacity);
    arena->head = node;
    if (arena->growth_coefficient == 0.)
        arena->growth_coefficient = 2.;
    if (arena->max_size == 0)
        arena->max_size = 1 << 15;
    if (arena->is_shared) {
        arena->is_shared = gp_mem_alloc(arena->allocator, sizeof(GPMutex));
        gp_mutex_init(arena->is_shared);
        arena->base.alloc = gp_arena_shared_alloc;
    } else if (arena->base.alloc == NULL)
        arena->base.alloc = gp_arena_alloc;
    arena->base.dealloc = gp_arena_dealloc;

    return (GPAllocator*)arena;
}

static bool gp_in_this_node(GPArenaNode* node, void* _pos)
{
    gp_assert(node != NULL,
        "gp_arena_rewind(): passed pointer was not allocated by the arena.");
    uint8_t* pos = _pos;
    uint8_t* block_start = (uint8_t*)(node + 1);
    return block_start <= pos && pos <= block_start + node->capacity;
}

static size_t gp_arena_node_delete(GPArena* arena)
{
    GPArenaNode* old_head = arena->head;
    size_t old_capacity = old_head->capacity;
    arena->head = arena->head->tail;
    gp_mem_dealloc(arena->allocator, old_head);
    return old_capacity;
}

void gp_arena_rewind(GPArena* arena, void* new_pos)
{
    while ( ! gp_in_this_node(arena->head, new_pos))
        gp_arena_node_delete(arena);
    arena->head->position = new_pos;
    if ((uint8_t*)new_pos < (uint8_t*)arena->head->position + arena->head->capacity)
        ASAN_POISON_MEMORY_REGION(new_pos,
            (uint8_t*)(arena->head + 1) + arena->head->capacity - (uint8_t*)new_pos);
}

size_t gp_arena_reset(GPArena* arena)
{
    size_t total_capacity = 0;
    while (arena->head->tail != NULL)
        total_capacity += gp_arena_node_delete(arena);
    arena->head->position = arena->head + 1;
    ASAN_POISON_MEMORY_REGION(arena->head->position, arena->head->capacity);
    return total_capacity + arena->head->capacity;
}

void gp_arena_delete(GPArena* arena)
{
    if (arena == NULL)
        return;

    size_t total_capacity = 0;
    while (arena->head != NULL)
        total_capacity += gp_arena_node_delete(arena);

    if (arena->is_shared)
        gp_mem_dealloc(arena->allocator, (GPMutex*)arena->is_shared);
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
    GPArena* arena = gp_mem_alloc_zeroes(gp_heap, sizeof*arena);
    #else
    GPArena* arena = &gp_scratch_allocator;
    #endif
    arena->max_size           = GP_SCRATCH_ARENA_DEFAULT_MAX_SIZE;
    arena->growth_coefficient = GP_SCRATCH_ARENA_DEFAULT_GROWTH_COEFFICIENT;
    gp_arena_init(arena, GP_SCRATCH_ARENA_DEFAULT_INIT_SIZE);
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
        return gp_virtual_alloc(varena, new_size, GP_ALLOC_ALIGNMENT);
    }

    GPArena* arena = (GPArena*)allocator;
    if (allocator->dealloc == gp_arena_dealloc && old_block != NULL &&
        (char*)old_block + old_size + 8*GP_HAS_SANITIZER == (char*)arena->head->position)
    { // extend block instead of reallocating and copying
        arena->head->position = old_block;
        void* new_block = gp_arena_alloc(allocator, new_size, GP_ALLOC_ALIGNMENT);
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
// Scope allocator

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

typedef struct gp_scope
{
    GPArena          arena;
    struct gp_scope* parent;
    GPDeferStack*    defer_stack;
} GPScope;

typedef struct gp_scope_factory
{
    GPArena arena;
    struct gp_scope* last_scope;
} GPScopeFactory;

static GPThreadKey  gp_scope_factory_key;
static GPThreadOnce gp_scope_factory_key_once = GP_THREAD_ONCE_INIT;

GP_NO_FUNCTION_POINTER_SANITIZE
static size_t gp_end_scopes(GPScope* scope, GPScope*const last_to_be_ended)
{
    if (scope == NULL)
        return 0;
    if (scope->defer_stack != NULL) {
        for (size_t i = scope->defer_stack->length - 1; i != (size_t)-1; --i) {
            scope->defer_stack->stack[i].f(scope->defer_stack->stack[i].arg);
        }
    }
    GPScope* previous = scope->parent;
    size_t scope_size = gp_arena_reset(&scope->arena);
    gp_arena_delete(&scope->arena);
    if (scope != last_to_be_ended)
        return gp_end_scopes(previous, last_to_be_ended);
    return scope_size;
}

GPAllocator* gp_last_scope(GPAllocator* fallback)
{
    GPScopeFactory* factory = gp_thread_local_get(gp_scope_factory_key);
    if (factory == NULL || factory->last_scope == NULL)
        return (GPAllocator*)fallback;
    return (GPAllocator*)factory->last_scope;
}

static void gp_delete_scope_factory(void*_factory)
{
    GPScopeFactory* factory = _factory;
    gp_end_scopes(factory->last_scope, NULL);
    gp_mem_dealloc(gp_heap, factory->arena.head);
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

static void* gp_scope_alloc(GPAllocator* scope, size_t size, size_t alignment)
{
    return gp_arena_alloc(scope, size, alignment);
}

GPScopeFactory* gp_new_scope_factory(void)
{
    const size_t nested_scopes = 64; // before reallocation
    GPArena scope_factory_arena = {0};
    gp_arena_init(&scope_factory_arena,
        (nested_scopes + 1/*self*/) * gp_round_to_aligned(sizeof(GPScope), GP_ALLOC_ALIGNMENT));

    // Extend lifetime
    GPScopeFactory* scope_factory = gp_arena_alloc(
        (GPAllocator*)&scope_factory_arena, sizeof*scope_factory, GP_ALLOC_ALIGNMENT);
    memset(scope_factory, 0, sizeof*scope_factory);
    scope_factory->arena = scope_factory_arena;

    gp_thread_local_set(gp_scope_factory_key, scope_factory);
    return scope_factory;
}

GPAllocator* gp_begin(const size_t _size)
{
    gp_thread_once(&gp_scope_factory_key_once, gp_make_scope_factory_key);

    GPScopeFactory* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    if (GP_UNLIKELY(scope_factory == NULL))
        scope_factory = gp_new_scope_factory();

    const size_t size = _size == 0 ?
        (size_t)GP_SCOPE_DEFAULT_INIT_SIZE
      : _size;
    GPScope* scope = gp_mem_alloc_zeroes((GPAllocator*)scope_factory, sizeof*scope);
    scope->arena.base.alloc   = gp_scope_alloc;
    scope->arena.max_size           = GP_SCOPE_DEFAULT_MAX_SIZE;
    scope->arena.growth_coefficient = GP_SCOPE_DEFAULT_GROWTH_COEFFICIENT;
    gp_arena_init(&scope->arena, size);
    scope->parent = scope_factory->last_scope;
    scope->defer_stack = NULL;
    scope_factory->last_scope = scope;

    return (GPAllocator*)scope;
}

size_t gp_end(GPAllocator*_scope)
{
    if (_scope == NULL)
        return 0;
    GPScope* scope = (GPScope*)_scope;

    GPScope* previous = scope->parent;
    GPScopeFactory* scope_factory = gp_thread_local_get(gp_scope_factory_key);
    size_t scope_size = gp_end_scopes(scope_factory->last_scope, scope);
    scope_factory->last_scope = previous;
    gp_arena_rewind(&scope_factory->arena, scope);
    return scope_size;
}

void gp_scope_defer(GPAllocator*_scope, void (*f)(void*), void* arg)
{
    GPScope* scope = (GPScope*)_scope;
    if (scope->defer_stack == NULL)
    {
        const size_t init_cap = 4;
        scope->defer_stack = gp_arena_alloc((GPAllocator*)scope,
            sizeof*(scope->defer_stack) + init_cap * sizeof(GPDefer), GP_ALLOC_ALIGNMENT);

        scope->defer_stack->length   = 0;
        scope->defer_stack->capacity = init_cap;
        scope->defer_stack->stack    = (GPDefer*)(scope->defer_stack + 1);
    }
    else if (scope->defer_stack->length == scope->defer_stack->capacity)
    {
        GPDefer* old_stack  = scope->defer_stack->stack;
        scope->defer_stack->stack = gp_arena_alloc((GPAllocator*)scope,
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

GPAllocator* gp_virtual_init(GPVirtualArena* alc, size_t size)
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

    alc->base.alloc   = (void*(*)(GPAllocator*,size_t,size_t))gp_virtual_alloc;
    alc->base.dealloc = gp_virtual_dealloc;
    alc->capacity = size;
    return (GPAllocator*)alc;
}

void gp_virtual_reset(GPVirtualArena* arena)
{
    arena->position = arena->start;

    #if _WIN32
    VirtualAlloc(arena->start, 0, MEM_RESET, PAGE_READWRITE);
    #else
    madvise(arena->start, arena->capacity, MADV_DONTNEED);
    #endif
}

void gp_virtual_delete(GPVirtualArena* arena)
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

#ifdef GP_USE_MISC_DEFINED
#undef __USE_MISC
#undef GP_USE_MISC_DEFINED
#endif
