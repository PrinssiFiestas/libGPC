// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file array.h
 * Dynamic array
 */

// TODO more assertions

#ifndef GP_ARRAY_INCLUDED
#define GP_ARRAY_INCLUDED 1

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


/** Dynamic array of a specific type.
 * In memory, a dynamic array is GPArrayHeader followed by the elements. An
 * object of type GPArray(T) is a pointer to the first element. Use this macro
 * to differentiate between any other type of pointer.
 */
#define GPArray(/* Type */...) GP_PTR_TO(__VA_ARGS__)

/** Dynamic array of any type */
typedef void* GPArrayAny;

#ifdef GP_DOXYGEN // unhide the real meaning of GPArrayAnyAddr from docs.
/** Pointer to a dynamic array of any type.
 * Most commonly an address of a dynamic array taken with the address operator
 * `&`. Mutating functions take a dynamic array by address to allow
 * reallocation, which in turn, allows mutation.
 */
typedef GPArrayAnyAddr GPArrayAny*
#else
/** Pointer to a dynamic array of any type.
 * Most commonly an address of a dynamic array taken with the address operator
 * `&`. Mutating functions take a dynamic array by address to allow
 * reallocation, which in turn, allows mutation.
 */
typedef void* GPArrayAnyAddr;
#endif

/** Array meta-data.
 * You can edit the fields directly with gp_arr_header(my_array)->field.
 * This might be useful for optimizations, but it is mostly recommended to use
 * the provided functions instead.
 */
typedef struct gp_array_header
{
    uintptr_t    capacity;
    void*        allocation; // allocated block start or NULL if on stack
    GPAllocator* allocator;
    uintptr_t    length;
} GPArrayHeader;

#if !__cplusplus && __STDC_VERSION__ < 202311L // empty arg list for BETTER type checks
typedef void  (*gp_arr_map_callback_t)(/* T* out_element, const T* in_element */);
typedef void* (*gp_arr_fold_callback_t)(/* Any* accumulator, const T* element */);
typedef bool  (*gp_arr_filter_callback_t)(/* const T* element */);
#else
typedef void* gp_arr_map_callback_t;
typedef void* gp_arr_fold_callback_t;
typedef void* gp_arr_filter_callback_t;
#endif

/** Getters */
GP_NONNULL_ARGS() GP_NODISCARD static inline size_t       gp_arr_length    (GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->length;     }
GP_NONNULL_ARGS() GP_NODISCARD static inline size_t       gp_arr_capacity  (GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->capacity;   }
GP_NONNULL_ARGS() GP_NODISCARD static inline void*        gp_arr_allocation(GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->allocation; }
GP_NONNULL_ARGS() GP_NODISCARD static inline GPAllocator* gp_arr_allocator (GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->allocator;  }

// TODO document better!
/** Setter */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline GPArrayHeader* gp_arr_header(GPArrayAny arr)
{
    return (GPArrayHeader*)arr - 1;
}

/** Create a new empty array.*/
GP_NONNULL_ARGS()
static inline GPArrayAny gp_arr_new(
    size_t element_size,
    GPAllocator* allocator,
    size_t element_count)
{
    const size_t size = gp_round_to_aligned(element_size * element_count, GP_ALLOC_ALIGNMENT);
    GPArrayHeader* me = (GPArrayHeader*)gp_mem_alloc(allocator, sizeof(*me) + size);
    *me = (GPArrayHeader) {
        .length = 0,
        .capacity = size / element_size,
        .allocator = allocator,
        .allocation = me
    };
    return me + 1;
}

/** Create a new dynamic array on stack. Not available in C++.
 * @p allocator determines how the array will be reallocated if length exceeds
 * capacity. If it is known that length will not exceed capacity, @p allocator
 * can be left NULL.
 *     Note that if you compile with Clang without GNU C extensions (with
 * `-std=cXX` or `-Wpedantic`), @p init_values must have at least one argument.
 * This means that to create an empty array, a trailing comma must be used in
 * the argument list.
 *     CompCert will not allow empty arrays, even with trailing comma.
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
// which does not have gp_arr_on_stack(). Example with int using memset():
/*
    struct optional_name { GPArrayHeader header; int data[2048]; } my_array_mem;
    memset(&my_array_mem.header, 0, sizeof my_array_mem.header);
    my_array_mem.header.capacity = 2048;
    GPArray(int) my_array = my_array_mem.data;
*/
// or more concisely: (C only)
/*
    struct { GPArrayHeader h; int data[2048];} arr_mem;
    arr_mem.h = (GPArrayHeader){.capacity = 2048 };
    GPArray(int) arr = arr_mem.data;
*/

