// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// This file contains basic scaffolding for quick microbenchmarking and
// throwaway code to be benchmarked. This is not supposed to be a fully fledged
// benchmarking framework, we just need something quick to make decisions that
// are better than guessing. Don't expect to see rigorous statistical analysis
// here.
//
// Timing may use system clock or wall clock. It's units are unspecified and
// only provided for reference.
//
// You can run this benchmark by runnin `make bench`, which compiles with
// optimizations and debug flags and runs build/bench. You can also use
// `make bench_debug` to compile a debug build without optimizations for
// debugging.

#include "../build/gpc.h"
#ifdef __x86_64__
#include <x86intrin.h> // __rdtsc()
#endif
#include <signal.h>

// Example of what benchmarked function might look like. The inline assembly is
// not necessary.
static void gp_s_no_op(void** output, const void* input)
{
    (void)output; (void)input;
    #if __GNUC__
    asm(""); // see noinline at https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
    #endif
}

// ----------------------------------------------------------------------------
// BEGIN THROWAWAY functions to be benchmarked
//
// You can put any necessary helper functions here as well.
//
// Functions must have the following signature:
// void f(void** output, const void* input)
// Input will be the return value of gp_bench_prepare_arguments().
// Output will point to an element of outputs[] in gp_bench_confirm_results().
// Functions defined here must be registered to gp_bench_functions[] below to be
// benchmarked.

void foo(void** output, const void* input)
{
    (void)output; (void)input;
    gp_sleep(.001);
}

void bar(void** output, const void* input)
{
    (void)output; (void)input;
    gp_sleep(.001);
}

// END THROWAWAY functions to be benchmarked
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// REGISTER FUNCTIONS TO BE BENCHMARKED HERE
//
// List your functions in a comma separated list here to benchmark them.
#define GP_BENCH_FUNCTIONS foo, bar
// ----------------------------------------------------------------------------

static void gp_bench_prepare_global_data(
    GPRandomState* rs) // seeded to time(NULL), can be seeded here to something else.
{
    (void)rs;

    // ------------------------------------------------------------------------
    // BEGIN THROWAWAY code to initialize global shared data

    // User globals can be initialized here.

    // END THROWAWAY code to initialize global shared data
    // ------------------------------------------------------------------------
}

static void* gp_bench_prepare_arguments(
    size_t         iteration,
    GPRandomState* random_state,
    GPAllocator*   allocator,
    void*          outputs[],
    size_t         outputs_length)
{
    (void)iteration; (void)random_state; (void)allocator; (void)outputs; (void)outputs_length;

    // ------------------------------------------------------------------------
    // BEGIN THROWAWAY argument preparation
    //
    // This function will be called for each iteration. Allocator is provided
    // for any memory needed for the current iterator. Any memory allocated by
    // it will be freed between iterations. Outputs will point to a pointers
    // pointing to NULL. You can use the allocator to allocate memory and set
    // outputs to point to the allocated memory instead, which will be passed to
    // benchmarked functions and eventually gp_bench_confirm_results(). The
    // return value will be passed as input to benchmarked functions and to
    // gp_bench_confirm_results().

    return NULL;

    // END THROWAWAY argument preparation
    // ------------------------------------------------------------------------
}

static bool gp_bench_confirm_results(
    void*       outputs[],      // outputs of benchmarked functions initialized to NULL
    size_t      outputs_length, // number of registered functions
    const void* input)          // return value of gp_bench_prepare_arguments()
{
    (void)outputs; (void)outputs_length; (void)input;
    #if __GNUC__
    asm(""); // see noinline at https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
    #endif

    // ------------------------------------------------------------------------
    // BEGIN THROWAWAY check that outputs from all benchmarked functions match

    return true;

    // END THROWAWAY check that outputs from all benchmarked functions match
    // ------------------------------------------------------------------------
}


// ----------------------------------------------------------------------------
//
// END OF ALL THROWAWAY CODE
//
// Code below is internal benchmarking implementation. Don't touch it!
//
// ----------------------------------------------------------------------------


