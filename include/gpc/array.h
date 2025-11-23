// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file array.h
 * Dynamic and static truncating array
 */

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


/** Array of a specific type.
 * In memory, an array is GPArrayHeader followed by the elements. An object of
 * type GPArray(T) is a pointer to the first element. Use this macro to
 * differentiate between any other type of pointer. Type should not be void.
 *     Arrays can be configured to be dynamic or truncating on per object basis.
 * Storing a pointer to an allocator allows the array to reallocate and grow. If
 * the pointer is NULL, the array is considered static and will not reallocate.
 * A static array will be truncated to prevent overflow and the number of
 * truncated elements will be returned by relevant functions.
 */
#define GPArray(/* Type */...) GP_PTR_TO(__VA_ARGS__)

/** Dynamic array of a specific type.
 * Use this type to signal the reader that this array is supposed to grow by
 * reallocation. This is obviously not enforced by the compiler or even our
 * functions, but you can enforce this simply by asserting that the allcoator is
 * not NULL.
 */
#define GPArrayDynamic(/* Type */...) GPArray(__VA_ARGS__)

/** Static array of a specific type.
 * Use this type to signal the reader that this array is supposed to never
 * reallocate and may truncate or is expected to be truncating.
 */
#define GPArrayStatic(/* Type */...) GPArray(__VA_ARGS__)

/** Array of any type */
typedef void* GPArrayAny;

#ifdef GP_DOXYGEN // unhide the real meaning of GPArrayAnyAddr from docs.
/** Pointer to a array of any type.
 * Most commonly an address of a array taken with the address operator `&`.
 * Mutating functions take a array by address to allow reallocation, which in
 * turn, allows mutation.
 */
typedef GPArrayAnyAddr GPArrayAny*
#else
/** Pointer to a array of any type.
 * Most commonly an address of a array taken with the address operator `&`.
 * Mutating functions take a array by address to allow reallocation, which in
 * turn, allows mutation.
 */
typedef void* /* GPArrayAny* */GPArrayAnyAddr;
#endif

/** Array meta-data.
 * You can edit the fields directly using `gp_arr_set(my_array)->field = x`.
 * TODO better docs, which fields are safe to modify and how?
 */
typedef struct gp_array_header
{
    uintptr_t    capacity;
    void*        allocation; // allocated block start or NULL if on stack
    GPAllocator* allocator;  // set this to NULL to make the array truncating (not dynamic)
    uintptr_t    length;
} GPArrayHeader;

/** Static array buffer.
 * Used to create a static or stack allocated GPArray(T). Create a variable of
 * this type, then pass it by address to @ref gp_arr_buffered() to initialize
 * and convert it to GPArray(T). This type is not meant to be used directly, it
 * is meant to be used as a statically allocated buffer to be converted to
 * GPArray(T).
 */
#define GPArrayBuffer(T, CAPACITY)  \
struct GP_ANONYMOUS_STRUCT(__LINE__) \
{ \
    uintptr_t         capacity;                                   \
    void*             allocation;                                  \
    GPAllocator*      allocator;                                    \
    uintptr_t         length;                                        \
    GP_TYPEOF_TYPE(T) data[(CAPACITY) + (sizeof(T) == sizeof(char))]; \
}

#if !__cplusplus && __STDC_VERSION__ < 202311L && !__clang__// empty arg list for BETTER type checks
typedef void  (*gp_arr_map_callback_t)(/* T* out_element, const T* in_element */);
typedef void* (*gp_arr_fold_callback_t)(/* Any* accumulator, const T* element */);
typedef bool  (*gp_arr_filter_callback_t)(/* const T* element */);
#else
typedef void* gp_arr_map_callback_t;
typedef void* gp_arr_fold_callback_t;
typedef void* gp_arr_filter_callback_t;
#endif

/** Getters.
 * Note: These will not get shadowed by type safe macros since there is no
 * arguments to check against. Use gp_arrt variants of these for type checking.
 */