/** Free array memory.
 * Passing arrays on stack is safe too.
 */
static inline void gp_arr_delete(GPArrayAny optional)
{
    if (optional != NULL && gp_arr_allocation(optional) != NULL)
        gp_mem_dealloc(gp_arr_allocator(optional), gp_arr_allocation(optional));
}

/** Free array memory trough pointer.
 * This should be used as destructor for GPDictionary(GPArray(T)) if needed.
 */
static inline void gp_arr_ptr_delete(GPArrayAnyAddr optional_address)
{
    if (optional_address != NULL)
        gp_arr_delete(*(GPArrayAny*)optional_address);
}

/** Always reallocate array.
 * Always reallocating may be useful for memory packing, but is not desirable in
 * general. Prefer @ref gp_arr_reserve() for better performance.
 */
void gp_arr_reallocate(
    size_t         element_size,
    GPArrayAnyAddr arr_address,
    size_t         capacity);

/** Reserve capacity.
 * If @p capacity > gp_arr_capacity(@p arr), reallocates, does nothing
 * otherwise. In case of reallocation, capacity will be rounded up
 * exponentially.
 */
GP_NONNULL_ARGS()
static inline void gp_arr_reserve(
    size_t         element_size,
    GPArrayAnyAddr arr_address,
    size_t         capacity)
{
    GPArrayAny* parr = arr_address;
    // This check is here so functions like gp_str_print() can try to
    // guesstimate buffer sizes even for strings on stack when allocator is NULL.
    //
    // TODO once truncating strings and arrays are implemented, this should be
    // an assertion instead.
    if (gp_arr_allocator(*parr) == NULL)
        return;

    if (capacity <= gp_arr_capacity(*parr))
        return;
    gp_arr_reallocate(element_size, parr, capacity);
}


/** Copy source array to destination */
GP_NONNULL_ARGS()
static inline void gp_arr_copy(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    GPArrayAny* pdest = (GPArrayAny*)dest_address;
    gp_arr_reserve(element_size, pdest, src_length);
    assert(gp_arr_capacity(*pdest) >= src_length); // analyzer false positive // TODO after testing inlines, try to remove this
    memcpy(*pdest, src, src_length * element_size);
    gp_arr_header(*pdest)->length = src_length;
}

/** Copy or remove elements.
 * Copies elements from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, elements from @p dest outside
 * @p start_index and @p end_index are removed and the remaining elements are
 * moved over.
 */
GP_NONNULL_ARGS(2)
static inline void gp_arr_slice(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT optional_src,
    size_t                 start_index,
    size_t                 end_index_exclusive)
{
    GPArrayAny* pdest = (GPArrayAny*)dest_address;
    gp_db_assert(start_index <= end_index_exclusive, "Invalid range.");
    const size_t length = end_index_exclusive - start_index, size = element_size;
    if (optional_src == NULL)
        memmove(*pdest, (uint8_t*)*pdest + start_index*size, length*size);
    else {
        gp_arr_reserve(size, pdest, length);
        memcpy(*pdest, (uint8_t*)optional_src + start_index*size, length*size);
    }
    gp_arr_header(*pdest)->length = length;
}


/** Add element to the end */
GP_NONNULL_ARGS()
static inline void gp_arr_push(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT element)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    const size_t length = gp_arr_length(*parr);
    gp_arr_reserve(element_size, parr, length + 1);
    memcpy((uint8_t*)*parr + length * element_size, element, element_size);
    gp_arr_header(*parr)->length++;
}

