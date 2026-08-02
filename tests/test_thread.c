// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Original: https://github.com/jtsiomb/c11threads/blob/master/test/test.c

#include "../src/gpthread.c"
#include <gpc/gpassert.h>
#include <stdio.h>

#define CHK_THRD_EXPECTED(a, b) gp_assert((a) == (b))
#define CHK_THRD(a) gp_assert(a)
#define CHK_EXPECTED(a, b) gp_assert((a) == (b))
#define NUM_THREADS 8

GPMutex mtx;
GPMutex mtx2;
GPCond cnd;
GPCond cnd2;
GPThreadKey tss;
GPOnce once = GP_ONCE_INIT;
int flag;

void run_thread_test(void);
void run_timed_mtx_test(void);
void run_cnd_test(void);
void run_tss_test(void);
void run_call_once_test(void);

int main(void)
{
    gp_suite("Threading");
    {
        gp_test("thread");
        run_thread_test();

        gp_test("timed mutex");
        run_timed_mtx_test();

        gp_test("condvar");
        run_cnd_test();

        gp_test("thread-specific storage");
        run_tss_test();

        gp_test("call once");
        run_call_once_test();
    }
}

int tfunc(void *arg)
{
    int num = (int)(size_t)arg;

    printf("\t\thello from thread %d\n", num);

    gp_sleep(1.0);

    printf("\t\tthread %d done\n", num);
    return 0;
}

void run_thread_test(void)
{
    int i;
    GPThread threads[NUM_THREADS];

    for (i = 0; i < NUM_THREADS; i++) {
        CHK_THRD(gp_thread_create(threads + i, tfunc, (void*)(size_t)i));
    }
    for (i = 0; i < NUM_THREADS; i++) {
        CHK_THRD(gp_thread_join(threads[i], NULL));
    }
}

#if !defined(_WIN32) || defined(C11THREADS_PTHREAD_WIN32) || !defined(C11THREADS_OLD_WIN32API)
int hold_mutex_for_one_second(void* arg)
{
    (void)arg;

    gp_mutex_lock(&mtx);

    gp_mutex_lock(&mtx2);
    flag = 1;
    gp_cond_signal(&cnd);
    gp_mutex_unlock(&mtx2);

    gp_sleep(1.0);
    gp_mutex_unlock(&mtx);
    return 0;
}

void run_timed_mtx_test(void)
{
    GPThread thread;

    gp_mutex_init(&mtx);
    gp_mutex_init(&mtx2);
    gp_cond_init(&cnd);
    flag = 0;

    CHK_THRD(gp_thread_create(&thread, hold_mutex_for_one_second, NULL));

    gp_mutex_lock(&mtx2);
    while (!flag) {
        gp_cond_wait(&cnd, &mtx2);
    }
    gp_mutex_unlock(&mtx2);
    gp_cond_destroy(&cnd);
    gp_mutex_destroy(&mtx2);

    CHK_THRD_EXPECTED(gp_mutex_timedlock(&mtx, 0.5), false);
    puts("\t\tthread has locked mutex & we timed out waiting for it");

    gp_sleep(1.0);

    CHK_THRD(gp_mutex_timedlock(&mtx, 0.5));
    puts("\t\tthread no longer has mutex & we grabbed it");
    gp_mutex_unlock(&mtx);
    gp_mutex_destroy(&mtx);
    CHK_THRD(gp_thread_join(thread, NULL));
}
#endif

int my_cnd_thread_func(void *arg)
{
    int thread_num;
    thread_num = (int)(size_t)arg;
    gp_mutex_lock(&mtx);
    ++flag;
    gp_cond_signal(&cnd2);
    do {
        gp_cond_wait(&cnd, &mtx);
        printf("\t\tthread %d: woke up\n", thread_num);
    } while (flag <= NUM_THREADS);
    printf("\t\tthread %d: flag > NUM_THREADS; incrementing flag and exiting\n", thread_num);
    ++flag;
    gp_cond_signal(&cnd2);
    gp_mutex_unlock(&mtx);
    return 0;
}