GP_NONNULL_ARGS() GP_NODISCARD static inline size_t       gp_arr_length    (GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->length;     }
GP_NONNULL_ARGS() GP_NODISCARD static inline size_t       gp_arr_capacity  (GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->capacity;   }
GP_NONNULL_ARGS() GP_NODISCARD static inline GPAllocator* gp_arr_allocator (GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->allocator;  }
GP_NONNULL_ARGS() GP_NODISCARD static inline void*        gp_arr_allocation(GPArrayAny arr) { return ((GPArrayHeader*)arr - 1)->allocation; }

/** Direct access to GPArrayHeader fields.
 * Note: This will not get shadowed by a type safe macro since there is no
 * arguments to check against. Use @ref gp_arrt_set() for type checking.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline GPArrayHeader* gp_arr_set(GPArrayAny arr)
{
    return (GPArrayHeader*)arr - 1;
}

/** Getters with type checking.
 * Pass the element type to @p T to be the type to be checked against. This
 * prevents double pointer bugs since it is very common to pass arrays by
 * address to allow reallocation.
 */
#define gp_arrt_length(T, GPArrayT_ARR)     (GP_ARRH_CHECK(T, GPArrayT_ARR), ((GPArrayHeader*)(GPArrayT_ARR) - 1)->length)
#define gp_arrt_capacity(T, GPArrayT_ARR)   (GP_ARRH_CHECK(T, GPArrayT_ARR), ((GPArrayHeader*)(GPArrayT_ARR) - 1)->capacity)
#define gp_arrt_allocator(T, GPArrayT_ARR)  (GP_ARRH_CHECK(T, GPArrayT_ARR), ((GPArrayHeader*)(GPArrayT_ARR) - 1)->allocator)
#define gp_arrt_allocation(T, GPArrayT_ARR) (GP_ARRH_CHECK(T, GPArrayT_ARR), ((GPArrayHeader*)(GPArrayT_ARR) - 1)->allocation)

/** Direct accces to GPArrayHeader fields with type checking.
 * Pass the element type to @p T to be the type to be checked against. This
 * prevents double pointer bugs since it is very common to pass arrays by
 * address to allow reallocation.
 */
#define gp_arrt_set(T, GPArrayT_ARR) \
( \
    GP_ARRH_CHECK(T, GPArrayT_ARR),   \
    (GPArrayHeader*)(GPArrayT_ARR) - 1 \
)

/** Create a new empty array.*/
GP_NONNULL_ARGS()
static inline GPArrayAny gp_arr_new(
    size_t element_size,
    GPAllocator* allocator,
    size_t element_count)
{
    bool is_char = element_size == sizeof(char);
    const size_t size = gp_round_to_aligned(element_size * element_count + is_char, GP_ALLOC_ALIGNMENT);
    GPArrayHeader* me = (GPArrayHeader*)gp_mem_alloc(allocator, sizeof(*me) + size);
    *me = (GPArrayHeader) {
        .length = 0,
        .capacity = size / element_size - is_char,
        .allocator = allocator,
        .allocation = me
    };
    return me + 1;
}

/** Create and initialize an array from a static array buffer.
 * Passing an allocator makes the array reallocateable (dynamic), static if
 * NULL.
 */
#define/* GPArray(T) */gp_arr_buffered( \
    T,                                  \
    GPAllocator_ptr_OPTIONAL,           \
    /* GPArrayBuffer(T, N)* BUFFER, */  \
    /* optional initial values */...)   \
( \
    (GP_1ST_ARG(__VA_ARGS__))->capacity =                                              \
        GP_SIZEOF_VALUE((GP_1ST_ARG(__VA_ARGS__))->data) /                             \
            GP_SIZEOF_VALUE((GP_1ST_ARG(__VA_ARGS__))->data[0]) -                      \
                (GP_SIZEOF_VALUE((GP_1ST_ARG(__VA_ARGS__))->data[0]) == sizeof(char)), \
    (GP_1ST_ARG(__VA_ARGS__))->allocation = NULL,                                      \
    (GP_1ST_ARG(__VA_ARGS__))->allocator  = (GPAllocator_ptr_OPTIONAL),                \
    (GP_1ST_ARG(__VA_ARGS__))->length =                                                \
        GP_ARR_STATIC_OPTIONAL_INITIALIZE_LENGTH(GP_TYPEOF_TYPE(T), __VA_ARGS__),      \
    GP_ARR_STATIC_OPTIONAL_INITIALIZE(GP_TYPEOF_TYPE(T), __VA_ARGS__),                 \
    /* return */(GP_1ST_ARG(__VA_ARGS__))->data                                        \
)

