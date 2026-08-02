// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Based on https://github.com/jtsiomb/c11threads.
// Authors:
//   John Tsiombikas <nuclear@member.fsf.org> - original POSIX threads wrapper
//   Oliver Old <oliver.old@outlook.com> - win32 implementation

#ifndef GP_THREAD_INCLUDED
#define GP_THREAD_INCLUDED 1

#include <gpc/gpattributes.h>
#include <gpc/gpassert.h>
#include <gpc/gptime.h>
#include <gpc/gpint128.h>

#if defined(GP_TARGET_OS_WINDOWS) && !defined(GP_USE_PTHREADS)
#define GP_USE_WINTHREADS 1
#else
#include <pthread.h>
#include <string.h> // strerror
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
/// #include <gpc/gpthread.h>
/// @endcode
/// Portable threading API based on [C11 threads API](https://en.cppreference.com/c/header/threads),
/// including threads, mutual exclusion, condition variables, and thread local
/// storage. Implementation is based on [c11threads](https://github.com/jtsiomb/c11threads).
///
/// At the time of writing, GNU libc does not implement standard threading on
/// Windows. Microsoft UCRT only added standard threading recently, but anyway
/// it's C11 and we are supposed to support C99 and C++. This module is a thin
/// wrapper over POSIX threads if not Windows, otherwise uses Win32 threads in
/// it's implementation. POSIX threads can be forced with @ref GP_USE_PTHREADS.
///
/// Our changes to C11 API:
///
/// - Added static initializers for mutexes.
/// - Added static initializers for condition variables.
/// - Added @ref GP_THREAD_LOCAL_MAX_SLOTS.
/// - C11 `mtx_timedlock()` and `cnd_timedwait()` and their POSIX counterparts
///   take calendar timestamp as the timeout bound argument, but Win32
///   counterparts use relative time instead. We added functions for both.
/// - Removed mutex types, only plain is available. This is due to `mtx_timed`
///   being implicit and redundant and `mtx_recursive` is a code smell that
///   doesn't work well portably with static initialization, which is much more
///   important.
/// - Replaced `struct timespec` with seconds and nanoseconds. Nobody likes to
///   deal with `struct timespec`.
/// - Removed `thrd_sleep()`, we already have @ref gp_sleep() and @ref gp_sleep_ns()
///   in our timing utilities.
/// - Simplified error handling. C11 defines multiple error constants for
///   different conditions. Some errors do not occur in our target platforms,
///   some cannot be handled without causing UB down the line, and others would
///   be difficult and/or unnecessary to distinguish in a portable manner. This
///   only left us with zero to one error condition per function, so we just
///   return a boolean for success status or `void`.
/// - The sheer amount of changes mean that this API is significantly different
///   from the C11 standard API. Therefore, we changed all names to follow our
///   naming conventions and to make a clear distinction between the APIs.
/// @{

// Our changes to the original [c11threads](https://github.com/jtsiomb/c11threads)
// implementation:
// - Any changes required by changes described above.
// - Pthreads wrappers are practically rewritten from scracth. Nothing wrong
//   with the original, but the wrapping was so thin that the other changes
//   just happened to cause an almost full rewrite.
// - Rewrote Win32 mutex implementation to use newer SRW locks instead of older
//   critical section objects. SRW locks are faster and support static
//   initialization. Unfortunately, this required dropping Windows XP support (RIP).
// - Internal critical section objects were also replaced with SRW locks.
// - Removed Windows XP support. Loading kernel32.dll functions manually is no
//   longer necessary. In fact, no initialization is necessary since internal
//   mutexes were changed to SRW locks that can be initialized statically.
// - With no Windows XP support, `WINNT` stuff is no longer necessary. Also had
//   to remove `WIN32_LEAN_AND_MEAN` and `_CRTDBG_MAP_ALLOC` for single header
//   users. All `if (winver < VISTA)` branches were removed.
// - Since we don't use `struct timespec`, anything using them had to be
//   rewritten. Related helpers were removed.
// - Some UB pointer casts were hacked away using @ref gp_launder(). That's the
//   best we can do without including `windows.h` in header files, which might
//   break user builds (namespace pollution like `min` and include order
//   problems).
// - Removed 32-bit internal timespec, use 64 bits always with C99 `uint64_t`.
//   All 32-bit specific functions were removed, they are not needed even for
//   32-bit builds.
// - Removed Win32 specific functions like register and cleanup.
// - Cleanup now runs automatically at program exit.

#ifdef GP_DOXYGEN
/** Use POSIX threads.
 *
 * Should be defined globally if needed.
 *
 * By default on Windows, this library uses Win32 threads. This macro forces
 * using POSIX threads instead.
 */
#define GP_USE_PTHREADS
#endif

// ----------------------------------------------------------------------------
#if !defined(GP_USE_WINTHREADS) || defined(GP_DOXYGEN) // use POSIX threads

