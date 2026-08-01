// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_COMMON_INCLUDED
#define GP_COMMON_INCLUDED 1

#ifdef __cplusplus
extern "C" {
#endif

#include <gpc/gputils.h>
#include <math.h>

#if __STDC_VERSION__ >= 201112L || defined(GP_TARGET_POSIX)
#  define GP_GOT_TIMESPEC 1
typedef struct timespec GPInternalTimespec;
#else
typedef struct GPInternalTimespec
{
    int64_t tv_sec;
    long tv_nsec;
} GPInternalTimespec;
#endif

// seconds must not be negative
static inline GPInternalTimespec gp_internal_timespec_from_time(double seconds)
{
    GPInternalTimespec ts;
    ts.tv_nsec = 1000000000*modf(seconds, &seconds);
    if (sizeof(ts.tv_sec) == sizeof(int32_t))
        ts.tv_sec = fmin(seconds, INT32_MAX);
    else
        ts.tv_sec = fmin(seconds, INT64_MAX);
    return ts;
}

// We know nanoseconds is not negative, use unsigned, which often leads to
// slightly better optimized integer modulus/division by constant due to rounding.
static inline GPInternalTimespec gp_internal_timespec_from_time_ns(uint64_t nanoseconds)
{
    GPInternalTimespec ts;
    ts.tv_nsec = nanoseconds % 1000000000;
    if (sizeof ts.tv_sec == sizeof(int32_t))
        ts.tv_sec = gp_signed_min(nanoseconds / 1000000000, INT32_MAX);
    else
        ts.tv_sec = nanoseconds / 1000000000;
    return ts;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_COMMON_INCLUDED
