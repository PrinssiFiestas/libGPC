// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ATTRIBUTES_H
#define GPC_ATTRIBUTES_H

// TODO check for C23 once it comes out for [[nodiscard]] and typeof()

// Are these checks needed now that I removed -pedantic?
/*#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wattributes"
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#define GPC_NODISCARD __attribute__((__warn_unused_result__))
#else
#define GPC_NODISCARD
#endif
*/

#if __STDC_VERSION__ >= 201112L
#define GPC_ATOMIC _Atomic
#else
#define GPC_ATOMIC
#endif

// ----------------------------------------------------------------------------

#ifdef _MSC_VER
#define GPC_THREAD_LOCAL __declspec(thread)
#else
#define GPC_THREAD_LOCAL _Thread_local
#endif

// ----------------------------------------------------------------------------

#if defined(__MINGW32__) && !defined(__clang__)
#define GPC_LONG_DOUBLE double
#define GPC_LG_FORMAT "%g"
#define GPC_SIZE_T_FORMAT "%llu"
#else
#define GPC_LONG_DOUBLE long double
#define GPC_LG_FORMAT "%Lg"
#define GPC_SIZE_T_FORMAT "%zu"
#endif

// ----------------------------------------------------------------------------

// Static array index in parameter declarations is a C99 feature, however, many
// compilers do not support it.
#if !defined(_MSC_VER) && !defined(__TINYC__) && !defined(__COMPCERT__) && !defined(__MSP430__)

// Use to specify an array argument with at least some number of valid elements,
// e.g. "void foo(int arr[GPC_STATIC 10];". This can be used for optimizations
// and some compilers may also emit warnings if they can detect that the array
// passed is too small or NULL.
#define GPC_STATIC static
#define GPC_NONNULL static 1

#elif defined(_MSC_VER)

#define GPC_STATIC
#define GPC_NONNULL _Notnull_

#else

// Static array index not supported by your compiler
#define GPC_STATIC
#define GPC_NONNULL
#endif

// ----------------------------------------------------------------------------

#if defined(__GNUC__)

// Type checking for format strings
#define GPC_PRINTF(FORMAT_STRING_INDEX, FIRST_TO_CHECK) \
__attribute__((format(printf, FORMAT_STRING_INDEX, FIRST_TO_CHECK)))

#define GPC_NODISCARD __attribute__((warn_unused_result))

#elif defined(_MSC_VER)

// Not supported by your compiler
#define GPC_PRINTF(STRING_INDEX, FIRST_TO_CHECK)

// Emit warning if return value is unused
#define GPC_NODISCARD _Check_return_

#else

// Not supported by your compiler
#define GPC_PRINTF(STRING_INDEX, FIRST_TO_CHECK)

// Not supported by your compiler
#define GPC_NODISCARD

#endif

#endif // GPC_ATTRIBUTES_H
