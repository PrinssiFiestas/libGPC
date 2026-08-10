// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Note to everybody: Compiler Explorer is your friend.

// TODO string conversions and GDB pretty printers.

#ifndef GP_INT128_INCLUDED
#define GP_INT128_INCLUDED 1

#include <gpc/gptypes.h>
#include <gpc/gpoverload.h>
#include <gpc/gpendian.h>
#include <gpc/gppreprocessor.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#if __STDC_VERSION__ >= 201112L || GP_HAS_INCLUDE(<stdalign.h>)
#include <stdalign.h>
#endif

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#pragma intrinsic (__shiftleft128, __shiftright128, _umul128, _mul128)
#endif

#ifdef __cplusplus
#include <type_traits>
#endif

#if (__GNUC__ && defined(__SIZEOF_INT128__)) || __clang__ || GP_TEST_INT128
#  ifndef GP_TEST_INT128
#    define GP_HAS_TETRA_INT 1
#  else
#    include <limits.h>
#  endif
// These are like __int128_t, but more often supported by Clang. Not as widely
// supported by GCC (e.g. missing on ARMv7), so keep the incompatibility in mind!
typedef unsigned gp_tetra_uint_t __attribute__((mode(TI)));
typedef int      gp_tetra_int_t  __attribute__((mode(TI)));
#endif

// We do not want use tetra ints on 32-bit targets, this breaks compatibility
// between Clang and GCC.
#if defined(GP_HAS_TETRA_INT) && SIZE_MAX == UINT64_MAX
#  define GP_USE_TETRA_INT 1
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup int128 128-bit Integers
/// @code
/// #include <gpc/gpint128.h>
/// @endcode
/// Portable 128-bit integer types with functions (most inline) and type generic
/// macros for 128-bit integer arithmetic. All [C arithmetic operators](https://en.cppreference.com/c/language/operator_arithmetic)
/// are implemented and loosely follow the semantics of regular C arithmetic
/// operators when applicable. Also conversions from/to all integer and floating
/// point types are provided.
///
/// These use compiler extensions or intrinsics when availaible for maximum
/// performance. Otherwise implementations fall back to pure C99 for
/// maximum portability.
/// @{

/** 128-bit unsigned integer.
 *
 * Use @ref gp_uint128() to construct from `uint64_t` parts or @ref gp_u128() to
 * construct from any built-in arithmetic type.
 */
typedef union GPUInt128
{
    #if (defined(GP_HAS_ANONYMOUS_STRUCT) && GP_ENDIAN == GP_ENDIAN_LITTLE) || defined(GP_DOXYGEN)
    struct {
        /** Low 64 bits.
         *
         * Only available if compiler supports anonymous structs and endianness
         * was detected in preprocessor. See @ref GP_HAS_ANONYMOUS_STRUCT and
         * @ref GP_ENDIAN. Maximum portability applications should use @ref gp_uint128_lo()
         * instead.
         */
        uint64_t lo;

        /** High 64 bits.
         *
         * Only available if compiler supports anonymous structs and endianness
         * was detected in preprocessor. See @ref GP_HAS_ANONYMOUS_STRUCT and
         * @ref GP_ENDIAN. Maximum portability applications should use @ref gp_uint128_hi()
         * instead.
         */
        uint64_t hi;
    };
    #elif defined(GP_HAS_ANONYMOUS_STRUCT) && GP_ENDIAN == GP_ENDIAN_BIG
    struct {
        uint64_t hi;
        uint64_t lo;
    };
    #endif

    struct {
        uint64_t lo; ///< Low 64 bits.
        uint64_t hi; ///< High 64 bits.
    } little_endian; ///< 64-bit parts in a little endian machine.

    struct {
        uint64_t hi; ///< High 64 bits.
        uint64_t lo; ///< Low 64 bits.
    } big_endian;    ///< 64-bit Parts in big endian machine.

    // Only align to 16 on 64-bit targets. GP_ALLOC_ALIGN is only 8, which would
    // cause ultimate mayhem.
    #if SIZE_MAX == UINT64_MAX
    #  if defined(GP_HAS_TETRA_INT) || defined(GP_TEST_INT128)
    gp_tetra_uint_t u128; // has alignment of 16
    // rest of the branches are for ABI consistency more than for performance.
    #  elif __STDC_VERSION__ >= 201112L || GP_HAS_INCLUDE(<stdalign.h>)
    alignas(16) int _align;
    #  elif defined(_MSC_VER)
    __declspec(align(16)) int _align;
    #  elif defined(GP_HAS_DIFFERENTIATED_LONG_DOUBLE)
    long double _align; // not guaranteed to be 16, but our only hope!
    #  endif
    #endif
} GPUInt128;

/** 128-bit signed integer.
 *
 * Use @ref gp_int128() to construct from `uint64_t` parts or @ref gp_i128() to
 * construct from any built-in arithmetic type.
 *
 * Overflow is undefined like with regular signed integers.
 */
