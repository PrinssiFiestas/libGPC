// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
/// @cond

#ifndef GP_ARRAY_INCLUDED
#define GP_ARRAY_INCLUDED 1

#include <gpc/gpmemory.h>

#ifdef __cplusplus
extern "C" {
#endif

// We implement all functions here as inline functions. This feels a bit nasty
// for larger functions, but there are a few of reasons to do this:
//
// - Our shadowing macros can use sizeof operator to use compile time element
//   size instead of reading it from the array header. This was observed to
//   produce significantly better code.
// - Allocation and array failure modes require exposing at least parts of the
//   implementation. These checks may be in the middle of the function, so might
//   as well expose the whole thing.
// - Exposing preconditions is always nice for optimizations and static analysis.
//
// We don't expose the sized functions to FFI. Bindings don't get the
// performance benefits of sizeof anyway.
//
// It would make sense to have the implementations of the exported functions in
// the source file, but macro shadowing makes that difficult considering single
// header users, so we'll just define them here as GP_INLINE.

static inline bool gp_arr_reserve_sized(void**, size_t, size_t);

static inline bool gp_arr_reallocate_sized(void**, size_t, size_t);

static inline void* gp_arr_finalize_sized(void*, size_t);

static inline void* gp_arr_recycle_sized(void*, size_t, size_t);

static inline void* gp_arr_push_sized(void**, size_t);

static inline void* gp_arr_pop_sized(void**, size_t);

static inline void* gp_arr_copy_sized(void**, const void*GP_RESTRICT, size_t, size_t);

static inline void* gp_arr_slice_sized(void**, const void*GP_RESTRICT, size_t, size_t, size_t);

static inline void* gp_arr_append_sized(void**, const void*, size_t, size_t);

static inline void* gp_arr_insert_sized(void**, size_t, const void*GP_RESTRICT, size_t, size_t);

static inline void* gp_arr_erase_sized(void**, size_t, size_t, size_t);

/// @endcond
//------------------------------------------------------------------------------
//
//          API REFERENCE
//
//------------------------------------------------------------------------------
/** @defgroup array Array
 * ```c
 * #include <gpc/gparray.h>
 * ```
 * This module provides a type safe generic array inspired by
 * [stb_ds](https://github.com/nothings/stb/blob/master/stb_ds.h) that is mostly
 * used as a dynamic array that utilizes @ref GPAllocator, but supports fixed
 * capacity as well. The static and dynamic functionality is unified to a single
 * interface and is somewhat analogous to a combination of C++
 * `std::pmr::vector` and `std::inplace_vector`. We don't have dedicated types
 * for both, the "staticness" is a runtime property of any given @ref GPArray
 * object determined by the allocator used if any.
 *
 * Like `stb_ds`, our arrays are just pointers of any type. The meta-data is
 * stored in memory before the first element. This gives several advantages:
 *
 * - Familiar syntax: elements are accessed with regular array subscript `[]`
 *   operator. Example: `my_int_array[7] += 3`.
 * - Type safety on most operations.
 * - Fits in register.
 * - True generality without requiring the user to instantiate macro templates.
 * - No need to allocate the structure separately. This also improves cache
 *   coherency.
 * - Functions that take a generic `void*' argument like @ref gp_thread_create
 *   accept arrays without separate allocation.
 * - Semantics somewhat familiar to regular C arrays in the sense that our arrays
 *   seemingly decay to pointers (not really, they already are) and can be used
 *   as inputs to any function that accepts pointers.
 *
 * Our arrays encapsulate a pointer to an allocator to determine how they are
 * created, destroyed, and how they reallocate. This is a required parameter to
 * all constructors. If you are not familiar with allocators, see @ref GPAllocator,
 * but you can get started immediately by simply passing @ref gp_heap as an
 * argument to all parameters of type `GPAllocator*`. We strongly encourage
 * users to learn to use allocators to break free from the shackles of the evil
 * default heap.
 *
 * We implement bounds checking in debug builds for our arrays using address
 * sanitizer poisoning (requires compiling with `-fsanitize=address`. Most
 * accesses beyond the length of the array will trap on sanitized builds. The
 * limitation is that trapping addresses are multiples of eight due to ASan
 * requirements, so bounds checks are not exact for element sizes smaller than
 * eight. If direct access to elements beyond array length (but obviously below
 * array capacity) is needed, then the memory can be manually unpoisoned using
 * @ref gp_arr_reserve() or @ref gp_asan_unpoison.
 *
 * While the implementation of all functions here rely on `void*`, the functions
 * are shadowed by macros that automatically perform appropriate casts and type
 * checking for convenience and type safety. This means that all functions are
 * exported to binary as usual despite being wrapped in macros in C/C++ code.
 *
 * This documentation documents the exported functions instead of the macros.
 * The macros are semantically close enough to the functions that they don't
 * need dedicated documentation, any significant differences are documented for
 * each function. However, there are a couple of significant differences shared
 * between all functions and macros:
 *
 * - Output arguments are of type `GPArrayAny*`, which is an alias to `void**`.
 *   However, you shouldn't cast output arguments to `GPArrayAny*` or `void**`
 *   when using the macros. This would break type checking and will not compile.
 *   The cast is done automatically by the macro under the hood.
 * - Input arguments are of type `void*`. However unlike plain `void*`, not all
 *   inputs are accepted: the type must match the output type. For example, for
 *   output argument of type `GPArray(short)*`, you cannot use an input argument
 *   of type `int*`, the input must be `short*`. Incompatible pointers would
 *   cause aliasing violations anyway, but regular `void*` wouldn't warn about it.
 * @{
 */ // TODO Examples.
    // TODO Documenting failure modes for each function would be too repetitive.
    //      Think about a good place to document them.

/** Array of given type.
 *
 * In memory, an array is @ref GPArrayHeader followed by the elements. An object
 * of type `GPArray(T)` is a pointer to the first element of type `T`. While `T*`
 * would work fine, functions expecting our arrays also expect a very specific
 * memory layout, not just any pointers, so this macro is used to differentiate
 * between pointers and our arrays. Type should not be `void`, use @ref GPArrayAny
 * if `GPArray(void)` is needed.
 *
 * If @ref GP_TYPEOF is defined, then this also avoids spiral rule. For example,
 * a `GPArray(void(*)(int))` is an array of function pointers that take an `int`
 * and return `void`. If @ref GP_TYPEOF is not defined, then requires a typedef.
 *
 * Example of a function declaration using integer arrays:
 *
 * ```c
 * // Appends src to the array pointed by dest.
 * void append_ints(GPArray(int)* dest, GPArray(int) src);
 * ```
 */
#define GPArray(...) GP_PTR_TO(__VA_ARGS__)

/** Possible value of @ref GP_ARR_ERROR_MODE.
 *
 * Indicates that array functions may return the number of truncated elements
 * when passing a fixed capacity array to array functions. This is the default
 * for functions exported to binary, because for FFI, the other options would be
 * limiting, the FFI has to give full control to the user.
 *
 * Indicates that fixed capacity array overflows can be handled by checking the
 * return value of the function, which is most commonly the number of truncated
 * elements.
 *
 * Best practice would be to always use this and handle errors appropriately.
 */
#define GP_ARR_ERROR_RETURN 0

/** Possible value of @ref GP_ARR_ERROR_MODE.
 *
 * Indicates that array functions abort execution when exceeding fixed capacity
 * array's capacity. This is the default for C/C++ code.
 *
 * We didn't make this the default, because of it being the best default (it
 * isn't). We made it the default, because we know that there are plenty of lazy
 * developers, who may or may not bother to check for errors. Ignoring the error
 * return value can lead to undefined behavior (e.g. buffer overflow), so this
 * protects from that resulting to a more deterministic failure, which forces
 * the user to fix their bugs instead of ignoring them leading to overall better
 * software quality globally. However, for maximum quality software, we
 * recommend using @ref GP_ARR_ERROR_RETURN instead and handle errors
 * appropriately.
 */
#define GP_ARR_ERROR_ABORT 1

/** Possible value of @ref GP_ARR_FAIL_UNDEFINED.
 *
 * Indicates that array functions invoke undefined behavior when exceeding fixed
 * capacity array's capacity.
 *
 * This practically for the most part just omits a couple of instructions in
 * release builds that check if arrays have fixed capacity (allocator is `NULL`).
 * However, UB is UB, so anything can happen, which might cause undeterministic
 * mayhem in production if a fixed array capacity is actually exceeded.
 * Therefore, using this is generally speaking discouraged.
 *
 * The purpose for this is to be used for maximally performance critical
 * applications. Usually the performance critical code is a small part of the
 * program, so only use this for the translation units that contain the
 * performance critical code.
 */
#define GP_ARR_ERROR_UNDEFINED 2

/** Determines behavior when exceeding fixed capacity arrays capacity.
 *
 * Can be defined per header file inclusion to control array overflow policy
 * for each translation unit separately. If defined when compiling this library,
 * then affects foreign function interface, which is probably not what you want,
 * the exported functions should use @ref GP_ARR_ERROR_RETURN, which is the
 * default when exporting.
 *
 * This only has an effect to fixed capacity arrays when they exceed array
 * capacity. This will not have an effect to dynamic arrays. To control error
 * policy when an allocator might fail to reallocate a dynamic array, see @ref GP_ALLOC_ERROR_MODE.
 *
 * Possible values:
 *
 * - @ref GP_ARR_ERROR_RETURN: Array functions can fail and return the number of truncated elements.
 * - @ref GP_ARR_ERROR_ABORT: Array functions abort on failure. Default for C/C++.
 * - @ref GP_ARR_ERROR_UNDEFINED: Array functions invoke undefined behavior on failure.
 *
 * See the documentation for those macros for more details about their meanings.
 */
#ifndef GP_ARR_ERROR_MODE
#  if !defined(GPC_IMPLEMENTATION)
#    define GP_ARR_ERROR_MODE GP_ARR_ERROR_ABORT
#  else
#    define GP_ARR_ERROR_MODE GP_ARR_ERROR_RETURN
#  endif
#elif GP_ARR_ERROR_MODE < 0 || 2 < GP_ARR_ERROR_MODE
#  error Invalid GP_ARR_ERROR_MODE.
#endif

/** Array of any type.
 *
 * @ref GPArray requires a complete type, `GPArray(void)` will not compile, so
 * this is needed for generic arrays.
 */
typedef void* GPArrayAny;

/** Constant array of any type.
 *
 * @ref GPArray requires a complete type, `GPArray(const void)` will not compile,
 * so this is needed for generic constant arrays.
 */
typedef const void* GPArrayConstAny;

/** Array meta-data.
 *
 * All fields have dedicated getters (e.g. @ref gp_arr_length() for the length
 * field) and a shared setter @ref gp_arr_set() for direct access, which is also
 * used to obtain the pointer to the header from an array. You should almost
 * always use the dedicated getters.
 *
 * Don't use pointer arithmetic for direct access, use @ref gp_arr_set()
 * instead. You rarely need direct access, you should be fine almost always
 * using functions provided by us. However, in case that direct access is ever
 * needed, we document how and why you might use direct access to each member.
 */
typedef struct // tagless for effective type compatibility with GPStringHeader.
{
    /** Number of elements that fit in the array.
     *
     * Get the value of this using @ref gp_arr_capacity().
     *
     * The author of this documentation can't really think of any good reasons
     * to access this directly. If you have a good reason to do so, let us know
     * and we'll update this documentation.
     */
    size_t capacity;

    /** Element size and alignment.
     *
     * Bit field containing alignment on most significant bits and element size
     * in least significant bits. Valid alignments are powers of two, so we only
     * store the bit index.
     *
     * There is no getter for the full bit field, but there are getters
     * @ref gp_arr_element_size() and @ref gp_arr_alignment() for the sub bit fields.
     *
     * Alignment takes six bits in 64-bit systems, five bits in 32-bit systems.
     * On 64-bit systems, the address space is only 48 bits, so both size and
     * alignment fit just fine, but on 32-bit systems, this restricts the
     * maximum element size to 128 MB.
     *
     * Alignment is used to find the start of the allocation, which is obviously
     * very important for reallocations and deallocation, so alignment should
     * always remain unchanged.
     *
     * The author of this documentation can't really think of any good reasons
     * to access this directly. The only potential reason would be to recycle
     * memory, but we have @ref gp_arr_recycle() for that.
     */
    size_t element_info;

    /** Allocator used to create, reallocate, and deallocate the array.
     *
     * Get the value of this using @ref gp_arr_allocator().
     *
     * This is the allocator passed to @ref gp_arr_new() or @ref gp_arr_new_aligned(),
     * or `NULL` if array has fixed capacity.
     *
     * Most common use case for direct access is to make a dynamic array fixed
     * by setting this to `NULL`. Just keep in mind that @ref gp_arr_delete()
     * obviously requires an allocator to be able to deallocate, so you must
     * remember to set the allocator back before deallocating.
     *
     * ### Example
     *
     * ```c
     * extern GPAllocator* alc;
     *
     * void foo(void)
     * {
     *     GPArray(int) arr = gp_arr_new(alc, 64, sizeof arr[0]);
     *     ... // fill arr with stuff...
     *     gp_arr_set(arr)->allocator = NULL; // make array fixed.
     *     ... // use arr...
     *     // Deallocate (optional for some allocators like arenas)
     *     gp_arr_set(arr)->allocator = alc;
     *     gp_arr_delete(arr);
     * }
     * ```
     */
    GPAllocator* allocator;

    /** Number of elements in the array.
     *
     * Get the value of this using @ref gp_arr_length(). Increasing length
     * beyond capacity is likely to lead to undefined behavior (buffer overflow).
     *
     * Directly accessing this is most useful for unchecked operations to
     * increase performance. Most common workflow is to first reserve space
     * using @ref gp_arr_reserve(), then assign elements to the reserved space
     * while increasing length. See @ref gp_arr_reserve() for details of this.
     *
     * Keep in mind that the memory might be poisoned by address sanitizer on
     * debug builds, so direct access might trap even if it is known that there
     * is enough capacity. This is normally not an issue, because @ref gp_arr_reserve()
     * unpoisons the newly reserved memory, but if you want to access without
     * calling @ref gp_arr_reserve(), then you might want to unpoison manually
     * using @ref gp_asan_unpoison(). Poisoning is not an issue in release builds.
     */
    size_t length;
} GPArrayHeader;

/** Static array buffer.
 *
 * Used to create a static or stack allocated `GPArray(T)`. Create a variable of
 * this type, then pass it by address to @ref gp_arr_buffered() to initialize
 * and convert it to `GPArray(T)`. This type is not meant to be used directly, it
 * is meant to be used as a statically allocated buffer to be converted to
 * GPArray(T).
 *
 * The elements are aligned to @ref GP_ALLOC_ALIGN boundary for most of our
 * targets (C11, C++11, and some compilers like TCC). However, in strict C99,
 * the structure is only pointer aligned.
 */
#define GPArrayBuffer(T, CAPACITY)  \
struct GP_ANONYMOUS_STRUCT \
{ \
    GP_ARRAY_ALIGN GPArrayHeader header; \
    GP_TYPEOF_TYPE(T) data[CAPACITY]; \
}

/** Create a statically allocated `GPArray(T)` from a @ref GPArrayBuffer.
 *
 * Initializes a fixed capacity array optionally with given elements. The
 * uninitialized elements will have undefined contents and are poisoned when
 * compiling with address sanitizer.
 *
 * First argument ís the element type. The second argument is a pointer to a
 * @ref GPArrayBuffer structure used to back the memory. The remaining arguments
 * are optional and are used to initialize the array.
 *
 * @return an initialized fixed capacity array of type `GPArray(T)`.
 *
 * ### Example
 *
 * ```c
 *     GPArrayBuffer(int, 16) buffer;
 *     GPArray(int) array = gp_arr_buffered(int, &buffer, 1, 2, 3, 4, 5);
 *     gp_assert(gp_arr_length(array) == 5);
 *     gp_assert(gp_arr_capacity(array) == 16);
 *     gp_assert(gp_arr_allocator(arra) == NULL);
 *     for (int i = 1; i <= 5; i++)
 *         gp_assert(array[i] = i);
 *     array = gp_arr_buffered(int, &buffer); // clear
 *     gp_assert(gp_arr_length(array) == 0);
 *     gp_assert(gp_arr_capacity(array) == 16);
 * ```
 */
#define/* GPArray(T) */gp_arr_buffered( \
    T,                                  \
    /* GPArrayBuffer(T, N)* BUFFER, */  \
    /* optional initial values */...)   \
