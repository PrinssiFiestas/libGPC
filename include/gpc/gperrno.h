// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_ERRNO_INCLUDED
#define GP_ERRNO_INCLUDED 1

#include <gpc/gpstring.h>
#include <errno.h> // IWYU pragma: keep // "unused include"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup errno System Error Codes and `errno`
 *
 * ```c
 * #include <gpc/gperrno.h>
 * ```
 *
 * @{
 */ // TODO motivation, detailed description, and usage.

/** Stores both standard `errno` and system error code (`_doserrno`). */
typedef struct GPErrno
{
    /** Used to store standard `errno` code. */
    int std;

    /** Used to store system error code.
     *
     * In Windows this stores `_dosserno`. In POSIX this is zero.
     */
    uint32_t dos;
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
    errs.dos = _doserrno;
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
        errno = optional_errno->std;
    #ifdef GP_TARGET_OS_WINDOWS
    errs.dos = _doserrno;
    if (optional_errno != NULL)
        _doserrno = optional_errno->dos;
    #endif
    return errs;
}

/** Convert `errno` or system error code (`_doserrno`) to string.
 *
 * TODO description
 */
GP_NONNULL_RETURN GP_API
char* gp_errno_str(GPString* optional_dest, const GPErrno* optional_errno);

/** Convert Win32 system error code to an `errno` value.
 *
 * This function is only available in Windows.
 *
 * Translates the return value of `GetLastError()` or the value of `_doserrno`
 * to an `errno` value. At the time of writing, only the first 500 of Windows
 * system error codes are translated. Any unknown error code returns -1.
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
