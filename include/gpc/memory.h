// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file memory.h
 * @brief Automatic memory handling and arenas
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stddef.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

typedef struct gpc_Owner gpc_Owner;

#define gpc_owner(object_count, parent) \
(gpc_Owner){ object_count, parent, (void*[object_count]){NULL} }

#define gpc_owner_auto(object_count) \
gpc_owner_make_auto(&(gpc_Owner){ \
    object_count, \
    gpc_owner_default() \
    (void*[object_count]){NULL} })

void gpc_owner_free(gpc_Owner, void* return_value);

void* gpc_allocate(size_t);
void gpc_reallocate(void** ptr_to_ptr);

void* gpc_malloc_assign(gpc_Owner*, size_t);
void* gpc_calloc_assign(gpc_Owner*, size_t nmemb, size_t size);
void* gpc_realloc_assign(gpc_Owner*, size_t);

void gpc_free(void* ptr);

gpc_Owner* gpc_owner_default(void);

// -------------------------

typedef struct gpc_Arena gpc_Arena;

gpc_Arena* gpc_arena_new(size_t pool_size);
void* gpc_arena_push(gpc_Arena*, size_t allocation_size);
void* gpc_arena_pop_to(gpc_Arena*, void* arena_object);
void gpc_arena_clear(gpc_Arena*);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

gpc_Owner gpc_owner_make_auto(gpc_Owner*);

struct gpc_Owner
{
    size_t objects_capacity;
    struct gpc_Owner* parent;
    void** objects;
    void* allocation; // Don't free!
};

#endif // GPC_MEMORY_H
