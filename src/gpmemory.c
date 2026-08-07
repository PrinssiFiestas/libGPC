// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpmemory.h>

#if __STDC_VERSION__ >= 201112L
#include <stdalign.h>
#endif

#if !defined(_WIN32)
#ifndef __USE_MISC // this should not be used, but _GNU_SOURCE had too many
#define __USE_MISC // portability related problems. Used for MAP_ANONYMOUS
#define GP_USE_MISC_DEFINED
#endif
#include <sys/mman.h>
#else
#include <windows.h>
#endif

static void* gp_s_global_heap_alloc(
    struct GPAllocator* unused,
    void*   optional_old_block,
    size_t  optional_old_block_size,
    size_t  block_size,
    size_t  alignment,
    bool    uninitialized,
    size_t* actual_size)
{
    (void)unused;
    (void)optional_old_block_size;

    // Standard aligned_alloc() and posix_memalign() require size to be a
    // multiple of alignment, but _aligned_realloc() and our manual alignment
    // implementation does not. Requiring size to be a multiple of alignment
    // makes this much harder to use: it should be allowed to allocate say
    // `char[3]` with alignment of GP_ALLOC_ALIGNMENT. Therefore, we'll just
    // round up the size for aligned_alloc() and posix_memalign().

    #ifdef GP_TARGET_OS_WINDOWS // aligned_alloc() not available even in C11
    void* mem = _aligned_realloc(optional_old_block, block_size, alignment);
    #elif __STDC_VERSION__ >= 201112L
    void* mem;
    block_size = gp_round_to_aligned(block_size, alignment);

    // aligned_alloc() doesn't play well with realloc(), so have to handle
    // separately. Prefer realloc(), because realloc() can sometimes just resize
    // the current allocation instead of actually reallocating.
    if (alignment <= alignof(max_align_t))
        mem = realloc(optional_old_block, block_size);
    else if (optional_old_block == NULL)
        mem = aligned_alloc(alignment, block_size);
    else if (block_size <= optional_old_block_size) {
        mem = optional_old_block;
        block_size = optional_old_block_size; // in case user uses actual_size for dealloc
    } else {
        mem = aligned_alloc(alignment, block_size);
        if (mem != NULL) {
            memcpy(mem, optional_old_block, optional_old_block_size);
            #if __STDC_VERSION__ >= 202311L
            free_aligned_sized(optional_old_block, alignment, optional_old_block_size);
            #else
            free(optional_old_block);
            #endif
        }
    }
    #elif _POSIX_C_SOURCE >= 200112L
    // No `alignof(max_align_t)`. It might be relatively safe to assume
    // `2*sizeof(size_t)`, but we'll play it even safer and just not use realloc()
    // for now. Might want to optimize later for known targets though.
    void* mem = NULL;
    if (alignment < sizeof(void*))
        alignment = sizeof(void*);
    block_size = gp_round_to_aligned(block_size, alignment);

    posix_memalign(&mem, alignment, block_size);
    if (optional_old_block != NULL && mem != NULL) {
        memcpy(mem, optional_old_block, optional_old_block_size);
        free(optional_old_block);
    }
    #else // have to align manually.
    void* mem = NULL;
    void* mem_start = realloc(
        optional_old_block, block_size + gp_round_to_aligned(sizeof(void*), alignment));
    if (mem_start != NULL) {
        mem = (char*)gp_round_to_aligned((uintptr_t)((void**)mem_start + 1), alignment);
        memcpy((void**)mem - 1, &mem_start, sizeof mem_start);
    }
    #endif

    // TODO: Better would be to use calloc() when possible, it might skip
    // zeroing for big mmapped blocks, which would be a huge win, but the code
    // above is already crazy enough considering that this is supposed to be a
    // simple malloc() wrapper, so we'll do it later once more important stuff
    // are implemented.
    if ( ! uninitialized && mem != NULL) {
        if (optional_old_block == NULL)
            memset(mem, 0x00, block_size);
        else
            memset(
                (char*)mem + optional_old_block_size,
                0x00,
                block_size - optional_old_block_size);
    }
    *actual_size = block_size;
    return mem;
}

static bool gp_s_global_heap_dealloc(
    struct GPAllocator* unused,
    void* block,
    size_t size,
    size_t alignment)
{
    (void)unused;
    (void)size;
    (void)alignment;
    #ifdef GP_TARGET_OS_WINDOWS
    _aligned_free(block);
    #elif __STDC_VERSION__ >= 202311L
    if (alignment <= alignof(max_align_t))
        free_sized(block, size);
    else
        free_aligned_sized(block, alignment, size);
    #elif __STDC_VERSION__ >= 201112L || _POSIX_C_SOURCE >= 200112L
    free(block);
    #else
    memcpy(&block, (void**)block - 1, sizeof block);
    free(block);
    #endif
    return true;
}

GPAllocator gp_mallocator =
{
    .alloc   = gp_s_global_heap_alloc,
    .dealloc = gp_s_global_heap_dealloc
};

GPAllocator* gp_heap = &gp_mallocator;

// Undef for single header users
#ifdef GP_USE_MISC_DEFINED
#undef __USE_MISC
#undef GP_USE_MISC_DEFINED
#endif
