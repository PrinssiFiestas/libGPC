// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_OVERLOAD_INCLUDED
#define GP_OVERLOAD_INCLUDED 1

#include <gpc/attributes.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup overload Overloading
/// @code
/// #include <gpc/overload.h>
/// @endcode
/// Macros for function/macro overloading by argument number or types.
/// @{

#ifdef GP_DOXYGEN
/** Overloading by argument count.
 * Overloading functions and macro functions by the number of arguments can be
 * done with these macros by defining a variadic macro that expands to
 * `GP_OVERLOADN`, which will do the dispatching based on the number of variadic
 * arguments passed. The first argument to `GP_OVERLOADN` is always
 * `__VA_ARGS__`, which is followed by names of functions/macros to be
 * overloaded in descending order. Some compiler settings may also require
 * trailing comma after names. The arguments also has to be passed to the
 * dispatched function. Zero arguments is not possible.
 *
 * The `N` in `GP_OVERLOADN` should be substituted to maximum number of
 * arguments. Maximum value for `N` is 64.
 *
 * Example use for maximum of three arguments:
 * @code
 * void func1(int arg1);
 * #define MACRO2(arg1, arg2) somefunc(arg1, arg2)
 * int func3(char arg1, void* arg2, const char* arg3);
 *
 * // Note N substituted with 3 in GP_OVERLOADN name and trailing comma.
 * #define func(...) OVERLOAD3(__VA_ARGS__, func3, MACRO2, func1,)(__VA_ARGS__)
 *
 * int main(void)
 * {
 *     func(1);
 *     func(1, 2);
 *     func('1', (void*)2, "3");
 * }
 * @endcode
 */
#define GP_OVERLOADN(...)

// ----------------------------------------------------------------------------
/// @defgroup C11Generic C11 _Generic helpers
/// C11 `_Generic` requires specifying all types explicitly and does not do
/// implicit conversions. This gives good control, but is inconvenient in some
/// cases e.g. you just want to differentiate between an integer and a float. It
/// is also not fully portable. For example, MSVC does not differentiate between
/// char and [un]signed char, but GCC does. Using these macros inside `_Generic`
/// selection fixes the portability issues and increases convenience.
///
/// Example use:
/// @code
/// intmax_t    fooi(intmax_t x);
/// uintmax_t   foou(uintmax_t x);
/// long double foof(long double x);
/// #define foo(X) _Generic((X), \
///     GP_C11_GENERIC_SIGNED_INTEGER(fooi), \
///     GP_C11_GENERIC_UNSIGNED_INTEGER(foou), \
///     GP_C11_GENERIC_FLOAT(foof)) (X)
/// @endcode
/// @{

/** All signed types.
 * Signed primitive integers, `GPInt128`, and plain `char` if signed.
 */
#define GP_C11_GENERIC_SIGNED_TYPE(A) \
    char: (A), signed char: (A), short: (A), int: (A), \
    long: (A), long long: (A), GPInt128: (A)

/** All unsigned types.
 * Unsigned primitive integers, `GPUInt128`, `bool`, and plain `char` if
 * unsigned.
 */
#define GP_C11_GENERIC_UNSIGNED_TYPE(A) \
    bool: (A), unsigned char: (A), unsigned short: (A), unsigned: (A), \
    unsigned long: (A), unsigned long long: (A), GPUInt128: (A)

/** All floats.
 * `float`, `double`, and `long double`.
 */
#define GP_C11_GENERIC_FLOAT(A) float: (A), double: (A), long double: (A)

/** All types that could be considered as numbers.
 * Primitive integers, floats, `bool`, plain `char`, and `GP[U]Int128`.
 */
#define GP_C11_GENERIC_NUMBER(A) \
    char: (A), signed char: (A), short: (A), int: (A), \
    long: (A), long long: (A), GPInt128: (A), \
    bool: (A), unsigned char: (A), unsigned short: (A), unsigned: (A), \
    unsigned long: (A), unsigned long long: (A), GPUInt128: (A), \
    float: (A), double: (A), long double: (A)

