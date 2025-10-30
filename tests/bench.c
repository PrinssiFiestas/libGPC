// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// This file contains basic scaffolding for quick microbenchmarking and
// throwaway code to be benchmarked. This is not supposed to be a fully fledged
// benchmarking framework, we just need something quick to make decisions that
// are better than guessing.
//
// Timing may use system clock or wall clock. It's units are unspecified.
//
// Timing overhead will be subtracted from all other measurements. Timing
// overhead includes indirect function call, timing itself, and optionally time
// that it takes to execute some user defined code to be ignored. It will also
// be displayed in result table so user can compare against it. This is mostly
// useful to detect if compiler optimized code of interest to be benchmarked
// away, although note that tiny functions (faster than function pointer call)
// may be take less time than timing overhead for perfectly valid reasons.
//
// You can run this benchmark by runnin `make bench`, which compiles with
// optimizations and debug flags and runs build/bench. You can also use
// `make bench_debug` to compile a debug build without optimizations for
// debugging.

#define _GNU_SOURCE
#include "../build/gpc.h"
#ifdef __x86_64__
#include <x86intrin.h> // __rdtsc()
#endif
#include <signal.h>
#if !_WIN32
#include <sys/mman.h>
#endif

#if __GNUC__
// Use this anywhere in function body to force it to not be inlined. This also
// prevents removing empty functions.
//
// GCC ignores -fno-crossjumping for some reason. This can also be used to
// prevent merging equivalent functions.
#define GP_NOINLINE asm(""); // see noinline at https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#else
#define GP_NOINLINE
#endif

