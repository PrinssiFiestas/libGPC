// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Based on https://github.com/jtsiomb/c11threads.
// Authors:
//   John Tsiombikas <nuclear@member.fsf.org> - original POSIX threads wrapper
//   Oliver Old <oliver.old@outlook.com> - win32 implementation

#include <gpc/thread.h>
#include "common.h"

/*
Win32 implementation for c11threads.

Authors:
John Tsiombikas <nuclear@member.fsf.org>
Oliver Old <oliver.old@outlook.com>

I place this piece of code in the public domain. Feel free to use as you see
fit. I'd appreciate it if you keep my name at the top of the code somewhere, but
whatever.

Main project site: https://github.com/jtsiomb/c11threads
*/

#if defined(GP_TARGET_OS_WINDOWS) && !defined(GP_USE_PTHREADS)

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <windows.h>

#ifndef TIME_UTC
#define TIME_UTC 1
#endif

/* ---- library ---- */

struct _c11threads_win32_timespec64_t {
    int64_t tv_sec;
    long tv_nsec;
};

struct _c11threads_win32_thrd_entry_t {
    struct _c11threads_win32_thrd_entry_t *next;
    void *h;
    GPThread thrd;
};

struct _c11threads_win32_tss_dtor_entry_t {
    struct _c11threads_win32_tss_dtor_entry_t *next;
    void(*dtor)(void*);
    tss_t key;
};

static SRWLOCK _c11threads_win32_thrd_list_srw_lock = SRWLOCK_INIT;
static struct _c11threads_win32_thrd_entry_t *_c11threads_win32_thrd_list_head = NULL;
static SRWLOCK _c11threads_win32_tss_dtor_list_srw_lock = SRWLOCK_INIT;
static struct _c11threads_win32_tss_dtor_entry_t *_c11threads_win32_tss_dtor_list_head = NULL;

void c11threads_win32_destroy(void)
{
    struct _c11threads_win32_thrd_entry_t *thrd_entry;
    struct _c11threads_win32_thrd_entry_t *thrd_entry_temp;
    struct _c11threads_win32_tss_dtor_entry_t *tss_dtor_entry;
    struct _c11threads_win32_tss_dtor_entry_t *tss_dtor_entry_temp;

    if (_c11threads_win32_initialized) {
        thrd_entry = _c11threads_win32_thrd_list_head;
        while (thrd_entry) {
            thrd_entry_temp = thrd_entry->next;
            CloseHandle(thrd_entry->h);
            free(thrd_entry);
            thrd_entry = thrd_entry_temp;
        }

        tss_dtor_entry = _c11threads_win32_tss_dtor_list_head;
        while (tss_dtor_entry) {
            tss_dtor_entry_temp = tss_dtor_entry->next;
            TlsFree(tss_dtor_entry->key);
            free(tss_dtor_entry);
            tss_dtor_entry = tss_dtor_entry_temp;
        }

        _c11threads_win32_initialized = 0;
        _c11threads_win32_thrd_list_head = NULL;
        _c11threads_win32_tss_dtor_list_head = NULL;
    }
}

/* ---- thread management ---- */

static int _c11threads_win32_thrd_register(GPThread thrd, HANDLE h)
{
    struct _c11threads_win32_thrd_entry_t *thread_entry;

    thread_entry = malloc(sizeof(*thread_entry));
    if (!thread_entry) {
        return 0;
    }

    thread_entry->thrd = thrd;
    thread_entry->h = h;

    AcquireSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);
    thread_entry->next = _c11threads_win32_thrd_list_head;
    _c11threads_win32_thrd_list_head = thread_entry;
    ReleaseSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);

    return 1;
}

static void *_c11threads_win32_thrd_pop_entry(GPThread thrd)
{
    void *h;
    struct _c11threads_win32_thrd_entry_t *prev;
    struct _c11threads_win32_thrd_entry_t *curr;
    struct _c11threads_win32_thrd_entry_t *next;

    h = NULL;
    prev = NULL;

    AcquireSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);
    curr = _c11threads_win32_thrd_list_head;
    while (curr)
    {
        if (curr->thrd == thrd) {
            h = curr->h;
            next = curr->next;
            if (prev) {
                prev->next = next;
            } else {
                _c11threads_win32_thrd_list_head = next;
            }
            break;
        }

        prev = curr;
        curr = curr->next;
    }
    ReleaseSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);

    free(curr);
    return h;
}

