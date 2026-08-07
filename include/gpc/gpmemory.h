// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_MEMORY_INCLUDED
#define GP_MEMORY_INCLUDED 1

#include <gpc/gpattributes.h>
#include <gpc/gpassert.h>
#include <gpc/gpthread.h>
#include <gpc/gputils.h>
#include <gpc/gpint128.h>
#include <stddef.h>

/// @cond
#ifdef __SANITIZE_ADDRESS__ // GCC and MSVC defines this with -fsanitize=address
#  include <sanitizer/asan_interface.h>
#  include <sanitizer/common_interface_defs.h>
#  define GP_HAS_SANITIZER 1
#  define GP_TRY_POISON_MEMORY_REGION(A, S)   ASAN_POISON_MEMORY_REGION(A, S)
#  define GP_TRY_UNPOISON_MEMORY_REGION(A, S) ASAN_UNPOISON_MEMORY_REGION(A, S)
#elif defined(__has_feature) // Clang defines this
#  if __has_feature(address_sanitizer)
#    include <sanitizer/asan_interface.h>
#    include <sanitizer/common_interface_defs.h>
#    define GP_HAS_SANITIZER 1
#    define GP_TRY_POISON_MEMORY_REGION(A, S)   ASAN_POISON_MEMORY_REGION(A, S)
#    define GP_TRY_UNPOISON_MEMORY_REGION(A, S) ASAN_UNPOISON_MEMORY_REGION(A, S)
#  else
#    define GP_HAS_SANITIZER 0
#    define GP_TRY_POISON_MEMORY_REGION(A, S)   ((void)(A), (void)(S))
#    define GP_TRY_UNPOISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
#  endif
#else
#  define GP_HAS_SANITIZER 0
#  define GP_TRY_POISON_MEMORY_REGION(A, S)   ((void)(A), (void)(S))
#  define GP_TRY_UNPOISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
#endif
/// @endcond

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
//
//          API REFERENCE
//
//------------------------------------------------------------------------------
/** @defgroup memory Memory Management
 *
 * ```c
 * #include <gpc/gpmemory.h>
 * ```
 *
 * @{
 */ // TODO detailed description/overview of this module. Focus on allocators.

/** Maximum allocation size.
 *
 * Can be user defined globally if needed usually to enforce stricter limits.
 *
 * If not defined by the user, then in 64-bit systems this will be defined to
 * the virtual address space size, which is 48 bits. There exists processors
 * with [57 bit address spaces](https://en.wikipedia.org/wiki/Intel_5-level_paging),
 * but 48 bits is a sensible default for maximum allocation anyway.
 *
 * If not defined by the user, then in 32-bit systems this will be defined to
 * `PTRDIFF_MAX`, which is the maximum that glibc `malloc()` accepts. Trying to
 * allocate more would anyway cause undefined behavior down the line (pointer
 * subtraction overflow), so it makes no sense to have it larger even if user
 * space is larger than 2 GB.
 */
#ifndef GP_ALLOC_MAX_SIZE
#  if SIZE_MAX <= UINT32_MAX
#    define GP_ALLOC_MAX_SIZE PTRDIFF_MAX
#  else
#    define GP_ALLOC_MAX_SIZE ((size_t)1 << 48)
#  endif
#endif

/** Default dynamic allocation alignment.
 *
 * All pointers allocated dynamically by this library can be assumed to have an
 * alignment of `2 * sizeof(size_t)` (8 on 32-bit systems, 16 on 64-bit systems).
 * Dynamically allocated pointers may have some other alignments, but they are
 * always explicitly requested (most notably using @ref GPAllocator.alloc()).
 *
 * For most common targets, this equals `alignof(max_align_t)`, but this is not
 * guaranteed and should not be relied upon. We intentionally simplified this to
 * always have the same value for the same architecture instead of using
 * `alignof(max_align_t)`, which depends not just on the target machine, but
 * sometimes also on the capabilities of the compiler.
 */
#define GP_ALLOC_ALIGNMENT (2 * sizeof(size_t))

/** Check if compiling with address sanitizer.
 *
 * Defined to 1 if compiling with `-fsanitize=address`, 0 otherwise. Tested with
 * GCC, Clang, and MSVC. Other compilers may be defined to a false negative.
 */