#include <pthread.h>
#include <sched.h> // sched_yield
#include <sys/time.h>
#include <stdint.h>
#include <errno.h> // IWYU pragma: keep // "unused include" no it's not (EBUSY)??? clangd flipping

// ------------------------------------
/// @defgroup thread_management Thread Management
/// @{

/** Opaque thread identifier.
 *
 * Unique identifier assigned to thread on call to @ref gp_thread_create(). The
 * ID is only unique within a process and can be reused once a thread
 * terminates. Trying to use an ID of a terminated thread is undefined. The ID
 * of the current thread can be fetched using @ref gp_thread_current().
 *
 * The underlying type might be an integer or it can be a structure, so
 * portable application should compare IDs using @ref gp_thread_equal() if
 * needed.
 */
#ifdef GP_DOXYGEN
typedef __unspecified__ GPThread;
#else
typedef pthread_t GPThread;
#endif

/** Create thread.
 *
 * Creates a new thread, whose ID will be stored to @ref thread, which is used
 * to refer to that thread. The new thread starts execution by calling @a routine.
 * @a optional_arg will be passed to the given routine.
 *
 * To release resourced used by threads, one must either join a thread using
 * @ref gp_thread_join() or detach it using @ref gp_thread_detach().
 *
 * The thread terminates once @a routine returns, calls @ref gp_thread_exit(),
 * or once the process terminates. The return value of @a routine or value
 * passed to @ref gp_thread_exit() can be obtained with @ref gp_thread_join().
 *
 * @return `true` on success, `false` if not enough resources to create a thread.
 */
GP_API GP_NONNULL_ARGS(1, 2)
bool gp_thread_create(
    GPThread* thread, int(*routine)(void*), void* optional_arg);

/** Exit current thread.
 *
 * Terminates current thread as if it was terminated by returning from routine
 * passed to @ref gp_thread_create().
 *
 * If the calling thread is the last thread running, then the process exits
 * with an exit status of zero, which is equivalent of calling `exit(0)`.
 */
GP_INLINE void gp_thread_exit(int return_value)
{
    pthread_exit((void*)(intptr_t)return_value);
}

/** Wait for given thread to finish and collect it's result.
 *
 * Wait for @a thread to terminate. The return value of the given thread will
 * be stored to @a optional_result if not `NULL`. The given thread must not be
 * detached and multiple threads must not attempt to join the same thread.
 *
 * @return `true` on success, `false` if @a thread is not joinable or not found.
 */
GP_INLINE bool gp_thread_join(GPThread thread, int *optional_result)
{
    void *retval;
    if (pthread_join(thread, &retval) != 0)
        return false;
    if (optional_result)
        *optional_result = (int)(intptr_t)retval;
    return true;
}

/** Detach thread.
 *
 * Detaches @a thread, which makes it unjoinable. Detaching a detached thread is
 * undefined. Resources allocated for the thread are freed once @a thread
 * exits.
 *
 * The thread ID @a thread should be considered freed and not be used after
 * detaching. The ID may be reused for other threads by the implementation.
 *
 * @return `true` on success, `false` if @a thread is not joinable or not found.
 */
GP_INLINE bool gp_thread_detach(GPThread thread)
{
    return ! pthread_detach(thread);
}

/** Get thread identifier of current thread.
 *
 * @return thread identifier of current thread, which is the same as the one
 * outputted by @ref gp_thread_create() when the thread was created.
 */
GP_INLINE GPThread gp_thread_current(void)
{
    return pthread_self();
}

/** Compare threads identifiers.
 *
 * The underlying type of @ref GPThread is unspecified and can be a structure
 * on some implementations. Therefore, portable applications should compare
 * thread identifiers using this function instead of `==` operator.
 */
GP_INLINE bool gp_thread_equal(GPThread a, GPThread b)
{
    return pthread_equal(a, b);
}

/** Yield execution to another thread.
 *
 * Hint the scheduler that other threads can run. Which thread gets ran next is
 * unspecified and can be the calling thread.
 */
GP_INLINE void gp_thread_yield(void)
{
    sched_yield();
}

/// @}
// ------------------------------------
/// @defgroup mutexes Mutexes
/// @{

/** Opaque mutex identifier.
 *
 * A mutex (mutual exclusion) is used to limit access of code segments and
 * shared data to a single thread at a time.
 *
 * Can be initialized statically by using @ref GP_MUTEX_INIT or
 * dynamically using @ref gp_mutex_init(). Dynamically created mutexes can be
 * destroyed using @ref gp_mutex_destroy().
 *
 * At the time of writing, only plain mutexes are supported. C11 `mtx_timed` is
 * redundant, timing is supported by default. C11 `mtx_recursive` is unnecessary
 * code smell that prevents static initialization, which is much more important.
 */
#ifdef GP_DOXYGEN
typedef __unspecified__ GPMutex;
#else
typedef pthread_mutex_t GPMutex;
#endif

