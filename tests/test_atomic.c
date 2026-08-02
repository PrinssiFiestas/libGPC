// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include "../src/gpatomic.c"

// Trying to test absolutely everything would drive us insane, we'll just test
// with GPAtomicInt64, which probably has to most differences between targets.

// The generic macros are not defined with the very most crazy obscure targets.
// However, it would be nice to test that they at least compile when they are
// supported, so test with the macros and define them to _i64 when they don't exist.
#ifndef gp_atomic_store
#define gp_atomic_store(A, C)                      gp_atomic_store_i64(A, C)
#define gp_atomic_load(A)                          gp_atomic_load_i64(A)
#define gp_atomic_exchange(A, C)                   gp_atomic_exchange_i64(A, C)
#define gp_atomic_fetch_add(A, C)                  gp_atomic_fetch_add_i64(A, C)
#define gp_atomic_fetch_sub(A, C)                  gp_atomic_fetch_sub_i64(A, C)
#define gp_atomic_fetch_or(A, C)                   gp_atomic_fetch_or_i64(A, C)
#define gp_atomic_fetch_xor(A, C)                  gp_atomic_fetch_xor_i64(A, C)
#define gp_atomic_fetch_and(A, C)                  gp_atomic_fetch_and_i64(A, C)
#define gp_atomic_compare_exchange_strong(A, B, C) gp_atomic_compare_exchange_strong_i64(A, B, C)
#define gp_atomic_compare_exchange_weak(A, B, C)   gp_atomic_compare_exchange_weak_i64(A, B, C)
#define gp_atomic_store_explicit(A, C, MO)         gp_atomic_store(A, C)
#define gp_atomic_load_explicit(A, C, MO)          gp_atomic_load(A, C)
#define gp_atomic_exchange_explicit(A, C, MO)      gp_atomic_exchange(A, C)
#define gp_atomic_fetch_add_explicit(A, C, MO)     gp_atomic_fetch_add(A, C)
#define gp_atomic_fetch_sub_explicit(A, C, MO)     gp_atomic_fetch_sub(A, C)
#define gp_atomic_fetch_or_explicit(A, C, MO)      gp_atomic_fetch_or(A, C)
#define gp_atomic_fetch_xor_explicit(A, C, MO)     gp_atomic_fetch_xor(A, C)
#define gp_atomic_fetch_and_explicit(A, C, MO)     gp_atomic_fetch_and(A, C)
#define gp_atomic_compare_exchange_strong_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_strong_explicit(A, B, C)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_weak_explicit(A, B, C)
#endif

// Binary semaphore
GPAtomicInt64 binary_semaphore;

void wait()
{
    do {
        int64_t old = gp_atomic_load_explicit(&binary_semaphore, GP_MEMORY_ORDER_ACQUIRE);
    } while (true);
}

void signal()
{
    gp_atomic_fetch_add_explicit(&binary_semaphore, 1, GP_MEMORY_ORDER_RELEASE);
}

int main(void)
{

}