#ifdef GP_DOXYGEN
#  define GP_HAS_SANITIZER /* 0 or 1 */
#endif

/** Poison memory region for address sanitizer.
 *
 * Marks user allocated memory region as poisoned when using address sanitizer.
 * Any attempts to use poisoned memory will trap. This is useful for writing
 * custom allocators that can catch use-after-free bugs even when the custom
 * allocator doesn't actually return the freed memory to the OS allocator. All
 * of our allocators either return freed memory to the OS or uses this to mark
 * memory as freed if not documented otherwise.
 *
 * The whole requested region is not necessarily poisoned. Only the region that
 * is aligned to 8-byte addresses is poisoned. If @a region and @a region_size
 * are multiples of 8, then the whole region is poisoned.
 *
 * Requires compiling with `-fsanitize=address` make any effect. Does nothing
 * otherwise. For limiting access of memory pages regardless of sanitizer
 * settings, see @ref gp_mem_advice().
 *
 * This function is _not_ thread safe: two threads trying to poison the same
 * memory simultaneously is a data race.
 */
GP_INLINE void gp_asan_poison(void* region, size_t region_size)
{
    GP_TRY_POISON_MEMORY_REGION(region, region_size);
}

/** Unpoison memory region for address sanitizer.
 *
 * Marks user allocated memory region as unpoisoned when using address
 * sanitizer. Like @ref gp_asan_poison(), this works on memory regions aligned
 * to 8-byte boundaries.
 *
 * This function is _not_ thread safe: two threads trying to unpoison the same
 * memory simultaneously is a data race.
 */
GP_INLINE void gp_asan_unpoison(void* region, size_t region_size)
{
    GP_TRY_UNPOISON_MEMORY_REGION(region, region_size);
}

/** Safely multiply sizes.
 *
 * Multiplies @a n with @a m. If the result of the multiplication does not
 * exceed @ref GP_ALLOC_MAX_SIZE, then the result is stored to @a result.
 * Otherwise, value pointed by @a result is left unmodified.
 *
 * Unlike `calloc()`, our allocation functions either do not check for size
 * multiplication overflow at all or assert that no overflow happened. This is
 * because many programmers wrongly treat `NULL` returned by `calloc()`
 * unconditionally as an out of memory error without checking `errno`. Failed
 * allocations and failed multiplications are very much different errors that
 * would require very different handling. Therefore, this should be called
 * before allocating if there is a possibility of an overflow.
 *
 * @return `true` if multiplication of @a n and @a m stayed within
 * @ref GP_ALLOC_MAX_SIZE, `false` otherwise.
 */
GP_INLINE GP_NODISCARD bool gp_size_mul(size_t* result, size_t n, size_t m)
{
    #if SIZE_MAX == UINT32_MAX
    uint64_t size = (uint64_t)n * (uint64_t)m;
    if (size < GP_ALLOC_MAX_SIZE) {
        *result = size;
        return true;
    }
    #else
    GPUInt128 size = gp_uint128_mul64(n, m);
    if (gp_uint128_lo(size) < GP_ALLOC_MAX_SIZE && gp_uint128_hi(size) == 0) {
        *result = gp_uint128_lo(size);
        return true;
    }
    #endif
    return false;
}

/** Polymorphic memory allocator.
 *
 * Polymorphic memory allocators can be used to dispatch allocation and
 * deallocation functions at runtime. Some allocators may provide concrete
 * functions, but runtime dispatch is useful when embedding allocators to other
 * data structure. For example, @ref GPArray takes a pointer to an allocator as
 * a parameter to it's constructor and the array object will use the specified
 * allocator to reallocate if needed without having to know the concrete type of
 * the given allocator.
 *
 * Calling the member functions directly gives maximum control and may allow
 * some error handling for some allocators. However, these are somewhat
 * difficult to use and have strict requirements, so it is generally recommended
 * to use @ref gp_mem_alloc(), @ref gp_mem_alloc_array(), @ref gp_realloc(), and
 * @ref gp_dealloc() instead. Those functions are used to pass some default
 * parameters and do some basic checks that makes them easier to use. However,
 * calling these member functions directly is currently the only way of handling
 * errors and allocating memory with non-default alignment.
 *
 * Custom memory allocators can be written by inheriting from this structure by
 * having this as the first member of the custom allocator structure. See the
 * definition of any of our other allocators for examples of this.
 *
 * Allocator implementations are free to loosen up some requirements documented
 * for the member functions (e.g. requiring matching size and alignment for
 * deallocations). However, users of the allocator can only break them if the
 * given allocator documents that it is safe to do so. Otherwise, breaking the
 * requirements leads to undefined behavior.
 *
 * Requirements for a custom allocator have "allocators must" or similar wording
 * in this documentation. Of course users are free to implement whatever they
 * like if they know that they are the only user of their custom allocator, but
 * this library assumes that these invariants hold and _will_ exploit them
 * internally, so generally speaking the requirements should be respected.
 */