( \
    (GP_1ST_ARG(__VA_ARGS__))->header.capacity =                                                \
        GP_SIZEOF_VALUE((GP_1ST_ARG(__VA_ARGS__))->data) /                                      \
            GP_SIZEOF_VALUE((GP_1ST_ARG(__VA_ARGS__))->data[0]),                                \
    (GP_1ST_ARG(__VA_ARGS__))->header.allocator = NULL,                                         \
    (GP_1ST_ARG(__VA_ARGS__))->header.element_info =                                            \
        ((size_t)GP_ALLOC_ALIGNMENT_BIT_INDEX << GP_ARR_ALIGNMENT_BIT_INDEX) | sizeof(T),       \
    (GP_1ST_ARG(__VA_ARGS__))->header.length =                                                  \
        GP_ARR_STATIC_OPTIONAL_INITIALIZE_LENGTH(GP_TYPEOF_TYPE(T), __VA_ARGS__),               \
    GP_ARR_STATIC_OPTIONAL_INITIALIZE(GP_TYPEOF_TYPE(T), __VA_ARGS__),                          \
    gp_asan_poison(                                                                             \
        (GP_1ST_ARG(__VA_ARGS__))->data + (GP_1ST_ARG(__VA_ARGS__))->header.length,             \
        ((GP_1ST_ARG(__VA_ARGS__))->header.capacity - (GP_1ST_ARG(__VA_ARGS__))->header.length) \
            * sizeof(T)),                                                                       \
    /* return */(GP_1ST_ARG(__VA_ARGS__))->data                                                 \
)