/** All types that could be considered as integers.
 * Signed primitive integers, unsigned primitive integers, `GPInt128`,
 * `GPUInt128`, `bool` and plain `char`.
 */
#define GP_C11_GENERIC_INTEGRAL_TYPE(A) \
    char: (A), signed char: (A), short: (A), int: (A), \
    long: (A), long long: (A), GPInt128: (A), \
    bool: (A), unsigned char: (A), unsigned short: (A), unsigned: (A), \
    unsigned long: (A), unsigned long long: (A), GPUInt128: (A)

/** Signed primitive integers.
 * Plain `char` is not considered as integer by this macro since it is mostly
 * commonly used to represent text or raw bytes.
 */
#define GP_C11_GENERIC_SIGNED_INTEGER(A) \
    signed char: (A), short: (A), int: (A), long: (A), long long: (A)

/** Unsigned primitive integers.
 * Plain `char` is not considered as integer by this macro since it is most
 * commonly used to represent text or raw bytes. `bool` is also not considered
 * as integer since it is most commonly used to represent logic.
 */
#define GP_C11_GENERIC_UNSIGNED_INTEGER(A) \
    unsigned char: (A), unsigned short: (A), unsigned: (A), unsigned long: (A), unsigned long long: (A)

/** All primitive integers.
 * Plain `char` is not considered as integer by this macro since it is most
 * commonly used to represent text or raw bytes. `bool` is also not considered
 * as integer since it is most commonly used to represent logic.
 */
#define GP_C11_GENERIC_INTEGER(A) \
    signed char: (A), short: (A), int: (A), long: (A), long long: (A), \
    unsigned char: (A), unsigned short: (A), unsigned: (A), unsigned long: (A), unsigned long long: (A)

/** All arithmetic types.
 * All primitive integers and floating point types. Plain `char` and `bool` are
 * excluded.
 */
#define GP_C11_GENERIC_ARITHMETIC_TYPE(A) \
    signed char: (A), short: (A), int: (A), long: (A), long long: (A), \
    unsigned char: (A), unsigned short: (A), unsigned: (A), unsigned long: (A), unsigned long long: (A), \
    float: (A), double: (A), long double: (A)

/** Null-terminated `char*` and `GPString`.*/
#define GP_C11_GENERIC_STRING(A) char*: (A), const char*: (A), struct gp_char*: (A)

/// @}
#endif // GP_DOXYGEN

#if CHAR_MAX == INT8_MAX // char is signed
#define/* bool */gp_type_is_signed(gp_type_t_TYPE)           ((bool)(GP_TYPE_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_INT128 ))
#define/* bool */gp_type_is_unsigned(gp_type_t_TYPE)         ((bool)(GP_TYPE_BOOL <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UINT128))
#else // char is unsigned
#define/* bool */gp_type_is_signed(gp_type_t_TYPE)           ((bool)(GP_TYPE_SIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_INT128))
#define/* bool */gp_type_is_unsigned(gp_type_t_TYPE)         ((bool)(GP_TYPE_BOOL        <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_CHAR  ))
#endif
#define/* bool */gp_type_is_float(gp_type_t_TYPE)            ((bool)(GP_TYPE_FLOAT         <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_DOUBLE       ))
#define/* bool */gp_type_is_number(gp_type_t_TYPE)           ((bool)(GP_TYPE_BOOL          <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_DOUBLE       ))
#define/* bool */gp_type_is_integral(gp_type_t_TYPE)         ((bool)(GP_TYPE_BOOL          <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_INT128            ))
#define/* bool */gp_type_is_signed_integer(gp_type_t_TYPE)   ((bool)(GP_TYPE_SIGNED_CHAR   <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_LONG         ))
#define/* bool */gp_type_is_unsigned_integer(gp_type_t_TYPE) ((bool)(GP_TYPE_UNSIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG))