/** Free array memory.
 * Passing arrays on stack is safe too.
 */
static inline void gp_arr_delete(GPArrayAny optional)
{
    if (optional != NULL && gp_arr_allocation(optional) != NULL)
        gp_mem_dealloc(gp_arr_allocator(optional), gp_arr_allocation(optional));
}

/** Free array memory trough pointer.
 * Useful for some destructor callbacks.
 */
static inline void gp_arr_ptr_delete(GPArrayAnyAddr optional_address)
{
    if (optional_address != NULL)
        gp_arr_delete(*(GPArrayAny*)optional_address);
}

/** Always reallocate array.
 * Always reallocating may be useful for memory packing, but is not desirable in
 * general. Prefer @ref gp_arr_reserve() for better performance. @p arr_address
 * must have an allocator obviously.
 */
void gp_arr_reallocate(
    size_t         element_size,
    GPArrayAnyAddr arr_address,
    size_t         capacity);

/** Reserve capacity.
 * If @p capacity > gp_arr_capacity(@p arr), reallocates, does nothing
 * otherwise. In case of reallocation, capacity will be rounded up
 * exponentially.
 * @return 0 if capacity will be large enough to hold @p capacity elements,
 * which is always the case for dynamic arrays. Otherwise returns the difference
 * of @p capacity and current capacity.
 */
GP_NONNULL_ARGS()
static inline size_t gp_arr_reserve(
    size_t         element_size,
    GPArrayAnyAddr arr_address,
    size_t         capacity)
{
    #ifdef GP_STATIC_ANALYSIS // GCC static analyzer can't keep up with conditional
                              // reallocations, which causes a lot of buffer
                              // overflow false positives.
    gp_arr_reallocate(element_size, arr_address, gp_next_power_of_2(capacity));
    return 0;
    #else
    GPArrayAny* parr = arr_address;
    if (capacity <= gp_arr_capacity(*parr))
        return 0;
    else if (gp_arr_allocator(*parr) == NULL)
        return capacity - gp_arr_capacity(*parr);
    gp_arr_reallocate(element_size, parr, gp_next_power_of_2(capacity));
    return 0;
    #endif
}

/** Copy source array to destination.
 * @return the number of truncated elements, which is always 0 for dynamic
 * arrays.
 */
GP_NONNULL_ARGS()
static inline size_t gp_arr_copy(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    GPArrayAny* pdest = (GPArrayAny*)dest_address;
    size_t trunced = gp_arr_reserve(element_size, pdest, src_length);
    src_length -= trunced;
    assert(gp_arr_capacity(*pdest) >= src_length); // analyzer false positive
    memcpy(*pdest, src, src_length * element_size);
    gp_arr_set(*pdest)->length = src_length;
    return trunced;
}

/** Copy or remove elements.
 * Copies elements from @p src starting from @p start_index to @p end_index
 * excluding @p end_index. If @p src is NULL, elements from @p dest outside
 * @p start_index and @p end_index are removed and the remaining elements are
 * moved over.
 * @return the number of truncated elements, which is always 0 for dynamic
 * arrays and in case of @p optional_src being NULL.
 */
GP_NONNULL_ARGS(2)
static inline size_t gp_arr_slice(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT optional_src,
    size_t                 start_index,
    size_t                 end_index_exclusive)
{
    GPArrayAny* pdest = (GPArrayAny*)dest_address;
    size_t trunced = 0;
    gp_db_assert(start_index <= end_index_exclusive, "Invalid range.");
    size_t length = end_index_exclusive - start_index;
    if (optional_src == NULL) {
        if (length != 0) {
            gp_db_assert(start_index < gp_arr_length(*pdest));
            gp_db_assert(end_index_exclusive <= gp_arr_length(*pdest));
        }
        memmove(*pdest, (uint8_t*)*pdest + start_index*element_size, length*element_size);
    } else {
        length -= trunced = gp_arr_reserve(element_size, pdest, length);
        memcpy(*pdest, (uint8_t*)optional_src + start_index*element_size, length*element_size);
    }
    gp_arr_set(*pdest)->length = length;
    return trunced;
}