#if SIZE_MAX == UINT32_MAX
#  define GP_ARR_ALIGNMENT_BIT_INDEX (32 - 5)
#else
#  define GP_ARR_ALIGNMENT_BIT_INDEX (64 - 6)
#endif

#define GP_ARR_ELEMENT_SIZE_MASK (((size_t)1 << GP_ARR_ALIGNMENT_BIT_INDEX) - 1)

//-------------------------------------
// Accessors

/** Number of elements in the given array.
 *
 * C/C++ code accepts an additional type argument for type checking to prevent
 * double pointer bugs (example: `gp_arr_length(int_array, int)`.
 */
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_length(GPArrayConstAny arr) { return ((GPArrayHeader*)arr - 1)->length; }

/** Number if elements that would fit to the given array without reallocation.
 *
 * C/C++ code accepts an additional type argument for type checking to prevent
 * double pointer bugs (example: `gp_arr_capacity(int_array, int)`.
 */
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_capacity(GPArrayConstAny arr) { return ((GPArrayHeader*)arr - 1)->capacity; }

/** Allocator used to create, reallocate, and deallocate the given array.
 *
 * C/C++ code accepts an additional type argument for type checking to prevent
 * double pointer bugs (example: `gp_arr_allocator(int_array, int)`.
 */
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
GPAllocator* gp_arr_allocator(GPArrayConstAny arr) { return ((GPArrayHeader*)arr - 1)->allocator; }

/** Runtime element size of the given array.
 *
 * Provided mostly for debugging, validation, and for foreign function interface.
 * C/C++ code should simply use the `sizeof` operator most of the time.
 */
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_element_size(GPArrayConstAny arr)
{
    return ((GPArrayHeader*)arr - 1)->element_info & GP_ARR_ELEMENT_SIZE_MASK;
}

/** Alignment requirement of the given array.
 *
 * Values below @ref GP_ALLOC_ALIGNMENT can be considered to indicate @ref GP_ALLOC_ALIGNMENT
 * due to internal requirements of this library.
 *
 * C/C++ code accepts an additional type argument for type checking to prevent
 * double pointer bugs (example: `gp_arr_alignment(int_array, int)`.
 */
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_alignment(GPArrayConstAny arr)
{
    return (size_t)1 << (((GPArrayHeader*)arr - 1)->element_info >> GP_ARR_ALIGNMENT_BIT_INDEX);
}

/** Pointer to the beginning of the memory block of the given array.
 *
 * Usually the beginning of the memory allocation of any @ref GPArray is the
 * address of the @ref GPArrayHeader. However, if the alignment requirement of
 * the array is larger than @ref GP_ALLOC_ALIGNMENT, then the memory block may
 * have padding before the header, so the beginning of the memory block would be
 * before the first element of the header.
 *
 * C/C++ code accepts an additional type argument for type checking to prevent
 * double pointer bugs (example: `gp_arr_allocation(int_array, int)`.
 */
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
void* gp_arr_allocation(GPArrayConstAny arr)
{
    size_t alignment = gp_arr_alignment(arr);
    if (alignment <= GP_ALLOC_ALIGNMENT)
        return (GPArrayHeader*)arr - 1;
    return (char*)arr - alignment;
}

/** Direct access to array header.
 *
 * Used to directly access array header members. See @ref GPArrayHeader for
 * detailed documentation on how to use this. This is almost always used to set
 * members of the array, hence the name.
 *
 * C/C++ code accepts an additional type argument for type checking to prevent
 * double pointer bugs (example: `gp_arr_set(int_array, int)->length = 0`.
 *
 * @return pointer to the array header structure of @a arr.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
GPArrayHeader* gp_arr_set(GPArrayAny arr)
{
    return (GPArrayHeader*)arr - 1;
}

//-------------------------------------
// Memory and Initialization

/** Creates a new empty array.
 *
 * Creates an array of elements of size @a element_size that can fit @a init_capacity
 * number of elements before having to reallocate. @a alc is used to allocate
 * the memory and will be used for reallocations and deallocation if needed.
 *
 * @return the newly created array.
 */
#if GP_ALLOC_ERROR_MODE != GP_ALLOC_ERROR_RETURN
GP_NONNULL_RETURN
#endif
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
GPArrayAny gp_arr_new(
    GPAllocator* alc,
    size_t init_capacity,
    size_t element_size)
{
    gp_assume(element_size <= GP_ARR_ELEMENT_SIZE_MASK,
              "Can't fit element size in element_info bit field.");
    size_t full_size;
    gp_assume(gp_size_mul(&full_size, init_capacity, element_size),
              "Multiplication exceeded GP_ALLOC_MAX_SIZE.");

    GPArrayHeader* header = (GPArrayHeader*)alc->alloc(
        alc,
        NULL, 0,
        full_size + sizeof *header,
        GP_ALLOC_ALIGNMENT,
        true,
        &full_size);
    #if GP_ALLOC_ERROR_MODE == GP_ALLOC_ERROR_RETURN
    if (header == NULL)
        return NULL;
    #else
    GP_ALLOC_CHECK(header != NULL);
    #endif

    header->length = 0;
    header->capacity = (full_size - sizeof *header) / element_size;
    header->allocator = alc;
    header->element_info = element_size;
    header->element_info |= (size_t)GP_ALLOC_ALIGNMENT_BIT_INDEX << GP_ARR_ALIGNMENT_BIT_INDEX;
    gp_asan_poison(header + 1, full_size - sizeof *header);

    return header + 1;
}

/** Creates a new empty array with aligned elements.
 *
 * Like @ref gp_arr_new() except the elements are guaranteed to be aligned to
 * @a alignment boundary. All alignment values below @ref GP_ALLOC_ALIGNMENT
 * will be rounded up to @ref GP_ALLOC_ALIGNMENT.
 *
 * @ref gp_arr_new() creates arrays with alignment of @ref GP_ALLOC_ALIGNMENT,
 * which enough for most use cases. However, some data (like SIMD) might require
 * larger alignments, which is why this function is important.
 */