#define/* bool */gp_type_is_integer(gp_type_t_TYPE)          ((bool)( \
    (GP_TYPE_SIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_LONG) \
    || (GP_TYPE_UNSIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG)))

#define/* bool */gp_type_is_arithmetic(gp_type_t_TYPE)       ((bool)( \
    (GP_TYPE_SIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_LONG_LONG) \
    || (GP_TYPE_UNSIGNED_CHAR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_UNSIGNED_LONG_LONG) \
    || (GP_TYPE_FLOAT <= gp_type_t_TYPE && gp_type_t_TYPE <= GP_TYPE_LONG_DOUBLE)))

#define/* bool */gp_type_is_string(gp_type_t_TYPE)           ((bool)(GP_TYPE_CHAR_PTR <= (gp_type_t_TYPE) && (gp_type_t_TYPE) <= GP_TYPE_STRING))

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

// _Generic() requires complete types, but we might not want to include
// int128.h, which is a relatively large header to avoid namespace bloat and to
// reduce compile times. These macros are defined in int128.h.
#ifndef GP_INT128_SELECTION
#define GP_INT128_SELECTION(...)
#define GP_UINT128_SELECTION(...)
#endif
#ifndef GP_TETRA_INT_SELECTION
#define GP_TETRA_INT_SELECTION(...)
#define GP_TETRA_UINT_SELECTION(...)
#endif

#ifdef GP_HAS_DIFFERENTIATED_LONG_DOUBLE
#define GP_LONG_DOUBLE_SELECTION(...) long double: __VA_ARGS__
#else
#define GP_LONG_DOUBLE_SELECTION(...)
#endif

#ifdef __SDCC
#define GP_DOUBLE_SELECTION(...) double: __VA_ARGS__
#else
#define GP_DOUBLE_SELECTION(...)
#endif

#ifndef _MSC_VER
#define GP_CHAR_SELECTION(...) char: __VA_ARGS__
#else
#define GP_CHAR_SELECTION(...)
#endif

#if __STDC_VERSION__ >= 201112L || defined(__COMPCERT__) || defined(__TINYC__)
#  define GP_HAS_C11_GENERIC 1
#endif

#ifndef _MSC_VER
#  if CHAR_MAX == SCHAR_MAX
#    define GP_C11_GENERIC_SIGNED_TYPE(A) \
       GP_INT128_SELECTION((A),) \
       char: (A), signed char: (A), short: (A), int: (A), \
       long: (A), long long: (A)
#    define GP_C11_GENERIC_UNSIGNED_TYPE(A) \
       GP_UINT128_SELECTION((A),) \
       bool: (A), unsigned char: (A), unsigned short: (A), unsigned: (A), \
       unsigned long: (A), unsigned long long: (A)
#  else
#    define GP_C11_GENERIC_SIGNED_TYPE(A) \
       GP_INT128_SELECTION((A),) \
       signed char: (A), short: (A), int: (A), \
       long: (A), long long: (A)
#    define GP_C11_GENERIC_UNSIGNED_TYPE(A) \
       GP_UINT128_SELECTION((A),) \
       bool: (A), char: (A): unsigned char: (A), unsigned short: (A), unsigned: (A), \
       unsigned long: (A), unsigned long long: (A)
#  endif
#else // MSVC doesn't differentiate between char and [un]signed char
#  if CHAR_MAX == SCHAR_MAX
#    define GP_C11_GENERIC_SIGNED_TYPE(A) \
       GP_INT128_SELECTION((A),) \
       signed char: (A), short: (A), int: (A), \
       long: (A), long long: (A)
#    define GP_C11_GENERIC_UNSIGNED_TYPE(A) \
       GP_UINT128_SELECTION((A),) \
       bool: (A), unsigned char: (A), unsigned short: (A), unsigned: (A), \
       unsigned long: (A), unsigned long long: (A)