/** Static mutex initialzer.
 *
 * Example:
 * @code
 * static GPMutex mutex = GP_MUTEX_INIT;
 * @endcode
 */
#ifdef GP_DOXYGEN
#  define GP_MUTEX_INIT /* unspecified */
#elif defined(GP_TARGET_DEBUG) && defined(_GNU_SOURCE)
#  define GP_MUTEX_INIT PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#else
#  define GP_MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#endif

/** Create mutex dynamically.
 *
 * Dynamically created mutexes must be destroyed using @ref gp_mutex_destroy().
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_mutex_init(GPMutex *mutex)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    #ifdef GP_TARGET_DEBUG
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    #else // use fast mutex for release builds. This also matches Win32.
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    #endif
    pthread_mutex_init(mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

/** Deallocate mutex resources.
 *
 * If @a optional_mutex is locked, then behavior is undefined.
 */
GP_INLINE void gp_mutex_destroy(GPMutex* optional_mutex)
{
    // Passing NULL makes no sense, but we check it anyway for consistency with
    // other destructors in our library (NULL always accepted by destructors).
    if (optional_mutex != NULL)
    #ifdef GP_TARGET_DEBUG // enforce proper usage for portability.
        gp_assert(pthread_mutex_destroy(optional_mutex) == 0, strerror(EBUSY));
    #else // asserting too harsh, pthread_mutex_destroy() is no-op in many implementations.
        pthread_mutex_destroy(optional_mutex);
    #endif
}

/** Lock mutex.
 *
 * Locks the @a mutex lock. If the lock is unlocked at the time of calling, the
 * calling thread can becomes the owner of the lock and the function returns
 * without blocking.
 *
 * If @a mutex was already locked by another thread, then calling thread will be
 * blocked until the thread owning the lock calls @ref gp_mutex_unlock(). After
 * that, the calling thread becomes the owner the lock until it calls
 * @ref gp_mutex_unlock().
 *
 * If @a mutex was already locked by the calling thread, then the calling thread
 * will be blocked causing a deadlock, which is undefined behavior by the C11
 * standard and cannot be meaningfully handled.
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_mutex_lock(GPMutex *mutex)
{
    int error = pthread_mutex_lock(mutex);
    gp_assume( ! error, strerror(error)); // deadlock probably
}

/** Try to lock mutex without blocking.
 *
 * Like @ref gp_mutex_lock(), except doesn't block when the mutex is already
 * locked.
 *
 * @return `true` if acquired lock ownership, `false` otherwise.
 */
GP_NONNULL_ARGS() GP_INLINE
bool gp_mutex_trylock(GPMutex *mtx)
{
    return ! pthread_mutex_trylock(mtx);
}

/** Try to lock mutex with a timeout in seconds.
 *
 * Like @ref gp_mutex_lock(), except only blocks for the maximum of @a time
 * amount of seconds.
 *
 * It is not an error to pass non-zero negative time, in such case the function
 * returns immediately. It is also not an error to pass `INFINITY`, in such case
 * the timeout is ignored and the call is equivalent to calling
 * @ref gp_mutex_lock(). Passing `NAN` is undefined.
 *
 * @return `true` if acquired lock ownership, `false` if timeout expired or
 * negative number passed.
 */
GP_API GP_NONNULL_ARGS()
bool gp_mutex_timedlock(GPMutex* mutex, double time);

/** Try to lock mutex with a timeout in nanoseconds.
 *
 * Like @ref gp_mutex_lock(), except only blocks for the maximum of @a time_ns
 * amount of nanoseconds. It is not an error to pass negative size, in such case
 * the function returns immediately.
 *
 * @return `true` if acquired lock ownership, `false` if timeout expired or
 * negative number passed.
 */
GP_API GP_NONNULL_ARGS()
bool gp_mutex_timedlock_ns(GPMutex* mutex, int64_t time_ns);

/** Try to lock mutex until the given absolute time point.
 *
 * Like @ref gp_mutex_lock(), except only blocks until a given absolute time
 * (time since epoch in nanoseconds). Current absolute time can be obtained
 * using @ref gp_time_begin().
 *
 * @return `true` if acquired lock ownership, `false` if timeout expired.
 */
GP_API GP_NONNULL_ARGS()
bool gp_mutex_timedlock_absolute(GPMutex* mutex, GPInt128 time_point_ns);