static void _c11threads_win32_thrd_run_tss_dtors(void)
{
    int ran_dtor;
    size_t i;
    struct _c11threads_win32_tss_dtor_entry_t *prev;
    struct _c11threads_win32_tss_dtor_entry_t *curr;
    struct _c11threads_win32_tss_dtor_entry_t *next;
    void *val;

    AcquireSRWLockExclusive(&_c11threads_win32_tss_dtor_list_srw_lock);
    ran_dtor = 1;
    for (i = 0; i < GP_THREAD_LOCAL_DESTRUCTOR_ITERATIONS && ran_dtor; ++i) {
        ran_dtor = 0;
        prev = NULL;
        curr = _c11threads_win32_tss_dtor_list_head;

        while (curr) {
            val = TlsGetValue(curr->key);
            if (val) {
                TlsSetValue(curr->key, NULL);
                curr->dtor(val);
                ran_dtor = 1;
            } else if (GetLastError() != ERROR_SUCCESS) {
                next = curr->next;
                free(curr);
                if (prev) {
                    prev->next = next;
                } else {
                    _c11threads_win32_tss_dtor_list_head = next;
                }
                curr = next;
                continue;
            }

            curr = curr->next;
        }
    }
    ReleaseSRWLockExclusive(&_c11threads_win32_tss_dtor_list_srw_lock);
}

struct _c11threads_win32_thrd_start_thunk_parameters_t {
    int(*func)(void*);
    void *arg;
};

static int __stdcall _c11threads_win32_thrd_start_thunk(struct _c11threads_win32_thrd_start_thunk_parameters_t *start_parameters)
{
    int res;
    struct _c11threads_win32_thrd_start_thunk_parameters_t local_start_params;
    local_start_params = *start_parameters;
    free(start_parameters);
    res = local_start_params.func(local_start_params.arg);
    _c11threads_win32_thrd_run_tss_dtors();
    return res;
}

static void gp_s_thread_init_exit_cleanup(void)
{
    atexit(c11threads_win32_destroy);
}

bool gp_thread_create(GPThread *thr, int(*func)(void*), void *arg)
{
    static GPOnce initialized = GP_ONCE_INITIALIZER;
    struct _c11threads_win32_thrd_start_thunk_parameters_t *thread_start_params;
    struct _c11threads_win32_thrd_entry_t *thread_entry;
    void *h;

    gp_call_once(&initialized, gp_s_thread_init_exit_cleanup); // shut up sanitizers

    int old_errno = errno;
    thread_start_params = malloc(sizeof(*thread_start_params));
    if (!thread_start_params) {
        errno = old_errno;
        return false;
    }

    thread_start_params->func = func;
    thread_start_params->arg = arg;

    thread_entry = malloc(sizeof(*thread_entry));
    if (!thread_entry) {
        errno = old_errno;
        free(thread_start_params);
        return false;
    }

    AcquireSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);
    h = CreateThread(
        NULL, 0, (PTHREAD_START_ROUTINE)_c11threads_win32_thrd_start_thunk, thread_start_params, 0, thr);
    if (!h) {
        ReleaseSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);
        free(thread_start_params);
        free(thread_entry);
        return false;
    }
    thread_entry->next = _c11threads_win32_thrd_list_head;
    thread_entry->h = h;
    thread_entry->thrd = *thr;
    _c11threads_win32_thrd_list_head = thread_entry;
    ReleaseSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);

    return true;
}

void gp_thread_exit(int res)
{
    _c11threads_win32_thrd_run_tss_dtors();
    ExitThread(res);
}

bool gp_thread_join(GPThread thr, int *res)
{
    int ret;
    void *h;

    ret = false;
    h = _c11threads_win32_thrd_pop_entry(thr);
    DWORD _res;
    if (h) {
        if (WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0
            && (!res || GetExitCodeThread(h, &_res))) {
            ret = true;
        }
        if (ret && res != NULL)
            *res = _res;
        CloseHandle(h);
    }

    return ret;
}

bool gp_thread_detach(GPThread thr)
{
    void *h;
    h = _c11threads_win32_thrd_pop_entry(thr);
    return h && CloseHandle(h);
}

GPThread gp_thread_current(void)
{
    return GetCurrentThreadId();
}

void gp_thread_yield(void)
{
    SwitchToThread();
}

/* ---- mutexes ---- */