typedef volatile GP_MAYBE_ATOMIC uint64_t gp_bench_t;

static sig_atomic_t gp_bench_signum;

GP_GNU_ATTRIB(always_inline)
static inline uint64_t gp_time_bench(void)
{
    #ifdef __x86_64__
    return __rdtsc();
    #else
    return gp_time_ns(NULL);
    #endif
}

void gp_bench_sighandler(int signum)
{
    gp_bench_signum = signum;
}

int main(void)
{
    static void (*ftable[])(void** output, const void* input) = {
        gp_s_no_op,
        GP_BENCH_FUNCTIONS
    };
    static const char* ftable_strings[] = {
        "timing overhead",
        GP_PROCESS_ALL_ARGS(GP_STRFY, GP_COMMA, GP_BENCH_FUNCTIONS)
    };

    if (sizeof ftable <= sizeof ftable[0]) {
        gp_file_println(stderr, "At least one function must be registered for benchmarking.");
        exit(EXIT_FAILURE);
    }

    GPRandomState rs = gp_random_state(time(NULL));
    GPArena* alc = gp_arena_new(NULL, 1024*1024); // size is arbitrary
    const size_t ftable_length = sizeof ftable / sizeof ftable[0];
    const size_t outputs_length = ftable_length - 1; // subtract no-op
    static void* outputs[sizeof ftable / sizeof ftable[0]];
    static gp_bench_t total_times[sizeof ftable / sizeof ftable[0]];

    // Random indices
    GPArray(size_t) is      = gp_arr_new(sizeof is[0], &alc->base, ftable_length);
    GPArray(size_t) is_temp = gp_arr_new(sizeof is[0], &alc->base, ftable_length);

    GPUInt128 progress_counter = gp_time_begin();

    gp_bench_prepare_global_data(&rs);
    signal(SIGINT, gp_bench_sighandler);

    size_t iteration = 0;
    void* arena_pos = gp_mem_alloc(&alc->base, 0);
    for (; gp_bench_signum == 0; gp_arena_rewind(alc, arena_pos), ++iteration)
    {
        void* input = gp_bench_prepare_arguments(
            iteration, &rs, &alc->base, outputs + 1, outputs_length);

        // Create random indices
        for (size_t i = 0; i < ftable_length; ++i)
            gp_arr_push(sizeof i, &is_temp, &i);
        while (gp_arr_length(is_temp) > 0) {
            size_t i = gp_random_range(&rs, 0, gp_arr_length(is_temp));
            gp_arr_push(sizeof is[0], &is, &is_temp[i]);
            gp_arr_erase(sizeof is[0], &is_temp, i, 1);
        }

        // Execute benchmarks
        while (gp_arr_length(is) > 0) {
            size_t i = *(size_t*)gp_arr_pop(sizeof i, &is);
            gp_bench_t t0 = gp_time_bench();
            ftable[i](&outputs[i], input);
            gp_bench_t t1 = gp_time_bench();
            total_times[i] += t1 - t0;
        }

        if ( ! gp_expect(gp_bench_confirm_results(outputs + 1, outputs_length, input), "%zu", iteration))
            exit(EXIT_FAILURE);

        if (gp_time(&progress_counter) > .2) {
            gp_print(" Press Ctrl+C to stop benchmarking. Iteration: %zu\r", iteration);
            fflush(stdout);
            progress_counter = gp_time_begin();
        }
    } // for

    for (size_t i = 1; i < ftable_length; ++i) // subtract timing overhead
        total_times[i] = gp_imax(0, total_times[i] - total_times[0]);

    pf_printf("\n----------------------------------------------------------------------------------\n");
    pf_printf("%-30s | %-20s | %-20s\n", "Function", "Relative time", "Time per iteration");
    pf_printf("----------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < ftable_length; ++i)
    {
        pf_printf("%-30s | %20.2f | %20.2f\n",
            ftable_strings[i],
            (double)total_times[i] / total_times[1],
            (double)total_times[i] / iteration);
    }
    gp_arena_delete(alc);
}