#if GP_ALLOC_ERROR_MODE != GP_ALLOC_ERROR_RETURN
GP_NONNULL_RETURN
#endif
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
GPArrayAny gp_arr_new_aligned(
    GPAllocator* alc,
    size_t init_capacity,
    size_t element_size,
    size_t alignment)
{
    gp_assume(element_size <= GP_ARR_ELEMENT_SIZE_MASK,
              "Can't fit element size in element_info bit field.");
    gp_assume((alignment & (alignment - 1)) == 0, "Alignment must be a power of two.");
    if (alignment < GP_ALLOC_ALIGNMENT)
        alignment = GP_ALLOC_ALIGNMENT;

    size_t full_size;
    gp_assume(gp_size_mul(&full_size, init_capacity, element_size),
              "Multiplication exceeded GP_ALLOC_MAX_SIZE.");

    size_t header_size = sizeof(GPArrayHeader);
    if (alignment > GP_ALLOC_ALIGNMENT)
        header_size = alignment;

    char* memory = (char*)alc->alloc(
        alc,
        NULL, 0,
        full_size + header_size,
        alignment,
        true,
        &full_size);
    #if GP_ALLOC_ERROR_MODE == GP_ALLOC_ERROR_RETURN
    if (memory == NULL)
        return NULL;
    #else
    GP_ALLOC_CHECK(memory != NULL);
    #endif

    GPArrayHeader* header = (GPArrayHeader*)memory;
    if (alignment > GP_ALLOC_ALIGNMENT)
        header = (GPArrayHeader*)(memory + alignment - sizeof *header);

    header->length = 0;
    header->capacity = (full_size - header_size) / element_size;
    header->allocator = alc;
    header->element_info = element_size;
    #if SIZE_MAX == UINT32_MAX
    size_t bit_index = 31 - gp_leading_zeros_32(alignment);
    #else
    size_t bit_index = 63 - gp_leading_zeros_64(alignment);
    #endif
    header->element_info |= bit_index << GP_ARR_ALIGNMENT_BIT_INDEX;
    gp_asan_poison(header + 1, full_size - header_size);

    return header + 1;
}

/** Free array memory.
 *
 * Passing fixed capacity arrays is safe too. In such case, this function does
 * nothing.
 */
GP_INLINE void gp_arr_delete(GPArrayAny optional_array)
{
    if (optional_array == NULL || gp_arr_allocator(optional_array) == NULL)
        return;

    char* arr = (char*)optional_array;
    char* end = arr + gp_arr_capacity(arr) * gp_arr_element_size(arr);
    char* start = (char*)gp_arr_allocation(arr);
    GPAllocator* alc = gp_arr_allocator(arr);

    alc->dealloc(alc, start, end - start, gp_arr_alignment(arr));
}
// delete() is not be macro shadowed, it is a common destructor, so users
// should be able to take function pointers of it.

/** Reserve capacity.
 *
 * If @a capacity is larger than `gp_arr_capacity(*arr)`, then reallocates,
 * otherwise does nothing and returns zero. In case of reallocation, capacity
 * will be rounded up exponentially (which differs from `std::vector::reserve()`).
 * Memory for the reserved space will be unpoisoned for direct access.
 *
 * The basic workflow is to reserve enough space for any given operation and
 * then do the operation by directly writing to the memory and manually updating
 * the length. Example concatenating two integer arrays:
 *
 * ```c
 * extern GPArray(int) is;
 *
 * size_t concat(GPArray(const int) src)
 * {
 *     size_t trunced = gp_arr_reserve(
 *         &is, gp_arr_capacity(is) + gp_arr_length(src));
 *     if (trunced)
 *         return trunced;
 *     // Space reserved, direct access safe
 *     for (size_t i = 0; i < gp_arr_length(src); i++)
 *         is[gp_arr_set(is)->length++] = src[i];
 *     return 0;
 * }
 * ```
 *
 * @return 0 if capacity will be large enough to hold @a capacity elements
 * (success). Otherwise, returns @a capacity to conceptually indicate that
 * truncation will happen.
 */
GP_INLINE bool gp_arr_reserve(GPArrayAny* arr, size_t capacity)
{
    return gp_arr_reserve_sized(arr, capacity, gp_arr_element_size(*arr));
}

/** Reallocate an array.
 *
 * Request exact capacity for an array, which may be smaller than than the
 * current capacity. Usually you want to use @ref gp_arr_reserve() instead, but
 * this can be useful for shrink-to-fit (release unused elements) operation.
 *
 * Unlike @ref gp_arr_reserve(), passing a fixed capacity array is undefined.
 * Therefore, @ref GP_ARR_ERROR_MODE will not affect the behavior of this function.
 *
 * @return 0 if capacity will be large enough to hold @a capacity elements
 * (success). Otherwise, returns @a capacity to conceptually indicate that
 * truncation will happen.
 *
 * ### Example
 *
 * ```c
 * void shrink_to_fit(GPArrayAny* arr)
 * {
 *     gp_arr_reallocate(arr, gp_arr_length(*arr));
 * }
 * ```
 *
 * @return 0 if capacity will be large enough to hold @a capacity elements
 * (success). Otherwise, returns @a capacity.
 */
GP_INLINE bool gp_arr_reallocate(GPArrayAny* arr, size_t capacity)
{
    return gp_arr_reallocate_sized(arr, capacity, gp_arr_element_size(*arr));
}

/** Recycle array memory for other purposes.
 *
 * Clear the array and reuse the memory allocated for the array for an array of
 * another type. Capacity and element size will be updated according to the new
 * element size. All other fields including alignment remain unchanged.
 *
 * You should only use the returned pointer instead of @a arr after calling
 * this. To emphasize, you should _not_ use the original pointer (which is
 * @a arr) after calling this. Accessing memory trough the original pointer
 * _will_ lead to aliasing violations that can cause very difficult bugs. The
 * original pointer should be considered as freed, even though the actual memory
 * remains unchanged.

 * @return pointer to the conceptually new array, which has the same address as
 * @a arr.
 *
 * @warning EXPERIMENTAL: This API is subject to change or removal.
 */
GP_INLINE GPArrayAny gp_arr_recycle(GPArrayAny arr, size_t new_element_size)
{
    return gp_arr_recycle_sized(arr, new_element_size, gp_arr_element_size(arr));
}

/** Convert array to a @ref sized_ptr[sized pointer].
 *
 * Converts an array to a sized pointer and potentially releases any unused
 * memory (shrink to fit) including the now unused elements of the array header.
 * A reallocation to a smaller memory block may happen depending on the
 * allocator used for the array. If a reallocation does not happen, then the
 * memory is moved within the allocation to remove unused array header elements.
 * Either way, @a arr should be considered as freed after the call.
 *
 * It is safe to call this for fixed capacity arrays, although there is no
 * memory savings for fixed capacity arrays.
 *
 * This function never fails regardless of values of @ref GP_ARR_ERROR_MODE and
 * @ref GP_ALLOC_ERROR_MODE. If reallocation fails, then old memory is used.
 * Fixed capacity arrays won't even try to reallocate.
 *
 * @return the array converted to a sized pointer.
 *
 * @warning EXPERIMENTAL: This API is subject to change or removal.
 */
GP_INLINE void* gp_arr_finalize(GPArrayAny arr)
{
    return gp_arr_finalize_sized(arr, gp_arr_element_size(arr));
}

//-------------------------------------
// Modification