void run_cnd_test(void)
{
    int i;
    GPThread threads[NUM_THREADS];

    flag = 0;
    gp_mutex_init(&mtx);
    gp_cond_init(&cnd);
    gp_cond_init(&cnd2);

    for (i = 0; i < NUM_THREADS; i++) {
        CHK_THRD(gp_thread_create(threads + i, my_cnd_thread_func, (void*)(size_t)i));
    }

    gp_mutex_lock(&mtx);
    while (flag != NUM_THREADS) {
        gp_cond_wait(&cnd2, &mtx);
    }
    gp_mutex_unlock(&mtx);
    puts("\t\tmain thread: threads are ready");

    /* No guarantees, but this might unblock a thread. */
    puts("\t\tmain thread: gp_cond_signal()");
    gp_cond_signal(&cnd);
    gp_sleep(0.5);

    /* No guarantees, but this might unblock all threads. */
    puts("\t\tmain thread: gp_cond_broadcast()");
    gp_cond_broadcast(&cnd);
    gp_sleep(0.5);

    gp_mutex_lock(&mtx);
    flag = NUM_THREADS + 1;
    gp_mutex_unlock(&mtx);
    puts("\t\tmain thread: set flag to NUM_THREADS + 1");

    /* No guarantees, but this might unblock two threads. */
    puts("\t\tmain thread: sending gp_cond_signal() twice");
    gp_cond_signal(&cnd);
    gp_cond_signal(&cnd);
    gp_sleep(0.5);

    gp_mutex_lock(&mtx);
    while (flag == NUM_THREADS + 1) {
        gp_cond_wait(&cnd2, &mtx);
    }
    gp_mutex_unlock(&mtx);

    puts("\t\tmain thread: woke up, flag != NUM_THREADS + 1; sending gp_cond_broadcast() and joining threads");
    gp_cond_broadcast(&cnd);
    for (i = 0; i < NUM_THREADS; i++) {
        CHK_THRD(gp_thread_join(threads[i], NULL));
    }

    gp_cond_destroy(&cnd2);
    gp_cond_destroy(&cnd);
    gp_mutex_destroy(&mtx);
}

void my_tss_dtor(void *arg)
{
    printf("\t\tdtor: content of tss: %d\n", (int)(size_t)arg);
    CHK_EXPECTED((int)(size_t)arg, 42);
}

int my_tss_thread_func(void *arg)
{
    void *tss_content;

    (void)arg;

    tss_content = gp_thread_local_get(tss);
    printf("\t\tthread func: initial content of tss: %d\n", (int)(size_t)tss_content);
    gp_thread_local_set(tss, (void*)42);
    tss_content = gp_thread_local_get(tss);
    printf("\t\tthread func: content of tss now: %d\n", (int)(size_t)tss_content);
    CHK_EXPECTED((int)(size_t)tss_content, 42);
    return 0;
}

void run_tss_test(void)
{
    GPThread thread;

    CHK_THRD(gp_thread_local_create(&tss, my_tss_dtor));
    CHK_THRD(gp_thread_create(&thread, my_tss_thread_func, NULL));
    CHK_THRD(gp_thread_join(thread, NULL));
    gp_thread_local_delete(tss);
}

void my_call_once_func(void)
{
    puts("\t\tmy_call_once_func() was called");
    ++flag;
}

int my_call_once_thread_func(void *arg)
{
    (void)arg;
    puts("\t\tmy_call_once_thread_func() was called");
    gp_call_once(&once, my_call_once_func);
    return 0;
}

void run_call_once_test(void)
{
    int i;
    GPThread threads[NUM_THREADS];

    flag = 0;

    for (i = 0; i < NUM_THREADS; i++) {
        CHK_THRD(gp_thread_create(threads + i, my_call_once_thread_func, NULL));
    }

    for (i = 0; i < NUM_THREADS; i++) {
        CHK_THRD(gp_thread_join(threads[i], NULL));
    }

    printf("\t\tcontent of flag: %d\n", flag);

    CHK_EXPECTED(flag, 1);
}