void gp_mutex_init(GPMutex *mtx)
{
    InitializeSRWLock(gp_launder(mtx));
}

void gp_mutex_lock(GPMutex *mtx)
{
    AcquireSRWLockExclusive(gp_launder(mtx));
}

bool gp_mutex_trylock(GPMutex *mtx)
{
    return TryAcquireSRWLockExclusive(gp_launder(mtx));
}

bool gp_mutex_timedlock_ns(GPMutex* mutex, int64_t time_ns)
{
    if (time_ns < 0)
        return false;

    bool success;
    GPUInt128 start = gp_time_begin();

    success = TryAcquireSRWLockExclusive(gp_launder(mtx));
    while ( ! success) {
        if (gp_time_ns(&start) >= time_ns)
            return false;
        Sleep(0);
        success = TryAcquireSRWLockExclusive(gp_launder(mtx));
    }
    return true;
}

bool gp_mutex_timedlock(GPMutex* mutex, double time)
{
    gp_assume( ! isnan(time));
    if (time < 0.)
        return false;
    if (isinf(time)) {
        AcquireSRWLockExclusive(gp_launder(mtx));
        return true;
    }

    bool success;
    GPUInt128 start = gp_time_begin();

    success = TryAcquireSRWLockExclusive(gp_launder(mtx));
    while ( ! success) {
        if (gp_time(&start) >= time)
            return false;
        Sleep(0);
        success = TryAcquireSRWLockExclusive(gp_launder(mtx));
    }
    return true;
}

bool gp_mutex_timedlock_absolute(GPMutex* mutex, GPInt128 time_ns)
{
    return gp_mutex_timedlock_ns(
        mutex, gp_int128_lo(gp_int128_sub(time_ns, gp_time_begin())));
}

void gp_mutex_unlock(GPMutex *mtx)
{
    ReleaseSRWLockExclusive(gp_launder(mtx));
}

/* ---- condition variables ---- */

void gp_cond_init(GPCond *cond)
{
    InitializeConditionVariable(gp_launder(cond));
}

void gp_cond_destroy(GPCond *cond)
{
    (void)cond;
}

void gp_cond_signal(GPCond *cond)
{
    WakeConditionVariable(gp_launder(cond));
}

void gp_cond_broadcast(GPCond *cond)
{
    WakeAllConditionVariable(gp_launder(cond));
}

// NOTE: Microsoft only documents timeout error for SleepConditionVariableSRW().
// Therefore, we cannot meaningfully handle other errors. Anyway real world code
// (including Microsoft's official examples) ignores other errors, so we'll do
// that too for simplicity and consistency with POSIX equivalent that man pages
// state that also do not fail.

void gp_cond_wait(GPCond *cond, GPMutex *mtx)
{
    SleepConditionVariableSRW(gp_launder(cond), gp_launder(mtx), INFINITE);
}

bool gp_cond_timedwait(GPCond* cond, GPMutex* mutex, double t)
{
    gp_assume( ! isnan(t));
    if (t < 0)
        return false;

    t *= 1000.;
    if (isinf(t)) {
        SleepConditionVariableSRW(gp_launder(cond), gp_launder(mtx), INFINITE);
        return true;
    }

    DWORD wait_time = round(t);
    if (t >= INFINITE - .5)
        wait_time = INFINITE - 1; // 49 days, pretend that beyond that is spurious wakeup.

    return SleepConditionVariableSRW(gp_launder(cond), gp_launder(mutex), wait_time);
}

bool gp_cond_timedwait_ns(GPCond* cond, GPMutex* mutex, int64_t t_ns)
{
    if (t_ns < 0)
        return false;

    // t_ns is positive and compilers often optimize division by constant
    // slightly better when unsigned due to rounding.
    t_ns = (uint64_t)t_ns / 1000000;
    DWORD wait_time = t_ns;
    if (t_ns > INFINITE - 1)
        wait_time = INFINITE - 1; // 49 days, pretend that beyond that is spurious wakeup.

    SleepConditionVariableSRW(gp_launder(cond), gp_launder(mutex), wait_time);
}

bool gp_cond_timedwait_absolute(GPCond* cond, GPMutex* mutex, GPInt128 time_ns)
{
    return gp_cond_timedwait_ns(
        cond, mutex, gp_int128_lo(gp_int128_sub(time_ns, gp_time_begin())));
}

/* ---- thread-specific data ---- */

