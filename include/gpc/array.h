// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include <gpc/memory.h>
#include <gpc/attributes.h>
#include <gpc/overload.h>
#include <stdint.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

#define GPArray(T) T*

#define GP_ARR_ATTRS(...) \
    GP_NONNULL_RETURN GP_NODISCARD GP_NONNULL_ARGS(__VA_ARGS__)

GPArray(void) gp_arr_new(
    GPAllocator*,
    size_t element_size,
    size_t element_count) GP_ARR_ATTRS();

void gp_arr_delete(GPArray(void) optional);

size_t             gp_length    (const void*) GP_NONNULL_ARGS();
size_t             gp_capacity  (const void*) GP_NONNULL_ARGS();
void*              gp_allocation(const void*) GP_NONNULL_ARGS();
const GPAllocator* gp_allocator (const void*) GP_NONNULL_ARGS();

GPArray(void) gp_arr_reserve(
    size_t        element_size,
    GPArray(void) arr,
    size_t        capacity) GP_ARR_ATTRS();

GPArray(void) gp_arr_copy(
    size_t              element_size,
    GPArray(void)       dest,
    const void*restrict src,
    size_t              src_length) GP_ARR_ATTRS();

GPArray(void) gp_arr_slice(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              start_index,
    size_t              end_index) GP_ARR_ATTRS(2);

GPArray(void) gp_arr_push(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict element) GP_ARR_ATTRS();

void* gp_arr_pop(
    size_t        element_size,
    GPArray(void) arr) GP_NONNULL_ARGS_AND_RETURN;

GPArray(void) gp_arr_append(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict src,
    size_t              element_count) GP_ARR_ATTRS();

GPArray(void) gp_arr_insert(
    size_t              element_size,
    GPArray(void)       arr,
    size_t              pos,
    const void*restrict src,
    size_t              element_count) GP_ARR_ATTRS();

GPArray(void) gp_arr_remove(
    size_t        element_size,
    GPArray(void) arr,
    size_t        pos,
    size_t        count) GP_NONNULL_ARGS_AND_RETURN;

GPArray(void) gp_arr_map(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              src_length,
    void (*f)(void* out, const void* in)) GP_ARR_ATTRS(2, 5);

void* gp_arr_fold(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element)) GP_NONNULL_ARGS(2, 4);

void* gp_arr_foldr(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element)) GP_NONNULL_ARGS(2, 4);

GPArray(void) gp_arr_filter(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              src_length,
    bool (*f)(const void* element)) GP_ARR_ATTRS(2, 5);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#ifndef GP_STRING_INCLUDED
typedef struct gp_array_header
{
    uintptr_t length;
    uintptr_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPArrayHeader;
#endif

#endif // GPC_ARRAY_H