void gp_bench_ignore(void** output, void* input)
{
    (void)output; (void)input;

    // ------------------------------------------------------------------------
    // BEGIN THROWAWAY ignored code
    //
    // Time that it takes to execute any code here will be subtracted from all
    // results.

    (void)0;

    // END THROWAWAY ignored code
    // ------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
// BEGIN THROWAWAY functions to be benchmarked
//
// You can put any necessary helper functions here as well.
//
// Functions must have the following signature:
// void f(void** output, void* input)
// Input will be the return value of gp_bench_prepare_arguments().
// Output will point to an element of outputs[] in gp_bench_confirm_results().
// Functions defined here must be registered using GP_BENCH_FUNCTIONS macro
// below to be benchmarked.

void ascii_trim(
    void** output,
    void* _str)
{
    GP_NOINLINE;
    const char* char_set = GP_ASCII_WHITESPACE;
    GPArray(unsigned char) str = _str;

    size_t start = 0;
    size_t end = gp_arr_length(str);

    uint8_t in_set[256] = { false };
    for (const uint8_t* c = (uint8_t*)char_set; *c != '\0'; ++c)
        in_set[*c] = true;

    for (; end > 0; --end)
        if ( ! in_set[str[end - 1]])
            break;

    for (; start < end; ++start)
        if ( ! in_set[str[start]])
            break;

    memmove(str, str + start, end - start);
    gp_arr_set(str)->length = end - start;
    *output = str;
}

void ascii_trim_unroll(
    void** output,
    void* _str)
{
    const char* char_set = GP_ASCII_WHITESPACE;
    GPArray(unsigned char) str = _str;

    size_t start = 0;
    size_t end = gp_arr_length(str);

    uint8_t in_set[256] = { false };
    // #pragma GCC unroll 4
    for (const uint8_t* c = (uint8_t*)char_set; *c != '\0'; ++c)
        in_set[*c] = true;

    // #pragma GCC unroll 4
    for (; end > 0; --end)
        if ( ! in_set[str[end - 1]])
            break;

    // #pragma GCC unroll 4
    for (; start < end; ++start)
        if ( ! in_set[str[start]])
            break;

    memmove(str, str + start, end - start);
    gp_arr_set(str)->length = end - start;
    *output = str;
}

// END THROWAWAY functions to be benchmarked
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// REGISTER FUNCTIONS TO BE BENCHMARKED HERE
//
// List your functions in a comma separated list here to benchmark them.
#define GP_BENCH_FUNCTIONS ascii_trim, ascii_trim_unroll
// ----------------------------------------------------------------------------

void gp_bench_prepare_global_data(
    GPRandomState* rs) // seeded to time(NULL), can be seeded here to something else.
{
    (void)rs;

    // ------------------------------------------------------------------------
    // BEGIN THROWAWAY code to initialize global shared data

    (void)0;

    // END THROWAWAY code to initialize global shared data
    // ------------------------------------------------------------------------
}

void* gp_bench_prepare_arguments(
    size_t         iteration,
    GPRandomState* random_state,
    GPAllocator*   allocator,
    void*          outputs[],
    size_t         outputs_length)
{
    GP_NOINLINE
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

    GPString str = gp_str_new(allocator, 0);
    const size_t max = 16;
    size_t seg_length;
    seg_length = gp_random_range(random_state, 0, max);

    for (size_t i = 0; i < seg_length; ++i)
        gp_str_append(
            &str,
            &GP_WHITESPACE[gp_random_range(random_state, 0, strlen(GP_WHITESPACE))],
            1);

    // Keep max small, otherwise we just benchmark memmove().
    gp_str_repeat(&str, gp_random_range(random_state, 0, 4), "_", 1);

    seg_length = gp_random_range(random_state, 0, max);
    for (size_t i = 0; i < seg_length; ++i)
        gp_str_append(
            &str,
            &GP_WHITESPACE[gp_random_range(random_state, 0, strlen(GP_WHITESPACE))],
            1);

    return str;

    // END THROWAWAY argument preparation
    // ------------------------------------------------------------------------
}

bool gp_bench_confirm_results(
    void*  outputs[],      // outputs of benchmarked functions initialized to NULL
    size_t outputs_length, // number of registered functions
    void*  input)          // return value of gp_bench_prepare_arguments()
{
    GP_NOINLINE
    (void)outputs; (void)outputs_length; (void)input;

    // ------------------------------------------------------------------------
    // BEGIN THROWAWAY check that outputs from all benchmarked functions match

    for (size_t i = 0; i < outputs_length - 1; ++i)
        if ( ! gp_str_equal(outputs[i], outputs[i + 1], gp_str_length(outputs[i + 1])))
            return false;
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

#define GP_BENCH_FTABLE_LENGTH (1 + GP_COUNT_ARGS(GP_BENCH_FUNCTIONS))

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

GPArray(size_t) gp_bench_random_indices(GPAllocator* alc, GPRandomState* rs)
{
    GPArray(size_t) is      = gp_arr_new(sizeof(size_t), alc, GP_BENCH_FTABLE_LENGTH);
    GPArray(size_t) is_temp = gp_arr_new(sizeof(size_t), alc, GP_BENCH_FTABLE_LENGTH);

    for (size_t i = 0; i < 1 + GP_COUNT_ARGS(GP_BENCH_FUNCTIONS); ++i)
        gp_arr_push(sizeof i, &is_temp, &i);

    while (gp_arr_length(is_temp) > 0) {
        size_t i = gp_random_range(rs, 0, gp_arr_length(is_temp));
        gp_arr_push(sizeof is[0], &is, &is_temp[i]);
        gp_arr_erase(sizeof is[0], &is_temp, i, 1);
    }

    return is;
}

void gp_bench_execute(
    size_t iteration,
    GPRandomState* rs,
    GPArena* arena,
    void* outputs[],
    size_t outputs_length,
    void(*const ftable[])(void**, void*),
    gp_bench_t total_times[])
{
    void* input = gp_bench_prepare_arguments(
        iteration, rs, &arena->base, outputs + 1, outputs_length);

    GPArray(size_t) is = gp_bench_random_indices(&arena->base, rs);

    while (gp_arr_length(is) > 0) // Execute benchmarks
    {
        size_t i = *(size_t*)gp_arr_pop(sizeof i, &is);
        gp_bench_t t0 = gp_time_bench();
        ftable[i](&outputs[i], input);
        gp_bench_t t1 = gp_time_bench();
        if (total_times != NULL)
            total_times[i] += t1 - t0;
    }

    if ( ! gp_expect(gp_bench_confirm_results(outputs + 1, outputs_length, input), "%zu", iteration))
        exit(EXIT_FAILURE);
}

int main(void)
{
    static void (*const ftable[])(void** output, void* input) = {
        gp_bench_ignore,
        GP_BENCH_FUNCTIONS
    };
    static const char*const ftable_strings[] = {
        "timing overhead",
        GP_PROCESS_ALL_ARGS(GP_STRFY, GP_COMMA, GP_BENCH_FUNCTIONS)
    };

    if (sizeof ftable <= sizeof ftable[0]) {
        gp_file_println(stderr, "At least one function must be registered for benchmarking.");
        exit(EXIT_FAILURE);
    }

    GPRandomState rs = gp_random_state(time(NULL));
    GPArena* arena = gp_arena_new(NULL, 1024*1024); // size is arbitrary
    const size_t outputs_length = GP_BENCH_FTABLE_LENGTH - 1; // subtract no-op
    static void* outputs[sizeof ftable / sizeof ftable[0]];
    static gp_bench_t total_times[sizeof ftable / sizeof ftable[0]];

    GPUInt128 result_update_time = gp_time_begin();

    gp_bench_prepare_global_data(&rs);
    signal(SIGINT, gp_bench_sighandler);

    size_t iteration = 0;
    void* arena_pos = gp_mem_alloc(&arena->base, 0);

    // Java Microbenchmark Harness uses 20 as default warmup iterations. We'll
    // assume that they have researched and concluded that it is a good default.
    for (; gp_bench_signum == 0 && iteration < 20; gp_arena_rewind(arena, arena_pos), ++iteration)
    { // warmup
        gp_bench_execute(
            iteration,
            &rs,
            arena,
            outputs,
            outputs_length,
            ftable,
            NULL);
    } // for warmup

    puts("");

    for (iteration = 0; gp_bench_signum == 0; gp_arena_rewind(arena, arena_pos), ++iteration)
    { // benchmark
        gp_bench_execute(
            iteration,
            &rs,
            arena,
            outputs,
            outputs_length,
            ftable,
            total_times);

        if (gp_time(&result_update_time) > .1)
        {
            pf_printf(" Press Ctrl+C to stop benchmarking. Iteration: %zu\n", iteration);
            result_update_time = gp_time_begin();
            pf_printf("\n----------------------------------------------------------------------------------\n");
            pf_printf("%-30s | %-20s | %-20s\n", "Function", "Relative time", "Time per iteration");
            pf_printf("----------------------------------------------------------------------------------\n");

            for (size_t i = 0; i < GP_BENCH_FTABLE_LENGTH; ++i)
            {
                gp_bench_t timing_overhead = (i != 0) * total_times[0];
                pf_printf("%-30s | %20.2f | %20.2f\n",
                    ftable_strings[i],
                    (double)gp_imax(0, total_times[i] - timing_overhead) / (total_times[1] - timing_overhead),
                    (double)gp_imax(0, total_times[i] - timing_overhead) / iteration);
            }
            pf_printf("----------------------------------------------------------------------------------\n");
            pf_printf(GP_CURSOR_UP(6));
            for (size_t i = 0; i < GP_BENCH_FTABLE_LENGTH; ++i)
                pf_printf(GP_CURSOR_UP(1));
            fflush(stdout);
        }
    } // for benchmarks

    puts("\n\n\n");
    for (size_t i = 0; i < GP_BENCH_FTABLE_LENGTH; ++i)
        puts("");
    puts("");
    gp_arena_delete(arena);
}