typedef union GPInt128
{
    #if (defined(GP_HAS_ANONYMOUS_STRUCT) && GP_ENDIAN == GP_ENDIAN_LITTLE) || defined(GP_DOXYGEN)
    struct {
        /** Low 64 bits.
         *
         * Only available if compiler supports anonymous structs and endianness
         * was detected in preprocessor. See @ref GP_HAS_ANONYMOUS_STRUCT and
         * @ref GP_ENDIAN. Maximum portability applications should use @ref gp_int128_lo()
         * instead.
         *
         * This is unsigned, because there is no way of determining if the
         * number is negative based on low bits alone since sign bit is the most
         * significant bit. Therefore, low bits are just interpreted as raw
         * bits, which should be unsigned. This also avoids undefined behavior
         * on overflow, which would happen all the time with large numbers.
         */
        uint64_t lo;

        /** High 64 bits.
         *
         * Only available if compiler supports anonymous structs and endianness
         * was detected in preprocessor. See @ref GP_HAS_ANONYMOUS_STRUCT and
         * @ref GP_ENDIAN. Maximum portability applications should use @ref gp_int128_hi()
         * instead.
         */
        int64_t hi;
    };
    #elif defined(GP_HAS_ANONYMOUS_STRUCT) && GP_ENDIAN == GP_ENDIAN_BIG
    struct {
        int64_t  hi;
        uint64_t lo;
    };
    #endif

    struct {
        uint64_t lo; ///< Low 64 bits.
        int64_t  hi; ///< High 64 bits.
    } little_endian; ///< 64-bit parts in a little endian machine.

    struct {
        int64_t  hi; ///< High 64 bits.
        uint64_t lo; ///< Low 64 bits.
    } big_endian;    ///< 64-bit Parts in big endian machine.

    // Only align to 16 on 64-bit targets. GP_ALLOC_ALIGN is only 8, which would
    // cause ultimate mayhem.
    #if SIZE_MAX == UINT64_MAX
    #  if defined(GP_HAS_TETRA_INT) || defined(GP_TEST_INT128)
    gp_tetra_int_t i128; // has alignment of 16.
    #  elif __STDC_VERSION__ >= 201112L || GP_HAS_INCLUDE(<stdalign.h>)
    alignas(16) int _align;
    #  elif defined(_MSC_VER)
    __declspec(align(16)) int _align;
    #  elif defined(GP_HAS_DIFFERENTIATED_LONG_DOUBLE)
    long double _align; // not guaranteed to be 16, but our only hope!
    #  endif
    #endif
} GPInt128;

// ----------------------------------------------------------------------------
// Limits

/** Maximum value of @ref GPUInt128. */
#define GP_UINT128_MAX gp_uint128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
/** Maximum value of @ref GPInt128. */
#define GP_INT128_MAX  gp_int128(0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF)
/** Minimum value of @ref GPInt128. */
#define GP_INT128_MIN  gp_int128(INT64_MIN, 0)
/// @cond
#define GP_TETRA_UINT_MAX ((gp_tetra_uint_t)-1)
#define GP_TETRA_INT_MAX  ((gp_tetra_int_t)((gp_tetra_uint_t)-1 >> 1))
#define GP_TETRA_INT_MIN  ((gp_tetra_int_t)-1 << 127)
/// @endcond

// ----------------------------------------------------------------------------
// Constructors and Accessors

/** Create 128-bit unsigned integer 64-bit from parts.*/
GP_NODISCARD GP_INLINE
GPUInt128 gp_uint128(uint64_t hi_bits, uint64_t lo_bits)
{
    GPUInt128 u128;
    if (gp_endian_is_big()) {
        u128.big_endian.hi = hi_bits;
        u128.big_endian.lo = lo_bits;
    } else {
        u128.little_endian.hi = hi_bits;
        u128.little_endian.lo = lo_bits;
    }
    return u128;
}
/** Create 128-bit signed integer from 64-bit parts.*/
GP_NODISCARD GP_INLINE
GPInt128 gp_int128(int64_t hi_bits, uint64_t lo_bits)
{
    GPInt128 i128;
    if (gp_endian_is_big()) {
        i128.big_endian.hi = hi_bits;
        i128.big_endian.lo = lo_bits;
    } else {
        i128.little_endian.hi = hi_bits;
        i128.little_endian.lo = lo_bits;
    }
    return i128;
}

#ifdef GP_DOXYGEN
/** Cast value to an 128-bit unsigned integer.
 *
 * Converts @a X to an 128-bit unsigned integer. @a X can be any arithmetic
 * type, @ref GPUInt128, or @ref GPInt128. Not available in strict C99.
 */
#define gp_u128(X) _Generic(X, ...)(X)

/** Cast value to an 128-bit signed integer.
 *
 * Converts @a X to an 128-bit signed integer. @a X can be any arithmetic
 * type, @ref GPUInt128, or @ref GPInt128. Not available in strict C99.
 */
#define gp_i128(X) _Generic(X, ...)(X)
#endif

/** Convert 128-bit signed integer to 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE
GPUInt128 gp_uint128_i128(GPInt128 i)
{
    GPUInt128 u;
    memcpy(&u, &i, sizeof u);
    return u;
}
/** Convert 128-bit unsigned integer to 128-bit signed integer.*/
GP_NODISCARD GP_INLINE
GPInt128 gp_int128_u128(GPUInt128 u)
{
    GPInt128 i;
    memcpy(&i, &u, sizeof i);
    return i;
}

/** Convert 64-bit unsigned integer to 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_u64(uint64_t u) { return gp_uint128(0, u)     ; }
/** Convert sign extended 64-bit signed integer to 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_i64(int64_t i)  { return gp_uint128(-(i<0), i); }
/** Convert 64-bit unsigned integer to 128-bit signed integer.*/
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_u64(uint64_t u)   { return gp_int128(0, u)      ; }
/** Convert sign extended 64-bit signed integer to 128-bit signed integer.*/
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_i64(int64_t i)    { return gp_int128(-(i<0), i) ; }

/** Get low bits of 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE
uint64_t gp_uint128_lo(GPUInt128 u)
{
    return gp_endian_is_little() ? u.little_endian.lo : u.big_endian.lo;
}
/** Get low bits of 128-bit signed integer.*/
GP_NODISCARD GP_INLINE
uint64_t gp_int128_lo(GPInt128 i)
{
    return gp_endian_is_little() ? i.little_endian.lo : i.big_endian.lo;
}

/** Get high bits of 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE
uint64_t gp_uint128_hi(GPUInt128 u)
{
    return gp_endian_is_little() ? u.little_endian.hi : u.big_endian.hi;
}
/** Get signed high bits of 128-bit signed integer.*/
GP_NODISCARD GP_INLINE
int64_t gp_int128_hi(GPInt128 i)
{
    return gp_endian_is_little() ? i.little_endian.hi : i.big_endian.hi;
}