typedef struct GPAllocator
{
    /** Allocate or reallocate memory.
     *
     * Allocates a new block of memory or reallocates an old one. Many
     * allocators like @ref gp_heap require freeing the memory using
     * @ref GPAllocator.dealloc(), but some allocators might have dedicated
     * functions to free multiple pointers at once, in which case deallocation
     * is optional. However, even when optional, it is recommended to deallocate
     * most pointers, many allocators might do optimizations and might do debug
     * poisoning to catch use-after-free bugs on deallocations even if actual
     * deallocation did not happen.
     *
     * This may fail and return `NULL` if the allocator or it's backing
     * allocator (if any) runs out of memory. However, our allocators are
     * implemented in a way that they almost never fail and some parts of this
     * library (including @ref gp_mem_alloc()) do in fact assume that failure is
     * a non-recoverable critical error that doesn't happen for most of our
     * targets. Custom allocators are encouraged to be implemented in a similar
     * manner. For example, any allocation request that cannot be satisfied by
     * the given custom allocator could be outsourced to @ref gp_heap().
     *
     * @param[inout] me
     *     Pointer to this object. Use like so: `alc->alloc(alc, ...);`
     *
     * @param optional_old_block
     *     If `NULL`, then the operation is basic memory allocation. Otherwise,
     *     the operation is a reallocation and this parameter must be a pointer
     *     previously returned by this function that has not been deallocated.
     *
     * @param optional_old_block_size
     *     If not reallocating, then this parameter is ignored. Otherwise, this
     *     should match the size passed to `size` or returned by `actual_size`
     *     parameter of the previous call to this function.
     *
     * @param size
     *     Requested allocation size. Must not be zero. Allocators must return a
     *     memory block that has at least this size. Should be below @ref GP_ALLOC_MAX_SIZE.
     *     Allocators must _not_ assume that size is a multiple of alignment.
     *
     * @param alignment
     *     Requested allocation alignment. Must be a power of two. This library
     *     mostly assumes that pointers are aligned to @ref GP_ALLOC_ALIGNMENT
     *     boundaries. However, this can be set to some other value to
     *     explicitly request some other alignment. If unsure, set this to
     *     @ref GP_ALLOC_ALIGNMENT.
     *
     *     Allocators must return a memory block whose address is a multiple of
     *     this value. Allocators must not assume that @ref GP_ALLOC_ALIGNMENT,
     *     `alignof(max_align_t)`, or whatever the system heap allocator uses
     *     is the minimum alignment: allocators must be able to handle smaller
     *     alignments as well, although simply rounding up the requested value
     *     to whatever the allocator requires is a valid implementation.
     *
     * @param uninitialized
     *     Determines if memory should be zeroed. If `false` or 0, then
     *     allocators must zero initialize the newly allocated memory block.
     *     Otherwise, the new contents have undefined contents and reading them
     *     before writing leads to undefined behavior.
     *
     * @param[out] actual_size
     *     Many allocators (but not all) might round up the `size` parameter
     *     most notably to respect alignment requirements. The rounded up number
     *     is returned via this parameter and allocators must set it to at least
     *     `size` on allocations. This is useful to reduce fragmentation when the exact size of
     *     the allocation is not important. However, the amount of rounding
     *     completely depends on the given allocator and some may not round at
     *     all, so you should not rely on any exact values. Do not pass `NULL`,
     *     pass an address of a dummy variable to ignore.
     *
     * @return pointer to allocated memory or `NULL` if out of memory.
     */
    void* (*alloc)(
        struct GPAllocator* me,
        void*   optional_old_block,
        size_t  optional_old_block_size,
        size_t  size,
        size_t  alignment,
        bool    uninitialized,
        size_t* actual_size);

    /** Deallocate memory.
     *
     * Deallocate memory allocated with @ref GPAllocator.alloc(). Memory that
     * has been deallocated should not be accessed after deallocation.
     *
     * @param[inout] me
     *     Pointer to this object. Use like so: `alc->dealloc(alc, ...);`
     *
     * @param block
     *     Pointer to the object to be freed. This must be the non-null return
     *     value of @ref GPAllocator.alloc() and must not be already deallocated.
     *
     * @param size
     *     This must either be the `size` parameter passed to @ref GPAllocator.alloc()
     *     or the value returned by `actual_size` parameter of @ref GPAllocator.alloc().
     *     Any other value invokes undefined behavior.
     *
     * @param alignment
     *     This must be the `alignment` parameter passed to @ref GPAllocator.alloc().
     *     Any other value invokes undefined behavior.
     *
     * @return `true` if deallocation succeeded, `false` otherwise. Most
     * allocators never return `false`.
     */
    bool (*dealloc)(
        struct GPAllocator* me,
        void* block,
        size_t size,
        size_t alignment);
} GPAllocator;

