// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_COMMON_INCLUDED
#define GP_COMMON_INCLUDED

#include <gpc/overload.h>
#include <gpc/attributes.h>
#include <gpc/bytes.h>
#include <gpc/array.h>
#include <gpc/utils.h>
#include <printf/conversions.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <wctype.h>

#define GP_FORMAT_SPECIFIERS "csSdioxXufFeEgGp"

size_t pf_vsnprintf_consuming_no_null_termination(
    char*restrict out_buf,
    const size_t max_size,
    const char* format,
    pf_va_list* args);

void gp_internal_arena_dealloc(GPAllocator*, void*);
void gp_internal_carena_dealloc(GPAllocator*, void*);

bool gp_internal_bytes_is_valid_codepoint(const void* str, size_t i);

// Input must be valid UTF-8
GP_NONNULL_ARGS()
size_t gp_internal_bytes_codepoint_count_unsafe(
    const void* _str,
    const size_t n);

GP_NONNULL_ARGS()
static inline size_t gp_internal_count_fmt_specs(const char* fmt)
{
    size_t i = 0;
    for (; (fmt = strchr(fmt, '%')) != NULL; ++fmt)
    {
        if (fmt[1] == '%') {
            ++fmt;
        } else  { // consuming more args
            const char* fmt_spec = strpbrk(fmt, GP_FORMAT_SPECIFIERS);
            for (const char* c = fmt; c < fmt_spec; ++c) if (*c == '*')
                ++i; // consume asterisks as well
            ++i;
        }
    }
    return i;
}

GP_NONNULL_ARGS(3)
size_t gp_internal_convert_va_arg(
    const size_t limit,
    void*restrict const out,
    pf_va_list*restrict const args,
    const gp_type_t type);

GP_NONNULL_ARGS(3, 4)
size_t gp_internal_bytes_print_objects(
    const size_t limit,
    void*restrict out,
    pf_va_list* args,
    size_t*const i,
    GPInternalReflectionData obj);

// ----------------------------------------------------------------------------
// Portability Assumptions

#if CHAR_BIT != 8
#error CHAR_BIT != 8: Crazy bytes not supported.
#endif
#if ~1 == -1
#error Ones complement integer arithmetic not supported. C23 deprecates it anyway.
#endif
#if __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(char*) == sizeof(int*), "");
_Static_assert(sizeof(void*) == sizeof(void(*)()), "");
#endif

// ----------------------------------------------------------------------------
// Typedefs for Argument Promoted Integers

// These address portability issues created by default argument promotions, when
// using va_arg(), which requires promoted types as arguments, which may be
// platform dependent. Some of these are pedantic, some provided for
// completeness.
//
// Implementation notes: according to C standards, roughly these hold:
// - bool <= char <= short <= int <= long <= long long
// - int16_t <= int
// - Any other type could be almost anything (not really)
//
// Using the standard limits interface may not be able to reliably detect
// underlying typedefs due to short == int in some systems, int == long in
// others, or even short == int == long, so some of the typedefs may be
// inaccurate.
//
// For any signed T, use T_MAX < INT_MAX when T is more likely to be long than
// short, use T_MAX <= INT_MAX when T is more likely to be short than long. The
// former prevents demoting long when int == long and the latter promotes short
// when short == int, which is more important, so in doubt use T_MAX <= INT_MAX.
//
// Sizes are guaranteed to match, so the inaccuracies have neglible impact on
// program correctness.
//
// These could be made public if ever proven to be useful outside libGPC.

typedef int gp_promoted_arg_bool_t;
typedef int gp_promoted_arg_char_t;
typedef int gp_promoted_arg_unsigned_char_t;
typedef int gp_promoted_arg_signed_char_t;
typedef int gp_promoted_arg_short_t;
#if USHRT_MAX < INT_MAX
typedef int gp_promoted_arg_unsigned_short_t;
#else
typedef unsigned gp_promoted_arg_unsigned_short_t;
#endif
typedef int gp_promoted_arg_int_t;
typedef unsigned gp_promoted_arg_unsigned_t;
typedef long gp_promoted_arg_long_t;
typedef unsigned long gp_promoted_arg_unsigned_long_t;
typedef long long gp_promoted_arg_long_long_t;
typedef unsigned long long gp_promoted_arg_unsigned_long_long_t;

#if SIZE_MAX < INT_MAX
typedef int gp_promoted_arg_size_t;
#else
typedef size_t gp_promoted_arg_size_t;
#endif
#if defined(SSIZE_MAX) && SSIZE_MAX < INT_MAX
typedef int gp_promoted_arg_ssize_t;
#elif defined(SSIZE_MAX)
typedef ssize_t gp_promoted_arg_ssize_t;
#endif
#if PTRDIFF_MAX < INT_MAX
typedef int gp_promoted_arg_ptrdiff_t;
#else
typedef ptrdiff_t gp_promoted_arg_ptrdiff_t;
#endif
#if INTPTR_MAX < INT_MAX
typedef int gp_promoted_arg_intptr_t;
typedef int gp_promoted_arg_uintptr_t;
#else
typedef intptr_t gp_promoted_arg_intptr_t;
typedef uintptr_t gp_promoted_arg_uintptr_t;
#endif

