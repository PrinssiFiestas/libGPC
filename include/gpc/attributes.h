// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPATTRIBUTES_INCLUDED
#define GPATTRIBUTES_INCLUDED

// ----------------------------------------------------------------------------
// Atomic

#if __STDC_VERSION__ >= 201112L
#define GP_ATOMIC _Atomic
#else
// Warning: not necessarily atomic with your compiler!
#define GP_ATOMIC
#endif

// ----------------------------------------------------------------------------
// Nodiscard

#ifdef __GNUC__
// Emit warning if return value is discarded.
#define GP_NODISCARD __attribute__((__warn_unused_result__))
#elif defined(_MSC_VER)
// Emit warning if return value is discarded.
#define GP_NODISCARD _Check_return_
#else
// Please, don't discard return value.
#define GP_NODISCARD
#endif

// ----------------------------------------------------------------------------
// Thread local

#ifdef _MSC_VER
#define GP_THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L
#define GP_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__)
#define GP_THREAD_LOCAL __thread
#else
#define GP_THREAD_LOCAL
#endif

// ----------------------------------------------------------------------------
// Long double

#if defined(__MINGW32__) && !defined(__clang__)
#define GP_LONG_DOUBLE double
#define GP_LG_FORMAT "%g"
#define GP_SIZE_T_FORMAT "%llu"
#else
#define GP_LONG_DOUBLE long double
#define GP_LG_FORMAT "%Lg"
#define GP_SIZE_T_FORMAT "%zu"
#endif

// ----------------------------------------------------------------------------
// Nonnull args

#if defined(__GNUC__)
#define GP_NONNULL_AGRGS(...) __attribute__((nonnull __VA_OPT__((__VA_ARGS__))))
#else
#define GP_NONNULL_ARGS(...)
#endif

// ----------------------------------------------------------------------------
// Array arg with static size

// TODO get rid of GP_NONNULL. It's causing problems with MSVC (as usual).

// Static array index in parameter declarations is a C99 feature, however, many
// compilers do not support it.
#if !defined(_MSC_VER) &&   \
    !defined(__TINYC__) &&   \
    !defined(__COMPCERT__) && \
    !defined(__MSP430__)

// Use to specify an array argument with at least some number of valid elements,
// e.g. "void foo(int arr[GPC_STATIC 10];". This can be used for optimizations
// and some compilers may also emit warnings if they can detect that the array
// passed is too small or NULL.

// You must provide a buffer with capacity at least the specified amount.
#define GP_STATIC static
#define GP_NONNULL static 1

#else

// Please, provide a buffer with capacity at least the specified amount.
#define GPC_STATIC
#define GPC_NONNULL
#endif

// ----------------------------------------------------------------------------
// Printf format string type checking

#if defined(__GNUC__)

// Type checking for format strings
#define GP_PRINTF(FORMAT_STRING_INDEX, FIRST_TO_CHECK) \
__attribute__((format(printf, FORMAT_STRING_INDEX, FIRST_TO_CHECK)))

#elif defined(_MSC_VER)

//
#define GP_PRINTF(STRING_INDEX, FIRST_TO_CHECK)

#else

//
#define GP_PRINTF(STRING_INDEX, FIRST_TO_CHECK)

#endif

#endif // GPATTRIBUTES_INCLUDED
