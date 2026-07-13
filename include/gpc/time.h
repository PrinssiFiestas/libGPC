// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TIME_INCLUDED
#define GP_TIME_INCLUDED 1

#include <gpc/int128.h>
#include <gpc/assert.h>
#include <gpc/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup time Timing
/// @code
/// #include <gpc/time.h>
/// @endcode
/// Timing utilities.
/// @{

/** Time since epoch in nanoseconds.
 *
 * @return time since epoch (00:00:00 UTC on 1 January 1970), which is supposed
 * used as a reference start argument for @ref gp_time() and @ref gp_time_ns().
 */
GP_NODISCARD GPInt128 gp_time_begin(void);

/** Set global reference time.
 *
 * First call to this function or first call to @ref gp_time() or
 * @ref gp_time_ns() with `NULL` argument will set global reference time to the
 * time of call since epoch. This will be used as a reference time for
 * @ref gp_time() and @ref gp_time_ns() if no reference start time is passed.
 *
 * Subsequent calls will not change the global reference time.
 *
 * @return global reference time since epoch.
 */
GPInt128 gp_time_init(void);

/** Time in nanoseconds.
 *
 * @return time since the reference start @a optional_start_ns, which must be a
 * pointer to a value value returned by @ref gp_time_begin() or `NULL`. If
 * reference start is `NULL`, then the reference start time will be the time
 * that this function or @ref gp_time() or @ref gp_time_init() was first called.
 */
GP_GNU_ATTRIB(always_inline) GP_NODISCARD GP_INLINE
uint64_t gp_time_ns(const GPInt128* optional_start_ns)
{
    GPInt128 start = optional_start_ns != NULL ? *optional_start_ns : gp_time_init();
    return gp_int128_lo(gp_int128_sub(gp_time_begin(), start));
}

/** Time in seconds.
 *
 * @return time since the reference start @a optional_start_ns, which must be a
 * pointer to a value returned by @ref gp_time_begin() or `NULL`. If reference
 * start is `NULL`, then the reference start time will be the time that this
 * function or @ref gp_time_ns() or @ref gp_time_init() was first called.
 */
GP_GNU_ATTRIB(always_inline) GP_NODISCARD GP_INLINE
double gp_time(const GPInt128* optional_start_ns)
{
    return (double)gp_time_ns(optional_start_ns) / 1000000000.;
}

/** Sleep specified number of seconds.
 *
 * Sleep at least specified amount of time. The actual sleep time may be longer
 * than requested, because scheduling and context switching overhead. This also
 * means that the time is not very precise, portable applications should not
 * expect sub millisecond accuracy.
 *
 * It is not an error to pass non-zero negative time, in such case the function
 * returns immediately. It is also not an error to pass `INFINITY`, in such case
 * the function will sleep indefinitely. Passing `NAN` is undefined.
 *
 * Unlike POSIX counterparts, this sleep cannot be interrupted by a signal. This
 * design decision was made to increase portability since there is no way of
 * implementing interruptible sleep in Windows due to the fundamentally
 * different signal handling in Windows. This also means that user does not need
 * to call this in a `EINTR` loop, the function is guaranteed to sleep at least
 * the given time.
 */
void gp_sleep(double seconds);

/// @}
#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_TIME_INCLUDED
