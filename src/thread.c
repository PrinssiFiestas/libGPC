// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Based on https://github.com/jtsiomb/c11threads.
// Authors:
//   John Tsiombikas <nuclear@member.fsf.org> - original POSIX threads wrapper
//   Oliver Old <oliver.old@outlook.com> - win32 implementation

#include <gpc/thread.h>

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

#include <windows.h>


/* ---- library ---- */

struct _c11threads_win32_thrd_entry_t {
    struct _c11threads_win32_thrd_entry_t *next;
    void *h;
    GPThread thrd;
};

struct _c11threads_win32_tss_dtor_entry_t {
    struct _c11threads_win32_tss_dtor_entry_t *next;
    tss_dtor_t dtor;
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

/* ---- utilities ---- */

static int _c11threads_win32_util_is_timespec32_valid(const struct _c11threads_win32_timespec32_t *ts)
{
    return ts->tv_sec >= 0 && ts->tv_nsec >= 0 && ts->tv_nsec <= 999999999;
}

static int _c11threads_win32_util_is_timespec64_valid(const struct _c11threads_win32_timespec64_t *ts)
{
    return ts->tv_sec >= 0 && ts->tv_nsec >= 0 && ts->tv_nsec <= 999999999;
}

/* Precondition: 'ts' validated. */
static __int64 _c11threads_win32_util_timespec32_to_file_time(const struct _c11threads_win32_timespec32_t *ts)
{
    unsigned __int64 sec_res;
    unsigned __int64 nsec_res;

    sec_res = (unsigned __int64)ts->tv_sec * (unsigned __int64)10000000;

    /* Add another 100 ns if division yields remainder. */
    nsec_res = (unsigned long)ts->tv_nsec / 100UL + !!((unsigned long)ts->tv_nsec % 100UL);

    return sec_res + nsec_res;
}

/* Precondition: 'ts' validated. */
static __int64 _c11threads_win32_util_timespec64_to_file_time(const struct _c11threads_win32_timespec64_t *ts, size_t *periods)
{
    unsigned __int64 sec_res;
    unsigned __int64 nsec_res;
    unsigned __int64 res;

    *periods = (unsigned long)((unsigned __int64)ts->tv_sec / (unsigned __int64)922337203685);
    sec_res = ((unsigned __int64)ts->tv_sec % (unsigned __int64)922337203685) * (unsigned __int64)10000000;

    /* Add another 100 ns if division yields remainder. */
    nsec_res = (unsigned long)ts->tv_nsec / 100UL + !!((unsigned long)ts->tv_nsec % 100UL);

    /* 64-bit time_t may cause overflow. */
    if (nsec_res > (unsigned __int64) - 1 - sec_res) {
        ++*periods;
        nsec_res -= (unsigned __int64) - 1 - sec_res;
        sec_res = 0;
    }

    res = sec_res + nsec_res;

    if (*periods && !res) {
        --*periods;
        return (__int64)9223372036850000000;
    }

    return res;
}

/* Precondition: 'ts' validated. */
static int _c11threads_win32_util_timespec32_to_milliseconds(const struct _c11threads_win32_timespec32_t *ts, unsigned long *ms)
{
    unsigned long sec_res;
    unsigned long nsec_res;

    /* Overflow. */
    if ((unsigned long)ts->tv_sec > (INFINITE - 1UL) / 1000UL) {
        return 0;
    }

    sec_res = (unsigned long)ts->tv_sec * 1000UL;
    /* Add another millisecond if division yields remainder. */
    nsec_res = (unsigned long)ts->tv_nsec / 1000000UL + !!((unsigned long)ts->tv_nsec % 1000000UL);

    /* Overflow. */
    if (nsec_res > INFINITE - 1UL - sec_res) {
        return 0;
    }

    *ms = sec_res + nsec_res;
    return 1;
}

/* Precondition: 'ts' validated. */
static int _c11threads_win32_util_timespec64_to_milliseconds(const struct _c11threads_win32_timespec64_t *ts, unsigned long *ms)
{
    unsigned long sec_res;
    unsigned long nsec_res;

    /* Overflow. */
    if ((unsigned __int64)ts->tv_sec > (INFINITE - 1UL) / 1000UL) {
        return 0;
    }

    sec_res = (unsigned long)ts->tv_sec * 1000UL;
    /* Add another millisecond if division yields remainder. */
    nsec_res = (unsigned long)ts->tv_nsec / 1000000UL + !!((unsigned long)ts->tv_nsec % 1000000UL);

    /* Overflow. */
    if (nsec_res > INFINITE - 1UL - sec_res) {
        return 0;
    }

    *ms = sec_res + nsec_res;
    return 1;
}

/* Precondition: 'current_time' and 'end_time' validated. */
static unsigned long _c11threads_win32_util_timepoint_to_millisecond_timespan32(
    const struct _c11threads_win32_timespec32_t *current_time,
    const struct _c11threads_win32_timespec32_t *end_time,
    int *clamped)
{
    unsigned long wait_time;
    struct _c11threads_win32_timespec32_t ts;

    *clamped = 0;
    if (current_time->tv_sec > end_time->tv_sec || (current_time->tv_sec == end_time->tv_sec && current_time->tv_nsec >= end_time->tv_nsec)) {
        wait_time = 0;
    } else {
        ts.tv_sec = end_time->tv_sec - current_time->tv_sec;
        ts.tv_nsec = end_time->tv_nsec - current_time->tv_nsec;
        if (ts.tv_nsec < 0) {
            --ts.tv_sec;
            ts.tv_nsec += 1000000000;
        }

        if (!_c11threads_win32_util_timespec32_to_milliseconds(&ts, &wait_time)) {
            /* Clamp wait_time. Pretend we've had a spurious wakeup if expired. */
            wait_time = INFINITE - 1;
            *clamped = 1;
        }
    }

    return wait_time;
}

/* Precondition: 'current_time' and 'end_time' validated. */
static unsigned long _c11threads_win32_util_timepoint_to_millisecond_timespan64(const struct _c11threads_win32_timespec64_t *current_time, const struct _c11threads_win32_timespec64_t *end_time, int *clamped) {
    unsigned long wait_time;
    struct _c11threads_win32_timespec64_t ts;

    *clamped = 0;
    if (current_time->tv_sec > end_time->tv_sec || (current_time->tv_sec == end_time->tv_sec && current_time->tv_nsec >= end_time->tv_nsec)) {
        wait_time = 0;
    } else {
        ts.tv_sec = end_time->tv_sec - current_time->tv_sec;
        ts.tv_nsec = end_time->tv_nsec - current_time->tv_nsec;
        if (ts.tv_nsec < 0) {
            --ts.tv_sec;
            ts.tv_nsec += 1000000000;
        }

        if (!_c11threads_win32_util_timespec64_to_milliseconds(&ts, &wait_time)) {
            /* Clamp wait_time. Pretend we've had a spurious wakeup if expired. */
            wait_time = INFINITE - 1;
            *clamped = 1;
        }
    }

    return wait_time;
}

// TODO remove timespec junk
#if defined(C11THREADS_NO_TIMESPEC_GET) || !defined(_MSC_VER)
int _c11threads_win32_timespec32_get(struct _c11threads_win32_timespec32_t *ts, int base)
{
    FILETIME file_time;
    ULARGE_INTEGER li;

    if (base != TIME_UTC) {
        return 0;
    }

    GetSystemTimeAsFileTime(&file_time);

    li.LowPart = file_time.dwLowDateTime;
    li.HighPart = file_time.dwHighDateTime;

    /* Also subtract difference between FILETIME and UNIX time epoch. It's 369 years by the way. */
    ts->tv_sec = (long)(li.QuadPart / (unsigned __int64)10000000 - (unsigned __int64)11644473600);
    ts->tv_nsec = (long)(li.QuadPart % (unsigned __int64)10000000) * 100;

    return base;
}

int _c11threads_win32_timespec64_get(struct _c11threads_win32_timespec64_t *ts, int base)
{
    FILETIME file_time;
    ULARGE_INTEGER li;

    if (base != TIME_UTC) {
        return 0;
    }

    GetSystemTimeAsFileTime(&file_time);

    li.LowPart = file_time.dwLowDateTime;
    li.HighPart = file_time.dwHighDateTime;

    /* Also subtract difference between FILETIME and UNIX time epoch. It's 369 years by the way. */
    ts->tv_sec = li.QuadPart / (unsigned __int64)10000000 - (unsigned __int64)11644473600;
    ts->tv_nsec = (long)(li.QuadPart % (unsigned __int64)10000000) * 100;

    return base;
}
#else
int _c11threads_win32_timespec32_get(struct _c11threads_win32_timespec32_t *ts, int base)
{
    return _timespec32_get((struct _timespec32*)ts, base);
}

int _c11threads_win32_timespec64_get(struct _c11threads_win32_timespec64_t *ts, int base)
{
    return _timespec64_get((struct _timespec64*)ts, base);
}
#endif

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
    for (i = 0; i < TSS_DTOR_ITERATIONS && ran_dtor; ++i) {
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

int c11threads_win32_thrd_self_register(void)
{
    unsigned long desired_access;
    void *process;
    void *thread;

    desired_access = SYNCHRONIZE | THREAD_QUERY_INFORMATION;
    if (_c11threads_win32_winver >= _WIN32_WINNT_VISTA) {
        desired_access = SYNCHRONIZE | THREAD_QUERY_LIMITED_INFORMATION;
    }

    process = GetCurrentProcess();
    thread = GetCurrentThread();
    if (!DuplicateHandle(process, thread, process, &thread, desired_access, 0, 0)) {
        return thrd_error; // TODO error code
    }
    if (!_c11threads_win32_thrd_register(GetCurrentThreadId(), thread)) {
        CloseHandle(thread);
        return ENOMEM;
    }
    return GP_THREAD_SUCCESS;
}

int c11threads_win32_thrd_register(unsigned long win32_thread_id)
{
    /* XXX temporary hack to make this build on MSVC6. Investigate further */
#ifdef _PROCESSTHREADSAPI_H_
    unsigned long desired_access;
    void *h;

    desired_access = SYNCHRONIZE | THREAD_QUERY_INFORMATION;
    if (_c11threads_win32_winver >= _WIN32_WINNT_VISTA) {
        desired_access = SYNCHRONIZE | THREAD_QUERY_LIMITED_INFORMATION;
    }

    h = OpenThread(desired_access, 0, win32_thread_id);
    if (!h) {
        return thrd_error; // TODO error code
    }
    if (!_c11threads_win32_thrd_register(win32_thread_id, h)) {
        CloseHandle(h);
        return ENOMEM;
    }
#endif
    return GP_THREAD_SUCCESS;
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

int gp_thread_create(GPThread *thr, int(*func)(void*), void *arg)
{
    struct _c11threads_win32_thrd_start_thunk_parameters_t *thread_start_params;
    struct _c11threads_win32_thrd_entry_t *thread_entry;
    void *h;

    int old_errno = errno;
    thread_start_params = malloc(sizeof(*thread_start_params));
    if (!thread_start_params) {
        errno = old_errno;
        return EAGAIN;
    }

    thread_start_params->func = func;
    thread_start_params->arg = arg;

    thread_entry = malloc(sizeof(*thread_entry));
    if (!thread_entry) {
        errno = old_errno;
        free(thread_start_params);
        return EAGAIN;
    }

    AcquireSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);
    h = CreateThread(
        NULL, 0, (PTHREAD_START_ROUTINE)_c11threads_win32_thrd_start_thunk, thread_start_params, 0, thr);
    if (!h) {
        ReleaseSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);
        free(thread_start_params);
        free(thread_entry);
        return EAGAIN;
    }
    thread_entry->next = _c11threads_win32_thrd_list_head;
    thread_entry->h = h;
    thread_entry->thrd = *thr;
    _c11threads_win32_thrd_list_head = thread_entry;
    ReleaseSRWLockExclusive(&_c11threads_win32_thrd_list_srw_lock);

    return GP_THREAD_SUCCESS;
}

void gp_thread_exit(int res)
{
    _c11threads_win32_thrd_run_tss_dtors();
    ExitThread(res);
}

int gp_thread_join(GPThread thr, int *res)
{
    int ret;
    void *h;

    // TODO error code
    ret = thrd_error;
    h = _c11threads_win32_thrd_pop_entry(thr);
    DWORD _res;
    if (h) {
        if (WaitForSingleObject(h, INFINITE) == WAIT_OBJECT_0
            && (!res || GetExitCodeThread(h, &_res))) {
            ret = GP_THREAD_SUCCESS;
        }
        *res = *_res;
        CloseHandle(h);
    }

    return ret;
}

int gp_thread_detach(GPThread thr)
{
    void *h;
    h = _c11threads_win32_thrd_pop_entry(thr);
    return h && CloseHandle(h) ? GP_THREAD_SUCCESS : thrd_error; // TODO error code
}

GPThread gp_thread_current(void)
{
    return GetCurrentThreadId();
}

static int _c11threads_win32_sleep_common(__int64 file_time_in)
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

int _c11threads_win32_thrd_sleep32(const struct _c11threads_win32_timespec32_t *ts_in, struct _c11threads_win32_timespec32_t *rem_out)
{
    __int64 file_time;
    int res;

    (void)rem_out;

    if (!_c11threads_win32_util_is_timespec32_valid(ts_in)) {
        return -ERROR_INVALID_PARAMETER;
    }

    file_time = _c11threads_win32_util_timespec32_to_file_time(ts_in);
    if (file_time < 0) {
        return -ERROR_INVALID_PARAMETER;
    }

    res = _c11threads_win32_sleep_common(file_time);

    return res;
}

int _c11threads_win32_thrd_sleep64(const struct _c11threads_win32_timespec64_t *ts_in, struct _c11threads_win32_timespec64_t *rem_out)
{
    __int64 file_time;
    size_t periods;
    int res;

    (void)rem_out;

    if (!_c11threads_win32_util_is_timespec64_valid(ts_in)) {
        return -ERROR_INVALID_PARAMETER;
    }

    file_time = _c11threads_win32_util_timespec64_to_file_time(ts_in, &periods);
    if (file_time < 0) {
        return -ERROR_INVALID_PARAMETER;
    }

restart_sleep:
    res = _c11threads_win32_sleep_common(file_time);

    if (!res && periods) {
        --periods;
        file_time = (__int64)9223372036850000000;
        goto restart_sleep;
    }

    return res;
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

bool gp_mutex_timedlock_ns(GPMutex* mutex, uint64_t time_ns)
{
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
    bool success;
    GPUInt128 start = gp_time_begin();

    success = TryAcquireSRWLockExclusive(gp_launder(mtx));
    while ( ! success) {
        if (gp_time(&start) >= time_ns)
            return false;
        Sleep(0);
        success = TryAcquireSRWLockExclusive(gp_launder(mtx));
    }
    return true;
}

void gp_mutex_unlock(GPMutex *mtx)
{
    ReleaseSRWLockExclusive(gp_launder(mtx));
}

/* ---- condition variables ---- */

struct _c11threads_win32_cnd_t {
    void *mutex;
    void *signal_sema;
    void *broadcast_event;
    size_t wait_count;
};

int gp_cond_init(GPCond *cond)
{
    if (_c11threads_win32_winver >= _WIN32_WINNT_VISTA) {
        InitializeConditionVariable(cond);
        return GP_THREAD_SUCCESS;
    } else {
        struct _c11threads_win32_cnd_t *cnd;

        cnd = malloc(sizeof(*cnd));
        if (!cnd) {
            return ENOMEM;
        }

        cnd->mutex = CreateMutexW(NULL, 0, NULL);
        if (cnd->mutex) {
            cnd->signal_sema = CreateSemaphoreW(NULL, 0, 0x7fffffff, NULL);
            if (cnd->signal_sema) {
                cnd->broadcast_event = CreateEventW(NULL, 1, 0, NULL);
                if (cnd->broadcast_event) {
                    cnd->wait_count = 0;
                    *cond = cnd;
                    return GP_THREAD_SUCCESS;
                }
                CloseHandle(cnd->signal_sema);
            }
            CloseHandle(cnd->mutex);
        }

        free(cnd);
        return thrd_error;
    }
}

void gp_cond_destroy(GPCond *cond)
{
    if (_c11threads_win32_winver < _WIN32_WINNT_VISTA) {
        struct _c11threads_win32_cnd_t *cnd;
        cnd = *cond;
        assert(!cnd->wait_count);
        CloseHandle(cnd->mutex);
        CloseHandle(cnd->signal_sema);
        CloseHandle(cnd->broadcast_event);
        free(cnd);
    }
}

int gp_cond_signal(GPCond *cond)
{
    if (_c11threads_win32_winver >= _WIN32_WINNT_VISTA) {
        WakeConditionVariable(cond);
        return GP_THREAD_SUCCESS;
    } else {
        struct _c11threads_win32_cnd_t *cnd;
        int success;
        unsigned long wait_status;

        cnd = *cond;

        success = 0;
        wait_status = WaitForSingleObject(cnd->mutex, INFINITE);
        if (wait_status == WAIT_OBJECT_0) {
            success = 1;
        } else if (wait_status == WAIT_ABANDONED) {
            abort();
        }

        if (success) {
            if (cnd->wait_count) {
                success = ReleaseSemaphore(cnd->signal_sema, 1, NULL) || GetLastError() == ERROR_TOO_MANY_POSTS;
            }
            if (!ReleaseMutex(cnd->mutex)) {
                success = 0;
            }
        }

        return success ? GP_THREAD_SUCCESS : thrd_error;
    }
}

int gp_cond_broadcast(GPCond *cond)
{
    if (_c11threads_win32_winver >= _WIN32_WINNT_VISTA) {
        WakeAllConditionVariable(cond);
        return GP_THREAD_SUCCESS;
    } else {
        struct _c11threads_win32_cnd_t *cnd;
        int success;
        unsigned long wait_status;

        cnd = *cond;

        success = 0;
        wait_status = WaitForSingleObject(cnd->mutex, INFINITE);
        if (wait_status == WAIT_OBJECT_0) {
            success = 1;
        } else if (wait_status == WAIT_ABANDONED) {
            abort();
        }

        if (success) {
            if (cnd->wait_count) {
                success = SetEvent(cnd->broadcast_event);
            }
            if (!ReleaseMutex(cnd->mutex)) {
                success = 0;
            }
        }

        return success ? GP_THREAD_SUCCESS : thrd_error;
    }
}

// TODO get rid of critical section stuff including SleepCondVarCS
static int _c11threads_win32_cnd_wait_common(GPCond *cond, GPMutex *mtx, unsigned long wait_time, int clamped)
{
    if (SleepConditionVariableSRW(cond, gp_launder(mtx), wait_time)) {
        return GP_THREAD_SUCCESS;
    }

    if (GetLastError() == ERROR_TIMEOUT) {
        return clamped ? GP_THREAD_SUCCESS : GPThreadimedout;
    }

        return thrd_error;
}

int gp_cond_wait(GPCond *cond, GPMutex *mtx)
{
    return _c11threads_win32_cnd_wait_common(cond, mtx, INFINITE, 0);
}

int _c11threads_win32_cnd_timedwait32(GPCond *cond, GPMutex *mtx, const struct _c11threads_win32_timespec32_t *ts)
{
    struct _c11threads_win32_timespec32_t current_time;
    unsigned long wait_time;
    int clamped;

    if (!_c11threads_win32_util_is_timespec32_valid(ts)) {
        return thrd_error;
    }

    if (!_c11threads_win32_timespec32_get(&current_time, TIME_UTC)) {
        return thrd_error;
    }

    wait_time = _c11threads_win32_util_timepoint_to_millisecond_timespan32(&current_time, ts, &clamped);

    return _c11threads_win32_cnd_wait_common(cond, mtx, wait_time, clamped);
}

int _c11threads_win32_cnd_timedwait64(GPCond *cond, GPMutex *mtx, const struct _c11threads_win32_timespec64_t *ts)
{
    struct _c11threads_win32_timespec64_t current_time;
    unsigned long wait_time;
    int clamped;

    if (!_c11threads_win32_util_is_timespec64_valid(ts)) {
        return thrd_error;
    }

    if (!_c11threads_win32_timespec64_get(&current_time, TIME_UTC)) {
        return thrd_error;
    }

    wait_time = _c11threads_win32_util_timepoint_to_millisecond_timespan64(&current_time, ts, &clamped);

    return _c11threads_win32_cnd_wait_common(cond, mtx, wait_time, clamped);
}

/* ---- thread-specific data ---- */

static int _c11threads_win32_tss_register(tss_t key, tss_dtor_t dtor) {
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

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    *key = TlsAlloc();
    if (*key == TLS_OUT_OF_INDEXES) {
        return thrd_error;
    }
    if (dtor && !_c11threads_win32_tss_register(*key, dtor)) {
        TlsFree(*key);
        return thrd_error;
    }
    return GP_THREAD_SUCCESS;
}

void tss_delete(tss_t key)
{
    _c11threads_win32_tss_deregister(key);
    TlsFree(key);
}

int tss_set(tss_t key, void *val)
{
    return TlsSetValue(key, val) ? GP_THREAD_SUCCESS : thrd_error;
}

void *tss_get(tss_t key)
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

void call_once(once_flag *flag, void (*func)(void))
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
// type would lead to UB when the function gets called. On some systems
// laundering is good enough, others need a thunk.

// TODO MSVC x64 macros and check if laundering is safe according to calling
// conventions on ARM.
// TODO we probably should define our own architecture macros.
#if defined(__x86_64__) || UINTPTR_MAX == UINT_MAX // no need for thunk

int gp_thread_create(GPThread* thr, int(*func)(void*), void *arg)
{
    return pthread_create(thr, 0, gp_launder(func), arg);
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

int gp_thread_create(GPThread* thr, int(*func)(void*), void *arg)
{
    int old_errno = errno;
    GPThreadThunk* thunk_args = malloc(sizeof *thunk_args);
    errno = old_errno;
    if (thunk_args == NULL)
        return EAGAIN;
    thunk_args->routine = func;
    thunk_args->arg = arg;
    return pthread_create(
        thr, 0,
        gp_s_thread_thunk,
        thunk_args);
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
    return !timedlock_return;
}

bool gp_mutex_timedlock(GPMutex* mutex, double t)
{
    return gp_s_mutex_timedlock_ts(mutex, gp_timespec_from_time(t));
}

bool gp_mutex_timedlock_ns(GPMutex* mutex, uint64_t t_ns)
{
    return gp_s_mutex_timedlock_ts(mutex, gp_timespec_from_time_ns(gp_uint128(0, t_ns)));
}

#endif // defined(GP_TARGET_OS_WINDOWS) && !defined(GP_USE_PTHREADS)