/** Allocate memory.
 *
 * Allocates uninitialized memory block of size @a size, where @a size must be
 * larger than zero and smaller than @ref GP_ALLOC_MAX_SIZE.
 *
 * This function asserts that the given allocator does not fail or that
 * recovering from a failure is practically impossible. This is true for all of
 * our allocators. Custom allocators that do not give this guarantee should
 * directly call @ref GPAllocator.alloc() instead.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_alloc(GPAllocator* alc, size_t size)
{
    // We shouldn't assert 0<size here, 0 is valid for GPStadium.
    gp_assume(size < GP_ALLOC_MAX_SIZE);
    void* memory = alc->alloc(
        alc, NULL, 0, size, GP_ALLOC_ALIGNMENT, true, &size);
    gp_assert(memory != NULL);
    return memory;
}

/** Allocate an array.
 *
 * Allocates memory block of size `n * m` bytes, where @a n and @a m are number
 * of elements and element size. The order of @a n and @a m arguments does not
 * matter. The result of `n * m` should be below @a GP_ALLOC_MAX_SIZE. This can
 * be checked using @ref gp_size_mul().
 *
 * @a uninitialized determines if memory should be zeroed. If `false` or 0, then
 * allocators must zero initialize the newly allocated memory block. Otherwise,
 * the new contents have undefined contents and reading them before writing
 * leads to undefined behavior.
 *
 * Like @ref gp_mem_alloc(), this asserts that allocators will not fail.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_alloc_array(GPAllocator* alc, size_t n, size_t m, bool uninitialized)
{
    size_t size;
    gp_assume(gp_size_mul(&size, n, m), "Multiplication exceeded GP_ALLOC_MAX_SIZE.");
    void* memory = alc->alloc(
        alc, NULL, 0, size, GP_ALLOC_ALIGNMENT, uninitialized, &size);
    gp_assert(memory != NULL);
    return memory;
}

/** Reallocate memory.
 *
 * Request changing size of @a optional_block. If @a optional_block is `NULL`,
 * then the call is equivalent to `gp_mem_alloc(alc, new_size)`. Otherwise, @a alc
 * and @a optional_size must match the values passed to an allocation function
 * and @a optional_block must be a pointer returned by that allocation function.
 *
 * @return new memory if @a optional_block is `NULL`, `optional_block` if the
 * allocator was able to change the size of the allocation without moving the
 * memory, or new memory with contents of @a optional_block copied to it
 * otherwise. The new memory is uninitialized aside from the copied contents.
 *
 * Like @ref gp_mem_alloc(), this asserts that allocators will not fail.
 */
GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_realloc(
    GPAllocator* alc,
    void* optional_block,
    size_t optional_size,
    size_t new_size)
{
    gp_assume(new_size < GP_ALLOC_MAX_SIZE);
    void* memory = alc->alloc(
        alc, optional_block, optional_size, new_size, GP_ALLOC_ALIGNMENT, true, &new_size);
    gp_assert(memory != NULL);
    return memory;
}

