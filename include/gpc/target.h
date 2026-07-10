// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TARGET_INCLUDED
#define GP_TARGET_INCLUDED 1

/// @defgroup target Build Target Detection
/// @code
/// #include <gpc/target.h>
/// @endcode
/// Macros to portably detect target build, OS, or CPU architecture.
/// @{

// ----------------------------------------------------------------------------
// Build Configuration

#ifdef GP_DOXYGEN
/// @addtogroup compile_options Build Configuration
/// Determine if debug or release build. These are defined automatically based
/// on `_DEBUG` and `NDEBUG` macros. The former is defined by default by
/// Visual Studio on debug builds, the latter is recognized by the C standard,
/// although it should not be assumed to be defined by any tooling that doesn't
/// document so.
/// @{
/** Defined if debug build (default).
 *
 * Defined if `_DEBUG` (defined by Visual Studio) is defined or `NDEBUG` is not
 * defined. Can be forced per translation unit by defining this manually, which
 * most notably makes @ref gp_assume() assertions fatal.
 */
#  define GP_TARGET_DEBUG 1
/** Defined if release build.
 *
 * Defined if `GP_TARGET_DEBUG` is not defined. Can be forced per translation
 * unit by defining this manually, which most notably makes @ref gp_assume()
 * assertions undefined behavior.
 */
#  define GP_TARGET_RELEASE 1
/// @}
#else
#  if !defined(GP_TARGET_DEBUG) && !defined(GP_TARGET_RELEASE)
#    ifdef _DEBUG
#      define GP_TARGET_DEBUG 1
#    elif !defined(NDEBUG)
#      define GP_TARGET_DEBUG 1
#    else
#      define GP_TARGET_RELEASE 1
#    endif
#  endif
#endif

// ----------------------------------------------------------------------------
// OS
/// @defgroup target_os Target OS
/// Operating systems recognized by this library. Only one `GP_TARGET_OS_*`
/// macro can be defined at a a time, but @ref GP_TARGET_POSIX may be defined
/// as well.
/// @{
#ifdef GP_DOXYGEN
#  define GP_TARGET_OS_WINDOWS 1 ///< Defined if OS is Windows.
#  define GP_TARGET_OS_LINUX   1 ///< Defined if OS is Linux.
#  define GP_TARGET_OS_MAC     1 ///< Defined if OS is MacOS.
#  define GP_TARGET_OS_BSD     1 ///< Defined if OS is any BSD variant.
#  define GP_TARGET_POSIX      1 ///< Defined if OS is POSIX compliant.
#else
#  if defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
#    define GP_TARGET_OS_WINDOWS 1
#  elif defined(__gnu_linux__) || defined(__linux__)
#    define GP_TARGET_OS_LINUX 1
#  elif defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#    define GP_TARGET_OS_MAC 1
#  elif defined(__bsdi__)  \
     || defined(__NetBSD__) \
     || defined(__FreeBSD__) \
     || defined(__OpenBSD__)  \
     || defined(_SYSTYPE_BSD)  \
     || defined(__DragonFly__)  \
     || defined(__MidnightBSD__) \
#  define GP_TARGET_OS_BSD 1
#  endif

#  if defined(__unix__)        \
   || defined(GP_TARGET_OS_MAC) \
   || defined(GP_TARGET_OS_BSD)  \
   || defined(GP_TARGET_OS_LINUX)
#    define GP_TARGET_POSIX 1
#  elif defined(__has_include)
#    if __has_include(<unistd.h>)
#      #define GP_TARGET_POSIX 1
#    endif
#  endif
#endif
/// @}

// ----------------------------------------------------------------------------
// Architecture
/// @defgroup target_arch Target CPU Architecture
/// CPU architectures recognized by this library. Only one `GP_TARGET_ARCH_*`
/// macro can be defined at a time.
/// @{
#ifdef GP_DOXYGEN
#  define GP_TARGET_ARCH_X86_64 1 ///< Defined if CPU is AMD64/x86_64/x64.
#  define GP_TARGET_ARCH_X86    1 ///< Defined if CPU is 32-bit X86.
#  define GP_TARGET_ARCH_ARM64  1 ///< Defined if CPU is 64-bit ARM.
#  define GP_TARGET_ARCH_ARM32  1 ///< Defined if CPU is 32-bit ARM.
#else
#  if defined(_M_X64) \
   || defined(__amd64) \
   || defined(_M_AMD64) \
   || defined(__x86_64)  \
   || defined(__amd64__)  \
   || defined(__x86_64__)
#     define GP_TARGET_ARCH_X86_64 1
#  elif defined(_M_ARM64) \
     || defined(__aarch64__)
#    define GP_TARGET_ARCH_ARM64 1
#  elif defined(i386) \
     || defined(_X86_) \
     || defined(__386)  \
     || defined(__i386)  \
     || defined(__X86__)  \
     || defined(__I86__)   \
     || defined(_M_IX86)    \
     || defined(__i386__)    \
     || defined(__IA32__)     \
     || defined(__INTEL__)     \
     || defined(__THW_INTEL__)
#    define GP_TARGET_ARCH_X86 1
#  elif defined(_ARM) \
     || defined(__arm) \
     || defined(_M_ARM) \
     || defined(_M_ARMT) \
     || defined(__arm__)  \
     || defined(__thumb__) \
     || defined(__TARGET_ARCH_ARM) \
     || defined(__TARGET_ARCH_THUMB)
#    define GP_TARGET_ARCH_ARM32 1
#  endif
#endif
/// @}

/// @}
#endif // GP_TARGET_INCLUDED
