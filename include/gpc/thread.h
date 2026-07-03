// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Based on https://github.com/jtsiomb/c11threads.
// Authors:
//   John Tsiombikas <nuclear@member.fsf.org> - original POSIX threads wrapper
//   Oliver Old <oliver.old@outlook.com> - win32 implementation

#ifndef GP_THREAD_INCLUDED
#define GP_THREAD_INCLUDED 1

#include <gpc/assert.h>

#if defined(_WIN32) && !defined(GP_USE_PTHREADS)
#define GP_USE_WINTHREADS 1
#else
#include <pthread.h>
#endif

#if __STDC_VERSION__ >= 201112L && !defined(_WIN32) // UCRT stdatomic.h broken
#include <stdatomic.h>
#endif

#include <time.h>

#ifndef TIME_UTC
#define TIME_UTC 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup thread Threading
/// @code
/// #include <gpc/thread.h>
/// @endcode
/// Portable threading API based on [C11 threads](https://en.cppreference.com/c/header/threads),
/// including threads, mutual exclusion, condition variables, and thread-local
/// storage. Implementation is based on [c11threads](https://github.com/jtsiomb/c11threads).
///
/// At the time of writing, GNU libc does not implement standard threading on
/// Windows. Microsoft UCRT only added standard threading recently, but anyway
/// it's C11 and we are supposed to support C99. This module is a thin wrapper
/// over POSIX threads if not Windows, otherwise uses Win32 threads in it's
/// implementation. POSIX threads can be forced with @ref GP_USE_PTHREADS.
///
/// Significant changes from C11 standard API has been made mostly to increase
/// portability and inter-op with other parts of the library. Due to these
/// changes, names have been also changed to follow our conventions and (more
/// importantly) to make it explicit that this is not the C11 standard header
/// provided by the implementation.
///
/// TODO implement stuff in the list below and remove from docs since outdated.
/// - Names of all symbols have been changed to be in `gp_` or `GP_` namespace.
///   This is for consistency and to make it explicit that this is not the
///   standard library header provided by the implementation. TODO
/// - Names have been changed to follow our naming conventions, so constants are
///   CAPITALIZED. TODO
/// - API using C11 `timespec` has been changed to use [our portable timing API](@ref timing),
///   so `timespec` has been changed to @ref GPUInt128, which is also easier to use. TODO
/// - Disabled `thread_local` on MinGW, because it's [completely broken](https://sourceforge.net/p/mingw-w64/bugs/445/).
/// - Added @ref GP_MTX_INIT, which is equivalent to `PTHREAD_MUTEX_INITIALIZER`. TODO
/// - Added assertion to enforce no deadlocks, which would be undefined behavior. TODO
/// - Added @ref GP_MAYBE_THREAD_LOCAL and @ref GP_MAYBE_ATOMIC.
/// - Added documentation. TODO
/// @{

/// @addtogroup compile_options
/// @{
#ifdef GP_DOXYGEN
/** Use POSIX threads even if C11 threads available.
 *
 * Should be defined globally if needed.
 *
 * By default on Windows, this library uses Win32 threads. This macro forces
 * using POSIX threads instead.
 */
#define GP_USE_PTHREADS
#endif
/// @}

/** Sometimes thread local. TODO add always thread local too like C11 standard threads. And atomics too.
 *
 * Use this only when thread local storage would be ideal but not necessary for
 * correctness. Example use might be something logging related that is meant for
 * developers, but end users might not care about.
 *
 * Note: MinGW thread locals are broken, so they are disabled. See
 * https://sourceforge.net/p/mingw-w64/bugs/445/
 */
#ifdef _MSC_VER
#  define GP_MAYBE_THREAD_LOCAL __declspec(thread)
#  define GP_HAS_THREAD_LOCAL 1
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__) && !defined(__MINGW32__)
#  define GP_MAYBE_THREAD_LOCAL _Thread_local
#  define GP_HAS_THREAD_LOCAL 1
#elif defined(__GNUC__) && !defined(__MINGW32__)
#  define GP_MAYBE_THREAD_LOCAL __thread
#  define GP_HAS_THREAD_LOCAL 1
#else
#  define GP_MAYBE_THREAD_LOCAL
#endif