/** Get address of low bits of 128-bit unsigned integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
GP_INLINE uint64_t* gp_uint128_lo_addr(GPUInt128* u)
{
    return gp_endian_is_little() ? &u->little_endian.lo : &u->big_endian.lo;
}
/** Get address of low bits of 128-bit signed integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
GP_INLINE uint64_t* gp_int128_lo_addr(GPInt128* i)
{
    return gp_endian_is_little() ? &i->little_endian.lo : &i->big_endian.lo;
}

/** Get address of high bits of 128-bit unsigned integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
GP_INLINE uint64_t* gp_uint128_hi_addr(GPUInt128* u)
{
    return gp_endian_is_little() ? &u->little_endian.hi : &u->big_endian.hi;
}
/** Get address of signed high bits of 128-bit signed integer.*/
GP_NODISCARD GP_NONNULL_ARGS_AND_RETURN
GP_INLINE int64_t* gp_int128_hi_addr(GPInt128* i)
{
    return gp_endian_is_little() ? &i->little_endian.hi : &i->big_endian.hi;
}

#if defined(GP_USE_TETRA_INT) || defined(GP_TEST_INT128)
GP_NODISCARD GP_INLINE
GPUInt128 gp_uint128_tetra_uint(gp_tetra_uint_t _u)
{
    GPUInt128 u;
    u.u128 = _u;
    return u;
}
GP_NODISCARD GP_INLINE
GPInt128 gp_int128_tetra_int(gp_tetra_int_t _i)
{
    GPInt128 i;
    i.i128 = _i;
    return i;
}
#endif

/** Convert double precision floating point number to 128-bit unsigned integer. */
GP_NODISCARD GP_INLINE GPUInt128 gp_uint128_f64(double d)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint((gp_tetra_uint_t)d);
    #else
    GP_HIDDEN GP_CONST_FUNCTION GPUInt128 gp_uint128_convert_f64(double);
    return gp_uint128_convert_f64(d);
    #endif
}
/** Convert double precision floating point number to 128-bit signed integer. */
GP_NODISCARD GP_INLINE GPInt128 gp_int128_f64(double d)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int((gp_tetra_uint_t)d);
    #else
    GP_HIDDEN GP_CONST_FUNCTION GPInt128 gp_int128_convert_f64(double);
    return gp_int128_convert_f64(d);
    #endif
}
/** Convert single precision floating point number to 128-bit unsigned integer. */
GP_NODISCARD GP_INLINE GPUInt128 gp_uint128_f32(float f)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint((gp_tetra_uint_t)f);
    #else
    GP_HIDDEN GP_CONST_FUNCTION GPUInt128 gp_uint128_convert_f32(float);
    return gp_uint128_convert_f32(f);
    #endif
}
/** Convert single precision floating point number to 128-bit signed integer. */
GP_NODISCARD GP_INLINE GPInt128 gp_int128_f32(float f)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int((gp_tetra_uint_t)f);
    #else
    GP_HIDDEN GP_CONST_FUNCTION GPInt128 gp_int128_convert_f32(float);
    return gp_int128_convert_f32(f);
    #endif
}
/** Convert 128-bit unsigned integer to double precision floating point number. */
GP_NODISCARD GP_INLINE double gp_f64_uint128(GPUInt128 u)
{
    #ifdef GP_USE_TETRA_INT
    return u.u128;
    #else
    GP_HIDDEN GP_CONST_FUNCTION double gp_f64_convert_uint128(GPUInt128);
    return gp_f64_convert_uint128(u);
    #endif
}
/** Convert 128-bit signed integer to double precision floating point number. */
GP_NODISCARD GP_INLINE double gp_f64_int128(GPInt128 i)
{
    #ifdef GP_USE_TETRA_INT
    return i.i128;
    #else
    GP_HIDDEN GP_CONST_FUNCTION double gp_f64_convert_int128(GPInt128);
    return gp_f64_convert_int128(i);
    #endif
}
/** Convert 128-bit unsigned integer to single precision floating point number. */
GP_NODISCARD GP_INLINE float gp_f32_uint128(GPUInt128 u)
{
    #ifdef GP_USE_TETRA_INT
    return u.u128;
    #else
    GP_HIDDEN GP_CONST_FUNCTION float gp_f32_convert_uint128(GPUInt128);
    return gp_f32_convert_uint128(u);
    #endif
}
/** Convert 128-bit signed integer to single precision floating point number. */
GP_NODISCARD GP_INLINE float gp_f32_int128(GPInt128 i)
{
    #ifdef GP_USE_TETRA_INT
    return i.i128;
    #else
    GP_HIDDEN GP_CONST_FUNCTION float gp_f32_convert_int128(GPInt128);
    return gp_f32_convert_int128(i);
    #endif
}

#ifdef __cplusplus // concise constructors, available in C as macros
GP_NODISCARD static inline constexpr GPUInt128 gp_u128(uint64_t hi, uint64_t lo) { return gp_uint128(hi, lo); }
GP_NODISCARD static inline constexpr GPUInt128 gp_u128(GPInt128 i)  { return gp_uint128_i128(i); }
GP_NODISCARD static inline constexpr GPUInt128 gp_u128(double f)    { return gp_uint128_f64(f);  }
GP_NODISCARD static inline constexpr GPUInt128 gp_u128(float f)     { return gp_uint128_f32(f);  }
GP_NODISCARD static inline constexpr GPUInt128 gp_u128(GPUInt128 u) { return u; } // useful for generics

/** Convert primitive integers to 128-bit unsigned integer.
 * Will sign extend if @p i is a negative signed integer.
 */
template <typename T> GP_NODISCARD static inline constexpr
typename std::enable_if<std::is_integral<T>::value, GPUInt128>::type
gp_u128(T i) { return gp_uint128(-(i<0), i); }

GP_NODISCARD static inline constexpr GPInt128 gp_i128(int64_t hi, uint64_t lo) { return gp_int128(hi, lo); }
GP_NODISCARD static inline constexpr GPInt128 gp_i128(GPUInt128 u) { return gp_int128_u128(u); }
GP_NODISCARD static inline constexpr GPInt128 gp_i128(double f)    { return gp_int128_f64(f);  }
GP_NODISCARD static inline constexpr GPInt128 gp_i128(float f)     { return gp_int128_f32(f);  }
GP_NODISCARD static inline constexpr GPInt128 gp_i128(GPInt128 i)  { return i; } // useful for generics

