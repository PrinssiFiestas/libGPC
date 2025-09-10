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
#  if (__STDC_VERSION__ >= 201112L && !defined(_MSC_VER)) || defined(__COMPCERT__)
#    define GP_ALLOC_ALIGNMENT (_Alignof(max_align_t))
#  else
#    define GP_ALLOC_ALIGNMENT (2*sizeof(void*))
#  endif
#elif (GP_ALLOC_ALIGNMENT < 8) || (GP_ALLOC_ALIGNMENT & (GP_ALLOC_ALIGNMENT - 1))
#  error GP_ALLOC_ALIGNMENT must be a power of 2 larger or equal to 8.
#endif

// ----------------------------------------------------------------------------
// Nodiscard

#if __GNUC__
#  define GP_NODISCARD __attribute__((__warn_unused_result__))
#elif _MSC_VER
#  define GP_NODISCARD _Check_return_
#elif __cplusplus >= 201703L || __STDC_VERSION__ >= 202311L
#  define GP_NODISCARD [[nodiscard]]
#else
#  define GP_NODISCARD
#endif

// ----------------------------------------------------------------------------
// Nonnull

#ifdef __GNUC__
#  define GP_NONNULL_ARGS(...) __attribute__((nonnull (__VA_ARGS__)))
#  define GP_NONNULL_RETURN    __attribute__((returns_nonnull))
#  define GP_NONNULL_ARGS_AND_RETURN \
      __attribute__((nonnull)) __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#  define GP_NONNULL_ARGS(...)
#  define GP_NONNULL_RETURN _Ret_notnull_
#  define GP_NONNULL_ARGS_AND_RETURN _Ret_notnull_
#else
#  define GP_NONNULL_ARGS(...)
#  define GP_NONNULL_RETURN
#  define GP_NONNULL_ARGS_AND_RETURN
#endif

// ----------------------------------------------------------------------------
// Require initialized memory

#if __GNUC__ >= 11 && !__clang__
#  define GP_INOUT(...) __attribute__((access(read_write, __VA_ARGS__)))
#else
#  define GP_INOUT(...)
#endif

// ----------------------------------------------------------------------------
// Malloc-like functions

// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

// TODO rename to GP_ATTRIB_ALLOC_ALIGN for a clearer distinction between
// GP_ALLOC_ALIGNMENT (or the other way around) to fix autocomplete.

#ifdef __GNUC__
#  define GP_ALLOC_ALIGN(...) __attribute__((alloc_align(__VA_ARGS__)))
#else
#  define GP_ALLOC_ALIGN(...)
#endif

// ----------------------------------------------------------------------------
// Restrict

#if __GNUG__ || _MSC_VER
#  define GP_RESTRICT __restrict
#elif __cplusplus
#  define GP_RESTRICT
#else
#  define GP_RESTRICT restrict
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
// e.g. "void foo(int arr[GP_STATIC 10];". This can be used for optimizations
// and some compilers may also emit warnings if they can detect that the array
// passed is too small or NULL.
#  define GP_STATIC static
#else
#  define GP_STATIC
#endif

// ----------------------------------------------------------------------------
// Printf Format String Type Checking

// User can #define GP_NO_FORMAT_STRING_CHECK before including this header to
// disable format string checking, which may be useful with custom formats.

#if __GNUC__ && !defined(GP_NO_FORMAT_STRING_CHECK)
// Type checking for format strings
#  define GP_CHECK_FORMAT_STRING(FORMAT_STRING_INDEX, FIRST_TO_CHECK) \
      __attribute__((format(printf, FORMAT_STRING_INDEX, FIRST_TO_CHECK)))
#else
#  define GP_CHECK_FORMAT_STRING(...)
#endif

// ----------------------------------------------------------------------------
// Disable sanitizers

#if __GNUC__
#  define GP_NO_SANITIZE __attribute__((no_sanitize("address", "leak", "undefined")))
#elif _MSC_VER
#  define GP_NO_SANITIZE __declspec(no_sanitize_address)
#else
#  define GP_NO_SANITIZE
#endif

