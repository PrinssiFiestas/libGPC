// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TIME_INCLUDED
#define GP_TIME_INCLUDED 1

#include <gpc/int128.h>
#include <gpc/target.h>

#if !defined(GP_HAS_TIMESPEC) && \
    (__STDC_VERSION__ >= 201112L || defined(__unix__) || GP_HAS_INCLUDE(<unistd.h>) == 1)
#include <time.h>
#define GP_HAS_TIMESPEC 1
#endif

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
GP_NODISCARD
GPUInt128 gp_time_begin(void);

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
GPUInt128 gp_time_init(void);

/** Time in nanoseconds.
 *
 * @return time since the reference start @a optional_start_ns, which must be
 * the return value of @ref gp_time_begin() or `NULL`. If reference start is
 * `NULL`, then the reference start time will be the time that this function or
 * @ref gp_time() or @ref gp_time_init() was first called.
 */
GP_GNU_ATTRIB(always_inline) GP_NODISCARD GP_INLINE
uint64_t gp_time_ns(const GPUInt128* optional_start_ns)
{
    GPUInt128 start = optional_start_ns != NULL ? *optional_start_ns : gp_time_init();
    return gp_uint128_lo(gp_uint128_sub(gp_time_begin(), start));
}

/** Time in seconds.
 *
 * @return time since the reference start @a optional_start_ns, which must be
 * the return value of @ref gp_time_begin() or `NULL`. If reference start is
 * `NULL`, then the reference start time will be the time that this function or
 * @ref gp_time_ns() or @ref gp_time_init() was first called.
 */
GP_GNU_ATTRIB(always_inline) GP_NODISCARD GP_INLINE
double gp_time(const GPUInt128* optional_start_ns)
{
    return (double)gp_time_ns(optional_start_ns) / 1000000000.;
}

#ifdef GP_HAS_TIMESPEC
/// @defgroup timespec Timespec Conversions
/// Conversion functions from seconds to `struct timespec` and vice versa for
/// interfacing with C11/POSIX APIs.
/// @{

struct timespec gp_timespec_from_time(double seconds);
struct timespec gp_timespec_from_time_ns(GPUInt128 nanoseconds);
double gp_timespec_to_time(struct timespec);
GPUInt128 gp_timespec_to_time_ns(struct timespec);

/// @}
#endif

/// @}
#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_TIME_INCLUDED