/** Deallocate memory.
 *
 * Deallocate @a optional_block. If @a optional_block is `NULL`, then this
 * function does nothing and the other parameters are ignored. Otherwise, @a alc
 * and @a optional_block_size must match the values passed to an allocation
 * function and @a optional_block mute be a pointer returned by that allocation
 * function.
 *
 * The success status of the deallocation is asserted. This prevents security
 * issues caused by invalid arguments.
 */
GP_INLINE
void gp_mem_dealloc(
    GPAllocator* alc,
    void* optional_block,
    size_t optional_block_size)
{
    if (optional_block == NULL)
        return;

    bool success = alc->dealloc(
        alc, optional_block, optional_block_size, GP_ALLOC_ALIGNMENT);
    gp_assert(success);
}

/** Pointer to the heap allocator.
 *
 * Allocator pointer initialized to point to @ref gp_mallocator, which is the
 * wrapper allocator for the standard heap. See the documentation of @ref gp_mallocator
 * for details about this allocator.
 *
 * Applications can mutate this pointer to point to their allocator of choice.
 * This is mostly useful for debugging purposes, but some applications might
 * also benefit from a custom heap. If the assigned custom allocator is just
 * used to wrap the default heap allocator, then it is recommended that the
 * custom allocator uses @ref gp_mallocator that handles requirements of this
 * library instead of manually calling `malloc()` and `free()`.
 */
extern GPAllocator* gp_heap;

/** Allocator wrapping the standard heap.
 *
 * Allocator used for standard heap allocations. You generally should use this
 * trough the @ref gp_heap pointer instead of directly using this to ensure that
 * the global heap is in fact globally overridable.
 *
 * Unlike C11 `aligned_alloc()`, this allocator accepts any power of two
 * alignment (meaning also less than what would be accepted by the
 * implementation) and allocation sizes do not have to be a multiples of
 * alignment. Also, reallocation is possible for any alignment.
 *
 * If C23, deallocation uses `free_aligned_sized()`, which requires size and
 * alignment to match allocation size and alignment, otherwise undefined
 * behavior is invoked. This improves performance and security and matches
 * requirements for some of our other allocators, but may be inconvenient
 * sometimes. If this is an issue, use @ref sized_ptr or @ref GPArray.
 *
 * Attempting to mutate this is undefined behavior and may trap on some targets.
 * This is to ensure that there the global heap can always be accessed even if
 * @ref gp_heap is reassigned to some other allocator. The main purpose of this
 * is to be able to access this from the allocator that is used to override the
 * heap.
 */
#ifdef __ELF__
GP_GNU_ATTRIB(section("rodata"))
#endif
extern GPAllocator gp_mallocator;

//-------------------------------------
/** @defgroup sized_ptr Sized Pointers
 *
 * Pointers with size and allocator information.
 *
 * Our default deallocation interface (@ref gp_mem_dealloc() and @ref GPAllocator.dealloc())
 * require passing allocator and size that matches the corresponding allocation.
 * This can massively reduce the complexity of some allocators (e.g. power of
 * two allocators) and can significantly improve performance. However, this
 * might be inconvenient for the user of the allocator since now they have to
 * keep track of allocation size and the allocator used for the allocation. For
 * some applications, the user of the pointer might not care how the pointer was
 * allocated. For example, this is the case for generic destructors like the one
 * passed to @ref gp_thread_local_create().
 *
 * Sized pointers do not require the user to keep track of size, allocator, nor
 * alignment for deallocations and reallocations regardless of allocator used.
 * Internally this works by allocating a bit of extra memory (two pointers) for
 * the size and allocator information, but of course the pointers returned to
 * the user point just past the metadata, so that the pointers can be used just
 * like any other pointer.
 *
 * Currently sized pointers can only be used with an alignment of @ref GP_ALLOC_ALIGNMENT.
 * If other alignments are needed, use @ref GPArray instead.
 * @{
 */

typedef struct GPSizePtrHeader
{
    GPAllocator* allocator; ///< Allocator used to allocate the sized pointer.
    size_t size;            ///< Requested allocation size in bytes.
} GPSizePtrHeader;

/** Get requested allocation size of a sized pointer.
 *
 * @return size passed to the function used to allocate the given sized pointer.
 */
GP_NODISCARD GP_INLINE size_t gp_sptr_size(const void* sptr)
{
    return ((GPSizePtrHeader*)sptr - 1)->size;
}

/** Get number of allocated elements pointed by a sized pointer.
 *
 * @return number of elements that fit in the given sized pointer.
 */