/** Unlock mutex.
 *
 * Unlocks @a mutex lock, which should have been locked by the calling thread.
 * Trying to unlock a lock owner by another thread is undefined.
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_mutex_unlock(GPMutex* mutex)
{
    int error = pthread_mutex_unlock(mutex);
    gp_assume( ! error, strerror(error));
}

/// @}
// ------------------------------------
/// @defgroup condition_variables Condition Variables
///
/// Condition variables are synchronization primitives used to make threads wait
/// until a given condition is met. The three operations for condition variables
/// are:
/// - Wait (@ref gp_cond_wait()): Sleep until signaled.
/// - Signal (@ref gp_cond_signal()): Wake up potentially sleeping threads.
/// - Broadcast (@ref gp_cond_broadcast()): Wake up all potentially sleeping threads.
/// Condition variables work together with mutexes to synchronize any arbitrary
/// data. The condition itself is any arbitrary condition.
/// @{

/** Opaque condition variable identifier.
 *
 * Used in conjunction with @ref GPMutex to wait until a condition is met.
 *
 * Can be initialized statically by using @ref GP_COND_INIT or
 * dynamically using @ref gp_cond_init(). Dynamically created condition
 * variables can be destroyed using @ref gp_cond_destroy().
 */
#ifdef GP_DOXYGEN
typedef __unspecified__ GPCond;
#else
typedef pthread_cond_t GPCond;
#endif

/** Static condition variable initialzer.
 *
 * Example:
 * @code
 * static GPCond mutex = GP_COND_INIT;
 * @endcode
 */
#ifdef GP_DOXYGEN
#  define GP_COND_INIT /* unspecified */
#else
#  define GP_COND_INIT PTHREAD_COND_INITIALIZER
#endif

/** Create condition variable dynamically.
 *
 * Dynamically created condition variables must be destroyed using
 * @ref gp_cond_destroy().
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_cond_init(GPCond* cond)
{
    pthread_cond_init(cond, 0);
}

/** Deallocate condition variable resources.
 *
 * If a thread is waiting on @a optional_cond, then behavior is undefined.
 */
GP_INLINE void gp_cond_destroy(GPCond* optional_cond)
{
    // Passing NULL makes no sense, but we check it anyway for consistency with
    // other destructors in our library (NULL always accepted by destructors).
    if (optional_cond != NULL)
    #ifdef GP_TARGET_DEBUG // enforce proper usage for portability.
        gp_assert(pthread_cond_destroy(optional_cond) == 0, strerror(EBUSY));
    #else // asserting too harsh, pthread_cond_destroy() is no-op in many implementations.
        pthread_cond_destroy(optional_cond);
    #endif
}

/** Wait on condition variable to be signaled.
 *
 * Atomically unlocks @a mutex and blocks until @a cond is signaled by calling
 * @ref gp_cond_signal() or @ref gp_cond_broadcast() by another thread. @a mutex
 * must be locked by the calling thread. @a mutex will be atomically locked on
 * return with the ownership returned to the calling thread.
 *
 * This function may return early due to a spurious wake-up. Therefore, the
 * function should be called in a loop that repeatedly checks the predicate.
 *
 * ### Example
 * @code
 * extern GPMutex my_mutex;
 * extern GPCond my_cond
 * extern struct data* my_data;
 *
 * void wait_for_data(void)
 * {
 *     gp_mutex_lock(&my_mutex);
 *     while (my_data->predicate != DATA_READY)
 *         gp_cond_wait(&my_cond, &my_mutex);
 *     my_data->consume(&my_data);
 *     gp_mutex_unlock(&my_mutex);
 * }
 * @endcode
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_cond_wait(GPCond* cond, GPMutex* mutex)
{
    pthread_cond_wait(cond, mutex);
}

/** Wait on condition variable with a timeout in seconds.
 *
 * Like @ref gp_cond_wait(), except only blocks for maximum of @a time amount of
 * seconds.
 *
 * It is not an error to pass non-zero negative time, in such case the function
 * returns immediately. It is also not an error to pass `INFINITY`, in such case
 * the timeout is ignored and call is equivalent to calling @ref gp_cond_wait().
 * Passing `NAN` is undefined.
 *
 * @return `true` if wake-up (spurious or signaled) happened before timeout,
 * `false`if timeout expired or negative number passed.
 */
GP_API GP_NONNULL_ARGS()
bool gp_cond_timedwait(GPCond* cond, GPMutex* mutex, double time);

/** Wait on condition variable with a timeout in nanoseconds.
 *
 * Like @ref gp_cond_wait(), except only blocks for maximum of @a time_ns amount
 * of seconds.
 *
 * It is not an error to pass non-zero negative time, in such case the function
 * returns immediately.
 *
 * @return `true` if wake-up (spurious or signaled) happened before timeout,
 * `false`if timeout expired or negative number passed.
 */
GP_API GP_NONNULL_ARGS()
bool gp_cond_timedwait_ns(GPCond* cond, GPMutex* mutex, int64_t time_ns);

/** Wait on condition variable until the given absolute time point.
 *
 * Like @ref gp_cond_wait(), except only blocks until a given absolute time
 * (time since epoch in nanoseconds). Cureent absolute time can be obtained
 * using @ref gp_time_begin().
 *
 * @return `true` if wake-up (spurious or signaled) happened before timeout,
 * `false` if timeout expired.
 */