#  else
#    define GP_C11_GENERIC_SIGNED_TYPE(A) \
       GP_INT128_SELECTION((A),) \
       signed char: (A), short: (A), int: (A), long: (A), long long: (A)
#    define GP_C11_GENERIC_UNSIGNED_TYPE(A) \
       GP_UINT128_SELECTION((A),) \
       bool: (A), unsigned char: (A), unsigned short: (A), unsigned: (A), \
       unsigned long: (A), unsigned long long: (A)
#  endif
#endif
#ifdef __SDCC // double not supported
#  define GP_C11_GENERIC_FLOAT(A) float: (A)
#elif defined(GP_HAS_DIFFERENTIATED_LONG_DOUBLE)
#  define GP_C11_GENERIC_FLOAT(A) float: (A), double: (A), long double: (A)
#else
#  define GP_C11_GENERIC_FLOAT(A) float: (A), double: (A)
#endif
#define GP_C11_GENERIC_INTEGRAL_TYPE(A) GP_C11_GENERIC_SIGNED_TYPE(A), GP_C11_GENERIC_UNSIGNED_TYPE(A)
#define GP_C11_GENERIC_NUMBER(A) \
    GP_C11_GENERIC_SIGNED_TYPE(A), GP_C11_GENERIC_UNSIGNED_TYPE(A), GP_C11_GENERIC_FLOAT(A)
#define GP_C11_GENERIC_SIGNED_INTEGER(A) \
    signed char: (A), short: (A), int: (A), long: (A), long long: (A)
#define GP_C11_GENERIC_UNSIGNED_INTEGER(A) \
    unsigned char: (A), unsigned short: (A), unsigned: (A), unsigned long: (A), unsigned long long: (A)
#define GP_C11_GENERIC_INTEGER(A) \
    signed char: (A), short: (A), int: (A), long: (A), long long: (A), \
    unsigned char: (A), unsigned short: (A), unsigned: (A), unsigned long: (A), unsigned long long: (A)
#define GP_C11_GENERIC_ARITHMETIC_TYPE(A) \
    signed char: (A), short: (A), int: (A), long: (A), long long: (A), \
    unsigned char: (A), unsigned short: (A), unsigned: (A), unsigned long: (A), unsigned long long: (A), \
    GP_C11_GENERIC_FLOAT(A)
#define GP_C11_GENERIC_STRING(A) char*: (A), const char*: (A), struct gp_char*: (A)

// ----------------------------------------------------------------------------

#if __clang__
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#define GP_OVERLOAD1(_0, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD2(_0, _1, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD3(_0, _1, _2, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD4(_0, _1, _2, _3, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD5(_0, _1, _2, _3, _4, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD6(_0, _1, _2, _3, _4, _5, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD7(_0, _1, _2, _3, _4, _5, _6, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD8(_0, _1, _2, _3, _4, _5, _6, _7, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD9(_0, _1, _2, _3, _4, _5, _6, _7, _8, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, RESOLVED, ...) 	\
RESOLVED
#define GP_OVERLOAD12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, RESOLVED, ...) 	\
RESOLVED
#define GP_OVERLOAD13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, RESOLVED, 	\
...) RESOLVED
#define GP_OVERLOAD14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, RESOLVED, ...) \
RESOLVED
#define GP_OVERLOAD31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, RESOLVED, \
...) RESOLVED
#define GP_OVERLOAD32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, RESOLVED, ...) \
RESOLVED
#define GP_OVERLOAD49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, RESOLVED, \
...) RESOLVED
#define GP_OVERLOAD50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, RESOLVED, ...) RESOLVED

// TODO support for zero arguments using __VA_OPT__ if available or potentially
// https://medium.com/@pauljlucas/using-advanced-c-preprocessor-macros-for-a-pre-c23-c-20-va-opt-substitute-bccefde27817
// also check
// https://www.reddit.com/r/C_Programming/comments/1u2twhq/mimicking_of_function_overloading/

///@endcond
#endif // GP_OVERLOAD_INCLUDED
