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
 */ // TODO more docs for description.

/** Check if compiling with address sanitizer.
 *
 * Defined to 1 if compiling with `-fsanitize=address`, 0 otherwise. Tested with
 * GCC, Clang, and MSVC. Other compilers may be defined to a false negative.
 */
#ifdef GP_DOXYGEN
#  define GP_HAS_SANITIZER /* 0 or 1 */
#endif

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
 * allocate more would anyway cause undefined behavior down the line, so it
 * makes no sense to have it larger even if user space is larger than 2 GB.
 */
#ifndef GP_MAX_ALLOC_SIZE
#  if SIZE_MAX <= UINT32_MAX
#    define GP_MAX_ALLOC_SIZE PTRDIFF_MAX
#  else // virtual address space is 48 bits, can be larger but this sensible default anyway.
#    define GP_MAX_ALLOC_SIZE ((size_t)1 << 48)
#  endif
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
 * exceed @ref GP_MAX_ALLOC_SIZE, then the result is stored to @a result.
 * Otherwise, value pointed by @a result is left unmodified.
 *
 * Unlike `calloc()`, our allocation functions either do not check for size
 * multiplication overflow at all or assert that no overflow happened. This is
 * because many programmers wrongly treat `NULL` returned by `calloc()`
 * unconditionally as an out of memory error without checking `errno`. Failed
 * allocations and failed multiplications are very much different errors that
 * would require very different handling. Therefore, this should be called
 * _before_ allocating if there is a possibility of an overflow.
 *
 * @return `true` if multiplication of @a n and @a m stayed within
 * @ref GP_MAX_ALLOC_SIZE, `false` otherwise.
 */
GP_INLINE GP_NODISCARD bool gp_size_mul(size_t* result, size_t n, size_t m)
{
    #if SIZE_MAX == UINT32_MAX
    uint64_t size = n * x;
    if (size < GP_MAX_ALLOC_SIZE) {
        *result = size;
        return true;
    }
    #else
    GPUInt128 size = gp_uint128_mul64(n, m);
    if (gp_uint128_lo(size) < GP_MAX_ALLOC_SIZE && gp_uint128_hi(size) == 0) {
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
     *     memory block that has at least this size. Should be below @ref GP_MAX_ALLOC_SIZE.
     *     Allocators must _not_ assume that size is a multiple of alignment.
     *
     * @param alignment
     *     Requested allocation alignment. Must be a power of two. Allocators
     *     must return a memory block whose address is a multiple of this value.
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

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_alloc(GPAllocator* alc, size_t size)
{
    gp_assume(size < GP_MAX_ALLOC_SIZE);
    void* memory = alc->alloc(
        alc, NULL, 0, size, GP_ALLOC_ALIGNMENT, true, &size);
    gp_assume(memory != NULL);
    return memory;
}

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_alloc_array(GPAllocator* alc, size_t n, size_t m, bool uninitialized)
{
    size_t size;
    gp_assume(gp_size_mul(&size, n, m), "Multiplication exceeded GP_MAX_ALLOC_SIZE.");
    void* memory = alc->alloc(
        alc, NULL, 0, size, GP_ALLOC_ALIGNMENT, uninitialized, &size);
    gp_assume(memory != NULL);
    return memory;
}

GP_NONNULL_ARGS(1) GP_NONNULL_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_realloc(
    GPAllocator* alc, void* old_block, size_t old_size, size_t new_size)
{
    gp_assume(new_size < GP_MAX_ALLOC_SIZE);
    void* memory = alc->alloc(
        alc, old_block, old_size, new_size, GP_ALLOC_ALIGNMENT, true, &new_size);
    gp_assume(memory != NULL);
    return memory;
}

GP_NONNULL_ARGS(1) GP_INLINE
void gp_mem_dealloc(
    GPAllocator* alc,
    void* optional_block,
    size_t optional_block_size)
{
    if (optional_block == NULL)
        return;
    bool success = alc->dealloc(
        alc, optional_block, optional_block_size, GP_ALLOC_ALIGNMENT);
    gp_assume(success);
}

typedef struct GPSizePtrHeader
{
    GPAllocator* allocator; ///< Allocator used to allocate the sized pointer.
    size_t size;            ///< Requested allocation size in bytes.
} GPSizePtrHeader;

GP_NODISCARD GP_INLINE size_t gp_sptr_size(void* sptr)
{
    return ((GPSizePtrHeader*)sptr - 1)->size;
}

#define gp_sptr_countof(SPTR) (gp_sptr_size(SPTR) / sizeof (SPTR)[0])

GP_NODISCARD GP_INLINE GPAllocator* gp_sptr_allocator(void* sptr)
{
    return ((GPSizePtrHeader*)sptr - 1)->allocator;
}

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

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_sptr_alloc_array(GPAllocator* alc, size_t n, size_t m, bool uninitialized)
{
    size_t size;
    size_t ignore_out_size;
    gp_assume(gp_size_mul(&size, n, m), "Multiplication exceeded GP_MAX_ALLOC_SIZE.");
    gp_assume(size + sizeof(GPSizePtrHeader) < GP_MAX_ALLOC_SIZE);
    char* memory = alc->alloc(
        alc, NULL, 0, size + sizeof(GPSizePtrHeader), GP_ALLOC_ALIGNMENT, uninitialized, &ignore_out_size);
    gp_assume(memory != NULL);
    ((GPSizePtrHeader*)memory)->size = size;
    ((GPSizePtrHeader*)memory)->allocator = alc;
    return memory + sizeof(GPSizePtrHeader);
}

/** TODO
 *
 * TODO
 *
 * Unlike standard `realloc()`, @a old_block must not be `NULL`. This is because
 * our library is made to be used with custom allocators and we cannot assume
 * that the user wants any specific one.
 *
 * TODO
 */
GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_sptr_realloc(void* old_block, size_t new_size)
{
    GPSizePtrHeader* sptr = ((GPSizePtrHeader*)old_block - 1);
    size_t ignore_out_size;
    char* memory = sptr->allocator->alloc(
        sptr->allocator,
        sptr,
        sptr->size + sizeof *sptr,
        new_size + sizeof *sptr,
        GP_ALLOC_ALIGNMENT,
        true,
        &ignore_out_size);
    gp_assume(memory != NULL);
    ((GPSizePtrHeader*)memory)->size = new_size;
    return memory + sizeof *sptr;
}

GP_INLINE void gp_sptr_dealloc(void* optional_block)
{
    if (optional_block == NULL)
        return;

    GPSizePtrHeader* sptr = ((GPSizePtrHeader*)optional_block - 1);
    bool success = sptr->allocator->dealloc(
        sptr->allocator, optional_block, sptr->size + sizeof(GPSizePtrHeader), GP_ALLOC_ALIGNMENT);
    gp_assume(success);
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
 * alignment and allocation sizes do not have to be a multiple of alignment. If
 * C23, deallocation uses `free_aligned_sized()`, which requires size and
 * alignment to match allocation size and alignment, otherwise undefined
 * behavior is invoked.
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