GP_API GP_NONNULL_ARGS()
bool gp_cond_timedwait_absolute(
    GPCond* cond, GPMutex* mutex, GPInt128 time_point_ns);

/** Wake a single thread waiting on condition variable.
 *
 * Wakes a single thread potentially waiting on @ref gp_cond_wait() or its
 * timeout equivalents. If no thread is waiting, then nothing happens. The
 * waiting thread will not wake up immediately if the calling thread has locked
 * the mutex passed to @ref gp_cond_wait(), it only wakes once the mutex is
 * unlocked by the calling thread.
 *
 * Threads are *not* necessarily woken up in the order they went sleeping.
 *
 * ### Example
 * @code
 * extern GPMutex my_mutex;
 * extern GPCond my_cond;
 * extern struct data* my_data;
 *
 * void send_data(const void* data, size_t data_size)
 * {
 *     gp_mutex_lock(&my_mutex);
 *     my_data->write(my_data, data, data_size);
 *     my_data->predicate = DATA_READY;
 *     gp_cond_signal(&my_cond);
 *     gp_mutex_unlock(&my_mutex);
 * }
 * @endcode
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_cond_signal(GPCond* cond)
{
    pthread_cond_signal(cond);
}

/** Wake all threads waiting on condition variable.
 *
 * Wakes all threads potentially waiting on @ref gp_cond_wait() or its timeout
 * equivalents. If no thread is waiting, then nothing happens. The waiting
 * threads will not wake up immediately if the calling thread has locked the
 * mutex passed to @ref gp_cond_wait(). They also do not wake simultaneously;
 * the first thread to be woken up wakes after the calling thread unlocks the
 * mutex. The waking thread gets the ownership of the mutex and has to unlock it
 * before the next thread wakes up. Subsequent threads wake up one by one in a
 * similar manner.
 *
 * Threads are *not* necessarily woken up in the order they went sleeping.
 *
 * ### Example
 * @code
 * void barrier(void)
 * {
 *     static GPMutex mutex = GP_MUTEX_INIT;
 *     static GPCond cond = GP_COND_INIT;
 *     static size_t threads_waiting = 0;
 *
 *     gp_mutex_lock(&mutex);
 *     threads_waiting++;
 *     if (threads_waiting < NUM_THREADS)
 *         while (threads_waiting < NUM_THREADS)
 *             gp_cond_wait(&cond, &mutex);
 *     else {
 *         threads_waiting = 0;
 *         gp_cond_broadcast(&cond);
 *     }
 *     gp_mutex_unlock(&mutex);
 * }
 * @endcode
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_cond_broadcast(GPCond* cond)
{
    pthread_cond_broadcast(cond);
}

/// @}
// ------------------------------------
/// @defgroup thread_local_storage Thread Local Storage
///
/// Thread local storage is global/static data that is unique to each thread.
/// Its main use is improving thread safety by limiting access of globals to
/// each thread by having separate memory slots for the given global per thread.
/// Runtime thread local storage supports automatic resource management on
/// thread exit using a user defined destructor.
///
/// A global/static variable can be declared thread local using @ref GP_THREAD_LOCAL
/// and @ref GP_MAYBE_THREAD_LOCAL. These macros expand to portably expand to
/// `thread_local` keyword or an equivalent compiler extension. Variables
/// declared as thread local will be automatically allocated and accessed by
/// the compiler.
///
/// Thread local objects can also be created during runtime using @ref GPThreadKey
/// and `gp_thread_local_*` family of functions. Unlike variables declared with
/// @ref GP_THREAD_LOCAL keyword, there are only finite number of runtime thread
/// local variables that can be allocated. However, runtime thread local
/// variables support destructors that perform automatic cleanup when threads
/// exits.
/// @{

/** Sometimes thread local.
 *
 * Expands to a variant of `thread_local` keyword if supported or nothing if not
 * supported. Support can be checked by checking if @ref GP_HAS_THREAD_LOCAL is
 * defined.
 *
 * Use this only when thread local storage would be ideal but not necessary for
 * correctness. Example use might be something logging related that is meant for
 * developers, but end users might not care about.
 */
#if ((__STDC_VERSION__ >= 202311L && !defined(__STDC_NO_THREADS__)) \
     || (defined(__cplusplus) && __cplusplus >= 201103L)) && !defined(__MINGW32__)
#  define GP_MAYBE_THREAD_LOCAL thread_local
#  define GP_HAS_THREAD_LOCAL 1
#elif defined(_MSC_VER)
#  define GP_MAYBE_THREAD_LOCAL __declspec(thread)
#  define GP_HAS_THREAD_LOCAL 1
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__) && !defined(__MINGW32__)
#  define GP_MAYBE_THREAD_LOCAL /* sometimes */_Thread_local
#  define GP_HAS_THREAD_LOCAL 1
#elif defined(__GNUC__) && !defined(__MINGW32__)
#  define GP_MAYBE_THREAD_LOCAL __thread
#  define GP_HAS_THREAD_LOCAL 1
#else
#  define GP_MAYBE_THREAD_LOCAL
#endif