/** Remove element from the end.
 * If @p arr is empty, the behavior is undefined.
 * @return a pointer to the last element, which is valid as long as no new
 * elements are added to @p arr. It is recommended to immediately dereference
 * and assign the return value to a variable. Will not reallocate, only takes
 * the array by address to signal mutation and to be consistent with other
 * mutating functions.
 */
GP_NONNULL_ARGS_AND_RETURN
static inline void* gp_arr_pop(
    size_t         element_size,
    GPArrayAnyAddr arr_address)
{
    GPArray(uint8_t) arr = *(GPArrayAny*)arr_address;
    gp_db_assert(gp_arr_length(arr) > 0, "Array passed to gp_arr_pop() must not be empty.");
    return (void*)(arr + --((GPArrayHeader*)arr - 1)->length * element_size);
}

/** Add elements to the end */
GP_NONNULL_ARGS()
static inline void gp_arr_append(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    const size_t length = gp_arr_length(*parr);
    gp_arr_reserve(element_size, parr, length + src_length);
    memcpy((uint8_t*)*parr + length*element_size, src, src_length*element_size);
    gp_arr_header(*parr)->length += src_length;
}

/** Add elements to specified position.
 * Moves rest of the array over.
 */
GP_NONNULL_ARGS()
static inline void gp_arr_insert(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    size_t                 position,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    const size_t length = gp_arr_length(*parr);
    gp_arr_reserve(element_size, parr, length + src_length);

    memmove(
        (uint8_t*)*parr + (position + src_length) * element_size,
        (uint8_t*)*parr +  position               * element_size,
        (length - position)                       * element_size);
    memcpy(
        (uint8_t*)*parr +  position*element_size, src, src_length*element_size);

    gp_arr_header(*parr)->length += src_length;
}

/** Remove elements.
 * Removes @p count elements starting from @p pos moving the rest of the
 * elements over. Will not reallocate, only takes the array by address to signal
 * mutation and to be consistent with other mutating functions.
 */
GP_NONNULL_ARGS()
static inline void gp_arr_erase(
    size_t         element_size,
    GPArrayAnyAddr dest_address,
    size_t         position,
    size_t         count)
{
    GPArrayAny arr = *(GPArrayAny*)dest_address;
    size_t* length = &((GPArrayHeader*)arr - 1)->length;
    const size_t tail_length = *length - (position + count);
    memmove(
        (uint8_t*)arr +  position          * element_size,
        (uint8_t*)arr + (position + count) * element_size,
        tail_length                        * element_size);
    *length -= count;
}

/** Null terminate.
 * Reserve memory for an extra element and zero the reserved memory. This does
 * not change the length of the array.
 * @return the passed array (not by address!) null terminated.
 */
GP_NONNULL_ARGS()
static inline GPArrayAny gp_arr_null_terminate(
    size_t         element_size,
    GPArrayAnyAddr arr_address)
{
    gp_arr_reserve(element_size, arr_address, gp_arr_length(*(GPArrayAny*)arr_address) + 1);
    GPArrayAny arr = *(GPArrayAny*)arr_address;
    memset((uint8_t*)arr + element_size*gp_arr_length(arr), 0, element_size);
    return arr;
}

/** Apply function to elements.
 * Calls @p f for all elements in source array. @p src will point to the element
 * in the source array and @p *dest will point to the corresponding element at
 * @p *dest. If @p src is NULL, the source array will be @p *dest. Reallocates
 * if gp_arr_capacity(@p *dest) < gp_arr_length(@p src). @p f will be called for
 * each element of source array. The first argument will be a pointer to mutable
 * output element, the second argument will be a pointer to mutable input
 * element.
 */
GP_NONNULL_ARGS(2, 5)
static inline void gp_arr_map(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT optional_src,
    size_t                 optional_src_length,
    gp_arr_map_callback_t  f)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    void(*func)(void*, const void*) = (void(*)(void*, const void*))f;

    if (optional_src == NULL) {
        for (size_t i = 0; i < gp_arr_length(*parr); i++)
            func((uint8_t*)*parr + i*element_size, (uint8_t*)*parr + i*element_size);
    } else {
        gp_arr_reserve(element_size, parr, optional_src_length);
        for (size_t i = 0; i < optional_src_length; i++)
            func((uint8_t*)*parr + i*element_size, (uint8_t*)optional_src + i*element_size);
        gp_arr_header(*parr)->length = optional_src_length;
    }
}