static int _c11threads_win32_tss_register(tss_t key, void(*dtor)(void*)) {
    struct _c11threads_win32_tss_dtor_entry_t *tss_dtor_entry;

    tss_dtor_entry = malloc(sizeof(*tss_dtor_entry));
    if (!tss_dtor_entry) {
        return 0;
    }

    tss_dtor_entry->key = key;
    tss_dtor_entry->dtor = dtor;

    AcquireSRWLockExclusive(&_c11threads_win32_tss_dtor_list_srw_lock);
    tss_dtor_entry->next = _c11threads_win32_tss_dtor_list_head;
    _c11threads_win32_tss_dtor_list_head = tss_dtor_entry;
    ReleaseSRWLockExclusive(&_c11threads_win32_tss_dtor_list_srw_lock);

    return 1;
}

static void _c11threads_win32_tss_deregister(tss_t key) {
    struct _c11threads_win32_tss_dtor_entry_t *prev;
    struct _c11threads_win32_tss_dtor_entry_t *curr;
    struct _c11threads_win32_tss_dtor_entry_t *next;

    prev = NULL;

    AcquireSRWLockExclusive(&_c11threads_win32_tss_dtor_list_srw_lock);
    curr = _c11threads_win32_tss_dtor_list_head;
    while (curr)
    {
        if (curr->key == key) {
            next = curr->next;
            if (prev) {
                prev->next = next;
            } else {
                _c11threads_win32_tss_dtor_list_head = next;
            }
            break;
        }

        prev = curr;
        curr = curr->next;
    }
    ReleaseSRWLockExclusive(&_c11threads_win32_tss_dtor_list_srw_lock);

    free(curr);
}

bool gp_thread_local_create(GPThreadKey* key, void(*dtor)(void*))
{
    *key = TlsAlloc();
    if (*key == TLS_OUT_OF_INDEXES) {
        return false;
    }
    if (dtor && !_c11threads_win32_tss_register(*key, dtor)) {
        TlsFree(*key);
        return false;
    }
    return true;
}

void gp_thread_local_delete(GPThreadKey key)
{
    _c11threads_win32_tss_deregister(key);
    TlsFree(key);
}

void gp_thread_local_set(GPThreadKey key, const void *val)
{
    TlsSetValue(key, (PVOID)val);
}

void* gp_thread_local_get(GPThreadKey key)
{
    return TlsGetValue(key);
}

/* ---- misc ---- */

static int __stdcall _c11threads_win32_call_once_thunk(void *init_once, void (*func)(void), void **context)
{
    (void)init_once;
    (void)context;
    func();
    return 1;
}

void gp_call_once(GPOnce *flag, void (*func)(void))
{
#ifdef _MSC_VER
#pragma warning(push)
/* Warning C4054: 'type cast' : from function pointer 'int (__stdcall *)(void *,void (__cdecl *)(void),void **)' to data pointer 'const void *' */
/* Warning C4054: 'type cast' : from function pointer 'void (__cdecl *)(void)' to data pointer 'void *' */
#pragma warning(disable: 4054)
#endif
    InitOnceExecuteOnce((void*)flag, (const void*)_c11threads_win32_call_once_thunk, (void*)func, NULL);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
}


// ----------------------------------------------------------------------------
#else // pthreads


#include <gpc/utils.h>
#include <gpc/time.h>
#include <limits.h>
#include <stdint.h>

// Pthreads uses routines of type void*(*)(void*), but the type should be
// int(*)(void*) for portability. However, just casting the function pointer
// type would lead to UB when the function gets called. On x86_64, a 32-bit
// return value will be zero extended and on 32-bit targets the return value
// fits perfectly. In those systems, pointer laundering is safe enough, others
// require a thunk with a matching return type.

#if defined(GP_TARGET_ARCH_X86_64) || UINTPTR_MAX == UINT_MAX // no need for thunk
bool gp_thread_create(GPThread* thr, int(*func)(void*), void *arg)
{
    return !pthread_create(thr, 0, gp_launder(func), arg);
}
#else // thunk
typedef struct gp_thread_thunk_args
{
    int(*routine)(void* arg);
    void* arg;
} GPThreadThunkArgs;

static void* gp_s_thread_thunk(void*_thunk_args)
{
    GPThreadThunkArgs* thunk_args = _thunk_args;
    void* result = (void*)(intptr_t)(thunk_args->routine(thunk_args->arg));
    free(thunk_args);
    return result;
}

