// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file endian.h
 * System endianness
 */

#ifndef GP_ENDIAN_INCLUDED
#define GP_ENDIAN_INCLUDED 1
#define GP_ENDIAN_LITTLE 1
#define GP_ENDIAN_BIG    2

// Preprocessor endianness check from RapidJSON with added check for C23
// standard endianness macros. If detected, GP_ENDIAN is defined to
// GP_ENDIAN_LITTLE, GP_ENDIAN_BIG, or nothing in case of mixed endianness.
// Undetected endianness leaves GP_ENDIAN undefined. GP_ENDIAN can be user
// defined to GP_ENDIAN_LITTLE or GP_ENDIAN_BIG. Unlike RapidJSON, undetected
// endianness will not #error since endianness can still be detected at runtime
// with gp_is_big_endian() and gp_is_little_endian().
#ifndef GP_ENDIAN
// Detect with C23. stdbit.h is missing during time of writing even with
// -std=c23. We can still check the macro, but do NOT include the header, even
// if the header pops up to support older libc versions.
#  ifdef __STDC_ENDIAN_NATIVE__
#    if __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__
#      define GP_ENDIAN GP_ENDIAN_LITTLE
#    elif __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__
#      define GP_ENDIAN GP_ENDIAN_BIG
#    elif
#      define GP_ENDIAN // mixed
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
#      define GP_ENDIAN // mixed
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define GP_ENDIAN GP_ENDIAN_BIG
#  elif defined(_LITTLE_ENDIAN) && defined(_BIG_ENDIAN)
#    define GP_ENDIAN // mixed
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define GP_ENDIAN GP_ENDIAN_BIG
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define GP_ENDIAN GP_ENDIAN_LITTLE
#  endif
#endif // GP_ENDIAN

#if GP_ENDIAN == GP_ENDIAN_LITTLE
#  define gp_is_big_endian()    0
#  define gp_is_little_endian() 1
#elif GP_ENDIAN == GP_ENDIAN_BIG
#  define gp_is_big_endian()    1
#  define gp_is_little_endian() 0
#elif defined(GP_ENDIAN) // mixed endianness
#  define gp_is_big_endian()    0
#  define gp_is_little_endian() 0
#else
/** Run-time check if system is big endian.*/
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
bool gp_is_big_endian(void)
{
    union Endianness {
        uint16_t u16;
        struct { uint8_t is_little; uint8_t is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_big;
}
/** Run-time check if system is little endian.*/
GP_NODISCARD static inline GP_CONSTEXPR_FUNCTION
bool gp_is_little_endian(void)
{
    union Endianness {
        uint16_t u16;
        struct { uint8_t is_little; uint8_t is_big; } endianness;
    } integer;
    integer.u16 = 1;
    return integer.endianness.is_little;
}
#endif // GP_ENDIAN

#endif // GP_ENDIAN_INCLUDED
