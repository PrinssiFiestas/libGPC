// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/time.h>
#include <gpc/thread.h> // GPOnce
#include <gpc/utils.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#  include <threads.h> // thrd_sleep()
#  define GP_USE_C11_THREAD_SLEEP 1
#elif defined(GP_TARGET_OS_WINDOWS)
#  include <windows.h>
#  define GP_USE_WIN_SLEEP 1
struct gp_internal_timespec
{
    int64_t tv_sec;
    long tv_nsec;
};
#else
#  define GP_USE_POSIX_SLEEP 1
#endif

#ifdef GP_USE_WIN_SLEEP // win helpers
static int64_t _c11threads_win32_util_timespec64_to_file_time(
    const struct gp_internal_timespec *ts, size_t *periods)
{
    uint64_t sec_res;
    uint64_t nsec_res;
    uint64_t res;

    *periods = (unsigned long)((uint64_t)ts->tv_sec / (uint64_t)922337203685);
    sec_res = ((uint64_t)ts->tv_sec % (uint64_t)922337203685) * (uint64_t)10000000;

    /* Add another 100 ns if division yields remainder. */
    nsec_res = (unsigned long)ts->tv_nsec / 100UL + !!((unsigned long)ts->tv_nsec % 100UL);

    /* 64-bit time_t may cause overflow. */
    if (nsec_res > (uint64_t) - 1 - sec_res) {
        ++*periods;
        nsec_res -= (uint64_t) - 1 - sec_res;
        sec_res = 0;
    }

    res = sec_res + nsec_res;

    if (*periods && !res) {
        --*periods;
        return (int64_t)9223372036850000000;
    }

    return res;
}

static int _c11threads_win32_sleep_common(int64_t file_time_in)
{
    void *timer;
    unsigned long error;
    LARGE_INTEGER due_time;

    assert(file_time_in >= 0);

    timer = CreateWaitableTimerW(NULL, 1, NULL);
    if (!timer) {
        error = GetLastError();
        return error > 1 ? -(long)error : -ERROR_INTERNAL_ERROR;
    }

    due_time.QuadPart = -file_time_in;
    if (!SetWaitableTimer(timer, &due_time, 0, NULL, NULL, 0)) {
        error = GetLastError();
        CloseHandle(timer);
        return error > 1 ? -(long)error : -ERROR_INTERNAL_ERROR;
    }

    if (WaitForSingleObject(timer, INFINITE) != WAIT_OBJECT_0) {
        error = GetLastError();
        CloseHandle(timer);
        return error > 1 ? -(long)error : -ERROR_INTERNAL_ERROR;
    }

    CloseHandle(timer);
    return 0; /* Success. */
}

static int _c11threads_win32_thrd_sleep64(
    const struct gp_internal_timespec *ts_in, struct gp_internal_timespec *rem_out)
{
    int64_t file_time;
    size_t periods;
    int res;

    (void)rem_out; // Win32 Sleep() and waitable objects are not interruptible.

    file_time = _c11threads_win32_util_timespec64_to_file_time(ts_in, &periods);
    if (file_time < 0) {
        return -ERROR_INVALID_PARAMETER;
    }

restart_sleep:
    res = _c11threads_win32_sleep_common(file_time);

    if (!res && periods) {
        --periods;
        file_time = (int64_t)9223372036850000000;
        goto restart_sleep;
    }

    return res;
}

#  ifndef _UCRT
void gp_internal_timespec_get(struct gp_internal_timespec *ts)
{
    FILETIME file_time;
    ULARGE_INTEGER li;

    GetSystemTimeAsFileTime(&file_time);

    li.LowPart = file_time.dwLowDateTime;
    li.HighPart = file_time.dwHighDateTime;

    /* Also subtract difference between FILETIME and UNIX time epoch. It's 369 years by the way. */
    ts->tv_sec = li.QuadPart / (uint64_t)10000000 - (uint64_t)11644473600;
    ts->tv_nsec = (long)(li.QuadPart % (uint64_t)10000000) * 100;
}
#  endif
#endif // win helpers

static GPInt128 gp_s_time;

static void gp_s_init_global_time(void)
{
    gp_s_time = gp_time_begin();
}

GPInt128 gp_time_init(void)
{
    static GPOnce init_time_once = GP_ONCE_INITIALIZER;
    gp_call_once(&init_time_once, gp_s_init_global_time);
    return gp_s_time;
}

GPInt128 gp_time_begin(void)
{
    #if __STDC_VERSION__ >= 201112L || defined(GP_USE_POSIX_SLEEP)
    struct timespec ts;
    #elif defined(_UCRT)
    struct _timespec64 ts;
    #else
    struct gp_internal_timespec ts;
    #endif

    #if __STDC_VERSION__ >= 201112L
    timespec_get(&ts, TIME_UTC);
    #elif defined(_UCRT)
    _timespec64_get(&ts, TIME_UTC);
    #elif defined(GP_USE_WIN_SLEEP)
    gp_internal_timespec_get(&ts);
    #else
    clock_gettime(CLOCK_REALTIME, &ts);
    #endif

    return gp_int128_add(
        gp_int128_mul64(1000000000, ts.tv_sec),
        gp_int128(0, ts.tv_nsec));
}

void gp_sleep(double seconds)
{
    gp_assume( ! isnan(seconds));
    if (seconds < 0.)
        return;

    #if defined(GP_USE_C11_THREAD_SLEEP) || defined(GP_USE_POSIX_SLEEP)
    struct timespec ts;
    #else
    struct gp_internal_timespec ts;
    #endif

    ts.tv_nsec = 1000000000*modf(seconds, &seconds);
    if (sizeof(ts.tv_sec) == sizeof(int32_t))
        ts.tv_sec = fmin(seconds, INT32_MAX);
    else
        ts.tv_sec = fmin(seconds, INT64_MAX);

    errno_t old_errno = errno;
    bool interrupted;
    do {
        #if defined(GP_USE_C11_THREAD_SLEEP)
        interrupted = thrd_sleep(&ts, &ts);
        #elif defined(GP_TARGET_OS_WINDOWS)
        interrupted = _c11threads_win32_thrd_sleep64(&ts, &ts);
        #else
        interrupted = nanosleep(&ts, &ts);
        #endif
        if (interrupted) {
            gp_assume(errno == EINTR);
            if (isinf(seconds)) // we promised indefinite sleep in docs, there you go
                ts.tv_sec = INT32_MAX;
        }
    } while (interrupted || isinf(seconds));
    errno = old_errno;
}

void gp_sleep_ns(int64_t nanoseconds)
{
    if (nanoseconds < 0)
        return;

    #if defined(GP_USE_C11_THREAD_SLEEP) || defined(GP_USE_POSIX_SLEEP)
    struct timespec ts;
    #else
    struct gp_internal_timespec ts;
    #endif

    ts.tv_nsec = nanoseconds % 1000000000;
    if (sizeof(ts.tv_sec) == sizeof(int32_t))
        ts.tv_sec = gp_signed_min(nanoseconds / 1000000000, INT32_MAX);
    else
        ts.tv_sec = nanoseconds / 1000000000;

    errno_t old_errno = errno;
    bool interrupted;
    do {
        #if defined(GP_USE_C11_THREAD_SLEEP)
        interrupted = thrd_sleep(&ts, &ts);
        #elif defined(GP_TARGET_OS_WINDOWS)
        interrupted = _c11threads_win32_thrd_sleep64(&ts, &ts);
        #else
        interrupted = nanosleep(&ts, &ts);
        #endif
        if (interrupted) {
            gp_assume(errno == EINTR);
        }
    } while (interrupted);
    errno = old_errno;
}