/** Convert primitive integers to 128-bit signed integer.
 * Will sign extend if @p i is a negative signed integer.
 */
template <typename T> GP_NODISCARD static inline constexpr
typename std::enable_if<std::is_integral<T>::value, GPInt128>::type
gp_i128(T i) { return gp_int128(-(i<0), i); }

#  ifdef GP_USE_TETRA_INT
GP_NODISCARD static inline constexpr GPUInt128 gp_u128(gp_tetra_uint_t u) {return gp_uint128_tetra_uint(u);}
GP_NODISCARD static inline constexpr GPInt128  gp_i128(gp_tetra_int_t  i) {return gp_int128_tetra_int(i)  ;}
#  endif

#else // C
#define gp_u128(...) GP_OVERLOAD2(__VA_ARGS__, gp_uint128, GP_U128_CTOR)(__VA_ARGS__)
#define gp_i128(...) GP_OVERLOAD2(__VA_ARGS__, gp_int128,  GP_I128_CTOR)(__VA_ARGS__)
#endif

// ----------------------------------------------------------------------------
/// @defgroup generic_int128 Type-generic Macros
///
/// Type generic macros for 128-bit arithmetic. These take any combination of
/// arithmetic types, @ref GPUInt128, and @ref GPInt128 as their arguments and
/// return a 128-bit integer. All macros with `gp_u128_` prefix return
/// @ref GPUInt128 and macros with `gp_i128_` prefix return @ref GPInt128. These
/// are not available in strict C99.
///
/// To cast any arithmetic type, @ref GPUInt128, or @ref GPInt128 types to
/// 128-bit integers, use @ref gp_u128() and @ref gp_i128().
/// @{

#define gp_u128_add(A, B)                gp_uint128_add(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_add(A, B)                gp_int128_add(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_sub(A, B)                gp_uint128_sub(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_sub(A, B)                gp_int128_sub(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_mul(A, B)                gp_uint128_mul(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_mul(A, B)                gp_int128_mul(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_div(A, B)                gp_uint128_div(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_div(A, B)                gp_int128_div(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_mod(A, B)                gp_uint128_mod(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_mod(A, B)                gp_int128_mod(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_negate(A)                gp_uint128_negate(gp_u128(A)) ///< @selfdocumenting
#define gp_i128_negate(A)                gp_int128_negate(gp_i128(A)) ///< @selfdocumenting
#define gp_u128_not(A)                   gp_uint128_not(gp_u128(A)) ///< @selfdocumenting
#define gp_i128_not(A)                   gp_int128_not(gp_i128(A)) ///< @selfdocumenting
#define gp_u128_and(A, B)                gp_uint128_and(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_and(A, B)                gp_int128_and(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_or(A, B)                 gp_uint128_or(gp_u128(A),  gp_u128(B)) ///< @selfdocumenting
#define gp_i128_or(A, B)                 gp_int128_or(gp_i128(A),  gp_i128(B)) ///< @selfdocumenting
#define gp_u128_xor(A, B)                gp_uint128_xor(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_xor(A, B)                gp_int128_xor(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_shift_left(A, B)         gp_uint128_shift_left(gp_u128(A), B) ///< @selfdocumenting
#define gp_i128_shift_left(A, B)         gp_int128_shift_left(gp_i128(A), B) ///< @selfdocumenting
#define gp_u128_shift_right(A, B)        gp_uint128_shift_right(gp_u128(A), B) ///< @selfdocumenting
#define gp_i128_shift_right(A, B)        gp_int128_shift_right(gp_i128(A), B) ///< @selfdocumenting
#define gp_u128_equal(A, B)              gp_uint128_equal(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_equal(A, B)              gp_int128_equal(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_not_equal(A, B)          gp_uint128_not_equal(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_not_equal(A, B)          gp_int128_not_equal(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_greater_than(A, B)       gp_uint128_greater_than(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_greater_than(A, B)       gp_int128_greater_than(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_less_than(A, B)          gp_uint128_less_than(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_less_than(A, B)          gp_int128_less_than(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_greater_than_equal(A, B) gp_uint128_greater_than_equal(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_greater_than_equal(A, B) gp_int128_greater_than_equal(gp_i128(A), gp_i128(B)) ///< @selfdocumenting
#define gp_u128_less_than_equal(A, B)    gp_uint128_less_than_equal(gp_u128(A), gp_u128(B)) ///< @selfdocumenting
#define gp_i128_less_than_equal(A, B)    gp_int128_less_than_equal(gp_i128(A), gp_i128(B)) ///< @selfdocumenting

/// @}
// ----------------------------------------------------------------------------
// Bitwise Operations

/** Unsigned bitwise NOT */
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_not(GPUInt128 a)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(~a.u128);
    #else
    return gp_uint128(~gp_uint128_hi(a), ~gp_uint128_lo(a));
    #endif
}
/** Signed bitwise NOT */
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_not(GPInt128 a)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(~a.i128);
    #else
    return gp_int128(~gp_int128_hi(a), ~gp_int128_lo(a));
    #endif
}

/** Unsigned bitwise AND */
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_and(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 & b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) & gp_uint128_hi(b), gp_uint128_lo(a) & gp_uint128_lo(b));
    #endif
}
/** Signed bitwise AND */
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_and(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 & b.i128);
    #else
    return gp_int128(gp_int128_hi(a) & gp_int128_hi(b), gp_int128_lo(a) & gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise OR */
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_or(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 | b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) | gp_uint128_hi(b), gp_uint128_lo(a) | gp_uint128_lo(b));
    #endif
}
/** Signed bitwise OR */
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_or(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 | b.i128);
    #else
    return gp_int128(gp_int128_hi(a) | gp_int128_hi(b), gp_int128_lo(a) | gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise XOR */
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_xor(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 ^ b.u128);
    #else
    return gp_uint128(gp_uint128_hi(a) ^ gp_uint128_hi(b), gp_uint128_lo(a) ^ gp_uint128_lo(b));
    #endif
}
/** Signed bitwise XOR */
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_xor(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 ^ b.i128);
    #else
    return gp_int128(gp_int128_hi(a) ^ gp_int128_hi(b), gp_int128_lo(a) ^ gp_int128_lo(b));
    #endif
}

