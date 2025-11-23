// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/thread.h>

#ifndef _MSC_VER

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

// Pthreads uses routines of type void*(*)(void*), but the type should be
// int(*)(void*) for portability. However, if we just cast the routine to
// void*(*)(void*), calling it becomes undefined (incompatible return types), so
// wrapper is needed. // TODO we should just cast like libc anyway in x86.

typedef struct gp_thread_routine_wrapper
{
    int (*routine)(void* arg);
    void* arg;
} GPThreadRoutineWrapper;

// Thread local storage allows allocating wrapper to heap (for lifetime), which
// can be automatically free()d on thread exit.
static pthread_key_t  gp_s_thread_wrapper_key;
static pthread_once_t gp_s_thread_wrapper_once = PTHREAD_ONCE_INIT;

void* gp_internal_thread_wrapper_routine(void*_wrapper)
{
    GPThreadRoutineWrapper* wrapper = _wrapper;
    pthread_setspecific(gp_s_thread_wrapper_key, wrapper);
    return (void*)(intptr_t)(wrapper->routine(wrapper->arg));
}

static void gp_s_make_thread_wrapper_key(void)
{
    pthread_key_create(&gp_s_thread_wrapper_key, free);
}

void* gp_internal_thread_wrapper_arg(int(*f)(void* arg), void* arg)
{
    pthread_once(&gp_s_thread_wrapper_once, gp_s_make_thread_wrapper_key);

    GPThreadRoutineWrapper* wrapper = malloc(sizeof*wrapper);
    assert(wrapper);
    wrapper->routine = f;
    wrapper->arg     = arg;
    return wrapper;
}

#endif // GP_USE_PTHREADS