// ----------------------------------------------------------------------------
// Static Assert

#if __STDC_VERSION__ >= 202311L || __cplusplus
#  define GP_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#elif __STDC_VERSION__ >= 201112L
#  define GP_STATIC_ASSERT_SELECT(_0, _1, THIS, ...) THIS // GP_OVERLOAD2, but removes need to include header
#  define GP_STATIC_ASSERT_NO_MSG(E) _Static_assert(E, "")
#  define GP_STATIC_ASSERT(...) \
       GP_STATIC_ASSERT_SELECT(__VA_ARGS__, _Static_assert, GP_STATIC_ASSERT_NO_MSG)(__VA_ARGS__)
#else // C99, message will be ignored, it is just there for compatibility
#  define GP_STATIC_ASSERT_SELECT(_0, _1, THIS, ...) THIS // GP_OVERLOAD2, but removes need to include header
#  define GP_STATIC_ASSERT_TOKEN_PASTE(A, B) A##B
#  define GP_STATIC_ASSERTION_NAME(LINE) GP_STATIC_ASSERT_TOKEN_PASTE(GPStaticAssertion_line_, LINE)
#  define GP_STATIC_ASSERT_MSG(E, MSG) char GP_STATIC_ASSERTION_NAME(__LINE__)[(E) ? 1 : -1]; (void)GP_STATIC_ASSERTION_NAME(__LINE__)
#  define GP_STATIC_ASSERT_NO_MSG(E)   char GP_STATIC_ASSERTION_NAME(__LINE__)[(E) ? 1 : -1]; (void)GP_STATIC_ASSERTION_NAME(__LINE__)
#  define GP_STATIC_ASSERT(...) \
       GP_STATIC_ASSERT_SELECT(__VA_ARGS__, GP_STATIC_ASSERT_MSG, GP_STATIC_ASSERT_NO_MSG)(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
// Predict

#ifdef __GNUC__
#  define GP_LIKELY(...)   __builtin_expect(!!(__VA_ARGS__), 1)
#  define GP_UNLIKELY(...) __builtin_expect(!!(__VA_ARGS__), 0)
#else
#  define GP_LIKELY(...)   (!!(__VA_ARGS__))
#  define GP_UNLIKELY(...) (!!(__VA_ARGS__))
#endif

// ----------------------------------------------------------------------------
// C Linkage

#if __cplusplus
#  define GP_EXTERN_C extern "C" // for single functions in mixed C/C++ headers
#  define GP_BEGIN_EXTERN_C extern "C" { // for many functions in mixed headers
#  define GP_END_EXTERN_C              }
#else
#  define GP_EXTERN_C
#  define GP_BEGIN_EXTERN_C
#  define GP_END_EXTERN_C
#endif

// ----------------------------------------------------------------------------
// Constexpr

// Use sparingly!
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2043r0.pdf

#if __cplusplus
#  define GP_CONSTEXPR_FUNCTION constexpr
#  define GP_CONSTEXPR_VARIABLE constexpr
#  define GP_CONST constexpr
#elif __STDC_VERSION__ >= 202311L
#  define GP_CONSTEXPR_FUNCTION
#  define GP_CONSTEXPR_VARIABLE constexpr
#  define GP_CONST constexpr
#else
#  define GP_CONSTEXPR_FUNCTION
#  define GP_CONSTEXPR_VARIABLE
#  define GP_CONST static const
#endif

// ----------------------------------------------------------------------------
// Long Double Support

// Compilers not supporting long double should be explicitly listed here,
// because long double has been in C standards since C89, so it is assumed to be
// supported by default.
#if !defined(__COMPCERT__) && !defined(__SDCC)

// Code using long double compiles. Doesn't mean that it is larger than double.
#  define GP_HAS_LONG_DOUBLE 1

// Code using long double compiles and type system differentiates between double
// and long double. This is important for C11 _Generic() selection, which does
// not allow duplicate types.
#  define GP_HAS_DIFFERENTIATED_LONG_DOUBLE 1

#endif // compilers (not) supporting long double

#endif // GP_ATTRIBUTES_INCLUDED
