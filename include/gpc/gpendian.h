// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_ENDIAN_INCLUDED
#define GP_ENDIAN_INCLUDED 1

#include <gpc/gpattributes.h>

/// @addtogroup target
/// @{

/// @defgroup endian Endianness
/// Machine byte order.
/// @{
#ifdef GP_DOXYGEN
/** Machine endianness.
 *
 * If endianness is detected, then this is defined to be equal to
 * @ref GP_ENDIAN_LITTLE, @ref GP_ENDIAN_BIG, or something else in case of mixed
 * endianness. If endianness cannot be detected, then this macro will not be
 * defined. In such case, user can define it globally themselves to @ref GP_ENDIAN_LITTLE
 * or @ref GP_ENDIAN_BIG if needed at compile time. Endianness can also be
 * checked during runtime using @ref gp_endian_is_big() and @ref gp_endian_is_little()
 * functions.
 */
#  define GP_ENDIAN /* implementation defined */

#  undef GP_ENDIAN
#  define GP_ENDIAN 0 // avoid preprocessing issues
#endif

#define GP_ENDIAN_LITTLE 1 ///< Value of @ref GP_ENDIAN if detected little endian machine.
#define GP_ENDIAN_BIG    2 ///< Value of @ref GP_ENDIAN if detected big endian machine.

// Preprocessor endianness check from RapidJSON with added check for C23
// standard endianness macros.
#ifndef GP_ENDIAN
// Detect with C23. stdbit.h is missing during time of writing even with
// -std=c23. We can still check the macro, but do NOT include the header, even
// if the header pops up to support older libc versions.
#  ifdef __STDC_ENDIAN_NATIVE__
#    if __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__
#      define GP_ENDIAN GP_ENDIAN_BIG
#    else
#      define GP_ENDIAN 0 // mixed
#    endif // __STDC_ENDIAN_NATIVE
// Detect with GCC 4.6's macro
#  elif defined(__BYTE_ORDER__)
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define GP_ENDIAN GP_ENDIAN_BIG
#    endif // __BYTE_ORDER__
// Detect with GLIBC's endian.h
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define GP_ENDIAN GP_ENDIAN_BIG
#    else
#      define GP_ENDIAN 0 // mixed
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define GP_ENDIAN GP_ENDIAN_BIG
#  elif defined(_LITTLE_ENDIAN) && defined(_BIG_ENDIAN)
#    define GP_ENDIAN 0 // mixed
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(__s390__)
#    define GP_ENDIAN GP_ENDIAN_BIG
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  endif
#endif // GP_ENDIAN

#if GP_ENDIAN == GP_ENDIAN_LITTLE
#  define gp_endian_is_big()    0
#  define gp_endian_is_little() 1
#elif GP_ENDIAN == GP_ENDIAN_BIG
#  define gp_endian_is_big()    1
#  define gp_endian_is_little() 0
#elif defined(GP_ENDIAN) && !defined(GP_DOXYGEN) // mixed endianness
#  define gp_endian_is_big()    0
#  define gp_endian_is_little() 0
#else

/** Run-time check if machine is big endian.
 *
 * If endianness is detected in the preprocessor, then this will be a macro that
 * expands to one if big endian, zero if little endian.
 */
GP_NODISCARD GP_ALWAYS_INLINE
bool gp_endian_is_big(void)
{
    union Endianness {
        short u16;
        struct { unsigned char is_little; unsigned char is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_big;
}

/** Run-time check if machine is little endian.
 *
 * If endianness is detected in the preprocessor, then this will be a macro that
 * expands to one if little endian, zero if big endian.
 */
GP_NODISCARD GP_ALWAYS_INLINE
bool gp_endian_is_little(void)
{
    union Endianness {
        short u16;
        struct { unsigned char is_little; unsigned char is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_little;
}
#endif // GP_ENDIAN

/// @}
/// @}
#endif // GP_ENDIAN_INCLUDED