/** Unsigned bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_shift_left(GPUInt128 a, uint8_t b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 << b);
    #else
    if (b == 0) // avoid UB in `<< (64-b)`
        return a;
    if (b >= 64)
        return gp_uint128(gp_uint128_lo(a) << (b-64), 0);
    #  if _MSC_VER && defined(_M_X64)
    uint64_t hi = __shiftleft128(gp_uint128_lo(a), gp_uint128_hi(a), b);
    return gp_uint128(hi, gp_uint128_lo(a) << b);
    #  else
    return gp_uint128(
        (gp_uint128_hi(a) << b) | (gp_uint128_lo(a) >> (64-b)),
         gp_uint128_lo(a) << b);
    #  endif
    #endif
}
/** Signed bitwise left shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_shift_left(GPInt128 a, uint8_t b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 << b);
    #else
    if (b == 0) // avoid UB in `<< (64-b)`
        return a;
    if (b >= 64)
        return gp_int128(gp_int128_lo(a) << (b-64), 0);
    return gp_int128(
        (gp_int128_hi(a) << b) | (gp_int128_lo(a) >> (64-b)),
         gp_int128_lo(a) << b);
    #endif
}

/** Unsigned bitwise right shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_shift_right(GPUInt128 a, uint8_t b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 >> b);
    #else
    if (b == 0) // avoid UB in `>> (64-b)`
        return a;
    if (b >= 64)
        return gp_uint128(0, gp_uint128_hi(a) >> (b-64));
    #  if _MSC_VER && defined(_M_X64)
    uint64_t lo = __shiftright128(gp_uint128_lo(a), gp_uint128_hi(a), b);
    return gp_uint128(gp_uint128_hi(a) >> b, lo);
    #  else
    return gp_uint128(
         gp_uint128_hi(a) >> b,
        (gp_uint128_lo(a) >> b) | (gp_uint128_hi(a) << (64-b)));
    #  endif
    #endif
}
/** Signed bitwise right shift.
 * Shifting by more than 127 is undefined.
 */
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_shift_right(GPInt128 a, uint8_t b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 >> b);
    #else
    if (b == 0) // avoid UB in `>> (64-b)`
        return a;
    if (b >= 64)
        return gp_int128(gp_int128_hi(a) >> 63/*sign extend*/, gp_int128_hi(a) >> (b-64));
    return gp_int128(
         gp_int128_hi(a) >> b,
        (gp_int128_lo(a) >> b) | ((uint64_t)gp_int128_hi(a) << (64-b)));
    #endif
}

// ----------------------------------------------------------------------------
// Arithmetic

/** Add 128-bit unsigned integers.*/
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_add(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 + b.u128);
    #else
    return gp_uint128(
        gp_uint128_hi(a) + gp_uint128_hi(b)
            + (gp_uint128_lo(b) > UINT64_MAX - gp_uint128_lo(a)), // carry
        gp_uint128_lo(a) + gp_uint128_lo(b));
    #endif
}
/** Add 128-bit signed integers.*/
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_add(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 + b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) + gp_int128_hi(b)
            + (gp_int128_lo(b) > UINT64_MAX - gp_int128_lo(a)), // carry
        gp_int128_lo(a) + gp_int128_lo(b));
    #endif
}

/** Subtract 128-bit unsigned integers.*/
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_sub(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 - b.u128);
    #else
    return gp_uint128(
        gp_uint128_hi(a) - gp_uint128_hi(b)
            - (gp_uint128_lo(b) > gp_uint128_lo(a)), // borrow
        gp_uint128_lo(a) - gp_uint128_lo(b));
    #endif
}
/** Subtract 128-bit signed integers.*/
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_sub(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 - b.i128);
    #else
    return gp_int128(
        gp_int128_hi(a) - gp_int128_hi(b)
            - (gp_int128_lo(b) > gp_int128_lo(a)), // borrow
        gp_int128_lo(a) - gp_int128_lo(b));
    #endif
}

/** Negate 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE   GPUInt128 gp_uint128_negate(GPUInt128 a)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(-a.u128);
    #else
    return gp_uint128(~gp_uint128_hi(a) + !gp_uint128_lo(a), ~gp_uint128_lo(a) + 1);
    #endif
}
/** Negate 128-bit signed integer.*/
GP_NODISCARD GP_INLINE   GPInt128 gp_int128_negate(GPInt128 a)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(-a.i128);
    #else
    return gp_int128_u128(gp_uint128_negate(gp_uint128_i128(a)));
    #endif
}

/** Multiply 64-bit unsigned integers to 128-bit unsigned integer.*/
GP_NODISCARD GP_INLINE GPUInt128 gp_uint128_mul64(uint64_t a, uint64_t b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint((gp_tetra_uint_t)a * b);
    #elif _MSC_VER && defined(_M_X64)
    uint64_t lo, hi;
    lo = _umul128(a, b, &hi);
    return gp_uint128(hi, lo);
    #else
    GP_HIDDEN GP_CONST_FUNCTION GPUInt128 gp_uint128_long_mul64(uint64_t a, uint64_t b);
    return gp_uint128_long_mul64(a, b);
    #endif
}
/** Multiply 128-bit unsigned integers.*/
GP_NODISCARD GP_INLINE GPUInt128 gp_uint128_mul(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 * b.u128);
    #else
    GPUInt128 y = gp_uint128_mul64(gp_uint128_lo(a), gp_uint128_lo(b));
    *gp_uint128_hi_addr(&y) += gp_uint128_hi(a)*gp_uint128_lo(b) + gp_uint128_lo(a)*gp_uint128_hi(b);
    return y;
    #endif
}
/** Multiply 64-bit signed integers to 128-bit signed integer.*/
GP_NODISCARD GP_INLINE GPInt128 gp_int128_mul64(int64_t a, int64_t b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int((gp_tetra_int_t)a * b);
    #elif _MSC_VER && defined(_M_X64)
    __int64 lo, hi;
    lo = _mul128(a, b, &hi);
    return gp_int128(hi, lo);
    #else
    return gp_int128_u128(gp_uint128_mul(gp_uint128(-(a<0), a), gp_uint128(-(b<0), b)));
    #endif
}

