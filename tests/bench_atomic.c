// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Quick and dirty atomics vs locks benchmark. Not much effort was spent on
// accuracy, we only care about major significant differences.
//
// This file compares different possible lock based implementations of atomics
// (used when no atomics available) to actual atomics. Since we promised ABI
// compatibility and don't get any help from the compiler, we cannot allocate a
// mutex with the atomic variable, so have to use a shared global lock instead.
// This was measured to be about five times slower than atomics, which is very
// bad, but not too insane, the locks are guaranteed to not being held by the
// owner for too long.
//
// A global lock for all atomic operations obviously generates quite a bit of
// unnecessary contention: all atomic variables wait on the same lock on all
// atomic operations. A simple optimization is to use a lock pool and use the
// address of the atomic variable to assign a somewhat unique lock for each
// variable. This probably doesn't make much meaningful difference when there is
// low contention and makes the absolute worst case performance worse, but it
// drastically reduces contention, which gives more predictable performance,
// which is one of the major reasons to use atomics in the first place.

#include <gpc/gptime.h>
#include <gpc/gpthread.h>
#include <gpc/gpatomic.h>
#include <gpc/gputils.h>

// At the time of writing, doing major rewrite, so use standard library (boo).
#include <stdio.h>
#include <assert.h>
#include <stdatomic.h>

#define THREADS_NUM 16

size_t limit;

GPMutex global_lock = GP_MUTEX_INIT;

size_t global_lock_load(const size_t* x)
{
    size_t y;
    gp_mutex_lock(&global_lock);
    y = *x;
    gp_mutex_unlock(&global_lock);
    return y;
}

size_t global_lock_increment(size_t* x)
{
    size_t old;
    gp_mutex_lock(&global_lock);
    old = *x;
    *x += 1;
    gp_mutex_unlock(&global_lock);
    return old;
}

// This uses about two memory pages when sizeof(GPMutex)==40, which is the case
// with our test machine. Bigger array would reduce contention, but would use
// more memory with diminishing returns. 512 already showed no improvement, and the
// size has to be a power of two.
GPMutex lock_pool[256] = {0}; // GP_MUTEX_INIT is just zeros on this machine.

GP_ALWAYS_INLINE GPMutex* get_lock(const void* p)
{
    // First bits of p are zero due to alignment requirements. This is good,
    // it reduces false sharing.
    return &lock_pool[((uintptr_t)p) & (gp_countof(lock_pool) - 1)];
}

size_t lock_pool_load(const size_t* x)
{
    size_t y;
    GPMutex* mutex = get_lock(x);
    gp_mutex_lock(mutex);
    y = *x;
    gp_mutex_unlock(mutex);
    return y;
}

size_t lock_pool_increment(size_t* x)
{
    size_t old;
    GPMutex* mutex = get_lock(x);
    gp_mutex_lock(mutex);
    old = *x;
    *x += 1;
    gp_mutex_unlock(mutex);
    return old;
}

//-------------------------------------

typedef struct RNG
{
    uint64_t state;
    uint64_t inc;
} RNG;

RNG rng;

uint32_t rand32(RNG* rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((0-rot) & 31));

}

RNG random_state(uint64_t init_state, uint64_t stream_id)
{
    RNG rng = {.inc = (stream_id << 1u) | 1u };
    rand32(&rng);
    rng.state += init_state;
    rand32(&rng);
    return rng;
}

//-------------------------------------
// Simple Counting
//
// Simply just increment a counter in multiple threads. Extremely contrived, but
// demonstrates the very absolute worst case.

size_t non_atomic_counter;
_Atomic size_t atomic_counter;

int count_atomically(void*_)
{
    (void)_;
    size_t i = 0;
    while (atomic_counter < limit) {
        atomic_counter++;
        i++;
    }
    return i;
}

int count_with_global_lock(void*_)
{
    (void)_;
    size_t i = 0;
    while (global_lock_load(&non_atomic_counter) < limit) {
        global_lock_increment(&non_atomic_counter);
        i++;
    }
    return i;
}

int count_with_lock_pool(void*_)
{
    (void)_;
    size_t i = 0;
    while (lock_pool_load(&non_atomic_counter) < limit) {
        lock_pool_increment(&non_atomic_counter);
        i++;
    }
    return i;
}

#define FIELD_WIDTH "30"

void benchmark_simple_counting(const char* title, int(*proc)(void*))
{
    GPInt128 start;
    double time;
    GPThread threads[THREADS_NUM];
    size_t sum;

    start = gp_time_begin();
    for (size_t i = 0; i < THREADS_NUM; i++)
        gp_thread_create(&threads[i], proc, NULL);
    sum = 0;
    for (size_t i = 0; i < THREADS_NUM; i++) {
        int ret;
        gp_thread_join(threads[i], &ret);
        sum += (unsigned)ret;
    }
    time = gp_time(&start);
    assert(sum == limit);
    atomic_counter = 0;
    non_atomic_counter = 0;
    printf("%-" FIELD_WIDTH "s:     %g\n", title, time);
}

