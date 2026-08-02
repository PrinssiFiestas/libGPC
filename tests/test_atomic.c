// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include "../src/gpatomic.c"
#include <gpc/gputils.h>

// Trying to test absolutely everything would drive us insane, we'll just test
// with GPAtomicInt64, which probably has to most differences between targets.
// We'll settle with either explicit or non explicit variants, no need to test
// both since the explicit functions are just trivial wrappers.

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
    gp_atomic_compare_exchange_strong(A, B, C)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_weak(A, B, C)
#endif

// TODO test list
//     | Function                          | Tested by
//------------------------------------------------------------------------------
// [x] | gp_atomic_store                   | spinlock
// [x] | gp_atomic_load                    | semaphore, spinlock
// [x] | gp_atomic_exchange                | spinlock
// [x] | gp_atomic_fetch_add               | semaphore, flip_bits
// [x] | gp_atomic_fetch_sub               | decrement
// [x] | gp_atomic_fetch_or                | gp_ansi_enable()
// [x] | gp_atomic_fetch_xor               | flip_bits
// [x] | gp_atomic_fetch_and               | gp_ansi_enable()
// [ ] | gp_atomic_compare_exchange_strong |
// [x] | gp_atomic_compare_exchange_weak   | semaphore
// [ ] | gp_atomic_thread_fence            |
// [ ] | gp_atomic_signal_fence            |

#define THREADS_NUM 16

GPAtomicInt64 atomic_counter;

// Doesnt' really need to be 128 bits, but this is a more realistic use case for
// locks.
GPInt128 non_atomic_counter;

#define LIMIT 1000000000

//-------------------------------------

int decrement(void*_)
{
    (void)_;

    int i = 0;
    while (gp_atomic_load(&atomic_counter) > -LIMIT) {
        gp_atomic_fetch_sub(&atomic_counter, 1);
        i++;
    }
    return i;
}

void check_decrement(int results[THREADS_NUM])
{
    int64_t sum = 0;
    for (size_t i = 0; i < THREADS_NUM; i++)
        sum += results[i];
    gp_expect(gp_atomic_load(&atomic_counter) == -LIMIT);
    gp_expect(sum == LIMIT);
}

//-------------------------------------

GPAtomicInt64 bits;

int flip_bits(void*_)
{
    (void)_;

    int total_enabled = 0;
    while (gp_atomic_load(&atomic_counter) < LIMIT) {
        gp_atomic_fetch_add(&atomic_counter, 1);
        total_enabled += gp_atomic_fetch_xor(&bits, 1);
    }
    return total_enabled;
}

void check_flip_bits(int results[THREADS_NUM])
{
    int64_t total_enabled = 0;
    for (size_t i = 0; i < THREADS_NUM; i++)
        total_enabled += results[i];

    gp_expect(gp_atomic_load(&atomic_counter) == LIMIT);
    gp_expect(total_enabled == LIMIT/2);
}

//-------------------------------------

GPAtomicInt64 binary_semaphore;

void wait()
{
    int64_t old;
    do {
        // Busy waiting is good here. We want maximum contention for testing.
        // Normally should at least gp_thread_yield() though.
        old = gp_atomic_load_explicit(&binary_semaphore, GP_MEMORY_ORDER_RELAXED);
    } while (old < 0 || ! gp_atomic_compare_exchange_weak_explicit(
        &binary_semaphore, &old, old - 1, GP_MEMORY_ORDER_ACQUIRE, GP_MEMORY_ORDER_RELAXED
    ));
}

void signal()
{
    gp_atomic_fetch_add_explicit(&binary_semaphore, 1, GP_MEMORY_ORDER_RELEASE);
}

int semaphore_count(void*_)
{
    (void)_;

    int i = 0;
    while (true) {
        wait();
        if (gp_int128_equal(non_atomic_counter, gp_int128(0, LIMIT)))
            break;
        non_atomic_counter = gp_int128_add(non_atomic_counter, gp_int128(0, 1));
        i++;
        signal();
    }
    signal();
    return i;
}

void check_semaphore_count(int results[THREADS_NUM])
{
    int64_t sum = 0;
    for (size_t i = 0; i < THREADS_NUM; i++)
        sum += results[i];

    gp_expect(gp_int128_lo(non_atomic_counter) == LIMIT);
    gp_expect(sum == LIMIT);
}

//-------------------------------------

// https://rigtorp.se/spinlock/
GPAtomicInt64 spinlock;

void ttas_lock(void)
{
    while (true) {
        if ( ! gp_atomic_exchange_explicit(&spinlock, true, GP_MEMORY_ORDER_ACQUIRE))
            return;

        while (gp_atomic_load_explicit(&spinlock, GP_MEMORY_ORDER_RELAXED))
            ; // Again, busy waiting is good for testing. And again, normally
              // should at least gp_thread_yield() here.
    }
}

void ttas_unlock(void)
{
    gp_atomic_store_explicit(&spinlock, false, GP_MEMORY_ORDER_RELEASE);
}

int ttas_count(void*_)
{
    (void)_;

    int i = 0;
    while (true) {
        ttas_lock();
        if (gp_int128_equal(non_atomic_counter, gp_int128(0, LIMIT)))
            break;
        non_atomic_counter = gp_int128_add(non_atomic_counter, gp_int128(0, 1));
        i++;
        ttas_unlock();
    }
    signal();
    return i;
}

void check_ttas_count(int results[THREADS_NUM])
{
    int64_t sum = 0;
    for (size_t i = 0; i < THREADS_NUM; i++)
        sum += results[i];

    gp_expect(gp_int128_lo(non_atomic_counter) == LIMIT);
    gp_expect(sum == LIMIT);
}

//-------------------------------------

void threaded_test(const char* name, int(*thread_proc)(void*), void(*confirm_result)(int[THREADS_NUM]))
{
    gp_test(name);

    GPThread threads[THREADS_NUM];
    for (size_t i = 0; i < gp_countof(threads); i++)
        gp_thread_create(&threads[i], thread_proc, (void*)i);

    int returns[gp_countof(threads)];
    for (size_t i = 0; i < gp_countof(threads); i++)
        gp_thread_join(threads[i], &returns[i]);
}

int main(void)
{
    gp_suite("Atomic operations");

    gp_atomic_store(&atomic_counter, 0);
    threaded_test("Decrement", decrement, check_decrement);
    gp_atomic_store(&atomic_counter, 0);
    threaded_test("Flip Bits", flip_bits, check_flip_bits);
    non_atomic_counter = gp_int128(0, 0);
    threaded_test("Semaphore", semaphore_count, check_semaphore_count);
    non_atomic_counter = gp_int128(0, 0);
    threaded_test("Spinlock", ttas_count, check_ttas_count);
}