#ifdef GP_HAS_THREAD_LOCAL
#  define GP_THREAD_LOCAL GP_MAYBE_THREAD_LOCAL
#elif defined(GP_DOXYGEN)
/** Check if a variant of `thread_local` keyword exist.
 *
 * Currently defined with MSVC regardless of C/C++ version and *not* defined
 * with MinGW regardless of C/C++ version due to a [bug](https://sourceforge.net/p/mingw-w64/bugs/445/)
 * that makes it completely unusable. Defined with GCC and Clang (if not MinGW)
 * regardless of C/C++ version. For any other compiler, defined if C11, C++11,
 * or higher.
 */
#  define GP_HAS_THREAD_LOCAL /* implementation defined */

/** Always thread local.
 *
 * Expands to a variant of `thread_local` keyword if supported or nothing if not
 * supported. Support can be checked by checking if @ref GP_HAS_THREAD_LOCAL is
 * defined.
 */
#  define GP_THREAD_LOCAL /* implementation defined */
#endif

/** Key to thread local storage.
 *
 * Key to load/store data from/to thread local storage. In most common
 * implementations, this is just an integer index to a thread local array of
 * pointers. Nevertheless, the type should not be assumed and should be treated
 * as opaque.
 *
 * Keys and thread local data are often created on demand using @ref GPOnce.
 * This can be used to create thread local scratch allocators (like we do with
 * @ref GPScope) or error strings with automatic memory management.
 *
 * ### Example
 *
 * This example demonstrates how to implement a function similar to `strerror()`.
 * @code
 * static GPThreadKey error_str_key;
 * static GPOnce error_str_once = GP_ONCE_INIT;
 * #define BUF_SIZE 64
 *
 * static void error_str_init(void)
 * {
 *     if ( ! gp_thread_local_create(&error_str_key, free))
 *         return; // gp_thread_local_get() will return NULL, handle later.
 *     char* buf = malloc(BUF_SIZE); // may return NULL, handle later.
 *     gp_thread_local_set(error_str_key, buf);
 *     // Note: free() was set as our destructor, it will deallocate buf when
 *     // thread exits.
 * }
 *
 * char* error_str(enum my_error error_code)
 * {
 *     static char backup_buf[BUF_SIZE]; // in case of failures
 *
 *     gp_call_once(&error_str_once, error_str_init);
 *     char* buf = gp_thread_local_get(error_str_key);
 *     if (buf == NULL) // either gp_thread_local_create() or malloc() failed.
 *         buf = backup_buf; // not thread safe, but will do for debugging.
 *
 *     switch (error_code) {
 *     case NO_ERROR:              strcpy(buf, "No error."); break;
 *     case BAD_THINGS_HAPPENED:   strcpy(buf, "Bad things happened."); break;
 *     case WORSE_THINGS_HAPPENED: strcpy(buf, "Worse things happened."); break;
 *     case WE_ARE_ALL_DOOMED:     strcpy(buf, "We are all doomed."); break;
 *     }
 *     return buf;
 * }
 * @endcode
 */
#ifdef GP_DOXYGEN
typedef __unspecified__ GPThreadKey;
#else
typedef pthread_key_t GPThreadKey;
#endif

/** Maximum number of thread local storage slots.
 *
 * The number of available slots is platform specific. On Windows, the number is
 * 1088. POSIX guarantees that the number is at least 128, but the smallest
 * we could find for desktop or mobile was on [FreeBSD with 256](https://nmsl.cs.nthu.edu.tw/wp-content/uploads/2011/09/images_courses_CS5432_2016_12-threadctrl.pdf)
 * slots. Linux has 1024 slots. This library uses one slot if @ref GPScope
 * allocator is ever used.
 */
#ifdef GP_DOXYGEN
#  define GP_THREAD_LOCAL_MAX_SLOTS /* implementation defined */
#else
#  define GP_THREAD_LOCAL_MAX_SLOTS PTHREAD_KEYS_MAX
#endif

/** Maximum number of times a thread local storage destructor is called.
 *
 * Thread local storage destructors are only called once by default. However,
 * if @ref gp_thread_local_set() is used in a destructor to set new values, then
 * the destructor might be called again.
 */
#ifdef GP_DOXYGEN
#  define GP_THREAD_LOCAL_DESTRUCTOR_ITERATIONS /* unspecified */
#else
#  define GP_THREAD_LOCAL_DESTRUCTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS
#endif