/** Sometimes atomic.
 *
 * Use this only when atomics would be ideal but not necessary for correctness.
 */
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define GP_HAS_ATOMICS 1
#  define GP_MAYBE_ATOMIC _Atomic
#else
#  define GP_MAYBE_ATOMIC
#endif

// TODO docs
enum {
    GP_MUTEX_PLAIN     = 0,
    GP_MUTEX_RECURSIVE = 1,
    GP_MUTEX_TIMED     = 2,
};

enum {
    GP_THREAD_SUCCESS,  ///< Successful return value.
    GP_THREAD_TIMEDOUT, ///< Timed out return value.
    GP_THREAD_BUSY,     ///< Unsuccessful return value due to resource temporarily unavailable.
    GP_THREAD_ERROR,    ///< Unsuccessful return value.
    GP_THREAD_NOMEM     ///< Unsuccessful return value due to out of memory.
};

// ----------------------------------------------------------------------------
#ifndef GP_USE_WINTHREADS // use POSIX threads

#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h> // sched_yield
#include <sys/time.h>

#define GP_ONCE_FLAG_INIT PTHREAD_ONCE_INIT
#define GP_THREAD_LOCAL_DESTRUCTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS

// ------------------------------------
/// @defgroup thread_management Thread Management
/// @{

/** Complete object thread identifier. */
typedef pthread_t GPThread;

int gp_thread_create(GPThread*, int(*func)(void*arg), void* arg);

GP_INLINE void thrd_exit(int res)
{
    pthread_exit((void*)(intptr_t)res);
}

GP_INLINE int gp_thread_join(GPThread thr, int *res)
{
    void *retval;

    if (pthread_join(thr, &retval) != 0) {
        return GP_THREAD_ERROR;
    }
    if (res) {
        *res = (int)(intptr_t)retval;
    }
    return GP_THREAD_SUCCESS;
}

GP_INLINE int thrd_detach(GPThread thr)
{
    return pthread_detach(thr) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE GPThread thrd_current(void)
{
    return pthread_self();
}

GP_INLINE int gp_thread_equal(GPThread a, GPThread b)
{
    return pthread_equal(a, b);
}

// TODO get rid of this timespec junk
GP_INLINE int gp_thread_sleep(const struct timespec *ts_in, struct timespec *rem_out)
{
	if(nanosleep(ts_in, rem_out) < 0) {
		if(errno == EINTR) return -1;
		return -2;
	}
	return 0;
}

GP_INLINE void gp_thread_yield(void)
{
    sched_yield();
}

/// @}
// ------------------------------------
/// @defgroup mutexes Mutexes
/// @{

/** Mutex identifier. */
typedef pthread_mutex_t GPMutex;

GP_INLINE int gp_mutex_init(GPMutex *mtx, int type)
{
	int res;
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);

	if(type & GP_MUTEX_TIMED) {
#ifdef PTHREAD_MUTEX_TIMED_NP
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP);
#else
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
#endif
	}
	if(type & GP_MUTEX_RECURSIVE) {
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	}

	res = pthread_mutex_init(mtx, &attr) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
	pthread_mutexattr_destroy(&attr);
	return res;
}

GP_INLINE void gp_mutex_destroy(GPMutex *mtx)
{
	pthread_mutex_destroy(mtx);
}

