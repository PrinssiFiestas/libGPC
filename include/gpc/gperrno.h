// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_ERRNO_INCLUDED
#define GP_ERRNO_INCLUDED 1

#include <gpc/gpstring.h>
#include <errno.h> // IWYU pragma: keep // "unused include"
#include <stdlib.h> // _doserrno

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup errno System Error Codes and `errno`
 *
 * ```c
 * #include <gpc/gperrno.h>
 * ```
 *
 * This is a minimal interface for working with `errno` and Windows `_doserrno`
 * (or `GetLastError()`) in a portable manner. The goal is to have an unified
 * API for working with `errno` and Windows system error codes without requiring
 * dedicated error handling interface that may require platform specific code
 * and that is incompatible with existing code.
 *
 * We achieve this by simply using `errno` and `_doserrno` and providing helpers
 * to work with both of them at once. This way user can simply ignore this API
 * completely if they don't care about `_doserrno`, they can simply always use
 * `errno` for portable code. However, more fine grained error handling (most
 * notably error code to string conversion) is available in Windows using this
 * interface.
 *
 * We don't think that static thread local error values is the best way to do
 * error handling, which is one of the reasons why this library somewhat rarely
 * uses these, but we also don't want to introduce a dedicated system for error
 * handling on syscall, C runtime, or WinAPI failures. We mostly do not
 * explicitly set `errno` or `_doserrno` ourselves, we simply keep them intact
 * when calling libc functions. We usually set these in Windows if a function in
 * it's POSIX implementation uses a POSIX specific function that may set `errno`
 * and the Windows implementation uses WinAPI.
 *
 * WinAPI has it's own error value that can be obtained using `GetLastError()`
 * from WinAPI. You should not assume that any of our functions keep that error
 * value intact, any call to our functions may corrupt it without us documenting
 * it. We instead internally set `_doserrno` to the return value of
 * `GetLastError()` and translate and set it to `errno` if the POSIX
 * implementation sets `errno`.
 *
 * We document when any function may set `errno` and `_doserrno`,
 * although in our documentation we simply refer to both values as `errno` for
 * brevity.
 * @{
 */

/** Stores both standard `errno` and system error code (`_doserrno`). */
typedef struct GPErrno
{
    /** Used to store standard `errno` code. */
    int errnocode;

    /** Used to store system error code.
     *
     * In Windows this stores `_dosserno`. In POSIX this is zero.
     */
    uint32_t doscode;
} GPErrno;

/** Set both `errno` and `_doserrno` to zero.
 *
 * @return old values of `errno` and `_doserrno` stored in @ref GPErrno structure.
 */
GP_INLINE
GPErrno gp_errno_clear(void)
{
    GPErrno errs = { errno, 0 };
    errno = 0;
    #ifdef GP_TARGET_OS_WINDOWS
    errs.doscode = _doserrno;
    _doserrno = 0;
    #endif
    return errs;
}

/** Set both `errno` and `_doserrno`.
 *
 * If `optional_errno` is `NULL`, then `errno` and `_doserrno` will not be set.
 *
 * @return old values of `errno` and `_doserrno` stored in @ref GPErrno structure.
 */
GP_INLINE
GPErrno gp_errno_set(const GPErrno* optional_errno)
{
    GPErrno errs = { errno, 0 };
    if (optional_errno != NULL)
        errno = optional_errno->errnocode;
    #ifdef GP_TARGET_OS_WINDOWS
    errs.doscode = _doserrno;
    if (optional_errno != NULL)
        _doserrno = optional_errno->doscode;
    #endif
    return errs;
}

/** Convert `errno` or system error code (`_doserrno`) to string.
 *
 * If @a optional_dest is not `NULL`, then the result will be stored in it.
 * If @a optional_dest is pointing to a fixed-capacity string and the result
 * does not fit in it (or dynamic string reallocation failed), then the
 * resulting string will be truncated.
 *
 * If @a optional_errno is `NULL`, then current values `errno` and `_doserrno`
 * are used as the input.
 *
 * `_doserrno` or @a optional_errno.doscode is checked first for potentially more
 * precise error message. If it contains no error, then `errno` or @a optional_errno.errnocode
 * is checked instead.
 *
 * Pointers returned by this function or by `strerror()` are invalidated by
 * subsequent calls to either function.
 *
 * @return pointer to an immutable thread local non-null C string that contains
 * the full message even if it got truncated to @a optional_dest.
 */
GP_NONNULL_RETURN GP_API
const char* gp_errno_str(GPString* optional_dest, const GPErrno* optional_errno);

/** Convert Win32 system error code to an `errno` value.
 *
 * This function is only available in Windows.
 *
 * Translates the return value of `GetLastError()` or the value of `_doserrno`
 * to an `errno` value. At the time of writing, only the first 500 of Windows
 * system error codes are translated. Any unknown error code returns -1. Zero
 * returns zero.
 */
#if defined(GP_TARGET_OS_WINDOWS) || defined(GP_DOXYGEN)
GP_API
int gp_errno_from_win32_error(uint32_t);
#endif

/// @}
#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_ERRNO_INCLUDED
