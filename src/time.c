// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/time.h>
#include <gpc/thread.h>
#include <time.h>

#if !_WIN32
int clock_gettime(clockid_t clockid, struct timespec *tp);
int nanosleep(const struct timespec *duration, struct timespec * rem);
#  ifndef CLOCK_REALTIME
#    define CLOCK_REALTIME 0
#  endif
#endif

GPUInt128 gp_global_time;

static void gp_s_init_global_time(void)
{
    gp_global_time = gp_time_begin();
}

GPUInt128 gp_internal_time(void)
{
    static GPThreadOnce init_time_once = GP_THREAD_ONCE_INIT;
    gp_thread_once(&init_time_once, gp_s_init_global_time);
    return gp_global_time;
}

GPUInt128 gp_time_begin(void)
{
    struct timespec ts;
    #if __STDC_VERSION__ >= 201112L
    timespec_get(&ts, TIME_UTC);
    #else
    clock_gettime(CLOCK_REALTIME, &ts);
    #endif
    return gp_uint128_add(
        gp_uint128_mul64(1000000000llu, ts.tv_sec),
        gp_uint128(0, ts.tv_nsec));
}

static int gp_sleep_ts(
    const struct timespec* duration,
    struct timespec*       remaining)
{
    #if __STDC_VERSION__ >= 201112L
    return thrd_sleep(duration, remaining);
    #else
    return nanosleep(duration, remaining);
    #endif
}

int gp_sleep(double t)
{
    struct timespec ts;
    ts.tv_sec  = t;
    ts.tv_nsec = 1000000000.* (t - ts.tv_sec);
    return gp_sleep_ts(&ts, NULL);
}