/** Combine elements from left to right.
 * Combine all elements in @p arr to @p accumulator using @p f. @p f will be
 * called for each element in input array. @p f returns @p accumulator which
 * will be assigned to the original @p accumulator. This allows for reallocating
 * @p accumulator in @p f if necessary. @p f takes the @p accumulator as it's
 * first argument, pointer to element as it's second argument.
 * @return @p accumulator which might be necessary to assign to @p accumulator
 * in case of reallocations.
 */
GP_NONNULL_ARGS(2, 4)
static inline void* gp_arr_fold(
    size_t                 elem_size,
    GPArrayAny             arr,
    void*                  accumulator,
    gp_arr_fold_callback_t f)
{
    void*(*func)(void*, const void*) = (void*(*)(void*, const void*))f;
    for (size_t i = 0; i < gp_arr_length(arr); ++i)
        accumulator = func(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

/** Combine elements from right to left.
 * Combine all elements in @p arr to @p accumulator using @p f. @p f will be
 * called for each element in input array. @p f returns @p accumulator which
 * will be assigned to the original @p accumulator. This allows for reallocating
 * @p accumulator in @p f if necessary. @p f takes the @p accumulator as it's
 * first argument, pointer to element as it's second argument.
 * @return @p accumulator which might be necessary to assign to @p accumulator
 * in case of reallocations.
 */
GP_NONNULL_ARGS(2, 4)
static inline void* gp_arr_foldr(
    size_t                 elem_size,
    GPArrayAny             arr,
    void*                  accumulator,
    gp_arr_fold_callback_t f)
{
    void*(*func)(void*, const void*) = (void*(*)(void*, const void*))f;
    for (size_t i = gp_arr_length(arr) - 1; i != (size_t)-1; --i)
        accumulator = func(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

/** Copies elements conditionally.
 * Copies all elements from @p src that make @p f return `true` when passed to
 * @p f by pointer. If @p src is NULL, removes all elements from @p dest that
 * make @p f return `false` when passed to @p f by pointer.
 */
GP_NONNULL_ARGS(2, 5)
static inline void gp_arr_filter(
    size_t                   element_size,
    GPArrayAnyAddr           dest_address,
    const void*GP_RESTRICT   optional_src, // mutates arr if NULL
    size_t                   optional_src_length,
    gp_arr_filter_callback_t f)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    bool(*func)(const void* x) = (bool(*)(const void* x))f;

    const void* src = optional_src == NULL ? *parr : optional_src;
    const size_t length = optional_src == NULL ? gp_arr_length(*parr) : optional_src_length;
    gp_arr_header(*parr)->length = 0;

    for (size_t i = 0; i < length; ++i)
        if (func((uint8_t*)src + i*element_size))
           memmove(
               (uint8_t*)*parr + gp_arr_header(*parr)->length++ * element_size,
               (uint8_t*)src + i*element_size,
               element_size);
}


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

#ifdef __clang__
// Allow {0} for any type, which is the most portable 0 init before C23
#pragma clang diagnostic ignored "-Wmissing-braces"
// Silence stupid clang-tidy bugprone-sizeof-expression, which actually just
// enforces more bugprone and unidiomatic C, what were they thinking??
#define GP_SIZEOF_VALUE(...) sizeof(GP_TYPEOF(__VA_ARGS__))
#else
#define GP_SIZEOF_VALUE(...) sizeof(__VA_ARGS__)
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

// ----------------------------------------------------------------------------
// Type Safe Macro Shadowing

#if !defined(GP_NO_TYPE_SAFE_MACRO_SHADOWING) && !defined(GPC_IMPLEMENTATION) && !defined(GP_DOXYGEN)

#  if __GNUC__ && !defined(GP_PEDANTIC) && __STDC_VERSION__ >= 201112L
#    define GP_CHECK_SIZE(SIZE, VALUE) ({ \
        _Static_assert((SIZE) == GP_SIZEOF_VALUE(VALUE), "Element size argument must match array element size."); \
        (SIZE); \
    })
#    define GP_CHECK_ARR_ARGS(SIZE, OUT, IN) ({ \
        _Static_assert((SIZE) == GP_SIZEOF_VALUE(**(OUT)), "Element size argument must match output array element size."); \
        _Static_assert((SIZE) == GP_SIZEOF_VALUE(*(IN)), "Element size argument must match input array element size."); \
        sizeof(**(OUT) = *(IN)); \
    })
