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

// The element size is almost known at compile time. Compilers produce way
// better code when using this compile time constant instead of reading the
// element size from the array header, especially with inline functions, so we
// implement all functions in such way that they take explicit element size
// parameter. However, we don't export these functions to FFI, that would waste
// an argument register redundantly since element size can be fetched from array
// header, which is always read anyway. So keep sized functions private, but
// implement everything in terms of sized functions.
//
// It would make sense to have the implementations of the exported functions in
// the source file, but macro shadowing makes that difficult considering single
// header users, so we'll just define them here as GP_INLINE.

// arr being dynamic is a precondition, it wouldn't ever make sense to call this
// for a fixed size array.
GP_HIDDEN GP_NONNULL_ARGS()
size_t gp_arr_reallocate_sized(void** arr, size_t capacity, size_t element_size);

GP_NONNULL_ARGS()
static inline size_t gp_arr_reserve_sized(void** arr, size_t capacity, size_t element_size);

GP_NONNULL_ARGS()
static inline bool gp_arr_push_sized(void**, const void* GP_RESTRICT, size_t);

GP_NONNULL_ARGS_AND_RETURN GP_HIDDEN
void* gp_arr_finalize_sized(void* arr, size_t element_size);

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
 * stored in memory before the first element. This gives several advantages
 * beyond `[n]` element access syntax:
 *
 * - Type safety on most operations.
 * - Fits in register.
 * - True generality without requiring the user to instantiate macro templates.
 * - No need to allocate the structure separately. This also improves cache
 *   coherency.
 * - Functions that take a generic `void*' argument like @ref gp_thread_create
 *   accept arrays.
 * - Semantics somewhat familiar to regular C arrays in the sense that our arrays
 *   seemingly decay to pointers (not really, they already are) and can be used
 *   as inputs to any function that accepts pointers.
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
 */ // TODO examples

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

/** Possible value of @ref GP_ARR_FAIL_MODE.
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

/** Possible value of @ref GP_ARR_FAIL_MODE.
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
 * the exported functions should use @ref GP_ARR_ERROR_ERTURN, which is the
 * default when exporting.
 *
 * This only has an effect to fixed capacity arrays when they exceed array
 * capacity. This will not have an effect to dynamic arrays. To control error
 * policy when an allocator might fail to reallocate a dynamic array, see @ref GP_ALLOC_FAIL_MODE.
 *
 * Possible values:
 *
 * - @ref GP_ARR_ERROR_RETURN: Array functions can fail and return the number of truncated elements.
 * - @ref GP_ARR_ERROR_ABORT: Array functions abort on failure. Default for C/C++.
 * - @ref GP_ARR_ERROR_UNDEFINED: Array functions invoke undefined behavior on failure.
 *
 * See the documentation for those macros for more details about their meanings.
 */
#ifndef GP_ARR_FAIL_MODE
#  if !defined(GPC_IMPLEMENTATION)
#    define GP_ARR_FAIL_MODE GP_ARR_ERROR_ABORT
#  else
#    define GP_ARR_FAIL_MODE GP_ARR_ERROR_RETURN
#  endif
#elif GP_ARR_FAIL_MODE < 0 || 2 < GP_ARR_FAIL_MODE
#  error Invalid GP_ARR_FAIL_MODE.
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
     * Access this using @ref gp_arr_capacity().
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
     * @ref gp_arr_element_size() and @ref gp_arr_alignment() for the subfields.
     *
     * Alignment takes six bits in 64-bit systems, five bits in 32-bit systems.
     * On 64-bit systems, the address space is only 48 bits, so both size and
     * alignment fit just fine, but on 32-bit systems, this restricts the
     * maximum element size to 128 MB. This restriction only affects exported
     * functions, C/C++ macros uses compile time size using `sizeof`.
     *
     * Any alignments below @ref GP_ALLOC_ALIGNMENT are internally rounded up
     * to @ref GP_ALLOC_ALIGNMENT, so it is acceptable to leave all bits as zero.
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
     * Access this using @ref gp_arr_allocator().
     *
     * This is the allocator passed to @ref gp_arr_new() or @ref gp_arr_new_aligned(),
     * or `NULL` if array has fixed capacity.
     *
     * Most common use case for direct access is to "finalize" the array by
     * making it fixed by setting the allocator to `NULL`. Example:
     *
     * ```c
     *     GPArray(int) arr = gp_arr_new(&my_arena->base, 64, sizeof arr[0]);
     *     // Fill arr with stuff...
     *     gp_arr_set(arr)->allocator = NULL; // finalize.
     * ```
     */
    GPAllocator* allocator;

    /** Number of elements in the array. */
    size_t length;
} GPArrayHeader;

#if SIZE_MAX == UINT32_MAX
#  define GP_ARR_ALIGNMENT_BIT_INDEX (32 - 5)
#else
#  define GP_ARR_ALIGNMENT_BIT_INDEX (64 - 6)
#endif

#define GP_ARR_ELEMENT_SIZE_MASK (((size_t)1 << GP_ARR_ALIGNMENT_BIT_INDEX) - 1)

//-------------------------------------
// Accessors

GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_length(GPArrayConstAny arr) { return ((GPArrayHeader*)arr - 1)->length; }

GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_capacity(GPArrayConstAny arr) { return ((GPArrayHeader*)arr - 1)->capacity; }

GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
GPAllocator* gp_arr_allocator(GPArrayConstAny arr) { return ((GPArrayHeader*)arr - 1)->allocator; }

GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_element_size(GPArrayConstAny arr)
{
    return ((GPArrayHeader*)arr - 1)->element_info & GP_ARR_ELEMENT_SIZE_MASK;
}

GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
size_t gp_arr_alignment(GPArrayConstAny arr)
{
    return (size_t)1 << (((GPArrayHeader*)arr - 1)->element_info >> GP_ARR_ALIGNMENT_BIT_INDEX);
}

// TODO remember to document that this is only meaningful for arrays not
// allocated statically.
GP_NONNULL_ARGS() GP_NODISCARD GP_INLINE
void* gp_arr_allocation(GPArrayConstAny arr)
{
    size_t alignment = gp_arr_alignment(arr);
    if (alignment <= GP_ALLOC_ALIGNMENT)
        return (void*)((GPArrayHeader*)arr - 1);
    return (void*)((char*)arr - alignment);
}

/** Direct access to array header.
 *
 * Used to access array header members. Example of clearing the array:
 *
 * ```c
 *     gp_arr_set(my_array)->length = 0;
 * ```
 *
 * This is probably the most common use case for this. Can also be used to
 * obtain pointer to array header without unsafe and error prone pointer
 * arithmetic.
 *
 * @return pointer to array header.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
GPArrayHeader* gp_arr_set(GPArrayAny arr)
{
    return (GPArrayHeader*)arr - 1;
}

// TODO overloads with type argument for type checking

//-------------------------------------
// Memory

GP_INLINE size_t gp_arr_reserve(GPArrayAny* arr, size_t size)
{
    return gp_arr_reserve_sized(arr, size, gp_arr_element_size(*arr));
}

GP_INLINE size_t gp_arr_reallocate(GPArrayAny* arr, size_t capacity)
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
 */
GP_INLINE GPArrayAny gp_arr_recycle(GPArrayAny arr, size_t element_size)
{
    #if SIZE_MAX == UINT32_MAX
    gp_assume(element_size <= GP_ARR_ELEMENT_SIZE_MASK,
              "Can't fit element size in element_info bit field.");
    #endif
    gp_arr_set(arr)->capacity *= gp_arr_element_size(arr);
    gp_arr_set(arr)->capacity /= element_size;
    gp_arr_set(arr)->element_info &= ~GP_ARR_ELEMENT_SIZE_MASK;
    gp_arr_set(arr)->element_info |= element_size;
    gp_arr_set(arr)->length = 0;
    gp_asan_poison(arr, gp_arr_capacity(arr) * element_size);

    // Spamming memcpy() everywhere gets around aliasing for most uses, but
    // laundering is still needed for direct access.
    return gp_launder(arr);
}

GP_INLINE void* gp_arr_finalize(GPArrayAny arr)
{
    gp_arr_finalize_sized(arr, gp_arr_element_size(arr));
}

//-------------------------------------
// Modification

/// @}
//------------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
//------------------------------------------------------------------------------
/// @cond

static inline size_t gp_arr_reserve_sized(
    GPArrayAny* arr, size_t capacity, size_t element_size)
{
    #ifdef GP_STATIC_ANALYSIS // GCC static analyzer can't keep up with
    gp_launder(arr);          // conditional reallocations, which causes a lot
    #endif                    // of buffer overflow false positives.
    if (capacity <= gp_arr_capacity(*arr))
        return 0;
    #if GP_ARR_FAIL_MODE == GP_ARR_ERROR_RETURN
    if (gp_arr_allocator(*arr) == NULL)
        return capacity - gp_arr_capacity(*arr);
    #elif GP_ARR_FAIL_MODE == GP_ARR_ERROR_ABORT
    gp_assert(gp_arr_allocator(*arr) != NULL, "Exceeding fixed size array capacity.");
    #else
    gp_assume(gp_arr_allocator(*arr) != NULL, "Exceeding fixed size array capacity.");
    #endif
    return gp_arr_reallocate_sized(arr, gp_next_power_of_two(capacity), element_size);
}

static inline bool gp_arr_push_sized(
    GPArrayAny* dest, const void*GP_RESTRICT element, size_t element_size)
{
    bool trunced = gp_arr_reserve_sized(dest, gp_arr_length(*dest) + 1, element_size);
    if ( ! trunced) {
        memcpy(
            (char*)*dest + gp_arr_length(*dest)*element_size, element, element_size);
        gp_arr_set(*dest)->length++;
    }
    return trunced;
}

// TODO overload gp_arr_reserve() and gp_arr_reallocate() to take an optional
// size argument for type checking since they don't have input argument to check
// against.

#ifdef __cplusplus
} // extern "C"
#endif

/// @endcond
#endif // GP_ARRAY_INCLUDED