//-------------------------------------
// "Realistic" Counting
//
// No useful program has been ever written that just increments a counter in
// multiple threads, which has way more contention than any real world program
// would. We simulate contention of a more realistic program by having multiple
// atomic counters and increment them randomly.
//
// atomic_counter used as total work amount counter to signal when to stop
// counting. It adds a bit of benchmarking overhead, but this is the simplest
// thing to do.

size_t counters_length;
size_t* non_atomic_counters;
_Atomic size_t* atomic_counters;

int realistic_count_atomically(void*_)
{
    (void)_;
    size_t i = 0;
    while (atomic_counter < limit) {
        atomic_fetch_add_explicit(&atomic_counter, 1, memory_order_relaxed);
        i++;

        atomic_counters[(rand32(&rng)) & (counters_length - 1)]++;
    }
    return i;
}

int realistic_count_with_global_lock(void*_)
{
    (void)_;
    size_t i = 0;
    while (atomic_counter < limit) {
        atomic_fetch_add_explicit(&atomic_counter, 1, memory_order_relaxed);
        i++;

        global_lock_increment(&non_atomic_counters[(rand32(&rng)) & (counters_length - 1)]);
    }
    return i;
}

int realistic_count_with_lock_pool(void*_)
{
    (void)_;
    size_t i = 0;
    while (atomic_counter < limit) {
        atomic_fetch_add_explicit(&atomic_counter, 1, memory_order_relaxed);
        i++;

        lock_pool_increment(&non_atomic_counters[(rand32(&rng)) & (counters_length - 1)]);
    }
    return i;
}

void benchmark_realistic_counting(const char* title, int(*proc)(void*))
{
    GPInt128 start;
    double time;
    GPThread threads[THREADS_NUM];
    size_t sum;

    start = gp_time_begin();
    for (size_t i = 0; i < THREADS_NUM; i++)
        gp_thread_create(&threads[i], proc, NULL);
    sum = 0;
    for (size_t i = 0; i < THREADS_NUM; i++) {
        int ret;
        gp_thread_join(threads[i], &ret);
        sum += (unsigned)ret;
    }
    time = gp_time(&start);
    assert(sum == limit);
    atomic_counter = 0;
    non_atomic_counter = 0;

    sum = 0;
    for (size_t i = 0; i < counters_length; ++i)
        sum += atomic_counters[i] + non_atomic_counters[i];
    assert(sum == limit);
    memset(atomic_counters, 0, counters_length * sizeof(size_t));
    memset(non_atomic_counters, 0, counters_length * sizeof(size_t));

    printf("%-" FIELD_WIDTH "s:     %g\n", title, time);
}

//-------------------------------------

int main(int argc, char* argv[])
{
    if (argc < 2)
        exit(!!fprintf(stderr, "Usage:\n\t%s <counter limit> [number of counters]\n", argv[0]));

    limit = atoll(argv[1]);
    assert(limit < UINT_MAX / 32);

    // The numbers in the comments are the measurements with argv[1]==10000000 and
    // argv[2]==50 on 12 x Intel i7-8750H CPU @ 2.20GHz (Linux).

    // Absolutely worst (and absolutely not realistic) case: All threads just
    // increment a counter: maximum contention. This is so freaking contrived
    // that it doesn't really matter for most purposes, but worst case can be
    // important for real-time applications with strict deadlines.
    benchmark_simple_counting("Maximum contention atomic", count_atomically); // baseline
    benchmark_simple_counting("Maximum contention non-atomic", count_with_global_lock); // 10x slower
    benchmark_simple_counting("Maximum contention lock pool", count_with_lock_pool); // 15x slower

    if (argc == 2)
        exit(0);

    rng = random_state(time(NULL), (uintptr_t)&argc);

    counters_length = gp_next_power_of_2(atoll(argv[2]));
    non_atomic_counters = calloc(counters_length, sizeof(size_t));
    atomic_counters     = calloc(counters_length, sizeof(size_t));

    benchmark_realistic_counting("\"Realistic\" atomic", realistic_count_atomically); // baseline
    benchmark_realistic_counting("\"Realistic\" non-atomic", realistic_count_with_global_lock); // 5x slower
    benchmark_realistic_counting("\"Realistic\" lock pool", realistic_count_with_lock_pool); // 2.5x slower

    // pedantic cleanup for sanitizers
    free(non_atomic_counters);
    free(atomic_counters);
}