#    define GP_CHECK_ARR_ARGS_OPTIONAL(SIZE, OUT, IN) ({ \
        _Static_assert((SIZE) == GP_SIZEOF_VALUE(**(OUT)), "Element size argument must match output array element size."); \
        _Static_assert((SIZE) == sizeof(_Generic((IN), GP_TYPEOF(NULL):**(OUT), default:*(IN))), \
            "Element size argument must match input array element size."); \
        sizeof(**(OUT) = _Generic((IN), GP_TYPEOF(NULL):**(OUT), default:*(IN))); \
    })
#  else
#    define GP_CHECK_SIZE(SIZE, VALUE) (sizeof(char[(SIZE) == sizeof(VALUE) ? (SIZE) : -1]))
#    define GP_CHECK_ARR_ARGS(SIZE, OUT, IN) (sizeof(char[ \
        (SIZE) == sizeof**(OUT) && (SIZE) == sizeof*(IN) ? sizeof(**(OUT) = *(IN)) : -1]))
#    define GP_CHECK_ARR_ARGS_OPTIONAL(SIZE, OUT, IN) (sizeof(char[ \
        (SIZE) == sizeof**(OUT) ? sizeof(**(OUT)) : -1]))
#  endif

#if defined(GP_TYPEOF) && !__cplusplus
#  define GP_CHECK_ACCUMULATOR(...) (GP_TYPEOF(__VA_ARGS__)){0} =
#else
#  define GP_CHECK_ACCUMULATOR(...)
#endif

/** Free array memory trough pointer.
 * This should be used as destructor for GPDictionary(GPArray(T)) if needed.
 */
#define gp_arr_ptr_delete(GPArrayT_ptr_OPTIONAL) \
    gp_arr_ptr_delete(sizeof(**(GPArrayT_ptr_OPTIONAL)) ? (GPArrayT_ptr_OPTIONAL) : (GPArrayT_ptr_OPTIONAL))

/** Reallocate array.
 * Always reallocates, which may be useful for memory packing, but is not
 * desirable in general. Prefer @ref gp_arr_reserve() for better performance.
 */
#define gp_arr_reallocate( \
    size_t_ELEMENT_SIZE,   \
    GPArrayT_ptr,          \
    size_t_CAPACITY)       \
\
    gp_arr_reallocate(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, **(GPArrayT_ptr)), \
        GPArrayT_ptr, size_t_CAPACITY)

/** Reserve capacity.
 * If @p capacity > gp_arr_capacity(@p arr), reallocates, does nothing
 * otherwise. In case of reallocation, capacity will be rounded up
 * exponentially.
 */
#define gp_arr_reserve(  \
    size_t_ELEMENT_SIZE, \
    GPArrayT_ptr,        \
    size_t_CAPACITY)     \
\
    gp_arr_reserve(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, **(GPArrayT_ptr)), (GPArrayT_ptr), (size_t_CAPACITY))

// TODO gp_arr_try_reserve()

/** Copy source array to destination */
#define gp_arr_copy(     \
    size_t_ELEMENT_SIZE, \
    GPArrayT_ptr_DEST,   \
    T_ptr_SRC,           \
    size_t_SRC_LENGTH)   \
\
    gp_arr_copy(GP_CHECK_ARR_ARGS(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_SRC), \
        GPArrayT_ptr_DEST, T_ptr_SRC, size_t_SRC_LENGTH)

/** Copy or remove elements.
 * Copies elements from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, elements from @p dest outside
 * @p start_index and @p end_index are removed and the remaining elements are
 * moved over.
 */
#define gp_arr_slice(      \
    size_t_ELEMENT_SIZE,    \
    GPArrayT_ptr_DEST,       \
    T_ptr_OPTIONAL_SRC,       \
    size_t_START_INDEX,        \
    size_t_END_INDEX_EXCLUSIVE) \
