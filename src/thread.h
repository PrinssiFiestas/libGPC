// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Portability wrappers for threading.

#ifndef GP_THREAD_INCLUDED
#define GP_THREAD_INCLUDED

#if __STDC_VERSION__ >= 201112L && !defined(__MINGW32__)
#include <threads.h>
#include <stdatomic.h>
#else
#include <pthread.h>
#endif

// Use this only when thread local storage is desirable but not necessary.
#ifdef _MSC_VER
#define GP_MAYBE_THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define GP_MAYBE_THREAD_LOCAL _Thread_local
#elif defined(__GNUC__)
#define GP_MAYBE_THREAD_LOCAL __thread
#else
#define GP_MAYBE_THREAD_LOCAL
#endif

// Use this only when atomis are desirable but not necessary.
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#define GP_MAYBE_ATOMIC _Atomic
#else
#define GP_MAYBE_ATOMIC
#endif

#if __STDC_VERSION__ >= 201112L && \
    !defined(__MINGW32__)       && \
    !defined(__STDC_NO_THREADS__)

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

#else // standard threads not supported, use POSIX threads

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