/** Add element to the end of an array.
 *
 * Increment array length and potentially assign an element to it. In C/C++ code,
 * the return type is casted to a pointer of an appropriate type if @ref GP_TYPEOF
 * is defined. In such case, you can simply use this like follows:
 *
 * ```c
 * extern GPArray(int) fds;
 *
 * void push_fd(int fd)
 * {
 *     *gp_arr_push(&fds) = fd;
 * }
 * ```
 *
 * The example above ignores `NULL` caused by potential reallocation failure,
 * which may be reasonable depending on the values of @ref GP_ARR_ERROR_MODE and
 * @ref GP_ALLOC_ERROR_MODE.
 *
 * C/C++ code accepts an additional element argument to be assigned to the new
 * element. For example, the expression `*gp_arr_push(&fds) = fd` is equivalent
 * to `gp_arr_push(&fds, fd)`. This is type safe regardless of availability of
 * @ref GP_TYPEOF, so this is generally recommended. However, note that this
 * ignores `NULL` check on failure, so again, only use when failure mode macros
 * are appropriately set.
 *
 * @return
 *
 * If no additional element argument is given, then returns a pointer to an
 * added uninitialized element at the end of the array or `NULL` if reallocation
 * failed or fixed capacity array ran out of capacity.
 *
 * If second argument (the element to be pushed) is given, then returns the
 * pushed element by value.
 */
GP_INLINE void* gp_arr_push(GPArrayAny* arr)
{
    return gp_arr_push_sized(arr, gp_arr_element_size(*arr));
}

/** Remove element from the end of an array.
 *
 * The array must not be empty if @ref GP_ARR_ERROR_MODE is not defined to
 * @ref GP_ARR_ERROR_RETURN.
 *
 * This will never reallocate. The array is only passed by address to signal
 * mutation for consistency with other mutating functions.
 *
 * C/C++ code accepts an additional type argument for type checking and
 * returned pointer cast (example: `int i = *gp_arr_pop(&int_array, int);`).
 *
 * @return a pointer to the removed element, which is valid as long as no new
 * elements are added to @a arr. If the array is empty and @ref GP_ARR_ERROR_MODE
 * is defined to @ref GP_ARR_ERROR_RETURN, then returns `NULL`. The returned
 * pointer is casted to a pointer of element type in C/C++ code if @ref GP_TYPEOF
 * is defined or if type checking argument is passed.
 */
GP_INLINE void* gp_arr_pop(GPArrayAny* arr)
{
    return gp_arr_pop_sized(arr, gp_arr_element_size(*arr));
}

/** Copy elements.
 *
 * Copy @a src_length elements from @a src to the array pointed by @a dest.
 *
 * @return `*dest` on success, `NULL` otherwise.
 */
GP_INLINE void* gp_arr_copy(GPArrayAny* dest, const void*GP_RESTRICT src, size_t src_length)
{
    return gp_arr_copy_sized(dest, src, src_length, gp_arr_element_size(*dest));
}

/** Copy or remove elements.
 *
 * Copy elements from @a optional_src starting from @a start_index to @a end_index
 * excluding @a end_index to the array pointed by @a dest.
 *
 * If @a optional_src is `NULL`, then removes elements outside of @a start_index
 * and @a end_index including the element at @a end_index are removed and the
 * remaining elements are moved to the beginning of the array.
 *
 * @return `*dest` on success, `NULL` otherwise.
 */
GP_INLINE void* gp_arr_slice(
    GPArrayAny* dest,
    const void*GP_RESTRICT optional_src,
    size_t start_index,
    size_t end_index)
{
    return gp_arr_slice_sized(
        dest, optional_src, start_index, end_index, gp_arr_element_size(*dest));
}

/** Add elements to the end.
 *
 * Copy @a src_length elements from @a src to the end of the array pointed by
 * @a dest.
 *
 * @return pointer to the first appended element on success, `NULL` otherwise.
 */
GP_INLINE void* gp_arr_append(GPArrayAny* dest, const void* src, size_t src_length)
{
    return gp_arr_append_sized(dest, src, src_length, gp_arr_element_size(*dest));
}

/** Add elements to the specified position.
 *
 * Copy @a src_length elements from @a src to the index specified by @a position.
 * The existing elements starting from @a position to the end of the array are
 * moved to the end of the array.
 *
 * @a position must be less than or equal to the length of the array. The error
 * behavior for this condition is determined by @ref GP_ARR_ERROR_MODE.
 *
 * @return pointer to the first inserted element on success, `NULL` otherwise.
 */
GP_INLINE void* gp_arr_insert(
    GPArrayAny* dest, size_t position, const void*GP_RESTRICT src, size_t src_length)
{
    return gp_arr_insert_sized(dest, position, src, src_length, gp_arr_element_size(*dest));
}

/** Remove elements
 *
 * Removes @a count elements starting from @a position moving the rest of the
 * elements over. If @a count is zero, then does nothing. Will not reallocate.
 *
 * @a position and @a count specify a range of elements to be removed. If @a count
 * is non-zero, then it is an error for the start of the range to be out of
 * bounds. The error behavior is determined by @ref GP_ARR_ERROR_MODE. However,
 * it is not an error for the end of the range to be out of bounds. In such case,
 * the range will be truncated. This allows to easily remove all elements after
 * @a position by setting @a count to `SIZE_MAX` or `(size_t)-1`. This is
 * equivalent to `gp_arr_set(arr)->length = position`, except that using this
 * function instead handles address sanitizer poisoning properly.
 *
 * @return pointer to the element following the last removed element or `NULL`
 * if @a position is out of bounds and @ref GP_ARR_ERROR_MODE is defined to
 * @ref GP_ARR_ERROR_RETURN. The returned pointer will be out of bounds
 * (`*dest + gp_arr_length(*dest)`) if the last removed element is also the last
 * element of the array.
 */
GP_INLINE void* gp_arr_erase(
    GPArrayAny* dest, size_t position, size_t count)
{
    return gp_arr_erase_sized(dest, position, count, gp_arr_element_size(*dest));
}

/// @}
//------------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
//------------------------------------------------------------------------------
/// @cond

//-------------------------------------
// Memory

GP_NONNULL_ARGS()
static inline bool gp_arr_reserve_sized(
    GPArrayAny* arr, size_t capacity, size_t element_size)
{
    #ifdef GP_STATIC_ANALYSIS // GCC static analyzer can't keep up with
    gp_launder(arr);          // conditional reallocations, which caused a lot
    #endif                    // of buffer overflow false positives.

    if (capacity <= gp_arr_capacity(*arr)) {
        if (capacity > gp_arr_length(*arr))
            gp_asan_unpoison(
                (char*)*arr + gp_arr_length(*arr) * element_size,
                (capacity - gp_arr_length(*arr)) * element_size);
        return true;
    }

    #if GP_ARR_ERROR_MODE == GP_ARR_ERROR_RETURN
    if (gp_arr_allocator(*arr) == NULL)
        return false;
    #elif GP_ARR_ERROR_MODE == GP_ARR_ERROR_ABORT
    gp_assert(gp_arr_allocator(*arr) != NULL, "Exceeding fixed size array capacity.");
    #else
    gp_assume(gp_arr_allocator(*arr) != NULL, "Exceeding fixed size array capacity.");
    #endif

    size_t success = gp_arr_reallocate_sized(
        arr, gp_next_power_of_two(capacity), element_size);

    if (success && capacity > gp_arr_length(*arr))
        gp_asan_unpoison(
            (char*)*arr + gp_arr_length(*arr) * element_size,
            (capacity - gp_arr_length(*arr)) * element_size);
    return success;
}

