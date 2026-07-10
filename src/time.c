// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/time.h>
#include <gpc/thread.h>

#ifndef GP_TARGET_WINDOWS
int clock_gettime(clockid_t clockid, struct timespec *tp);
#  ifndef CLOCK_REALTIME
#    define CLOCK_REALTIME 0
#  endif
#endif

GPUInt128 gp_global_time;

static void gp_s_init_global_time(void)
{
    gp_global_time = gp_time_begin();
}

GPUInt128 gp_time_init(void)
{
    static GPOnce init_time_once = GP_ONCE_INITIALIZER;
    gp_call_once(&init_time_once, gp_s_init_global_time);
    return gp_global_time;
}

GPUInt128 gp_time_begin(void)
{
    #ifdef GP_HAS_TIMESPEC
    struct timespec ts;
    #  if __STDC_VERSION__ >= 201112L
    timespec_get(&ts, TIME_UTC);
    #  else
    clock_gettime(CLOCK_REALTIME, &ts);
    #  endif
    return gp_uint128_add(
        gp_uint128_mul64(1000000000llu, ts.tv_sec),
        gp_uint128(0, ts.tv_nsec));
    #else // no timespec, win32?
    // TODO
    #endif
}