/** Multiply 128-bit signed integers.*/
GP_NODISCARD GP_INLINE GPInt128 gp_int128_mul(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 * b.i128);
    #else
    return gp_int128_u128(
        gp_uint128_mul(
            gp_uint128_i128(a),
            gp_uint128_i128(b)));
    #endif
}

/** Divide 128-bit unsigned integers.*/
GP_NODISCARD GP_INLINE GPUInt128 gp_uint128_div(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 / b.u128);
    #else
    GP_HIDDEN GP_PURE
    GPUInt128 gp_uint128_divmod(GPUInt128 a, GPUInt128 b, GPUInt128* optional_remainder);
    return gp_uint128_divmod(a, b, NULL);
    #endif
}
/** Divide 128-bit signed integers.*/
GP_NODISCARD GP_INLINE GPInt128 gp_int128_div(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 / b.i128);
    #else
    GP_HIDDEN GP_CONST_FUNCTION
    GPInt128 gp_int128_idiv(GPInt128 a, GPInt128 b);
    return gp_int128_idiv(a, b);
    #endif
}

/** 128-bit unsigned integer modulus.*/
GP_NODISCARD GP_INLINE GPUInt128 gp_uint128_mod(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_uint128_tetra_uint(a.u128 % b.u128);
    #else
    GP_HIDDEN GP_PURE
    GPUInt128 gp_uint128_divmod(GPUInt128 a, GPUInt128 b, GPUInt128* optional_remainder);
    GPUInt128 remainder;
    gp_uint128_divmod(a, b, &remainder);
    return remainder;
    #endif
}
/** 128-bit signed integer modulus.*/
GP_NODISCARD GP_INLINE GPInt128 gp_int128_mod(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return gp_int128_tetra_int(a.i128 % b.i128);
    #else
    GP_HIDDEN GP_CONST_FUNCTION GPInt128 gp_int128_imod(GPInt128 a, GPInt128 b);
    return gp_int128_imod(a, b);
    #endif
}

// ----------------------------------------------------------------------------
// Comparisons

/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_uint128_equal(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.u128 == b.u128;
    #else
    return a.little_endian.lo == b.little_endian.lo && a.little_endian.hi == b.little_endian.hi;
    #endif
}
/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_int128_equal(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.i128 == b.i128;
    #else
    return a.little_endian.lo == b.little_endian.lo && a.little_endian.hi == b.little_endian.hi;
    #endif
}

/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_uint128_not_equal(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.u128 != b.u128;
    #else
    return a.little_endian.lo != b.little_endian.lo || a.little_endian.hi != b.little_endian.hi;
    #endif
}
/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_int128_not_equal(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.i128 != b.i128;
    #else
    return a.little_endian.lo != b.little_endian.lo || a.little_endian.hi != b.little_endian.hi;
    #endif
}

/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_uint128_greater_than(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.u128 > b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) > gp_uint128_lo(b);
    return gp_uint128_hi(a) > gp_uint128_hi(b);
    #endif
}
/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_int128_greater_than(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.i128 > b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) > gp_int128_lo(b);
    return gp_int128_hi(a) > gp_int128_hi(b);
    #endif
}

/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_uint128_less_than(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.u128 < b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) < gp_uint128_lo(b);
    return gp_uint128_hi(a) < gp_uint128_hi(b);
    #endif
}
/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_int128_less_than(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.i128 < b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) < gp_int128_lo(b);
    return gp_int128_hi(a) < gp_int128_hi(b);
    #endif
}

/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_uint128_greater_than_equal(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.u128 >= b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) >= gp_uint128_lo(b);
    return gp_uint128_hi(a) >= gp_uint128_hi(b);
    #endif
}
/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_int128_greater_than_equal(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.i128 >= b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) >= gp_int128_lo(b);
    return gp_int128_hi(a) >= gp_int128_hi(b);
    #endif
}

/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_uint128_less_than_equal(GPUInt128 a, GPUInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.u128 <= b.u128;
    #else
    if (gp_uint128_hi(a) == gp_uint128_hi(b))
        return gp_uint128_lo(a) <= gp_uint128_lo(b);
    return gp_uint128_hi(a) <= gp_uint128_hi(b);
    #endif
}
/** @selfdocumenting */
GP_NODISCARD GP_INLINE   bool gp_int128_less_than_equal(GPInt128 a, GPInt128 b)
{
    #ifdef GP_USE_TETRA_INT
    return a.i128 <= b.i128;
    #else
    if (gp_int128_hi(a) == gp_int128_hi(b))
        return gp_int128_lo(a) <= gp_int128_lo(b);
    return gp_int128_hi(a) <= gp_int128_hi(b);
    #endif
}

#ifdef __cplusplus // operator overloads

// No implicit conversions provided for simplicity and to avoid pitfalls of C++
// template meta-programming and to avoid implicit conversion bugs.

// Arithmetic
GP_NODISCARD static inline constexpr GPUInt128 operator +(GPUInt128 a, GPUInt128 b) { return gp_uint128_add(a, b); }
GP_NODISCARD static inline constexpr GPInt128  operator +(GPInt128  a, GPInt128  b) { return gp_int128_add(a, b) ; }
GP_NODISCARD static inline constexpr GPUInt128 operator -(GPUInt128 a, GPUInt128 b) { return gp_uint128_sub(a, b); }
GP_NODISCARD static inline constexpr GPInt128  operator -(GPInt128  a, GPInt128  b) { return gp_int128_sub(a, b) ; }
GP_NODISCARD static inline           GPUInt128 operator *(GPUInt128 a, GPUInt128 b) { return gp_uint128_mul(a, b); }
GP_NODISCARD static inline           GPInt128  operator *(GPInt128  a, GPInt128  b) { return gp_int128_mul(a, b) ; }
GP_NODISCARD static inline           GPUInt128 operator /(GPUInt128 a, GPUInt128 b) { return gp_uint128_div(a, b); }
GP_NODISCARD static inline           GPInt128  operator /(GPInt128  a, GPInt128  b) { return gp_int128_div(a, b) ; }
GP_NODISCARD static inline           GPUInt128 operator %(GPUInt128 a, GPUInt128 b) { return gp_uint128_mod(a, b); }
GP_NODISCARD static inline           GPInt128  operator %(GPInt128  a, GPInt128  b) { return gp_int128_mod(a, b) ; }
GP_NODISCARD static inline constexpr GPUInt128 operator -(GPUInt128 a) { return gp_uint128_negate(a); }
GP_NODISCARD static inline constexpr GPInt128  operator -(GPInt128  a) { return gp_int128_negate(a);  }

