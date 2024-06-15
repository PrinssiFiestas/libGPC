// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include "memory.h"
#include "attributes.h"
#include "overload.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

//
#define GPArray(T) T*

typedef struct gp_array_header
{
    size_t length;
    size_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPArrayHeader;

#define GP_ARR_ATTRS(...) \
    GP_NONNULL_RETURN GP_NODISCARD GP_NONNULL_ARGS(__VA_ARGS__)

GP_ARR_ATTRS()
GPArray(void) gp_arr_new(
    const GPAllocator*,
    size_t element_size,
    size_t element_count);

#define/* GPArray(T) */gp_arr_on_stack( \
    optional_allocator_ptr, \
    size_t_capacity, \
    T, ...) \
(struct GP_C99_UNIQUE_STRUCT(__LINE__) \
{ GPArrayHeader header; T data[size_t_capacity]; }) { \
{ \
    .length     = sizeof((T[]){__VA_ARGS__})/sizeof(T), \
    .capacity   = size_t_capacity, \
    .allocator  = optional_allocator_ptr, \
    .allocation = NULL \
}, {__VA_ARGS__} }.data

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, arrays can be created on stack manually. Example with int:
/*
    struct optional_name { GPArrayHeader header; int data[2048]; } my_array_mem;
    my_array_mem.header = (GPArrayHeader) {.capacity = 2048 };
    GPArray(int) my_array = my_array_mem.data;
*/

// Passing arrays on stack is safe too.
void gp_arr_delete(GPArray(void) optional);

size_t             gp_arr_length    (const GPArray(void)) GP_NONNULL_ARGS();
size_t             gp_arr_capacity  (const GPArray(void)) GP_NONNULL_ARGS();
void*              gp_arr_allocation(const GPArray(void)) GP_NONNULL_ARGS();
const GPAllocator* gp_arr_allocator (const GPArray(void)) GP_NONNULL_ARGS();

GP_ARR_ATTRS()
GPArray(void) gp_arr_reserve(
    size_t        element_size,
    GPArray(void) arr,
    size_t        capacity);

GP_ARR_ATTRS()
GPArray(void) gp_arr_copy(
    size_t              element_size,
    GPArray(void)       dest,
    const void*restrict src,
    size_t              src_length);

GP_ARR_ATTRS(2)
GPArray(void) gp_arr_slice(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              start_index,
    size_t              end_index);

GP_ARR_ATTRS()
GPArray(void) gp_arr_push(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict element);

GP_NONNULL_ARGS_AND_RETURN
void* gp_arr_pop(
    size_t        element_size,
    GPArray(void) arr);

GP_ARR_ATTRS()
GPArray(void) gp_arr_append(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict src,
    size_t              element_count);

GP_ARR_ATTRS()
GPArray(void) gp_arr_insert(
    size_t              element_size,
    GPArray(void)       arr,
    size_t              pos,
    const void*restrict src,
    size_t              element_count);

GP_ARR_ATTRS()
GPArray(void) gp_arr_erase(
    size_t        element_size,
    GPArray(void) arr,
    size_t        pos,
    size_t        count);

GP_NONNULL_RETURN GP_NONNULL_ARGS(2, 5)
GPArray(void) gp_arr_map(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              src_length,
    void (*f)(void* out, const void* in));

GP_NONNULL_ARGS(2, 4)
void* gp_arr_fold(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element));

GP_NONNULL_ARGS(2, 4)
void* gp_arr_foldr(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element));

GP_ARR_ATTRS(2, 5)
GPArray(void) gp_arr_filter(
    size_t              element_size,
    GPArray(void)       arr,
    const void*restrict optional_src, // mutates arr if NULL
    size_t              src_length,
    bool (*f)(const void* element));

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#ifdef _MSC_VER
// unnamed struct in parenthesis in gp_arr_on_stack()
#pragma warning(disable : 4116)
// sizeof returns 0 when creating an empty array using gp_arr_on_stack()
#pragma warning(disable : 4034)
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPC_ARRAY_H