GP_HIDDEN GP_NONNULL_ARGS()
static inline bool gp_arr_reallocate_sized(
    GPArrayAny* arrptr, size_t capacity, size_t element_size)
{
    gp_assume(gp_arr_allocator(*arrptr) != NULL, "Cannot reallocate fixed capacity array.");

    if (capacity < gp_arr_length(*arrptr))
        capacity = gp_arr_length(*arrptr);

    char* arr = (char*)*arrptr;
    size_t old_capacity = gp_arr_capacity(arr);
    char* end = arr + old_capacity * element_size;
    char* start = (char*)gp_arr_allocation(arr);
    GPAllocator* alc = gp_arr_allocator(arr);
    size_t alignment = gp_arr_alignment(arr);
    size_t length = gp_arr_length(arr);

    size_t full_size;
    size_t new_size;
    gp_assume(gp_size_mul(&new_size, capacity, element_size),
              "Multiplication exceeded GP_ALLOC_MAX_SIZE.");
    size_t header_size = arr - start;

    char* memory = (char*)alc->alloc(
        alc,
        start,
        end - start,
        new_size + header_size,
        alignment,
        true,
        &full_size);

    if (memory == NULL) {
        if (capacity <= old_capacity)
            return true;
        else
            #if GP_ALLOC_ERROR_MODE == GP_ALLOC_ERROR_RETURN
            return false;
            #else
            GP_ALLOC_CHECK(memory != NULL);
            #endif
    }

    GPArrayHeader* header = (GPArrayHeader*)memory;
    if (alignment > GP_ALLOC_ALIGNMENT)
        header = (GPArrayHeader*)(memory + alignment - sizeof *header);

    header->capacity = (full_size - header_size) / element_size;
    gp_asan_poison(
        (char*)(header + 1) + length * element_size,
        full_size - header_size - length * element_size);

    *arrptr = header + 1;
    return true;
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline void* gp_arr_finalize_sized(GPArrayAny arr, size_t element_size)
{
    gp_assume(gp_arr_alignment(arr) <= GP_ALLOC_ALIGNMENT,
              "Sized pointers do not support alignments larger than GP_ALLOC_ALIGNMENT.");

    size_t length = gp_arr_length(arr);
    size_t old_capacity = gp_arr_capacity(arr);
    size_t new_size = length*element_size;

    GPSizedPtrHeader* header = memmove(
        gp_arr_set(arr),
        (char*)arr - sizeof(GPSizedPtrHeader),
        sizeof(GPSizedPtrHeader) + length*element_size);

    // We cannot dereference header yet, the effective type is wrong. Use a
    // dummy variable and memcpy() to update effective type. Any halfway decent
    // compiler will optimize this out in optimized builds (GCC confirmed).
    if (header->allocator != NULL) {
        GPSizedPtrHeader dummy;
        memcpy(&dummy, header, sizeof dummy);
        memcpy(header, &dummy, sizeof dummy);
    } else // cannot modify effective type of static arrays.
        header = gp_launder(header);

    if (header->allocator != NULL) { // shrink to fit
        size_t ignore_out_size;
        GPSizedPtrHeader* new = header->allocator->alloc(
            header->allocator,
            header,
            sizeof(GPArrayHeader) + old_capacity*element_size,
            sizeof *header + new_size,
            GP_ALLOC_ALIGNMENT,
            true,
            &ignore_out_size);

        // If shrink to fit failed, then deallocation will fail for any
        // allocator that requires matching size. However, some allocators may
        // ignore size and some allocations do not need deallocation to begin
        // with, so failing is not reasonable. Anyway any reasonable allocator
        // would not fail on shrink to fit, but warn the user in debug builds
        // just in case.
        #ifdef GP_TARGET_DEBUG
        gp_expect(new != NULL,
                  "Warning: Shrink to fit failed when converting array to sized pointer. "
                  "Deallocating the sized pointer may fail.");
        #endif
        if (new != NULL)
            header = new;
    }
    // else array is fixed, shrink to fit would be meaningless.

    header->size = new_size;
    return header + 1;
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD
static inline GPArrayAny gp_arr_recycle_sized(
    GPArrayAny arr, size_t new_element_size, size_t old_element_size)
{
    gp_assume(new_element_size <= GP_ARR_ELEMENT_SIZE_MASK,
              "Can't fit element size in element_info bit field.");

    gp_arr_set(arr)->capacity *= gp_arr_element_size(arr);

    // If capacity is not a multiple of new_element_size, then the division
    // after this if-statement truncates, which would break deallocation size
    // calculation. Therefore, the allocation size must be updated for the
    // allocator to avoid UB. We first check if old_element_size is a multiple
    // of new_element_size, these are usually obtained using sizeof, so the
    // compiler might have a chance to get rid of the branch completely.
    if (gp_arr_allocator(arr) != NULL
        && old_element_size % new_element_size
        && gp_arr_capacity(arr) % new_element_size)
    {
        bool reallocated = gp_arr_reallocate_sized(
            &arr,
            gp_arr_capacity(arr) - gp_arr_capacity(arr) % new_element_size,
            sizeof(char));

        // Check comment in gp_arr_finalize_sized() for explanation of this.
        #ifdef GP_TARGET_DEBUG
        gp_expect(reallocated,
                  "Warning: Truncating extra array capacity failed when recycling array. "
                  "Deallocating the array may fail.");
        #else
        (void)reallocated;
        #endif
    }

    gp_arr_set(arr)->capacity /= new_element_size;
    gp_arr_set(arr)->element_info &= ~GP_ARR_ELEMENT_SIZE_MASK;
    gp_arr_set(arr)->element_info |= new_element_size;
    gp_arr_set(arr)->length = 0;
    gp_asan_poison(arr, gp_arr_capacity(arr) * new_element_size);

    // Spamming memcpy() everywhere gets around aliasing for most uses, but
    // laundering is still needed for statically allocated arrays and direct access.
    return gp_launder(arr);
}

//-------------------------------------
// Modifiers

#if GP_ARR_ERROR_MODE != GP_ARR_ERROR_RETURN && GP_ALLOC_ERROR_MODE != GP_ALLOC_ERROR_RETURN
#  define GP_ARR_NONNULL_RETURN GP_NONNULL_RETURN
#  define GP_ARR_CHECK_RESERVE(RESERVED) gp_assume(RESERVED)
#else
#  define GP_ARR_NONNULL_RETURN
#  define GP_ARR_CHECK_RESERVE(RESERVED) if ( ! (RESERVED)) return NULL
#endif

GP_ARR_NONNULL_RETURN GP_NONNULL_ARGS()
static inline void* gp_arr_push_sized(GPArrayAny* dest, size_t element_size)
{
    bool reserved = gp_arr_reserve_sized(dest, gp_arr_length(*dest) + 1, element_size);
    GP_ARR_CHECK_RESERVE(reserved);
    return (char*)*dest + gp_arr_set(*dest)->length++ * element_size;
}

#if GP_ARR_ERROR_MODE != GP_ARR_ERROR_RETURN
GP_NONNULL_RETURN
#endif
GP_NONNULL_ARGS()
static inline void* gp_arr_pop_sized(GPArrayAny* a, size_t element_size)
{
    GPArray(unsigned char) arr = (GPArray(unsigned char))*a;
    #if GP_ARR_ERROR_MODE == GP_ARR_ERROR_RETURN
    if (gp_arr_length(arr) == 0)
        return NULL;
    #elif GP_ARR_ERROR_MODE == GP_ARR_ERROR_ABORT
    gp_assert(gp_arr_length(arr) > 0, "Array must not be empty.");
    #else
    gp_assume(gp_arr_length(arr) > 0, "Array must not be empty.");
    #endif
    return arr + --gp_arr_set(arr)->length * element_size;
}

GP_ARR_NONNULL_RETURN GP_NONNULL_ARGS()
static inline void* gp_arr_copy_sized(
    GPArrayAny* arrptr, const void*GP_RESTRICT src, size_t src_length, size_t element_size)
{
    bool reserved = gp_arr_reserve_sized(arrptr, src_length, element_size);
    GP_ARR_CHECK_RESERVE(reserved);

    #ifdef GP_STATIC_ANALYSIS // analyzer false positive
    gp_assume(gp_arr_capacity(*arrptr) >= src_length);
    #endif
    size_t old_length = gp_arr_length(*arrptr);
    gp_arr_set(*arrptr)->length = src_length;
    memcpy(*arrptr, src, src_length * element_size);

    if (src_length < old_length)
        gp_asan_poison(
            (char*)*arrptr + src_length * element_size,
            (old_length - src_length) * element_size);

    return *arrptr;
}

GP_ARR_NONNULL_RETURN GP_NONNULL_ARGS(1)
static inline void* gp_arr_slice_sized(
    GPArrayAny* pdest,
    const void*GP_RESTRICT optional_src,
    size_t start_index,
    size_t end_index_exclusive,
    size_t element_size)
{
    gp_assume(start_index <= end_index_exclusive, "Invalid range.");

    size_t length = end_index_exclusive - start_index;

    if (optional_src == NULL) {
        if (length != 0) {
            #if GP_ARR_ERROR_MODE == GP_ARR_ERROR_RETURN
            if (start_index >= gp_arr_length(*pdest)
                || end_index_exclusive > gp_arr_length(*pdest))
                return length;
            #elif GP_ARR_ERROR_MODE == GP_ARR_ERROR_ABORT
            gp_assert(start_index < gp_arr_length(*pdest));
            gp_assert(end_index_exclusive <= gp_arr_length(*pdest));
            #else
            gp_assume(start_index < gp_arr_length(*pdest));
            gp_assume(end_index_exclusive <= gp_arr_length(*pdest));
            #endif
        }
        memmove(*pdest, (char*)*pdest + start_index*element_size, length*element_size);
    } else {
        bool reserved = gp_arr_reserve_sized(pdest, length, element_size);
        GP_ARR_CHECK_RESERVE(reserved);
        memcpy(*pdest, (char*)optional_src + start_index*element_size, length*element_size);
    }

    size_t old_length = gp_arr_length(*pdest);
    gp_arr_set(*pdest)->length = length;
    if (length < old_length)
        gp_asan_poison(
            (char*)*pdest + length * element_size,
            (old_length - length) * element_size);

    return *pdest;
}

GP_ARR_NONNULL_RETURN GP_NONNULL_ARGS()
static inline void* gp_arr_append_sized(
    GPArrayAny* parr,
    const void* src, // note: it's ok to pass *parr as src
    size_t src_length,
    size_t element_size)
{
    size_t length = gp_arr_length(*parr);
    bool reserved = gp_arr_reserve_sized(parr, length + src_length, element_size);
    GP_ARR_CHECK_RESERVE(reserved);

    gp_arr_set(*parr)->length += src_length;
    return memcpy((char*)*parr + length*element_size, src, src_length*element_size);
}

GP_ARR_NONNULL_RETURN GP_NONNULL_ARGS()
static inline void* gp_arr_insert_sized(
    GPArrayAny* parr,
    size_t position,
    const void*GP_RESTRICT src,
    size_t src_length,
    size_t element_size)
{
    size_t length = gp_arr_length(*parr);
    #if GP_ARR_ERROR_MODE == GP_ARR_ERROR_RETURN
    if (position > length)
        return NULL;
    #elif GP_ARR_ERROR_MODE == GP_ARR_ERROR_ABORT
    gp_assert(position <= length, "Index out of bounds.");
    #else
    gp_assume(position <= length, "Index out of bounds.");
    #endif

    bool reserved = gp_arr_reserve_sized(parr, length + src_length, element_size);
    GP_ARR_CHECK_RESERVE(reserved);

    size_t tail_length = length - position;

    gp_arr_set(*parr)->length += src_length;
    memmove(
        (char*)*parr + (position + src_length) * element_size,
        (char*)*parr +  position               * element_size,
        tail_length                            * element_size);
    return memcpy(
        (char*)*parr + position*element_size, src, src_length*element_size);
}

static inline void* gp_arr_erase_sized(
    GPArrayAny* parr, size_t position, size_t count, size_t element_size)
{
    GPArrayAny arr = *parr;
    if (count != 0)
        #if GP_ARR_ERROR_MODE == GP_ARR_ERROR_RETURN
        if (position >= gp_arr_length(arr))
            return NULL;
        #elif GP_ARR_ERROR_MODE == GP_ARR_ERROR_ABORT
        gp_assert(position < gp_arr_length(arr), "Index out of bounds.");
        #else
        gp_assume(position < gp_arr_length(arr), "Index out of bounds.");
        #endif
    else if (position + count > gp_arr_length(arr))
        count = gp_arr_length(arr) - position;

    size_t tail_length = gp_arr_length(arr) - (position + count);
    gp_arr_set(arr)->length -= count;

    void* p = memmove(
        (char*)arr +  position          * element_size,
        (char*)arr + (position + count) * element_size,
        tail_length                     * element_size);
    gp_asan_poison((char*)arr + gp_arr_length(arr)*element_size, count*element_size);
    return p;
}

//------------------------------------------------------------------------------
//
//
//          MACRO SHADOWING
//
//
//------------------------------------------------------------------------------
// Limited docs here to be shown in IDE.

#ifdef __clang__
// Allow {0} for any type, which is the most portable 0 init before C23. This is
// mostly used for type safe shadowing macros.
#  pragma clang diagnostic ignored "-Wmissing-braces"
// Silence clang-tidy bugprone-sizeof-expression needed for type safe macro
// shadows.
#  define GP_SIZEOF_VALUE(...) sizeof(GP_TYPEOF(__VA_ARGS__))
#else
#  define GP_SIZEOF_VALUE(...) sizeof(__VA_ARGS__)
#endif

#if __STDC_VERSION__ >= 201112L || GP_HAS_INCLUDE(<stdalign.h>)
#  include <stdalign.h>
#  define GP_ARRAY_ALIGN alignas(GP_ALLOC_ALIGNMENT)
#elif defined(__cplusplus) && __cplusplus >= 201103L
#  define GP_ARRAY_ALIGN alignas(GP_ALLOC_ALIGNMENT)
#else // alignment of stack arrays is a bit pedantic, it doesn't really hurt
      // to only have pointer alignment.
#  define GP_ARRAY_ALIGN
#endif

#if !defined(__cplusplus)
#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE_LENGTH(T, ARR, ...) \
( \
    gp_static_assert(sizeof( (GP_TYPEOF_TYPE(T)[]){(T){0},__VA_ARGS__} ) - sizeof(T) \
        <= sizeof((ARR)->data), "Initializer list larger than array capacity."), \
    sizeof( (GP_TYPEOF_TYPE(T)[]){(T){0},__VA_ARGS__} ) / sizeof(T) - 1 \
)
#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE(T, ARR, ...) \
( \
    0 ? \
        (GP_PTR_TO(T)){0} = (ARR)->data \
    : \
        memcpy( \
            (ARR)->data, \
            (GP_TYPEOF_TYPE(T)[GP_COUNT_ARGS(__VA_ARGS__) + 1]){(T){0},__VA_ARGS__} + 1, \
            sizeof(T) * (ARR)->header.length) \
)
#else
#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE_LENGTH(T, ARR, ...) \
    sizeof( GP_TYPEOF_TYPE(T)[]{__VA_ARGS__} ) / sizeof(T)
#  define GP_ARR_STATIC_OPTIONAL_INITIALIZE(T, ARR, ...) \
    memcpy((ARR)->data, T[]{__VA_ARGS__}, (ARR)->length)
#endif // GP_ARR_STATIC stuff

#if !defined(GP_NO_MACRO_SHADOWING) && !defined(GPC_IMPLEMENTATION) //----------

//-------------------------------------
// Accessors

#ifndef __cplusplus
#  define GP_ARRH_CHECK(ARR, T) (0 ? (void)((GPArray(T)){0} = (ARR)) : (void)0)
#else
template <typename T> static inline void gp_arrh_check_type(T* arr) { (void)arr; }
#  define GP_ARRH_CHECK(ARR, T) gp_arrh_check_type<T>(ARR)
#endif

// Type checked trivial getters are fully implemented as macros. For example,
// GP_ARR_LENGTH() is not implemented using gp_arr_length(). This is to prevent
// the debugger from jumping in a trivial function that we almost never care
// about.

#define GP_ARR_LENGTH_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    ((GPArrayHeader*)(ARR) - 1)->length \
)
#define gp_arr_length(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_LENGTH_WITH_TYPE_CHECK, gp_arr_length)(__VA_ARGS__)

#define GP_ARR_CAPACITY_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    ((GPArrayHeader*)(ARR) - 1)->capacity \
)
#define gp_arr_capacity(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_CAPACITY_WITH_TYPE_CHECK, gp_arr_capacity)(__VA_ARGS__)

