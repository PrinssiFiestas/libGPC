// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_ATTRIBUTES_INCLUDED
#define GP_ATTRIBUTES_INCLUDED 1

// ----------------------------------------------------------------------------
// Alignment

// Aligment of all pointers returned by any valid allocators. Can be globally
// overridden to a power of 2 larger or equal to 8.
#ifndef GP_ALLOC_ALIGNMENT
#if (__STDC_VERSION__ >= 201112L && !defined(_MSC_VER)) || defined(__COMPCERT__)
#define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#else
#define GP_ALLOC_ALIGNMENT (sizeof(long double))
#endif
#endif

// ----------------------------------------------------------------------------
// Nodiscard

#if __GNUC__
#define GP_NODISCARD __attribute__((__warn_unused_result__))
#elif _MSC_VER
#define GP_NODISCARD _Check_return_
#else
#define GP_NODISCARD
#endif

// ----------------------------------------------------------------------------
// Nonnull

#ifdef __GNUC__
#define GP_NONNULL_ARGS(...) __attribute__((nonnull (__VA_ARGS__)))
#define GP_NONNULL_RETURN    __attribute__((returns_nonnull))
#define GP_NONNULL_ARGS_AND_RETURN \
    __attribute__((nonnull)) __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#define GP_NONNULL_ARGS(...)
#define GP_NONNULL_RETURN _Ret_notnull_
#define GP_NONNULL_ARGS_AND_RETURN _Ret_notnull_
#else
#define GP_NONNULL_ARGS(...)
#define GP_NONNULL_RETURN
#define GP_NONNULL_ARGS_AND_RETURN
#endif

// ----------------------------------------------------------------------------
// Malloc-like functions

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

#ifdef __GNUC__
#define GP_MALLOC_SIZE(...) __attribute__((alloc_size (__VA_ARGS__)))
#else
#define GP_MALLOC_SIZE(...)
#endif

// ----------------------------------------------------------------------------
// Restrict

#if __GNUG__ || _MSC_VER
#define GP_RESTRICT __restrict
#elif __cplusplus
#define GP_RESTRICT
#else
#define GP_RESTRICT restrict
#endif

// ----------------------------------------------------------------------------
// Static array index

// Static array index in parameter declarations is a C99 feature, however, many
// compilers do not support it.
#if !defined(_MSC_VER) && \
    !defined(__TINYC__) && \
    !defined(__MSP430__) && \
    !defined(__cplusplus) && \
    !defined(__COMPCERT__)
// Use to specify an array argument with at least some number of valid elements,
// e.g. "void foo(int arr[GPC_STATIC 10];". This can be used for optimizations
// and some compilers may also emit warnings if they can detect that the array
// passed is too small or NULL.
#define GP_STATIC static
#else
#define GP_STATIC
#endif

// ----------------------------------------------------------------------------
// Printf format string type checking

#if __GNUC__
// Type checking for format strings
#define GP_PRINTF(FORMAT_STRING_INDEX, FIRST_TO_CHECK) \
    __attribute__((format(printf, FORMAT_STRING_INDEX, FIRST_TO_CHECK)))
#else
#define GP_PRINTF(...)
#endif

// ----------------------------------------------------------------------------
// Disable sanitizers

#if __GNUC__
#define GP_NO_SANITIZE __attribute__((no_sanitize("address", "leak", "undefined")))
#elif _MSC_VER
#define GP_NO_SANITIZE __declspec(no_sanitize_address)
#else
#define GP_NO_SANITIZE
#endif

#endif // GP_ATTRIBUTES_INCLUDED
