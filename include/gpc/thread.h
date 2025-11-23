// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file thread.h
 * Portability wrappers for threads. Uses C11 if available, pthreads otherwise.
 * TODO a lot of important functions still missing!
 * TODO C++?
 */

#ifndef GP_THREAD_INCLUDED
#define GP_THREAD_INCLUDED

#include <gpc/attributes.h>
#include <stdbool.h>

#if __STDC_VERSION__ >= 201112L && !defined(__MINGW32__)
#include <threads.h>
#else
#include <pthread.h>
#endif

#if __cplusplus
extern "C" {
#endif
void* gp_internal_thread_wrapper_routine(void*);
void* gp_internal_thread_wrapper_arg(int(*f)(void*), void*);
#if __cplusplus
} // extern "C"
#endif

// Use this only when thread local storage is desirable but not necessary.
// Note: MinGW thread locals are broken, so they are disabled.
// https://sourceforge.net/p/mingw-w64/bugs/445/
#ifdef _MSC_VER
#define GP_MAYBE_THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__) && !defined(__MINGW32__)
#define GP_MAYBE_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__) && !defined(__MINGW32__)
#define GP_MAYBE_THREAD_LOCAL __thread
#else
#define GP_MAYBE_THREAD_LOCAL
#define GP_NO_THREAD_LOCALS
#endif

// Use this only when atomics are desirable but not necessary.
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#define GP_HAS_ATOMICS 1
#define GP_MAYBE_ATOMIC _Atomic
#else
// Not atomic by any means, but at least prevents some catastrophic compiler
// optimizations.
#define GP_MAYBE_ATOMIC volatile
#endif

#if __STDC_VERSION__ >= 201112L && \
    !defined(__MINGW32__)       && \
    !defined(__STDC_NO_THREADS__)

#define GP_USE_C11_THREADS 1

typedef thrd_t GPThread;

static inline int gp_thread_create(GPThread* t, int(*f)(void*), void* arg)
{
    return thrd_create(t, f, arg);
}

static inline int gp_thread_join(GPThread t, int* ret)
{
    return thrd_join(t, ret);
}

// ----------------------------------------------------------------------------
// Mutual exclusion

typedef mtx_t GPMutex;

static inline bool gp_mutex_init(GPMutex* mutex)
{
    return mtx_init(mutex, mtx_plain) == thrd_success;
}
static inline bool gp_mutex_lock(GPMutex* mutex)
{
    return mtx_lock(mutex) == thrd_success;
}
static inline bool gp_mutex_unlock(GPMutex* mutex)
{
    return mtx_unlock(mutex) == thrd_success;
}
static inline void gp_mutex_destroy(GPMutex* mutex)
{
    mtx_destroy(mutex);
}

// ----------------------------------------------------------------------------
// Thread local storage

typedef tss_t     GPThreadKey;
typedef once_flag GPThreadOnce;
#define GP_THREAD_ONCE_INIT ONCE_FLAG_INIT
static inline int gp_thread_key_create(GPThreadKey* key, void(*destructor)(void*))
{
    return tss_create(key, destructor);
}
static inline void* gp_thread_local_get(GPThreadKey key)
{
    return tss_get(key);
}
static inline int gp_thread_local_set(GPThreadKey key, void* value)
{
    return tss_set(key, value);
}
static inline void gp_thread_once(GPThreadOnce* flag, void(*init)(void))
{
    call_once(flag, init);
}

#else // standard threads not supported, use POSIX threads // --------------- //

#include <stdint.h>

#define GP_USE_PTHREADS 1

typedef pthread_t GPThread;

static inline int gp_thread_create(GPThread* t, int(*f)(void*), void* arg)
{
    return pthread_create(t, NULL, gp_internal_thread_wrapper_routine, gp_internal_thread_wrapper_arg(f, arg));
}

static inline int gp_thread_join(GPThread t, int* return_value)
{
    void* void_return_value;
    int result = pthread_join(t, &void_return_value);
    if (return_value != NULL)
        *return_value = (int)(intptr_t)void_return_value;
    return result;
}
// ----------------------------------------------------------------------------
// Mutual exclusion

typedef pthread_mutex_t GPMutex;

static inline bool gp_mutex_init(GPMutex* mutex)
{
    return pthread_mutex_init(mutex, NULL) == 0;
}
static inline bool gp_mutex_lock(GPMutex* mutex)
{
    return pthread_mutex_lock(mutex) == 0;
}
static inline bool gp_mutex_unlock(GPMutex* mutex)
{
    return pthread_mutex_unlock(mutex) == 0;
}
static inline void gp_mutex_destroy(GPMutex* mutex)
{
    pthread_mutex_destroy(mutex);
}

// ----------------------------------------------------------------------------
// Thread local storage

typedef pthread_key_t  GPThreadKey;
typedef pthread_once_t GPThreadOnce;
#define GP_THREAD_ONCE_INIT PTHREAD_ONCE_INIT
static inline int gp_thread_key_create(GPThreadKey* key, void(*destructor)(void*))
{
    return pthread_key_create(key, destructor);
}
static inline void* gp_thread_local_get(GPThreadKey key)
{
    return pthread_getspecific(key);
}
static inline int gp_thread_local_set(GPThreadKey key, void* value)
{
    return pthread_setspecific(key, value);
}
static inline void gp_thread_once(GPThreadOnce* flag, void(*init)(void))
{
    pthread_once(flag, init);
}

#endif // environment specific wrappers

#endif // GP_THREAD_INCLUDED