GP_INLINE int gp_mutex_lock(GPMutex *mtx)
{
    int res = pthread_mutex_lock(mtx);
    return res == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE int gp_mutex_trylock(GPMutex *mtx)
{
    int res = pthread_mutex_trylock(mtx);
    if(res == EBUSY) {
        return GP_THREAD_BUSY;
    }
    return res == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

int mtx_timedlock(GPMutex *mtx, const struct timespec *ts);

GP_INLINE int mtx_unlock(GPMutex *mtx)
{
	return pthread_mutex_unlock(mtx) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

/// @}
// ------------------------------------
/// @defgroup condition_variables Condition Variables
/// @{

/** Condition variable identifier. */
typedef pthread_cond_t  gp_cnd_t;

GP_INLINE int cnd_init(cnd_t *cond)
{
	return pthread_cond_init(cond, 0) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE void cnd_destroy(cnd_t *cond)
{
	pthread_cond_destroy(cond);
}

GP_INLINE int cnd_signal(cnd_t *cond)
{
	return pthread_cond_signal(cond) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE int cnd_broadcast(cnd_t *cond)
{
	return pthread_cond_broadcast(cond) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE int cnd_wait(cnd_t *cond, GPMutex *mtx)
{
	return pthread_cond_wait(cond, mtx) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE int cnd_timedwait(cnd_t *cond, GPMutex *mtx, const struct timespec *ts)
{
	int res;

	if((res = pthread_cond_timedwait(cond, mtx, ts)) != 0) {
		return res == ETIMEDOUT ? GP_THREAD_TIMEDOUT : GP_THREAD_ERROR;
	}
	return GP_THREAD_SUCCESS;
}

/// @}
// ------------------------------------
/// @defgroup thread_local_storage Thread-Local Storage
/// @{

/** Thread-local storage pointer. */
typedef pthread_key_t gp_thread_key_t;

GP_INLINE int tss_create(tss_t *key, tss_dtor_t dtor)
{
	return pthread_key_create(key, dtor) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE void tss_delete(tss_t key)
{
	pthread_key_delete(key);
}

GP_INLINE int tss_set(tss_t key, void *val)
{
	return pthread_setspecific(key, val) == 0 ? GP_THREAD_SUCCESS : GP_THREAD_ERROR;
}

GP_INLINE void *tss_get(tss_t key)
{
	return pthread_getspecific(key);
}

/// @}
// ------------------------------------
/// @defgroup call_once Call Once
/// @{

/** Complete object type capable of holding flag used by @ref gp_call_once(). */
typedef pthread_once_t gp_once_flag_t;

GP_INLINE void call_once(once_flag *flag, void (*func)(void))
{
	pthread_once(flag, func);
}

/// @}
# else // use Win32 threads ---------------------------------------------------

// ------------------------------------
// Thread Management

/** Complete object thread identifier. */
typedef pthread_t gp_thrd_t;

GP_INLINE int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);

GP_INLINE void thrd_exit(int res);

GP_INLINE int thrd_join(thrd_t thr, int *res);

GP_INLINE int thrd_detach(thrd_t thr);

GP_INLINE thrd_t thrd_current(void);

GP_INLINE int thrd_equal(thrd_t a, thrd_t b);

GP_INLINE int thrd_sleep(const struct timespec *ts_in, struct timespec *rem_out);

GP_INLINE void thrd_yield(void);

// ------------------------------------
// Mutexes

/** Mutex identifier. */
typedef pthread_mutex_t gp_mtx_t;

GP_INLINE int mtx_init(mtx_t *mtx, int type);

GP_INLINE void mtx_destroy(mtx_t *mtx);

GP_INLINE int mtx_lock(mtx_t *mtx);

GP_INLINE int mtx_trylock(mtx_t *mtx);

GP_INLINE int mtx_timedlock(mtx_t *mtx, const struct timespec *ts);

GP_INLINE int mtx_unlock(mtx_t *mtx);

// ------------------------------------
// Condition Variables

/** Condition variable identifier. */
typedef pthread_cond_t  gp_cnd_t;

GP_INLINE int cnd_init(cnd_t *cond);

GP_INLINE void cnd_destroy(cnd_t *cond);

GP_INLINE int cnd_signal(cnd_t *cond);

GP_INLINE int cnd_broadcast(cnd_t *cond);

GP_INLINE int cnd_wait(cnd_t *cond, mtx_t *mtx);

GP_INLINE int cnd_timedwait(cnd_t *cond, mtx_t *mtx, const struct timespec *ts);

// ------------------------------------
// Thread-Local Storage

/** Thread-local storage pointer. */
typedef pthread_key_t   gp_tss_t;

GP_INLINE int tss_create(tss_t *key, tss_dtor_t dtor);

GP_INLINE void tss_delete(tss_t key);

GP_INLINE int tss_set(tss_t key, void *val);

GP_INLINE void *tss_get(tss_t key);

// ------------------------------------
// Call Once

/** Complete object type capable of holding flag used by @ref gp_call_once(). */
typedef pthread_once_t gp_once_flag_t;

GP_INLINE void call_once(once_flag *flag, void (*func)(void));

#endif // GP_USE_WINTHREADS ---------------------------------------------------

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_THREAD_INCLUDED