typedef int gp_promoted_arg_int8_t;
typedef int gp_promoted_arg_uint8_t;
typedef int gp_promoted_arg_int16_t; // int16_t <= int => int16_t assumed short
                                     // or int, cannot be long since int32_t <= long
#if UINT16_MAX < INT_MAX
typedef int gp_promoted_arg_uint16_t;
#else // uint16_t == unsigned
typedef unsigned gp_promoted_arg_uint16_t;
#endif
#if INT32_MAX < INT_MAX // short most likely 16 bits => int32_t more likely long,
                        // int most likely not 64 bits
typedef int gp_promoted_arg_int32_t;
typedef int gp_promoted_arg_uint32_t;
#else
typedef int32_t gp_promoted_arg_int32_t;
typedef uint32_t gp_promoted_arg_uint32_t;
#endif
typedef int64_t gp_promoted_arg_int64_t;
typedef uint64_t gp_promoted_arg_uint64_t;

typedef int gp_promoted_arg_int_least8_t;
typedef int gp_promoted_arg_uint_least8_t;
typedef int gp_promoted_arg_int_least16_t;
#if UINT_LEAST16_MAX < INT_MAX
typedef int gp_promoted_arg_uint_least16_t;
#else // least types are the smallest, so cannot be long, so this is accurate
typedef unsigned gp_promoted_arg_uint_least16_t;
#endif
#if INT_LEAST32_MAX < INT_MAX
typedef int gp_promoted_arg_int_least32_t;
typedef int gp_promoted_arg_uint_least32_t;
#else
typedef int_least32_t gp_promoted_arg_int_least32_t;
typedef uint_least32_t gp_promoted_arg_uint_least32_t;
#endif
typedef int_least64_t gp_promoted_arg_int_least64_t;
typedef uint_least64_t gp_promoted_arg_uint_least64_t;

// Fast type extreme cases:
// - fast types are the same as least types.
// - all fast types are 64 bits on 64-bit systems
// Both extremes are unlikely, but possible. Therefore, we have to check all
// three cases (T_MAX < INT_MAX, T_MAX > INT_MAX, T_MAX == INT_MAX) for most
// fast types.
#if INT_FAST8_MAX < INT_MAX
typedef int gp_promoted_arg_int_fast8_t;
typedef int gp_promoted_arg_uint_fast8_t;
#elif INT_FAST8_MAX > INT_MAX
typedef int_fast8_t gp_promoted_arg_int_fast8_t;
typedef uint_fast8_t gp_promoted_arg_uint_fast8_t;
#else // same size, may demote long (it won't, we're being pedantic)
typedef int gp_promoted_arg_int_fast8_t;
typedef unsigned gp_promoted_arg_uint_fast8_t;
#endif
#if INT_FAST16_MAX < INT_MAX
typedef int gp_promoted_arg_int_fast16_t;
typedef int gp_promoted_arg_uint_fast16_t;
#elif INT_FAST16_MAX > INT_MAX
typedef int_fast16_t gp_promoted_arg_int_fast16_t;
typedef uint_fast16_t gp_promoted_arg_uint_fast16_t;
#else
typedef int gp_promoted_arg_int_fast16_t;
typedef unsigned gp_promoted_arg_uint_fast16_t;
#endif
#if INT_FAST32_MAX < INT_MAX // int may be 64 bits (probably not)
typedef int gp_promoted_arg_int_fast32_t;
typedef int gp_promoted_arg_uint_fast32_t;
#elif INT_FAST32_MAX > INT_MAX
typedef int_fast32_t gp_promoted_arg_int_fast32_t;
typedef uint_fast32_t gp_promoted_arg_uint_fast32_t;
#else
typedef int gp_promoted_arg_int_fast32_t;
typedef unsigned gp_promoted_arg_uint_fast32_t;
#endif
typedef int_fast64_t gp_promoted_arg_int_fast64_t;
typedef uint_fast64_t gp_promoted_arg_uint_fast64_t;

typedef intmax_t gp_promoted_arg_intmax_t;
typedef uintmax_t gp_promoted_arg_uintmax_t;

#if WCHAR_MAX <= INT_MAX // may demote (probably not, known to be short in Windows)
typedef int gp_promoted_arg_wchar_t;
#else
typedef wchar_t gp_promoted_arg_wchar_t;
#endif
#if WINT_MAX <= INT_MAX // may demote (probably not, known to be short in Windows)
typedef int gp_promoted_arg_wint_t;
#else
typedef wint_t gp_promoted_arg_wint_t;
#endif

#if !defined(__SDCC)
typedef double gp_promoted_arg_float_t;
typedef double gp_promoted_arg_double_t;
#else // SDCC doesn't have double
typedef float gp_promoted_arg_float_t;
typedef float gp_promoted_arg_double_t;
#endif

#endif // GP_COMMON_INCLUDED