/** Allocate thread local slot and create a key to access it.
 *
 * Allocates thread local slot and creates a key to access it, which will be
 * stored to @a key. There are only @ref GP_THREAD_LOCAL_MAX_SLOTS number of
 * slots available and the function will fail if all of them are used. In such
 * case @a key is left unspecified. The key is shared between threads.
 *
 * Thread local data pointer will be initialized with `NULL`. Use
 * @ref gp_thread_local_set() to store data to thread local storage.
 *
 * If @a optional_destructor is not `NULL`, and a non `NULL` value has been
 * stored to thread local slot using @ref gp_thread_local_set(), and @a key has
 * not been deallocated using @ref gp_thread_local_delete(), then @a optional_destructor
 * will be called for each thread when they exit. In such case, the value stored to
 * thread's thread local storage slot on call to @ref gp_thread_local_set() will
 * be passed as the argument for the destructor. Calling this function from the
 * destructor is undefined.
 *
 * @return `true` on success, `false` if maximum number of thread local storage
 * slots have been allocated.
 */
GP_INLINE GP_NONNULL_ARGS(1)
bool gp_thread_local_create(GPThreadKey* key, void(*optional_destructor)(void*))
{
    return ! pthread_key_create(key, optional_destructor);
}

/** Deallocate thread local storage slot.
 *
 * Deallocates thread local storage slot and allows the given key to be reused.
 *
 * A destructor potentially registered for @a key will not be called. This is
 * because there is no way for the calling thread to call the destructor for
 * other threads reliably. Therefore, it is not recommended to call this before
 * all threads potentially using @a key have finished execution.
 */
GP_INLINE void gp_thread_local_delete(GPThreadKey key)
{
    pthread_key_delete(key);
}

/** Store value to thread local storage slot.
 *
 * Stores the given value to the thread local storage slot associated with @a key.
 * The value can be later retrieved using @ref gp_thread_local_get().
 */
GP_INLINE void gp_thread_local_set(GPThreadKey key, const void* optional_value)
{
    int error = pthread_setspecific(key, optional_value);
    // pthread_setspecific() errors according to POSIX:
    // - ENOMEM: Doesn't happen: Implementations we support use preallocated block.
    // - EINVAL: Very rare, but possible. The most likely scenarios are:
    //   - uninitialized key,
    //   - key deleted (basically use after free),
    //   - sloppy memset() or similar that overwrote key in a struct.
    //   All of those conditions are clearly programming mistakes that have to
    //   be fixed, not "handled". Continuing execution would anyway cause mayhem
    //   since user tries to store data to undefined location, so not only UB
    //   already happened, but also later they would anyway dereference NULL or
    //   propagate the bug further. Good news is that this error doesn't
    //   practically happen for any remotely halfway decent use of TLS.
    gp_assert( ! error, strerror(error));
}

/** Access thread local storage slot.
 *
 * @return the value stored the calling thread's thread local storage slot on
 * call to @ref gp_thread_local_set(), or `NULL` if no value has been stored or
 * @a key is invalid.
 */
GP_INLINE void* gp_thread_local_get(GPThreadKey key)
{
    return pthread_getspecific(key);
}

/// @}
// ------------------------------------
/// @defgroup call_once Call Once
///
/// @ref gp_call_once() is used for thread safe initialization. A common way to
/// initialize static objects might look as follows:
///
/// @code
/// static void* foo;
///
/// void* get_foo(void)
/// {
///     static bool initialized;
///     if ( ! initialized) {
///         initialized = true;
///         init_foo();
///     }
///     return foo;
/// }
/// @endcode
///
/// The code above is not correct in multi threaded code. It has a race
/// condition at `if ( ! initialized)`. There can be two threads both reading
/// the boolean flag to be `false` at the same time before the other thread sets
/// it to `true` which causes `init_foo()` to be ran twice. The correct way to
/// initialize global data in multi threaded code is to use @ref gp_call_once()
/// that makes sure that the given initialization function (`init_foo()` in this
/// case) really only ever gets called once regardless of how many threads call
/// `get_foo()`.
///
/// The example above corrected looks like as follows:
///
/// @code
/// static void* foo;
///
/// void* get_foo(void)
/// {
///     static GPOnce initialized = GP_ONCE_INIT;
///     gp_call_once(&initialized, init_foo);
///     return foo;
/// }
/// @endcode
/// @{

/** Opaque flag used for @ref gp_call_once().
 *
 * Indicates if function passed to @ref gp_call_once() has been called.
 * Should be initialized to @ref GP_ONCE_INIT to indicate that the
 * function has not been called. After calling @ref gp_call_once(), the value
 * changes to indicate that the function has been called.
 */
#ifdef GP_DOXYGEN
typedef __unspecified__ GPOnce;
#else
typedef pthread_once_t GPOnce;
#endif

/** Static initializer for @ref GPOnce. */
#ifdef GP_DOXYGEN
#  define GP_ONCE_INIT /* unspecified */
#else
#  define GP_ONCE_INIT PTHREAD_ONCE_INIT
#endif