\
    gp_arr_slice(GP_CHECK_ARR_ARGS_OPTIONAL(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_OPTIONAL_SRC), \
        GPArrayT_ptr_DEST, T_ptr_OPTIONAL_SRC, size_t_START_INDEX, size_t_END_INDEX_EXCLUSIVE)

/** Add element to the end */
#define gp_arr_push(     \
    size_t_ELEMENT_SIZE, \
    GPArrayT_ptr_DEST,   \
    T_ptr_ELEMENT)       \
\
    gp_arr_push(GP_CHECK_ARR_ARGS(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_ELEMENT), \
        GPArrayT_ptr_DEST, T_ptr_ELEMENT)

/** Remove element from the end.
 * If @p arr is empty, the behavior is undefined.
 * @return a pointer to the last element, which is valid as long as no new
 * elements are added to @p arr. It is recommended to immediately dereference
 * and assign the return value to a variable. Will not reallocate, only takes
 * the array by address to signal mutation and to be consistent with other
 * mutating functions.
 */
#define/* T* */gp_arr_pop( \
    size_t_ELEMENT_SIZE,   \
    GPArrayT_ptr)          \
\
    gp_arr_pop(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, **(GPArrayT_ptr)), GPArrayT_ptr)

/** Add elements to the end */
#define gp_arr_append(   \
    size_t_ELEMENT_SIZE, \
    GPArrayT_ptr_DEST,   \
    T_ptr_SRC,           \
    size_t_SRC_LENGTH)   \
\
    gp_arr_append(GP_CHECK_ARR_ARGS(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_SRC), \
        GPArrayT_ptr_DEST, T_ptr_SRC, size_t_SRC_LENGTH)

/** Add elements to specified position.
 * Moves rest of the array over.
 */
#define gp_arr_insert(   \
    size_t_ELEMENT_SIZE, \
    GPArrayT_ptr_DEST,   \
    size_t_POSITION,     \
    T_ptr_SRC,           \
    size_t_SRC_LENGTH)   \
\
    gp_arr_insert(GP_CHECK_ARR_ARGS(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_SRC), \
        GPArrayT_ptr_DEST, size_t_POSITION, T_ptr_SRC, size_t_SRC_LENGTH)

/** Remove elements.
 * Removes @p count elements starting from @p pos moving the rest of the
 * elements over. Will not reallocate, only takes the array by address to signal
 * mutation and to be consistent with other mutating functions.
 */
#define gp_arr_erase(    \
    size_t_ELEMENT_SIZE, \
    GPArrayT_ptr,        \
    size_t_POSITION,     \
    size_t_COUNT)        \
\
    gp_arr_erase(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, **(GPArrayT_ptr)), \
        GPArrayT_ptr, size_t_POSITION, size_t_COUNT)

/** Null terminate.
 * Reserve memory for an extra element and zero the reserved memory. This does
 * not change the length of the array.
 * @return the passed array (not by address!) null terminated.
 */
#define/* GPArray(T) */gp_arr_null_terminate( \
    size_t_ELEMENT_SIZE,                      \
    GPArrayT_ptr)                             \
\
    gp_arr_null_terminate(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, **(GPArrayT_ptr)), GPArrayT_ptr)

/** Apply function to elements.
 * Calls @p f for all elements in source array. @p in will point to the element
 * in the source array and @p out will point to the corresponding element at
 * @p arr. If @p src is NULL, the source array will be @p out. Reallocates if
 * gp_arr_capacity(@p arr) < gp_arr_length(@p src). @p f will be called for each
 * element of source array. The first argument will be a pointer to mutable
 * output element, the second argument will be a pointer to mutable input
 * element.
 */
#define gp_arr_map(                           \
    size_t_ELEMENT_SIZE,                      \
    GPArrayT_ptr_DEST,                        \
    T_ptr_OPTIONAL_SRC,                       \
    size_t_OPTIONAL_SRC_LENGTH,               \
    void_FUNC_void_ptr_OUT_const_void_ptr_IN) \
