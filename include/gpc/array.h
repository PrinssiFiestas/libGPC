// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file array.h
 * Dynamic array.
 */

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include "memory.h"
#include "attributes.h"
#include "overload.h"

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


/** Dynamic array of any type.
 * In memory, a dynamic array is GPArrayHeader followed by the elements. An
 * object of type GPArray(T) is a pointer to the first element. Use this macro
 * to differentiate between any other type of pointer.
 */
#define GPArray(T) T*

/** Array meta-data.
 * You can edit the fields directly with ((GPArrayHeader)my_array - 1)->field.
 * This might be useful for micro-optimizations, but it is mostly recommended to
 * use the provided functions instead.
 */
typedef struct gp_array_header
{
    size_t length;
    size_t capacity;
    const GPAllocator* allocator;
    void* allocation; // pointer to self or NULL if on stack
} GPArrayHeader;

#define GP_ARR_ATTRS(...) \
    GP_NONNULL_RETURN GP_NODISCARD GP_NONNULL_ARGS(__VA_ARGS__)

/** Create a new empty array.*/
GP_ARR_ATTRS()
GPArray(void) gp_arr_new(
    const GPAllocator*,
    size_t element_size,
    size_t element_count);

/** Create a new dynamic array on stack.
 * @p allocator determines how the array will be reallocated if length exceeds
 * capacity. If it is known that length will not exceed capacity, @p allocator
 * can be left NULL. Note that @p init_values must have at least one element if
 * compiling with -Wpedantic.
 * Not available in C++.
 */
#define/* GPArray(T) */gp_arr_on_stack( \
    optional_allocator, \
    size_t_capacity, \
    T/*type*/, \
    /*init_values*/...) \
    \
    GP_ARR_ON_STACK(optional_allocator, size_t_capacity, T,__VA_ARGS__)

// If not zeroing memory for performance is desirable and/or macro magic is
// undesirable, arrays can be created on stack manually. This is required in C++
// which does not have gp_arr_on_stack(). Example with int:
/*
    struct optional_name { GPArrayHeader header; int data[2048]; } my_array_mem;
    memset(&my_array_mem.header, 0, sizeof my_array_mem.header);
    my_array_mem.header.capacity = 2048;
    GPArray(int) my_array = my_array_mem.data;
*/

/** Getters */
size_t             gp_arr_length    (const GPArray(void)) GP_NONNULL_ARGS();
size_t             gp_arr_capacity  (const GPArray(void)) GP_NONNULL_ARGS();
void*              gp_arr_allocation(const GPArray(void)) GP_NONNULL_ARGS();
const GPAllocator* gp_arr_allocator (const GPArray(void)) GP_NONNULL_ARGS();

/** Free array memory.
 * Passing arrays on stack is safe too.
 */
inline void gp_arr_delete(GPArray(void) optional)
{
    if (optional != NULL && gp_arr_allocation(optional) != NULL)
        gp_mem_dealloc(gp_arr_allocator(optional), gp_arr_allocation(optional));
}

/** Free array memory trough pointer.
 * The parameter should be of type GPArray(T)*.
 * This should be used as destructor for GPDictionary(GPArray(T)) if needed.
 */
inline void gp_arr_ptr_delete(void* optional)
{
    if (optional != NULL)
        gp_arr_delete(*(GPArray(void)*)optional);
}

/** Reserve capacity.
 * If @p capacity > gp_arr_capacity(@p arr), reallocates, does nothing
 * otherwise.
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_ARR_ATTRS()
GPArray(void) gp_arr_reserve(
    size_t        element_size,
    GPArray(void) arr,
    size_t        capacity);

/** Copy source array to destination.
 * @return possibly reallocated @p dest which should be assigned to @p dest.
 */
GP_ARR_ATTRS()
GPArray(void) gp_arr_copy(
    size_t                 element_size,
    GPArray(void)          dest,
    const void*GP_RESTRICT src,
    size_t                 src_length);

/** Copy or remove elements.
 * Copies elements from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, elements from @p arr outside
 * @p start_index and @p end_index are removed and the remaining elements are
 * moved over.
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_ARR_ATTRS(2)
GPArray(void) gp_arr_slice(
    size_t                 element_size,
    GPArray(void)          arr,
    const void*GP_RESTRICT optional_src,
    size_t                 start_index,
    size_t                 end_index);

/** Add element to the end.
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_ARR_ATTRS()
GPArray(void) gp_arr_push(
    size_t                 element_size,
    GPArray(void)          arr,
    const void*GP_RESTRICT element);

/** Remove element from the end.
 * If @p arr is empty, the behavior is undefined.
 * @return a pointer to the last element, which is valid as long as no new
 * elements are added to @p arr. It is recommended to immediately dereference
 * and assign the return value to a variable.
 */
