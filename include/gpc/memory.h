// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file memory.h
 * @brief Automatic memory handling and arenas
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include "attributes.h"
#include <stddef.h>
#include <stdbool.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

//
typedef struct gpc_Owner gpc_Owner;

#define gpc_owner(object_count, parent) \
(gpc_Owner){ object_count, parent, (void*[object_count]){NULL} }

#define gpc_owner_auto(object_count) \
gpc_owner_make_auto(&(gpc_Owner){ \
    object_count, \
    gpc_owner_default() \
    (void*[object_count]){NULL} })

void gpc_owner_free(gpc_Owner, void* return_value);

extern GPC_THREAD_LOCAL gpc_Owner* gpc_g_owner;

#define GPC_NO_OWNER ((gpc_Owner*)0)

GPC_NODISCARD void* gpc_allocate(gpc_Owner*, size_t bytes);
GPC_NODISCARD void* gpc_allocate_zeroed(gpc_Owner*, size_t bytes);
GPC_NODISCARD void* gpc_reallocate(void* ptr, size_t new_size);
void gpc_deallocate(void* ptr);

void* gpc_make_managed(gpc_Owner*, void* ptr);

// -------------------------

typedef struct gpc_Arena gpc_Arena;

gpc_Arena* gpc_arena_new(size_t pool_size);
void* gpc_arena_push(gpc_Arena*, size_t allocation_size);
void* gpc_arena_pop_to(gpc_Arena*, void* arena_object);
void gpc_arena_clear(gpc_Arena*);

#ifdef GPC_DEBUG_MEMORY

#define gpc_allocate(owner, bytes) \
gpc_db_allocate(__FILE__, __LINE__, __func__, #owner, owner, bytes)

#define gpc_allocate_zeroed(owner, bytes) \
gpc_db_allocate_zeroed(__FILE__, __LINE__, __func__, #owner, owner, bytes)

#define gpc_reallocate(ptr, new_size) \
gpc_db_reallocate(__FILE__, __LINE_, __func__, #ptr, ptr, new_size)

#define gpc_deallocate(ptr) gpc_db_deallocate(ptr)

#define gpc_arena_new(pool_size) \
gpc_db_arena_new(__FILE__, __LINE__, __func__,  pool_size)

#endif // GPC_DEBUG_MEMORY

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

//
gpc_Owner gpc_owner_make_auto(gpc_Owner*);

struct gpc_Owner
{
    size_t objects_capacity;
    const struct gpc_Owner* parent;
    void** objects;
    size_t pos;
};

// ------------------------
// Debug functions
// var_name usually comes from functions calling these

GPC_NODISCARD void* gpc_db_allocate(
    const char* file, int line, const char* func, const char* var_name,
    gpc_Owner*, size_t bytes);

GPC_NODISCARD void* gpc_db_allocate_zeroed(
    const char* file, int line, const char* func, const char* var_name,
    gpc_Owner*, size_t bytes);

GPC_NODISCARD void* gpc_db_reallocate(
    const char* file, int line, const char* func, const char* var_name,
    void* ptr, size_t new_size);

void gpc_db_deallocate(void* ptr);

gpc_Arena* gpc_db_arena_new(
    const char* file, int line, const char* func, const char* var_name,
    size_t pool_size);

#endif // GPC_MEMORY_H
