// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file time.h
 * Timing utilities for portability and convenience.
 */

#ifndef GP_TIME_INCLUDED
#define GP_TIME_INCLUDED 1

#include <gpc/int128.h>

#if __cplusplus
extern "C" {
#endif


/** Time since epoch in nanoseconds. */
GPUInt128 gp_time_begin(void);

/** Time in nanoseconds.
 * @return time since the first call with NULL parameter to this or
 * @ref gp_time() or optional reference pointed by @p optional_start_ns, which
 * must point to a value returned by @ref gp_time_begin_ns() if not NULL.
 */
GP_GNU_ATTRIB(always_inline)
static inline uint64_t gp_time_ns(const GPUInt128* optional_start_ns)
{
    GPUInt128 gp_internal_time();
    GPUInt128 start = optional_start_ns != NULL ? *optional_start_ns : gp_internal_time();
    return gp_uint128_lo(gp_uint128_sub(gp_time_begin(), start));
}

/** Time in seconds.
 * @return time since the first call with NULL parameter to this or
 * @ref gp_time_ns() or optional reference pointed by @p optional_start_ns,
 * which must point to a value returned by @ref gp_time_begin_ns() if not NULL.
 */
GP_GNU_ATTRIB(always_inline)
static inline double gp_time(const GPUInt128* optional_start_ns)
{
    return (double)gp_time_ns(optional_start_ns) / 1000000000.;
}

/** Sleep specified number of seconds.
 * The sleep may resume earlier if signal that is not ignored is received. The
 * actual sleep time may be longer than requested because it is rounded up to
 * the timer granularity and because of scheduling and context switching
 * overhead. Actual time is not very precise.
 * @return 0 on successful sleep, -1 if a signal occurred, other negative value
 * if an error occurred.
 */
int gp_sleep(double seconds);

#if __cplusplus
} // extern "C"
#endif

#endif // GP_TIME_INCLUDED