#define GP_ARR_ALLOCATOR_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    ((GPArrayHeader*)(ARR) - 1)->allocator \
)
#define gp_arr_allocator(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_ALLOCATOR_WITH_TYPE_CHECK, gp_arr_allocator)(__VA_ARGS__)

#define GP_ARR_ELEMENT_SIZE_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    ((GPArrayHeader*)(ARR) - 1)->element_info & GP_ARR_ELEMENT_SIZE_MASK \
)
#define gp_arr_element_size(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_ELEMENT_SIZE_WITH_TYPE_CHECK, gp_arr_element_size)(__VA_ARGS__)

#define GP_ARR_ALIGNMENT_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    (size_t)1 << (((GPArrayHeader*)(ARR) - 1)->element_info >> GP_ARR_ALIGNMENT_BIT_INDEX) \
)
#define gp_arr_alignment(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_ALIGNMENT_WITH_TYPE_CHECK, gp_arr_alignment)(__VA_ARGS__)

#define GP_ARR_ALLOCATION_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    gp_arr_allocation(ARR) \
)
#define gp_arr_allocation(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_ALLOCATION_WITH_TYPE_CHECK, gp_arr_allocation)(__VA_ARGS__)

#define GP_ARR_SET_WITH_TYPE_CHECK(ARR, T) \
( \
    GP_ARRH_CHECK(ARR, T), \
    (GPArrayHeader*)(ARR) - 1 \
)
#define gp_arr_set(...) GP_OVERLOAD2(__VA_ARGS__, \
    GP_ARR_SET_WITH_TYPE_CHECK, gp_arr_set)(__VA_ARGS__)

