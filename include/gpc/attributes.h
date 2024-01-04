// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPATTRIBUTES_INCLUDED
#define GPATTRIBUTES_INCLUDED

// TODO check for C23 once it comes out for [[nodiscard]] and typeof()

// ----------------------------------------------------------------------------

#if __STDC_VERSION__ >= 201112L
#define GP_ATOMIC _Atomic
#else
#define GP_ATOMIC
#endif

// ----------------------------------------------------------------------------

#ifdef __GNUC__
// Emit warning if return value is discarded.
#define GP_NODISCARD __attribute__((__warn_unused_result__))
#elif defined(_MSC_VER)
// Emit warning if return value is discarded.
#define GP_NODISCARD _Check_return_
#else
// Not supported by your compiler
#define GP_NODISCARD
#endif

// ----------------------------------------------------------------------------

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

// Static array index in parameter declarations is a C99 feature, however, many
// compilers do not support it.
#if !defined(_MSC_VER) && !defined(__TINYC__) && !defined(__COMPCERT__) && !defined(__MSP430__)

// Use to specify an array argument with at least some number of valid elements,
// e.g. "void foo(int arr[GPC_STATIC 10];". This can be used for optimizations
// and some compilers may also emit warnings if they can detect that the array
// passed is too small or NULL.
#define GP_STATIC static
#define GP_NONNULL static 1

#elif defined(_MSC_VER)

#define GP_STATIC
#define GP_NONNULL _Notnull_

#else

// Static array index not supported by your compiler
#define GPC_STATIC
#define GPC_NONNULL
#endif

// ----------------------------------------------------------------------------

#if defined(__GNUC__)

// Type checking for format strings
#define GP_PRINTF(FORMAT_STRING_INDEX, FIRST_TO_CHECK) \
__attribute__((format(printf, FORMAT_STRING_INDEX, FIRST_TO_CHECK)))

#elif defined(_MSC_VER)

// Not supported by your compiler
#define GP_PRINTF(STRING_INDEX, FIRST_TO_CHECK)

#else

// Not supported by your compiler
#define GP_PRINTF(STRING_INDEX, FIRST_TO_CHECK)

#endif

#endif // GPATTRIBUTES_INCLUDED