/** Thread-safe initialization.
 *
 * Call function @a func exactly once regardless of how many threads might call
 * this function. This is usually used to initialize global objects in a thread
 * safe manner. For any given @a flag, the function @a func will be called and a
 * value is stored in @a flag to indicate that the function has been called.
 * Subsequent calls with the same flag do nothing.
 *
 * Value pointed by @a flag should be allocated with global lifetime if one
 * wants @a func to be truly only ever called once. Value pointed by @a flag
 * also should be initialized using @ref GP_ONCE_INIT before call.
 */
GP_NONNULL_ARGS() GP_INOUT(1) GP_INLINE
void gp_call_once(GPOnce* flag, void (*func)(void))
{
    pthread_once(flag, func);
}

/// @}
# else // use Win32 threads ---------------------------------------------------

// ------------------------------------
// Thread Management

typedef unsigned long GPThread;

GP_API GP_NONNULL_ARGS(1, 2) bool gp_thread_create(GPThread *thr, int(*func)(void*), void *optional_arg);

GP_API GP_NORETURN void gp_thread_exit(int res);

GP_API bool gp_thread_join(GPThread thr, int *res);

GP_API bool gp_thread_detach(GPThread thr);

GP_API GPThread gp_thread_current(void);

GP_INLINE bool gp_thread_equal(GPThread a, GPThread b)
{
    // In case you're reading this and missed the docs, GPThread is not always
    // an integer, so don't do this yourself!
    return a == b;
}

GP_API void gp_thread_yield(void);

// ------------------------------------
// Mutexes

typedef struct GPMutex
{
    void* _ptr;
} GPMutex;

#define GP_MUTEX_INIT {0}

GP_API GP_NONNULL_ARGS() void gp_mutex_init(GPMutex* mtx);

GP_INLINE void gp_mutex_destroy(GPMutex* optional_mtx)
{
    (void)optional_mtx;
}

GP_API GP_NONNULL_ARGS() void gp_mutex_lock(GPMutex* mtx);

GP_API GP_NONNULL_ARGS() bool gp_mutex_trylock(GPMutex* mtx);

GP_API GP_NONNULL_ARGS() bool gp_mutex_timedlock(GPMutex* mutex, double time);

GP_API GP_NONNULL_ARGS() bool gp_mutex_timedlock_ns(GPMutex* mutex, int64_t time_ns);

GP_API GP_NONNULL_ARGS() bool gp_mutex_timedlock_absolute(GPMutex* mutex, GPInt128 time_absolute_ns);

GP_API GP_NONNULL_ARGS() void gp_mutex_unlock(GPMutex *mtx);

// ------------------------------------
// Condition Variables

typedef struct GPCond
{
    void* _ptr;
} GPCond;

#define GP_COND_INIT {0}

GP_API GP_NONNULL_ARGS() void gp_cond_init(GPCond *cond);

GP_API void gp_cond_destroy(GPCond* optional_cond);

GP_API GP_NONNULL_ARGS() void gp_cond_signal(GPCond *cond);

GP_API GP_NONNULL_ARGS() void gp_cond_broadcast(GPCond *cond);

GP_API GP_NONNULL_ARGS() void gp_cond_wait(GPCond *cond, GPMutex *mtx);

GP_API GP_NONNULL_ARGS() bool gp_cond_timedwait(GPCond *cond, GPMutex *mtx, double time);

GP_API GP_NONNULL_ARGS() bool gp_cond_timedwait_ns(GPCond* cond, GPMutex* mutex, int64_t time_ns);

GP_API GP_NONNULL_ARGS()
bool gp_cond_timedwait_absolute(
    GPCond* cond, GPMutex* mutex, GPInt128 time_absolute_ns);

// ------------------------------------
// Thread-Local Storage

// NOTE: Microsoft documentation only guarantees 64 slots without further
// explanation. What they don't mention is that they raised the number to 1088
// already on Windows 2000 (see https://bugs.python.org/msg79503). We only
// support Vista and newer, so it is safe for us to define this.
#define GP_THREAD_LOCAL_MAX_SLOTS 1088

#define GP_THREAD_LOCAL_DESTRUCTOR_ITERATIONS 4

typedef unsigned long GPThreadKey;

GP_API GP_NONNULL_ARGS(1)
int gp_thread_local_create(GPThreadKey *key, void(*destructor)(void*));

GP_API void gp_thread_local_delete(GPThreadKey key);

GP_API void gp_thread_local_set(GPThreadKey key, const void *val);

GP_API void* gp_thread_local_get(GPThreadKey key);

// ------------------------------------
// Call Once

#define GP_ONCE_INIT NULL

typedef void* GPOnce;

GP_API GP_NONNULL_ARGS() void gp_call_once(GPOnce* flag, void (*func)(void));

#endif // GP_USE_WINTHREADS ---------------------------------------------------

/// @}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_THREAD_INCLUDED