// Bitwise operators
GP_NODISCARD static inline constexpr GPUInt128 operator ~(GPUInt128  a) { return gp_uint128_not(a); }
GP_NODISCARD static inline constexpr GPInt128  operator ~(GPInt128   a) { return gp_int128_not(a) ; }
GP_NODISCARD static inline constexpr GPUInt128 operator &(GPUInt128  a, GPUInt128 b) { return gp_uint128_and(a, b); }
GP_NODISCARD static inline constexpr GPInt128  operator &(GPInt128   a, GPInt128  b) { return gp_int128_and(a, b) ; }
GP_NODISCARD static inline constexpr GPUInt128 operator |(GPUInt128  a, GPUInt128 b) { return gp_uint128_or(a, b) ; }
GP_NODISCARD static inline constexpr GPInt128  operator |(GPInt128   a, GPInt128  b) { return gp_int128_or(a, b)  ; }
GP_NODISCARD static inline constexpr GPUInt128 operator ^(GPUInt128  a, GPUInt128 b) { return gp_uint128_xor(a, b); }
GP_NODISCARD static inline constexpr GPInt128  operator ^(GPInt128   a, GPInt128  b) { return gp_int128_xor(a, b) ; }
GP_NODISCARD static inline constexpr GPUInt128 operator <<(GPUInt128 a, uint8_t   b) { return gp_uint128_shift_left(a, b) ; }
GP_NODISCARD static inline constexpr GPInt128  operator <<(GPInt128  a, uint8_t   b) { return gp_int128_shift_left(a, b)  ; }
GP_NODISCARD static inline constexpr GPUInt128 operator >>(GPUInt128 a, uint8_t   b) { return gp_uint128_shift_right(a, b); }
GP_NODISCARD static inline constexpr GPInt128  operator >>(GPInt128  a, uint8_t   b) { return gp_int128_shift_right(a, b) ; }

// Assignments
static inline constexpr GPUInt128 operator +=(GPUInt128&  a, GPUInt128 b) { return a = a + b ; }
static inline constexpr GPInt128  operator +=(GPInt128&   a, GPInt128  b) { return a = a + b ; }
static inline constexpr GPUInt128 operator -=(GPUInt128&  a, GPUInt128 b) { return a = a - b ; }
static inline constexpr GPInt128  operator -=(GPInt128&   a, GPInt128  b) { return a = a - b ; }
static inline           GPUInt128 operator *=(GPUInt128&  a, GPUInt128 b) { return a = a * b ; }
static inline           GPInt128  operator *=(GPInt128&   a, GPInt128  b) { return a = a * b ; }
static inline           GPUInt128 operator /=(GPUInt128&  a, GPUInt128 b) { return a = a / b ; }
static inline           GPInt128  operator /=(GPInt128&   a, GPInt128  b) { return a = a / b ; }
static inline           GPUInt128 operator %=(GPUInt128&  a, GPUInt128 b) { return a = a % b ; }
static inline           GPInt128  operator %=(GPInt128&   a, GPInt128  b) { return a = a % b ; }
static inline constexpr GPUInt128 operator &=(GPUInt128&  a, GPUInt128 b) { return a = a & b ; }
static inline constexpr GPInt128  operator &=(GPInt128&   a, GPInt128  b) { return a = a & b ; }
static inline constexpr GPUInt128 operator |=(GPUInt128&  a, GPUInt128 b) { return a = a | b ; }
static inline constexpr GPInt128  operator |=(GPInt128&   a, GPInt128  b) { return a = a | b ; }
static inline constexpr GPUInt128 operator ^=(GPUInt128&  a, GPUInt128 b) { return a = a ^ b ; }
static inline constexpr GPInt128  operator ^=(GPInt128&   a, GPInt128  b) { return a = a ^ b ; }
static inline constexpr GPUInt128 operator <<=(GPUInt128& a, uint8_t   b) { return a = a << b; }
static inline constexpr GPInt128  operator <<=(GPInt128&  a, uint8_t   b) { return a = a << b; }
static inline constexpr GPUInt128 operator >>=(GPUInt128& a, uint8_t   b) { return a = a >> b; }
static inline constexpr GPInt128  operator >>=(GPInt128&  a, uint8_t   b) { return a = a >> b; }

// Increment/decrement
static inline constexpr GPUInt128& operator ++(GPUInt128& a) { return a = a + gp_u128(1); }
static inline constexpr GPInt128&  operator ++(GPInt128&  a) { return a = a + gp_i128(1); }
static inline constexpr GPUInt128& operator --(GPUInt128& a) { return a = a - gp_u128(1); }
static inline constexpr GPInt128&  operator --(GPInt128&  a) { return a = a - gp_i128(1); }
static inline constexpr GPUInt128  operator ++(GPUInt128& a, int _) { (void)_; return ++a - gp_u128(1); }
static inline constexpr GPInt128   operator ++(GPInt128&  a, int _) { (void)_; return ++a - gp_i128(1); }
static inline constexpr GPUInt128  operator --(GPUInt128& a, int _) { (void)_; return --a + gp_u128(1); }
static inline constexpr GPInt128   operator --(GPInt128&  a, int _) { (void)_; return --a + gp_i128(1); }

