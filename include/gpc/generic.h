// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file generic.h
 * Type generic macros for arrays and strings.
 */

#ifndef GP_GENERIC_INCLUDED
#define GP_GENERIC_INCLUDED

#include <gpc/array.h>
#include <gpc/string.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

#define gp_length(X)            GP_LENGTH(X)
#define gp_capacity(X)          GP_CAPACITY(X)
#define gp_allocation(X)        GP_ALLOCATION(X)
#define gp_allocator(X)         GP_ALLOCATOR(X)
#define gp_reserve(X, CAPACITY) GP_RESERVE(X, CAPACITY)
#define gp_copy(X, ...)         GP_COPY(X, __VA_ARGS__)
#define gp_slice(X, ...)        GP_SLICE(X, __VA_ARGS__)
#define gp_append(X, ...)       GP_APPEND(X, __VA_ARGS__)
#define gp_insert(X, ...)       GP_INSERT(X, __VA_ARGS__)

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#define GP_LENGTH(A)            gp_arr_length(A)
#define GP_CAPACITY(A)          gp_arr_capacity(A)
#define GP_ALLOCATION(A)        gp_arr_allocation(A)
#define GP_ALLOCATOR(A)         gp_arr_allocator(A)

void gp_reserve99(size_t elem_size, void* px, const size_t capacity);
#define GP_RESERVE(A, CAPACITY) gp_reserve99(sizeof**(A), A, CAPACITY)

void* gp_copy99(size_t y_size, void* y,
    const void* x, const char* x_ident, size_t x_length, const size_t x_size);

#define GP_COPY2(A, B)    gp_copy99(sizeof*(A), A, B, #B, sizeof(B) - sizeof"", sizeof*(B))
#define GP_COPY3(A, B, C) gp_copy99(sizeof*(A), A, B, NULL, C, sizeof*(B))
#define GP_COPY(A, ...) GP_OVERLOAD2(__VA_ARGS__, GP_COPY3, GP_COPY2)(A,__VA_ARGS__)

void* gp_slice99(
    const size_t y_size, const void* y,
    const size_t x_size, const void* x,
    const size_t start, const size_t end);

#define GP_SLICE_WITH_INPUT(Y, X, START, END) \
    gp_slice99(sizeof*(Y), Y, sizeof*(X), X, START, END)
#define GP_SLICE_WOUT_INPUT(Y, START, END) \
    ((void*){0} = gp_arr_slice(sizeof**(Y), *(void**)(Y), NULL, START, END))
#define GP_SLICE(A, START, ...) \
    GP_OVERLOAD2(__VA_ARGS__, GP_SLICE_WITH_INPUT, GP_SLICE_WOUT_INPUT)(A, START, __VA_ARGS__)

void* gp_append99(
    const size_t a_size, void* a,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);

#define GP_IS_ALC(A) (sizeof*(A) == sizeof(GPAllocator))

#define GP_APPEND2(A, B) \
    gp_append99(sizeof*(A), A, B, #B, sizeof(B), sizeof*(B), NULL, NULL, 0)
    //gp_append99(sizeof*(A), A, B, #B, sizeof(B) - sizeof"", sizeof*(B), NULL, NULL, 0)

#define GP_APPEND3(A, B, C) \
    gp_append99(sizeof*(A), A, \
        B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? sizeof(B) : (uintptr_t)(C), sizeof*(B), \
        GP_IS_ALC(A) ? (void*)(C) : NULL, #C, sizeof(typeof(C)))
        // B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? sizeof(B) - sizeof"" : (uintptr_t)(C), sizeof*(B), \
        //GP_IS_ALC(A) ? (void*)(C) : NULL, #C, sizeof((char[]){C}) - sizeof"")

#define GP_APPEND4(A, B, C, D) \
    gp_append99(sizeof*(A), A, \
        B, #B, sizeof(B) : sizeof*(B), \
        C, NULL, D)
        // B, #B, sizeof(B) - sizeof"" : sizeof*(B), \
        // C, NULL, D)

#define GP_APPEND5(A, B, C, D, E) \
    gp_append99(sizeof*(A), A, B, NULL, C, sizeof*(B), D, NULL, E)
#define GP_APPEND(A, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_APPEND5, GP_APPEND4, GP_APPEND3, GP_APPEND2)(A, __VA_ARGS__)

void* gp_insert99(
    const size_t a_size, void* a, const size_t pos,
    const void* b, const char* b_ident, size_t b_length, const size_t b_size,
    const void* c, const char* c_ident, size_t c_length);

#define GP_INSERT3(A, POS, B) \
    gp_insert99(sizeof*(A), A, POS, B, #B, sizeof(B), sizeof*(B), NULL, NULL, 0)
//    gp_insert99(sizeof*(A), A, POS, B, #B, sizeof(B) - sizeof"", sizeof*(B), NULL, NULL, 0)

#define GP_INSERT4(A, POS, B, C) \
    gp_insert99(sizeof*(A), A, POS, \
        B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? sizeof(B) : (uintptr_t)(C), sizeof*(B), \
        GP_IS_ALC(A) ? (void*)(C) : NULL, #C, sizeof((char[]){C}))
        // B, GP_IS_ALC(A) ? #B : NULL, GP_IS_ALC(A) ? sizeof(B) - sizeof"" : (uintptr_t)(C), sizeof*(B), \
        // GP_IS_ALC(A) ? (void*)(C) : NULL, #C, sizeof((char[]){C}) - sizeof"")

#define GP_INSERT5(A, POS, B, C, D) \
    gp_insert99(sizeof*(A), A, POS, \
        B, #B, sizeof(B): sizeof*(B), \
        C, NULL, D)
        //B, #B, sizeof(B) - sizeof"" : sizeof*(B), \
        //C, NULL, D)

#define GP_INSERT6(A, POS, B, C, D, E) \
    gp_insert99(sizeof*(A), A, POS, B, NULL, C, sizeof*(B), D, NULL, E)

#define GP_INSERT(A, POS, ...) GP_OVERLOAD4(__VA_ARGS__, \
    GP_INSERT6, GP_INSERT5, GP_INSERT4, GP_INSERT3)(A, POS, __VA_ARGS__)

#endif // GP_GENERIC_INCLUDED