/** Add element to the end.
 * @return true if a truncating array got truncated.
 */
GP_NONNULL_ARGS()
static inline bool gp_arr_push(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT element)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    bool trunced = gp_arr_reserve(element_size, parr, gp_arr_length(*parr) + 1);
    if ( ! trunced) {
        memcpy(
            (uint8_t*)*parr + gp_arr_length(*parr) * element_size, element, element_size);
        gp_arr_set(*parr)->length++;
    }
    return trunced;
}

/** Remove element from the end.
 * The argument must not be an empty array.
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

/** Add elements to the end.
 * @return the number of truncated elements, which is always 0 for dynamic
 * arrays.
 */
GP_NONNULL_ARGS()
static inline size_t gp_arr_append(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    const size_t length = gp_arr_length(*parr);
    size_t trunced = gp_arr_reserve(element_size, parr, length + src_length);
    src_length -= trunced;
    memcpy((uint8_t*)*parr + length*element_size, src, src_length*element_size);
    gp_arr_set(*parr)->length += src_length;
    return trunced;
}

/** Add elements to specified position.
 * Moves rest of the array over.
 * @return the number of truncated elements, which is always 0 for dynamic
 * arrays.
 */
GP_NONNULL_ARGS()
static inline size_t gp_arr_insert(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    size_t                 position,
    const void*GP_RESTRICT src,
    size_t                 src_length)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    const size_t length = gp_arr_length(*parr);

    // If array is dynamic or has space left, then an out of bound index is
    // meaningless and most certainly a bug, thus should be asserted. Buf if a
    // truncating array is full, then it is reasonable to assume that the user
    // expects truncation regardless of the insertion position (I know I did).
    if (gp_arr_allocator(*parr) != NULL || gp_arr_length(*parr) != gp_arr_capacity(*parr))
        gp_db_assert(position <= length, "Index out of bounds.");
    else if (position >= gp_arr_length(*parr))
        return src_length;

    size_t trunced = gp_arr_reserve(element_size, parr, length + src_length);
    size_t tail_length = length - position;

    if (trunced > tail_length) {
        src_length -= trunced - tail_length;
        tail_length = 0;
    } else
        tail_length -= trunced;

    memmove(
        (uint8_t*)*parr + (position + src_length) * element_size,
        (uint8_t*)*parr +  position               * element_size,
        tail_length                               * element_size);
    memcpy(
        (uint8_t*)*parr +  position*element_size, src, src_length*element_size);

    gp_arr_set(*parr)->length = position + src_length + tail_length;
    return trunced;
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
    if (count != 0) {
        // Check comment in gp_arr_insert() for explanation of this precondition.
        if (gp_arr_allocator(arr) != NULL || gp_arr_length(arr) != gp_arr_capacity(arr))
            gp_db_assert(position < gp_arr_length(arr), "Index out of bounds.");
        else if (position >= gp_arr_length(arr))
            return;
    }
    else if (position + count > gp_arr_length(arr))
        count = gp_arr_length(arr) - position;

    size_t tail_length = gp_arr_length(arr) - (position + count);
    gp_arr_set(arr)->length -= count;

    memmove(
        (uint8_t*)arr +  position          * element_size,
        (uint8_t*)arr + (position + count) * element_size,
        tail_length                        * element_size);
}

/** Null terminate.
 * Reserve memory for an extra element and zero the reserved memory. This does
 * not change the length of the array.
 * @return the passed array (not by address!) null terminated or NULL if the
 * null terminator got truncated away, which will only happen for truncating
 * arrays with element size larger than 1.
 */