// Comparisons
GP_NODISCARD static inline constexpr bool operator ==(GPUInt128 a, GPUInt128 b) { return gp_uint128_equal(a, b)             ; }
GP_NODISCARD static inline constexpr bool operator ==(GPInt128  a, GPInt128  b) { return gp_int128_equal(a, b)              ; }
GP_NODISCARD static inline constexpr bool operator !=(GPUInt128 a, GPUInt128 b) { return gp_uint128_not_equal(a, b)         ; }
GP_NODISCARD static inline constexpr bool operator !=(GPInt128  a, GPInt128  b) { return gp_int128_not_equal(a, b)          ; }
GP_NODISCARD static inline constexpr bool operator  <(GPUInt128 a, GPUInt128 b) { return gp_uint128_less_than(a, b)         ; }
GP_NODISCARD static inline constexpr bool operator  <(GPInt128  a, GPInt128  b) { return gp_int128_less_than(a, b)          ; }
GP_NODISCARD static inline constexpr bool operator <=(GPUInt128 a, GPUInt128 b) { return gp_uint128_less_than_equal(a, b)   ; }
GP_NODISCARD static inline constexpr bool operator <=(GPInt128  a, GPInt128  b) { return gp_int128_less_than_equal(a, b)    ; }
GP_NODISCARD static inline constexpr bool operator  >(GPUInt128 a, GPUInt128 b) { return gp_uint128_greater_than(a, b)      ; }
GP_NODISCARD static inline constexpr bool operator  >(GPInt128  a, GPInt128  b) { return gp_int128_greater_than(a, b)       ; }
GP_NODISCARD static inline constexpr bool operator >=(GPUInt128 a, GPUInt128 b) { return gp_uint128_greater_than_equal(a, b); }
GP_NODISCARD static inline constexpr bool operator >=(GPInt128  a, GPInt128  b) { return gp_int128_greater_than_equal(a, b) ; }
#endif // __cplusplus // operator overloads

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
/// @cond

#ifdef __cplusplus
GP_NODISCARD static inline constexpr GPInt128  GP_AS_SIGNED(GPUInt128 x) { return gp_int128_u128(x); }
GP_NODISCARD static inline constexpr GPUInt128 gp_min(GPUInt128 a, GPUInt128 b) { return a < b ? a : b; }
GP_NODISCARD static inline constexpr GPUInt128 gp_max(GPUInt128 a, GPUInt128 b) { return a > b ? a : b; }
GP_NODISCARD static inline constexpr GPInt128  gp_min(GPInt128  a, GPInt128  b) { return a < b ? a : b; }
GP_NODISCARD static inline constexpr GPInt128  gp_max(GPInt128  a, GPInt128  b) { return a > b ? a : b; }
GP_NODISCARD static inline constexpr gp_type_t gp_type(GPInt128  x) { (void)x; return GP_TYPE_INT128 ; }
GP_NODISCARD static inline constexpr gp_type_t gp_type(GPUInt128 x) { (void)x; return GP_TYPE_UINT128; }
#endif

#if defined(GP_USE_TETRA_INT) || defined(GP_TEST_INT128)
GP_NODISCARD static inline   gp_tetra_uint_t gp_mintu(gp_tetra_uint_t a, gp_tetra_uint_t b) { return a < b ? a : b; }
GP_NODISCARD static inline   gp_tetra_uint_t gp_maxtu(gp_tetra_uint_t a, gp_tetra_uint_t b) { return a > b ? a : b; }
GP_NODISCARD static inline   gp_tetra_int_t  gp_minti(gp_tetra_int_t a,  gp_tetra_int_t b)  { return a < b ? a : b; }
GP_NODISCARD static inline   gp_tetra_int_t  gp_maxti(gp_tetra_int_t a,  gp_tetra_int_t b)  { return a > b ? a : b; }
#endif

#ifdef GP_INT128_SELECTION
#  undef GP_INT128_SELECTION
#  undef GP_UINT128_SELECTION
#endif
#define GP_INT128_SELECTION(...)  GPInt128:  __VA_ARGS__
#define GP_UINT128_SELECTION(...) GPUInt128: __VA_ARGS__
#if defined(GP_USE_TETRA_INT) || defined(GP_TEST_INT128)
#  ifdef GP_TETRA_INT_SELECTION
#    undef GP_TETRA_INT_SELECTION
#    undef GP_TETRA_UINT_SELECTION
#  endif
#  define GP_TETRA_INT_SELECTION(...)  gp_tetra_int_t:  __VA_ARGS__
#  define GP_TETRA_UINT_SELECTION(...) gp_tetra_uint_t: __VA_ARGS__
#endif

#ifdef GP_HAS_C11_GENERIC
GP_NODISCARD static inline GPUInt128 gp_uint128_uint128(GPUInt128 u) { return u; }
GP_NODISCARD static inline GPInt128  gp_int128_int128(GPInt128 i) { return i;    }
#  if defined(GP_USE_TETRA_INT) || defined(GP_TEST_INT128) // use implicit integer conversions
#    define GP_U128_CTOR(A) _Generic(A, GPUInt128: gp_uint128_uint128, GPInt128: gp_uint128_i128, default: gp_uint128_tetra_uint)(A)
#    define GP_I128_CTOR(A) _Generic(A, GPUInt128: gp_int128_u128, GPInt128: gp_int128_int128, default: gp_int128_tetra_int)(A)
#  else
#    define GP_U128_CTOR(A) _Generic(A, \
         GP_C11_GENERIC_SIGNED_INTEGER(gp_uint128_i64), \
         GP_C11_GENERIC_UNSIGNED_INTEGER(gp_uint128_u64), \
         GP_C11_GENERIC_FLOAT(gp_uint128_f64), \
         GPUInt128: gp_uint128_uint128, GPInt128: gp_uint128_i128)(A)
#    define GP_I128_CTOR(A) _Generic(A, \
         GP_C11_GENERIC_SIGNED_INTEGER(gp_int128_i64), \
         GP_C11_GENERIC_UNSIGNED_INTEGER(gp_int128_u64), \
         GP_C11_GENERIC_FLOAT(gp_iint128_f64), \
         GPUInt128: gp_int128_u128, GPInt128: gp_int128_int128)(A)
#  endif
#endif

/// @endcond
#endif // GP_INT128_INCLUDED