( \
    0 ? (void_FUNC_void_ptr_OUT_const_void_ptr_IN)(*(GPArrayT_ptr_DEST), *(GPArrayT_ptr_DEST)) : (void)0, \
    gp_arr_map(GP_CHECK_ARR_ARGS_OPTIONAL(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_OPTIONAL_SRC), \
        GPArrayT_ptr_DEST, T_ptr_OPTIONAL_SRC, size_t_OPTIONAL_SRC_LENGTH, \
        void_FUNC_void_ptr_OUT_const_void_ptr_IN) \
)

/** Combine elements from left to right.
 * Combine all elements in @p arr to @p accumulator using @p func. @p func will
 * be called for each element in input array. @p func returns @p accumulator
 * which will be assigned to the original @p accumulator. This allows for
 * reallocating @p accumulator in @p func if necessary. @p func takes the
 * @p accumulator as it's first argument, pointer to element as it's second
 * argument.
 * @return @p accumulator which might be necessary to assign to @p accumulator
 * in case of reallocations.
 */
#define/* void* */gp_arr_fold(                    \
    size_t_ELEMENT_SIZE,                          \
    GPArrayT_IN,                                  \
    ptr_ACCUMULATOR,                              \
    ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT) \
( \
    0 ? (void)(GP_CHECK_ACCUMULATOR(ptr_ACCUMULATOR) (ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT)(ptr_ACCUMULATOR, GPArrayT_IN)) : (void)0, \
    gp_arr_fold(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, *(GPArrayT_IN)), \
        GPArrayT_IN, ptr_ACCUMULATOR, ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT) \
)

/** Combine elements from right to left.
 * Combine all elements in @p arr to @p accumulator using @p f. @p f will be
 * called for each element in input array. @p f returns @p accumulator which
 * will be assigned to the original @p accumulator. This allows for reallocating
 * @p accumulator in @p f if necessary. @p f takes the @p accumulator as it's
 * first argument, pointer to element as it's second argument.
 * @return @p accumulator which might be necessary to assign to @p accumulator
 * in case of reallocations.
 */
#define/* void* */gp_arr_foldr(                   \
    size_t_ELEMENT_SIZE,                          \
    GPArrayT_IN,                                  \
    ptr_ACCUMULATOR,                              \
    ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT) \
( \
    0 ? (void)(GP_CHECK_ACCUMULATOR(ptr_ACCUMULATOR) (ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT)(ptr_ACCUMULATOR, GPArrayT_IN)) : (void)0, \
    gp_arr_foldr(GP_CHECK_SIZE(size_t_ELEMENT_SIZE, *(GPArrayT_IN)), \
        GPArrayT_IN, ptr_ACCUMULATOR, ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT) \
)

/** Copies elements conditionally.
 * Copies all elements from @p src that make @p func return `true` when passed
 * to @p func by pointer. If @p src is NULL, removes all elements from @p dest
 * that make @p func return `false` when passed to @p func by pointer.
 */
#define gp_arr_filter(             \
    size_t_ELEMENT_SIZE,           \
    GPArrayT_ptr_DEST,             \
    T_ptr_OPTIONAL_SRC,            \
    size_t_OPTIONAL_SRC_LENGTH,    \
    bool_FUNC_const_T_ptr_ELEMENT) \
( \
    0 && bool_FUNC_const_T_ptr_ELEMENT(*(GPArrayT_ptr_DEST)) ? (void)0 : (void)0, \
    gp_arr_filter(GP_CHECK_ARR_ARGS_OPTIONAL(size_t_ELEMENT_SIZE, GPArrayT_ptr_DEST, T_ptr_OPTIONAL_SRC), \
        GPArrayT_ptr_DEST, T_ptr_OPTIONAL_SRC, size_t_OPTIONAL_SRC_LENGTH, bool_FUNC_const_T_ptr_ELEMENT) \
)

#endif // !defined(GP_NO_TYPE_SAFE_MACRO_SHADOWING) && !defined(GPC_IMPLEMENTATION) && !defined(GP_DOXYGEN)

#else // __cplusplus
} // extern "C"
#endif

#endif // GP_ARRAY_INCLUDED