GP_NONNULL_ARGS()
static inline GPArrayAny gp_arr_null_terminate(
    size_t         element_size,
    GPArrayAnyAddr arr_address)
{
    if (element_size == sizeof(char)) {
        (*(uint8_t**)arr_address)[gp_arr_length(*(GPArrayAny*)arr_address)] = '\0';
        return *(GPArrayAny*)arr_address;
    }
    bool trunced = gp_arr_reserve(
        element_size, arr_address, gp_arr_length(*(GPArrayAny*)arr_address) + 1);
    if (trunced)
        return NULL;
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
 * @return the number of truncated elements, which is always 0 for dynamic
 * arrays and in case of @p optional_src being NULL.
 */
GP_NONNULL_ARGS(2, 5)
static inline size_t gp_arr_map(
    size_t                 element_size,
    GPArrayAnyAddr         dest_address,
    const void*GP_RESTRICT optional_src,
    size_t                 optional_src_length,
    gp_arr_map_callback_t  f)
{
    GPArrayAny* parr = (GPArrayAny*)dest_address;
    void(*func)(void*, const void*);
    memcpy(&func, &f, sizeof f);

    size_t trunced = 0;
    if (optional_src == NULL) {
        for (size_t i = 0; i < gp_arr_length(*parr); i++)
            func((uint8_t*)*parr + i*element_size, (uint8_t*)*parr + i*element_size);
    } else {
        optional_src_length -= trunced = gp_arr_reserve(element_size, parr, optional_src_length);
        for (size_t i = 0; i < optional_src_length; i++)
            func((uint8_t*)*parr + i*element_size, (uint8_t*)optional_src + i*element_size);
        gp_arr_set(*parr)->length = optional_src_length;
    }
    return trunced;
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
    void*(*func)(void*, const void*);
    memcpy(&func, &f, sizeof f);
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
    void*(*func)(void*, const void*);
    memcpy(&func, &f, sizeof f);
    for (size_t i = gp_arr_length(arr) - 1; i != (size_t)-1; --i)
        accumulator = func(accumulator, (uint8_t*)arr + i * elem_size);
    return accumulator;
}

/** Copies elements conditionally.
 * Copies all elements from @p src that make @p f return `true` when passed to
 * @p f by pointer. If @p src is NULL, removes all elements from @p dest that
 * make @p f return `false` when passed to @p f by pointer.
 * @return the number of truncated elements, which is always 0 for dynamic
 * arrays and in case of @p optional_src being NULL.
 */
GP_NONNULL_ARGS(2, 5)
size_t gp_arr_filter(
    size_t                   element_size,
    GPArrayAnyAddr           dest_address,
    const void*GP_RESTRICT   optional_src, // mutates arr if NULL
    size_t                   optional_src_length,
    gp_arr_filter_callback_t f);


// ----------------------------------------------------------------------------
// Type Safe Macro Shadowing
//
// By default, to mitigate type safety problems of void* and casts, all GPArray
// related functions are shadowed by their corresponding type safe macros. These
// emit no runtime instructions, even in debug builds, they just do type checks.
// These checks can be disabled if needed (see below), but it is recommended to
// leave them in and only disable for any source files that require it.
//
// Care has been taken to make the compiler emit as informative error/warning
// messages as possible. Most comprehensive checks and most informative messages
// are given by GNUC compatible compilers. Others are ok too, but some specific
// messages might say something confusing about negative array sizes.
//
// The following checks are performed:
//
// * Output arrays are pointers to GPArray, so dereferencing them twice returns
//   a value whose size matches element size parameter and type is compatible to
//   input array element if present.
// * Dereferencing a non-null input pointer/GPArray returns a value whose size
//   matches element size parameter and type can be assigned to output array
//   if present.
// * Arguments, apart from optional NULL inputs, have a complete type, so
//   void*, GPArray(void), GPArrayAny, and GPArrayAny are invalid.
// * element size parameter should be a compile time constant and match array
//   element size (usually sizeof(array[0]) or sizeof(Type)).
//
// There are situations that require disabling macro shadowing. As macros, it is
// impossible to get function pointers to the original functions. More notably,
// requiring concrete types and constant element size prevent writing user
// defined generic functions. Using compound literals as arguments might also
// fail, but this can be mitigated by wrapping the literal in parenthesis, so it
// is not a good reason alone to disable them. If disabling the macros is
// required, define GP_NO_TYPE_SAFE_MACRO_SHADOWING before #including this
// header.

#if !defined(GP_NO_TYPE_SAFE_MACRO_SHADOWING) && !defined(GPC_IMPLEMENTATION) && !defined(GP_DOXYGEN)

/** Free array memory trough pointer.
 * Useful for some destructor callbacks.
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
        GP_FPTR_TO_VOIDPTR(void_FUNC_void_ptr_OUT_const_void_ptr_IN)) \
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
        GPArrayT_IN, ptr_ACCUMULATOR, GP_FPTR_TO_VOIDPTR(ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT)) \
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
        GPArrayT_IN, ptr_ACCUMULATOR, \
        GP_FPTR_TO_VOIDPTR(ptr_FUNC_ptr_ACCUMULATOR_const_T_ptr_ELEMENT)) \
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
        GPArrayT_ptr_DEST, \
        T_ptr_OPTIONAL_SRC, \
        size_t_OPTIONAL_SRC_LENGTH, \
        GP_FPTR_TO_VOIDPTR(bool_FUNC_const_T_ptr_ELEMENT)) \
)


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


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

#endif // !defined(GP_NO_TYPE_SAFE_MACRO_SHADOWING) && !defined(GPC_IMPLEMENTATION) && !defined(GP_DOXYGEN)
// ----------------------------------------------------------------------------

#ifdef __clang__
// Allow {0} for any type, which is the most portable 0 init before C23. This is
// mostly used for type safe shadowing macros.
#pragma clang diagnostic ignored "-Wmissing-braces"
// Silence clang-tidy bugprone-sizeof-expression needed for type safe macro
// shadows.
#define GP_SIZEOF_VALUE(...) sizeof(GP_TYPEOF(__VA_ARGS__))
#else
#define GP_SIZEOF_VALUE(...) sizeof(__VA_ARGS__)
#endif

#if __cplusplus // TODO C++ untested!
template <typename T> static inline void gp_arrh_check_type(T* arr) { (void)arr; }
#  define GP_ARRH_CHECK(T, ARR) gp_arrh_check_type<T>(ARR)
#else
#  define GP_ARRH_CHECK(T, ARR) 0 ? (void)((GPArray(T)){0} = (ARR)) : (void)0
#endif

#if __GNUC__ && !defined(GP_PEDANTIC) && __STDC_VERSION__ >= 201112L
#  define GP_CHECK_ARR_STATIC_LENGTH(T, ARR, ...) \
({ \
    _Static_assert( \
        sizeof( (T[]){(T){0},__VA_ARGS__} ) - sizeof(T) <= sizeof((ARR)->data), \
        "Initializer list larger than array capacity."); \
    (void)0; \
})
#else // TODO C++ and C99, we'll just ignore the check for now
#  define GP_CHECK_ARR_STATIC_LENGTH(T, ARR, ...) (void)0
#endif

#if !__cplusplus // GP_ARR_STATIC stuff // TODO does this work with function pointer arrays?? (spiral rule)

#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE_LENGTH(T, ARR, ...) \
( \
    GP_CHECK_ARR_STATIC_LENGTH(T, ARR, __VA_ARGS__), \
    sizeof( (T[]){(T){0},__VA_ARGS__} ) / sizeof(T) - 1 \
)

#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE(T, ARR, ...) \
( \
    0 ? \
        (T*){0} = (ARR)->data \
    : \
        memcpy((ARR)->data, (T[GP_COUNT_ARGS(__VA_ARGS__) + 1]){(T){0},__VA_ARGS__} + 1, sizeof(T) * (ARR)->length) \
)

#else
#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE_LENGTH(T, ARR, ...) \
\
    sizeof( T[]{__VA_ARGS__} ) / sizeof(T)

#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE(T, ARR, ...) \
\
    memcpy((ARR)->data, T[]{__VA_ARGS__}, (ARR)->length)

#endif // GP_ARR_STATIC stuff

// Avoid -Wpedantic warning about function pointer to void* conversion
#if __STDC_VERSION__ >= 202311L
#  define GP_FPTR_TO_VOIDPTR(FPTR) \
( \
    *(void**)memcpy(&(void*){0}, &(void(*)()){(void(*)())(FPTR)}, sizeof(void(*)())) \
)
#else
#  define GP_FPTR_TO_VOIDPTR(FPTR) (FPTR)
#endif

#if __cplusplus
} // extern "C"
#endif

#endif // GP_ARRAY_INCLUDED