//-------------------------------------
// Memory

// Request capacity of CAP.
#define gp_arr_reserve(ARRPTR, CAP) gp_arr_reserve_sized( \
    (void**)(ARRPTR), CAP, GP_SIZEOF_VALUE(**(ARRPTR)))

// Request exact capacity of CAP.
#define gp_arr_reallocate(ARRPTR, CAP) gp_arr_reallocate_sized( \
    (void**)(ARRPTR), CAP, GP_SIZEOF_VALUE(**(ARRPTR)))

// Use array memory for an array of another type.
#define gp_arr_recycle(ARR, NEW_SIZE) gp_arr_recycle_sized(ARR, NEW_SIZE, GP_SIZEOF_VALUE(*(ARR)))

// Convert array to a sized pointer releasing unused memory.
#define gp_arr_finalize(ARR) GP_TYPEOF_CAST(ARR)gp_arr_finalize(ARR)
// We don't type checking argument for finalize(), users usually assigns the
// returned pointer to an appropriate type, which is where type checks happen.

//-------------------------------------
// Modification

#define GP_ARR_PUSH(ARRPTR) \
    (GP_TYPEOF_CAST(*(ARRPTR))gp_arr_push_sized((void**)(ARRPTR), GP_SIZEOF_VALUE(**(ARRPTR))))

#ifdef GP_TYPEOF
#  define GP_ARR_PUSH_ELEM(ARRPTR, ...) (*GP_ARR_PUSH(ARRPTR) = (__VA_ARGS__))
#else
#  define GP_ARR_PUSH_ELEM(ARRPTR, ...) \
( \
    gp_arr_reserve((void**)(ARRPTR), gp_arr_length(*(ARRPTR)) + 1), \
    (*(ARRPTR))[gp_arr_set(ARRPTR)->length++] = (__VA_ARGS__) \
)
#endif

// First argument is pointer to GPArray. Second optional argument is the
// element to be inserted to the end of the array. If no second argument is
// provided, then returns pointer to the new uninitialized element at the end of
// the array. If the second argument is provided, then returns the added element.
#define gp_arr_push(...) GP_OVERLOAD64(__VA_ARGS__, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, \
    GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH_ELEM, GP_ARR_PUSH,)(__VA_ARGS__)
// The crazy overload enables using compound literals containing commas as the element argument.

#define GP_ARR_POP(ARRPTR) \
    (GP_TYPEOF_CAST(*(ARRPTR))gp_arr_pop_sized((void**)(ARRPTR), GP_SIZEOF_VALUE(**(ARRPTR))))

#define GP_ARR_POP_WITH_TYPE_CHECK(ARRPTR, T) \
( \
    GP_ARRH_CHECK(*(ARRPTR), T), \
    (GP_PTR_TO(T))gp_arr_pop_sized((void**)(ARRPTR), GP_SIZEOF_VALUE(**(ARRPTR)))) \
)

// Remove element at the end of the array. Returns pointer to the removed element.
// The first argument is a pointer to an array. The second optional argument is
// type used for type checking and to cast the returned pointer to an appropriate type.
#define gp_arr_pop(...) GP_OVERLOAD2(__VA_ARGS__, GP_ARR_POP_WITH_TYPE_CHECK, GP_ARR_POP)(__VA_ARGS__)

#ifdef GP_TYPEOF
// Check for double pointer first for best error message (it is very common to
// forget to take the address of output arrays).
#  define GP_ARR_TYPEOF_CAST(ARRPTR) (GP_TYPEOF(**(ARRPTR))*)
#else
#  define GP_ARR_TYPEOF_CAST(ARRPTR)
#endif

#define GP_ARR_CHECK_ARGS(OUT, IN) sizeof((*(OUT) = (IN))[0])

#if __STDC_VERSION__ >= 201112L
// Accept int and void* so that the user may use NULL or 0 constants.
#  define GP_ARR_CHECK_ARGS_OPTIONAL(OUT, IN) sizeof(**(OUT) = _Generic( \
    (IN), void*: **(OUT), int: **(OUT), default: *(IN)))
// #elif defined(__cplusplus) // TODO
#else
// Can't do anything, ((void*)0) breaks sizeof.
#  define GP_ARR_CHECK_ARGS_OPTIONAL(OUT, IN)
#endif

// Copy SRC_LEN elements from SRC to array pointed by DEST.
#define gp_arr_copy(DEST, SRC, SRC_LEN) \
    (GP_ARR_TYPEOF_CAST(*(DEST))gp_arr_copy_sized( \
        (void**)(DEST), SRC, SRC_LEN, GP_ARR_CHECK_ARGS(DEST, SRC)))

// Copy elements from SRC beginning from index START to index END to array
// array pointed by DEST. If SRC is NULL, then elements of array pointed by DEST
// before index START and after index END will be removed moving the remaining
// elements to the beginning of the array.
#define gp_arr_slice(DEST, SRC, START, END) \
    (GP_ARR_TYPEOF_CAST(*(DEST))gp_arr_slice_sized( \
        (void**)(DEST), SRC, START, END, GP_ARR_CHECK_ARGS_OPTIONAL(DEST, SRC)))

// Copy SRC_LEN elements from SRC to the end of the array pointed by DEST.
#define gp_arr_append(DEST, SRC, SRC_LEN) \
    (GP_ARR_TYPEOF_CAST(*(DEST))gp_arr_append_sized( \
        (void**)(DEST), SRC, SRC_LEN, GP_ARR_CHECK_ARGS(DEST, SRC)))

// Copy SRC_LEN elements from SRC to the index POS moving the rest of the
// elements to the end of the array.
#define gp_arr_insert(DEST, POS, SRC, SRC_LEN) \
    (GP_ARR_TYPEOF_CAST(*(DEST))gp_arr_insert_sized( \
        (void**)(DEST), POS, SRC, SRC_LEN, GP_ARR_CHECK_ARGS(DEST, SRC)))

// Remove COUNT elements at index POS.
#define gp_arr_erase(DEST, POS, COUNT) \
    (GP_ARR_TYPEOF_CAST(*(DEST))gp_arr_erase_sized( \
        (void**)(DEST), POS, COUNT, GP_SIZEOF_VALUE(**(DEST))))

#endif // MACRO SHADOWING

#ifdef __cplusplus
} // extern "C"
#endif

/// @endcond
#endif // GP_ARRAY_INCLUDED