#define gp_sptr_countof(SPTR) (gp_sptr_size(SPTR) / sizeof (SPTR)[0])

/** Get allocator used to allocate a sized pointer.
 *
 * @return allocator passed to the function used to allocate the given sized pointer.
 */
GP_NODISCARD GP_INLINE GPAllocator* gp_sptr_allocator(const void* sptr)
{
    return ((GPSizePtrHeader*)sptr - 1)->allocator;
}

/** Allocate a sized pointer.
 *
 * Like @ref gp_mem_alloc() except returns a sized pointer.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_sptr_alloc(GPAllocator* alc, size_t size)
{
    size_t ignore_out_size;
    char* memory = alc->alloc(
        alc, NULL, 0, size + sizeof(GPSizePtrHeader), GP_ALLOC_ALIGNMENT, true, &ignore_out_size);
    gp_assume(memory != NULL);
    ((GPSizePtrHeader*)memory)->size = size;
    ((GPSizePtrHeader*)memory)->allocator = alc;
    return memory + sizeof(GPSizePtrHeader);
}

/** Allocate an array as a sized pointer.
 *
 * Like @ref gp_mem_alloc_array() except returns a sized pointer.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_sptr_alloc_array(GPAllocator* alc, size_t n, size_t m, bool uninitialized)
{
    size_t size;
    size_t ignore_out_size;
    gp_assume(gp_size_mul(&size, n, m), "Multiplication exceeded GP_ALLOC_MAX_SIZE.");
    gp_assume(size + sizeof(GPSizePtrHeader) < GP_ALLOC_MAX_SIZE);
    char* memory = alc->alloc(
        alc, NULL, 0, size + sizeof(GPSizePtrHeader), GP_ALLOC_ALIGNMENT, uninitialized, &ignore_out_size);
    gp_assert(memory != NULL);
    ((GPSizePtrHeader*)memory)->size = size;
    ((GPSizePtrHeader*)memory)->allocator = alc;
    return memory + sizeof(GPSizePtrHeader);
}

/** Reallocate a sized pointer.
 *
 * Like @ref gp_mem_realloc() except doesn't need old size as @a old_block is
 * a sized pointer. @a old block must be allocated with functions allocating
 * sized pointers like @ref gp_sptr_alloc() instead of our basic allocation
 * functions like @ref gp_mem_alloc().
 *
 * Unlike @ref gp_mem_realloc() and unlike standard `realloc()`, @a old_block
 * must _not_ be `NULL`. This is because our library is made to be used with
 * custom allocators and we cannot assume that the user wants to use any
 * specific one.
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_sptr_realloc(void* old_block, size_t new_size)
{
    gp_assume(new_size < GP_ALLOC_MAX_SIZE - sizeof(GPSizePtrHeader));
    GPSizePtrHeader* header = ((GPSizePtrHeader*)old_block - 1);
    size_t ignore_out_size;
    char* memory = header->allocator->alloc(
        header->allocator,
        header,
        header->size + sizeof *header,
        new_size + sizeof *header,
        GP_ALLOC_ALIGNMENT,
        true,
        &ignore_out_size);
    gp_assert(memory != NULL);
    ((GPSizePtrHeader*)memory)->size = new_size;
    return memory + sizeof *header;
}

/** Deallocate a sized pointer.
 *
 * Deallocate @a optional_block. If @a optional_block is `NULL`, then this
 * function does nothing. Otherwise, @a optional_block must be a sized pointer
 * returned by a function that allocates sized pointers like @ref gp_sptr_alloc().
 *
 * The success status of the deallocation is asserted. This prevents security
 * issues caused by passing an invalid pointer.
 */
GP_INLINE void gp_sptr_dealloc(void* optional_block)
{
    if (optional_block == NULL)
        return;

    GPSizePtrHeader* header = ((GPSizePtrHeader*)optional_block - 1);
    bool success = header->allocator->dealloc(
        header->allocator, header, header->size + sizeof(GPSizePtrHeader), GP_ALLOC_ALIGNMENT);
    gp_assert(success);
}

/// @}

/// @}
//------------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
//------------------------------------------------------------------------------
/// @cond

#ifdef __cplusplus
} // extern "C"
#endif

/// @endcond
#endif // GP_MEMORY_INCLUDED
