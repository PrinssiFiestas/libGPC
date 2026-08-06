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
 * Memory allocators and other memory management utilities.
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

typedef struct GPAllocator
{
    void* (*alloc)(
        struct GPAllocator*,
        void*   optional_old_block,
        size_t  optional_old_block_size,
        size_t  size,
        size_t  alignment,
        bool    uninitialized,
        size_t* actual_size);

    bool (*dealloc)(
        struct GPAllocator*,
        void* block,
        size_t size,
        size_t alignment);
} GPAllocator;

GP_NONNULL_ARGS_AND_RETURN GP_NODISCARD GP_INLINE
void* gp_mem_alloc(GPAllocator* alc, size_t size)
{
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
    GPAllocator* alc, void*restrict old_block, size_t old_size, size_t new_size)
{
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