GP_NONNULL_ARGS_AND_RETURN
void* gp_arr_pop(
    size_t        element_size,
    GPArray(void) arr);

/** Add elements to the end.
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_ARR_ATTRS()
GPArray(void) gp_arr_append(
    size_t                 element_size,
    GPArray(void)          arr,
    const void*GP_RESTRICT src,
    size_t                 element_count);

/** Add elements to specified position.
 * Moves rest of the array over to make room for added elements.
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_ARR_ATTRS()
GPArray(void) gp_arr_insert(
    size_t                 element_size,
    GPArray(void)          arr,
    size_t                 pos,
    const void*GP_RESTRICT src,
    size_t                 element_count);

/** Remove elements.
 * Removes @p count elements starting from @p pos moving the rest of the
 * elements over.
 * @return @p arr.
 */
GP_NONNULL_ARGS_AND_RETURN
GPArray(void) gp_arr_erase(
    size_t        element_size,
    GPArray(void) arr,
    size_t        pos,
    size_t        count);

/** Apply function to elements.
 * Calls @f for all elements in source array. @p in will point to the element
 * in the source array and @p out will point to the corresponding element at
 * @p arr. If @p src is NULL, the source array will be @p arr. Reallocates if
 * gp_arr_capacity(@p arr) < gp_arr_length(@p src).
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_NONNULL_RETURN GP_NONNULL_ARGS(2, 5)
GPArray(void) gp_arr_map(
    size_t                 element_size,
    GPArray(void)          arr,
    const void*GP_RESTRICT optional_src,
    size_t                 src_length,
    void (*f)(void* out, const void* in));

/** Combine elements from left to right.
 * Combine all elements in @p arr to @p accumulator using @p f. @p f returns
 * @p accumulator which will be assigned to the original @p accumulator.
 * This allows for reallocating @p accumulator in @p f if necessary.
 * @return @p accumulator which might be necessary to assign to @p accumulator
 * in case of reallocations.
 */
GP_NONNULL_ARGS(2, 4)
void* gp_arr_fold(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element));

/** Combine elements from right to left.
 * Combine all elements in @p arr to @p accumulator using @p f. @p f returns
 * @p accumulator which will be assigned to the original @p accumulator.
 * This allows for reallocating @p accumulator in @p f if necessary.
 * @return @p accumulator which might be necessary to assign to @p accumulator
 * in case of reallocations.
 */
GP_NONNULL_ARGS(2, 4)
void* gp_arr_foldr(
    size_t              elem_size,
    const GPArray(void) arr,
    void*               accumulator,
    void* (*f)(void* accumulator, const void* element));

/** Copies elements conditionally.
 * Copies all elements from @p src that make @p f return `true` when passed to
 * @p f. If @p src is NULL, removes all elements from @p arr that make @p f
 * return `false` when passed to @p f.
 * @return possibly reallocated @p arr which should be assigned to @p arr.
 */
GP_ARR_ATTRS(2, 5)
GPArray(void) gp_arr_filter(
    size_t                 element_size,
    GPArray(void)          arr,
    const void*GP_RESTRICT optional_src, // mutates arr if NULL
    size_t                 src_length,
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

#ifdef __clang__ // Allow {0} for any type
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifndef __cplusplus

#ifndef GP_PEDANTIC
#define/* GPArray(T) */GP_ARR_ON_STACK( \
    optional_allocator_ptr, \
    size_t_capacity, \
    T, ...) \
(struct GP_C99_UNIQUE_STRUCT(__LINE__) \
{ GPArrayHeader header; T data[size_t_capacity]; }) { \
{ \
    .length     = sizeof((T[]){(T){0},__VA_ARGS__}) / sizeof(T) - 1, \
    .capacity   = size_t_capacity, \
    .allocator  = optional_allocator_ptr, \
    .allocation = NULL \
}, {__VA_ARGS__} }.data
#else
#include <string.h>
#define/* GPArray(T) */GP_ARR_ON_STACK( \
    optional_allocator_ptr, \
    size_t_capacity, \
    T, ...) \
memcpy( \
    (struct GP_C99_UNIQUE_STRUCT(__LINE__) \
    { GPArrayHeader header; T data[size_t_capacity]; }) { \
    { \
        .length     = sizeof((T[]){(T){0},__VA_ARGS__}) / sizeof(T) - 1, \
        .capacity   = size_t_capacity, \
        .allocator  = optional_allocator_ptr, \
        .allocation = NULL \
    }, {0} }.data, \
    (T[]){(T){0},__VA_ARGS__} + 1, \
    sizeof((T[]){(T){0},__VA_ARGS__}) - sizeof(T))
#endif // GP_PEDANTIC

#else // __cplusplus
} // extern "C"
#endif

#endif // GPC_ARRAY_H