bool gp_thread_create(GPThread* thr, int(*func)(void*), void *arg)
{
    int old_errno = errno;
    GPThreadThunkArgs* thunk_args = malloc(sizeof *thunk_args);
    errno = old_errno;
    if (thunk_args == NULL)
        return false;
    thunk_args->routine = func;
    thunk_args->arg = arg;
    int error = pthread_create(thr, 0, gp_s_thread_thunk, thunk_args);
    if (error)
        free(thunk_args);
    return ! error;
}
#endif // thunk

static bool gp_s_mutex_timedlock_ts(GPMutex *mtx, struct timespec ts)
{
    int timedlock_return = 0;
    #ifdef __APPLE__
    /* Darwin doesn't implement timed mutexes currently */
    /* fake a timedlock by polling trylock in a loop and waiting for a bit */
    struct timeval now;
    struct timespec sleeptime;

    sleeptime.tv_sec = 0;
    #define GP_TIMEDLOCK_POLL_INTERVAL 5000000
    sleeptime.tv_nsec = GP_TIMEDLOCK_POLL_INTERVAL;

    while((timedlock_return = pthread_mutex_trylock(mtx)) == EBUSY) {
        gettimeofday(&now, NULL);

        if(now.tv_sec > ts.tv_sec || (now.tv_sec == ts.tv_sec &&
                    (now.tv_usec * 1000) >= ts.tv_nsec)) {
            return false;
        }

        nanosleep(&sleeptime, NULL);
    }
    #else
    timedlock_return = pthread_mutex_timedlock(mtx, &ts);
    #endif
    gp_assume(timedlock_return == 0 || timedlock_return == ETIMEDOUT, strerror(timedlock_return));
    return ! timedlock_return;
}

// FIXME: Ignoring hi bits of 128 bit timestamp in many functions. But no rush to
// fix, as of 2026, we still have a couple of hundreds of years before this matters.

bool gp_mutex_timedlock_absolute(GPMutex* mutex, GPInt128 t)
{
    if (gp_int128_hi(t) < 0)
        return false;

    return gp_s_mutex_timedlock_ts(mutex, gp_internal_timespec_from_time_ns(gp_int128_lo(t)));
}

bool gp_mutex_timedlock(GPMutex* mutex, double t)
{
    gp_assume( ! isnan(t));
    if (t < 0)
        return false;
    if (isinf(t)) {
        gp_mutex_lock(mutex);
        return true;
    }
    int64_t t_ns = gp_int128_lo(gp_time_begin()) + (int64_t)(1000000000.*t);
    return gp_s_mutex_timedlock_ts(mutex, gp_internal_timespec_from_time_ns(t_ns));
}

bool gp_mutex_timedlock_ns(GPMutex* mutex, int64_t t_ns)
{
    if (t_ns < 0)
        return false;
    t_ns += gp_int128_lo(gp_time_begin());
    return gp_s_mutex_timedlock_ts(mutex, gp_internal_timespec_from_time_ns(t_ns));
}

bool gp_cond_timedwait(GPCond* cond, GPMutex* mutex, double t)
{
    gp_assume( ! isnan(t));
    if (t < 0)
        return false;
    if (isinf(t)) {
        pthread_cond_wait(cond, mutex);
        return true;
    }
    int64_t t_ns = gp_int128_lo(gp_time_begin()) + (int64_t)(1000000000.*t);
    struct timespec ts = gp_internal_timespec_from_time(t_ns);
    return ! pthread_cond_timedwait(cond, mutex, &ts);
}

bool gp_cond_timedwait_ns(GPCond* cond, GPMutex* mutex, int64_t t_ns)
{
    if (t_ns < 0)
        return false;

    t_ns += gp_int128_lo(gp_time_begin());
    struct timespec ts = gp_internal_timespec_from_time_ns(t_ns);
    return ! pthread_cond_timedwait(cond, mutex, &ts);
}

bool gp_cond_timedwait_absolute(GPCond* cond, GPMutex* mutex, GPInt128 t)
{
    if (gp_int128_hi(t) < 0)
        return false;

    struct timespec ts = gp_internal_timespec_from_time_ns(gp_int128_lo(t));
    return ! pthread_cond_timedwait(cond, mutex, &ts);
}

#endif // defined(GP_TARGET_OS_WINDOWS) && !defined(GP_USE_PTHREADS)
