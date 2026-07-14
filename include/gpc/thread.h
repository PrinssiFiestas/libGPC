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
#include <gpc/time.h>
#include <gpc/int128.h>

#if defined(GP_TARGET_OS_WINDOWS) && !defined(GP_USE_PTHREADS)
#define GP_USE_WINTHREADS 1
#else
#include <pthread.h>
#include <string.h> // strerror
#endif

// TODO move atomics to appropriate header.
#if __STDC_VERSION__ >= 201112L && !defined(GP_TARGET_OS_WINDOWS) // UCRT stdatomic.h broken
#include <stdatomic.h>
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
/// Portable threading API based on [C11 threads API](https://en.cppreference.com/c/header/threads),
/// including threads, mutual exclusion, condition variables, and thread-local
/// storage. Implementation is based on [c11threads](https://github.com/jtsiomb/c11threads).
///
/// At the time of writing, GNU libc does not implement standard threading on
/// Windows. Microsoft UCRT only added standard threading recently, but anyway
/// it's C11 and we are supposed to support C99 and C++. This module is a thin
/// wrapper over POSIX threads if not Windows, otherwise uses Win32 threads in
/// it's implementation. POSIX threads can be forced with @ref GP_USE_PTHREADS.
///
/// Our changes to C11 API:
/// - Added static initializers for mutexes.
/// - Added static initializers for condition variables.
/// - C11 `mtx_timedlock()` and `cnd_timedwait()` and their POSIX counterparts
///   take calendar timestamp as the timeout bound argument, but Win32
///   counterparts use relative time instead. We added options for both.
/// - Removed mutex types, only plain is available. This is due to `mtx_timed`
///   being implicit and redundant and `mtx_recursive` is a code smell that
///   doesn't work well portably with static initialization, which is much more
///   important.
/// - Replaced `struct timespec` with seconds and nanoseconds. Nobody likes to
///   deal with `struct timespec`.
/// - Removed `thrd_sleep()`, we already have @ref gp_sleep() and @ref gp_sleep_ns()
///   in our timing utilities module.
/// - Simplified error handling:
///   - Errors that don't happen or cannot be meaningfully handled are either
///     undefined (asserted using @ref gp_assume()) or removed.
///   - Functions that can return many different errors return `errno` constants,
///     so we don't define C11 threading error constants, they are redundant and
///     don't have `strerror()`. The only constant we define is @ref GP_THREAD_SUCCESS.
///   - Functions that only return one meaningful error return a boolean instead.
/// - The sheer amount of changes mean that this API is significantly different
///   from the C11 standard API. Therefore, we changed all names to follow our
///   naming conventions and to make a clear distinction between the APIs.
///
/// Our changes to the original [c11threads](https://github.com/jtsiomb/c11threads)
/// implementation:
/// - Any changes required by changes described above.
/// - Pthreads wrappers are practically rewritten from scracth. Nothing wrong
///   with the original, but the wrapping was so thin that the other changes
///   just happened to cause an almost full rewrite.
/// - Rewrote Win32 mutex implementation to use newer SRW locks instead of older
///   critical section objects. SRW locks are faster and support static
///   initialization. Unfortunately, this required dropping Windows XP support (RIP).
/// - Internal critical section objects were also replaced with SRW locks.
/// - Removed Windows XP support. Loading kernel32.dll functions manually is no
///   longer necessary. In fact, no initialization is necessary since internal
///   mutexes were changed to SRW locks that can be initialized statically.
/// - With no Windows XP support, `WINNT` stuff is no longer necessary. Also had
///   to remove `WIN32_LEAN_AND_MEAN` and `_CRTDBG_MAP_ALLOC` for single header
///   users. All `if (winver < VISTA)` branches were removed.
/// - Since we don't use `struct timespec`, anything using them had to be
///   rewritten. Related helpers were removed.
/// - Some UB pointer casts were hacked away using @ref gp_launder(). That's the
///   best we can do without including `windows.h` in header files, which might
///   break user builds (namespace pollution like `min` and include order
///   problems).
/// - Removed 32-bit internal timespec, use 64 bits always with C99 `uint64_t`.
///   All 32-bit specific functions were removed, they are not needed even for
///   32-bit builds.
/// @{

    // TODO bad doxygen
/// @addtogroup compile_options
/// @{
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
/// @}

// TODO C23 keywords
/** Sometimes thread local. TODO add always thread local too like C11 standard threads. And atomics too.
 *
 * Use this only when thread local storage would be ideal but not necessary for
 * correctness. Example use might be something logging related that is meant for
 * developers, but end users might not care about.
 *
 * NOTE: MinGW thread locals are broken, so they are disabled. See
 * https://sourceforge.net/p/mingw-w64/bugs/445/
 */
#ifdef _MSC_VER
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

// TODO move atomics to appropriate header
/** Sometimes atomic.
 *
 * Use this only when atomics would be ideal but not necessary for correctness.
 */
#if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define GP_HAS_ATOMICS 1
#  define GP_MAYBE_ATOMIC /* sometimes */_Atomic
#else
#  define GP_MAYBE_ATOMIC
#endif

/** Successfull return value.
 *
 * Functions in threading API return this on success, otherwise an appropriate
 * `errno` constant is returned instead.
 */
#define GP_THREAD_SUCCESS 0

// ----------------------------------------------------------------------------
#ifndef GP_USE_WINTHREADS // use POSIX threads

#include <pthread.h>
#include <sched.h> // sched_yield
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>

#define GP_THREAD_LOCAL_DESTRUCTOR_ITERATIONS PTHREAD_DESTRUCTOR_ITERATIONS

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
typedef /* unspecified */ GPThread;
#else
typedef pthread_t GPThread;
#endif

/** Create thread.
 *
 * Creates a new thread, whose ID will be stored to @ref thread, which is used
 * to refer to that thread. The new thread starts execution by calling @a routine.
 * @a arg will be passed to the given routine.
 *
 * The thread terminates once @a routine returns, calls @ref gp_thread_exit(),
 * or once the process terminates. The return value of @a routine or value
 * passed to @ref gp_thread_exit() can be obtained with @ref gp_thread_join().
 *
 * @return @ref GP_THREAD_SUCCESS (zero) on success, `EAGAIN` if not enough
 * resources to create a thread.
 */
GP_NONNULL_ARGS(1, 2)
errno_t gp_thread_create(GPThread* thread, int(*routine)(void*), void* arg);

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
 * be stored to @a optional_result if not null. The given thread must not be
 * detached and multiple threads must not attempt to join the same thread.
 *
 * @return @ref GP_THREAD_SUCCESS (zero) on success, `EINVAL` if @a thread is
 * not joinable or if another thread is trying to join, or `ESRCH` if @a thread
 * does not exist.
 */
GP_INLINE errno_t gp_thread_join(GPThread thread, int *optional_result)
{
    errno_t status;
    void *retval;

    if ((status = pthread_join(thread, &retval)) != GP_THREAD_SUCCESS) {
        return status;
    }
    if (optional_result) {
        *optional_result = (int)(intptr_t)retval;
    }
    return GP_THREAD_SUCCESS;
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
 * @retrun GP_THREAD_SUCCESS (zero) on success, `EINVAL` if @a thread is not
 * joinable, `ESRCH` if @a thread does not exist.
 */
GP_INLINE errno_t gp_thread_detach(GPThread thread)
{
    return pthread_detach(thread);
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
 * Can be initialized statically by using @ref GP_MUTEX_INITIALIZER or
 * dynamically using @ref gp_mutex_init(). Dynamically created mutexes can be
 * destroyed using @ref gp_mutex_destroy().
 *
 * At the time of writing, only plain mutexes are supported. C11 `mtx_timed` is
 * redundant, timing is supported by default. C11 `mtx_recursive` is unnecessary
 * code smell that prevents static initialization, which is much more important.
 */
#ifdef GP_DOXYGEN
typedef /* unspecified */ GPMutex;
#else
typedef pthread_mutex_t GPMutex;
#endif

/** Static mutex initialzer.
 *
 * Example:
 * @code
 * static GPMutex mutex = GP_MUTEX_INITIALIZER;
 * @endcode
 */
#ifdef GP_DOXYGEN
#  define GP_MUTEX_INITIALIZER /* unspecified */
#elif defined(GP_TARGET_DEBUG)
#  define GP_MUTEX_INITIALIZER PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP
#else
#  define GP_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif

/** Create mutex dynamically.
 *
 * Dynamically created mutexes can be destroyed using @ref gp_mutex_destroy().
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
    int result = pthread_mutex_lock(mutex);
    gp_assume(result == GP_THREAD_SUCCESS, strerror(result));
}

/** Try lock mutex without blocking.
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

/** Try lock mutex in given time.
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
GP_NONNULL_ARGS()
bool gp_mutex_timedlock(GPMutex* mutex, double time);

/** Try lock mutex in given time.
 *
 * Like @ref gp_mutex_lock(), except only blocks for the maximum of @a time_ns
 * amount of nanoseconds. It is not an error to pass negative size, in such case
 * the function returns immediately.
 *
 * @return `true` if acquired lock ownership, `false` if timeout expired or
 * negative number passed.
 */
GP_NONNULL_ARGS()
bool gp_mutex_timedlock_ns(GPMutex* mutex, int64_t time_ns);

/** Try lock mutex until given time.
 *
 * Like @ref gp_mutex_lock(), except only blocks until a given absolute time
 * (time since epoch in nanoseconds). Current absolute time can be obtained
 * using @ref gp_time_begin().
 *
 * @return `true` if acquired lock ownership, `false` if timeout expired.
 */
GP_NONNULL_ARGS()
bool gp_mutex_timedlock_absolute(GPMutex* mutex, GPInt128 time_point_ns);

/** Unlock mutex.
 *
 * Unlocks @a mutex lock, which should have been locked by the calling thread.
 * Trying to unlock a lock owner by another thread is undefined.
 */
GP_NONNULL_ARGS() GP_INLINE
void gp_mutex_unlock(GPMutex* mutex)
{
    int result = pthread_mutex_unlock(mutex);
    gp_assume(result == GP_THREAD_SUCCESS, strerror(result));
}

/// @}
// ------------------------------------
/// @defgroup condition_variables Condition Variables
/// @{

/** Opaque condition variable identifier.
 *
 * Used in conjunction with @ref GPMutex to wait until a condition is met.
 *
 * Can be initialized statically by using @ref GP_COND_INITIALIZER or
 * dynamically using @ref gp_cond_init(). Dynamically created condition
 * variables can be destroyed using @ref gp_cond_destroy().
 */
#ifdef GP_DOXYGEN
typedef /* unspecified */ GPCond;
#else
typedef pthread_cond_t GPCond;
#endif

/** Static condition variable initialzer.
 *
 * Example:
 * @code
 * static GPCond mutex = GP_COND_INITIALIZER;
 * @endcode
 */
#ifdef GP_DOXYGEN
#  define GP_COND_INITIALIZER /* unspecified */
#else
#  define GP_COND_INITIALIZER PTHREAD_COND_INITIALIZER
#endif

/** Create condition variable dynamically.
 *
 * Dynamically created condition variables can be destroyed using
 * @ref gp_cond_destroy().
 */
GP_INLINE void gp_cond_init(GPCond* cond)
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
        pthread_cond_destroy(cond);
    #endif
}

// TODO docs
GP_INLINE void gp_cond_wait(GPCond *cond, GPMutex *mtx)
{
    pthread_cond_wait(cond, mtx);
}

// TODO docs and implementation
bool gp_cond_timedwait(GPCond* cond, GPMutex* mutex, double time);

// TODO docs and implementation
bool gp_cond_timedwait_ns(GPCond* cond, GPMutex* mutex, int64_t time_ns);

// TODO docs
GP_INLINE void gp_cond_signal(GPCond *cond)
{
    pthread_cond_signal(cond);
}

// TODO docs
GP_INLINE void gp_cond_broadcast(GPCond *cond)
{
    pthread_cond_broadcast(cond);
}

/// @}
// ------------------------------------
/// @defgroup thread_local_storage Thread-Local Storage
/// @{

/** Thread-local storage pointer. */
// TODO docs
typedef pthread_key_t GPThreadKey;

// TODO docs
GP_INLINE int tss_create(GPThreadKey *key, tss_dtor_t dtor)
{
    return pthread_key_create(key, dtor);
}

// TODO docs
GP_INLINE void tss_delete(GPThreadKey key)
{
    pthread_key_delete(key);
}

// TODO docs
GP_INLINE int tss_set(GPThreadKey key, void *val)
{
    return pthread_setspecific(key, val);
}

// TODO docs
GP_INLINE void *tss_get(GPThreadKey key)
{
    return pthread_getspecific(key);
}

/// @}
// ------------------------------------
/// @defgroup call_once Call Once
/// @{

/** Opaque flag used for @ref gp_call_once().
 *
 * Indicates if function passed to @ref gp_call_once() has been called.
 * Should be initialized to @ref GP_ONCE_INIT to indicate that the function has
 * not been called. After calling @ref gp_call_once(), the value changes to
 * indicate that the function has been called.
 */
#ifdef GP_DOXYGEN
typedef /* unspecified */ GPOnce;
#else
typedef pthread_once_t GPOnce;
#endif

/** Static initializer for @ref GPOnce. */
#ifdef GP_DOXYGEN
#  define GP_ONCE_INITIALIZER /* unspecified */
#else
#  define GP_ONCE_INITIALIZER PTHREAD_ONCE_INIT
#endif

/** Thread-safe initialization.
 *
 * Call function @a func exactly once regardless of how many threads might call
 * this function. This is usually used to initialize global objects in a thread
 * safe manner. For any given @a flag, the function @a func will be called and a
 * value is stored in @a flag to indicate that the function has been called.
 * Subsequent calls with the same flag do nothing.
 *
 * @snippet thread.h gp_call_once_example
 */
GP_NONNULL_ARGS() GP_INLINE void gp_call_once(GPOnce* flag, void (*func)(void))
{
    pthread_once(flag, func);
}

/// @}
# else // use Win32 threads ---------------------------------------------------

// ------------------------------------
// Thread Management

typedef unsigned long GPThread;

GP_NONNULL_ARGS(1, 2) errno_t gp_thread_create(GPThread *thr, int(*func)(void*), void *arg);

GP_NORETURN void gp_thread_exit(int res);

errno_t gp_thread_join(GPThread thr, int *res);

errno_t gp_thread_detach(GPThread thr);

GPThread gp_thread_current(void);

bool gp_thread_equal(GPThread a, GPThread b);

void gp_thread_yield(void);

// ------------------------------------
// Mutexes

typedef struct
{
    void* _ptr;
} GPMutex;

#define GP_MUTEX_INITIALIZER {0}

GP_NONNULL_ARGS() void gp_mutex_init(GPMutex* mtx);

GP_INLINE void gp_mutex_destroy(GPMutex* optional_mtx)
{
    (void)optional_mtx;
}

GP_NONNULL_ARGS() void gp_mutex_lock(GPMutex* mtx);

GP_NONNULL_ARGS() bool gp_mutex_trylock(GPMutex* mtx);

GP_NONNULL_ARGS() bool gp_mutex_timedlock(GPMutex* mutex, double time);

GP_NONNULL_ARGS() bool gp_mutex_timedlock_ns(GPMutex* mutex, int64_t time_ns);

GP_NONNULL_ARGS() void gp_mutex_unlock(GPMutex *mtx);

// ------------------------------------
// Condition Variables

// TODO attributes and return values

typedef struct
{
    void* _ptr;
} GPCond;

#define GP_COND_INITIALIZER {0}

int gp_cond_init(GPCond *cond);

void gp_cond_destroy(GPCond *cond);

int gp_cond_signal(GPCond *cond);

int gp_cond_broadcast(GPCond *cond);

int gp_cond_wait(GPCond *cond, GPMutex *mtx);

// TODO get rid of this
int gp_cond_timedwait(GPCond *cond, GPMutex *mtx, const struct timespec *ts);

// ------------------------------------
// Thread-Local Storage

// TODO attributes

typedef unsigned long GPThreadKey;

int gp_thread_local_create(GPThreadKey *key, gp_thread_local_dtor_t dtor);

void gp_thread_local_delete(GPThreadKey key);

int gp_thread_local_set(GPThreadKey key, void *val);

void* gp_thread_local_get(GPThreadKey key);

// ------------------------------------
// Call Once

typedef void* GPOnce;

GP_NONNULL_ARGS() void call_once(GPOnce* flag, void (*func)(void));

#endif // GP_USE_WINTHREADS ---------------------------------------------------

// TODO I don't like the examples block, just move large examples to dedicated
// sections. Less jumping around and easier to find.
// ----------------------------------------------------------------------------
//
//          EXAMPLES
//
// ----------------------------------------------------------------------------
/// @cond
#ifdef GP_DOXYGEN_EXAMPLE

// ----------------------------------------------------------------------------
//! [gp_call_once_example]
// A simple yet performant and thread safe object pool/allocator. Any thread
// can call entity_alloc() at any time and entity pool will be initialized
// without race conditions. Deallocation logic is not relevant, so it is omitted
// for brevity. We assume that entities cannot be statically allocated, maybe
// it would waste memory and/or maybe there is a risk that not enough allocated.

#include <gpc/thread.h>
#include <gpc/assert.h> // gp_assume()
#include "entity.h" // Entity, read_entity_config()

static size_t entity_pool_size;
static Entity* entity_pool;
static _Atomic size_t entity_pool_index;

void init_entities(void)
{
    entity_pool_size = read_entity_config("config.txt").max_entities;
    entity_pool = calloc(entity_pool_size, sizeof(Entity));
    gp_assume(entity_pool != NULL, "Allocation failed.");
}

Entity* entity_alloc(void)
{
    #if NOT_THREAD_SAFE // this is logically what we want to do:
    static bool initialized = false;
    if ( ! initialized) { // not thread safe!
        // potential race condition here even if initialized was atomic.
        initialized = true;
        init_entities();
    }
    #else // correct way of initializing globals in multi-threaded application:
    static GPOnce initialized = GP_ONCE_INIT;
    gp_call_once(&initialized, init_entities); // gp_call_once() is thread safe.
    #endif
    // Using atomic index, so thread safe.
    size_t index = entity_pool_index++;
    gp_assume(index < entity_pool_size, "Too many entities.");
    return &entity_pool[index];
}
//! [gp_call_once_example]
// ----------------------------------------------------------------------------
/// @endcond
#endif // GP_DOXYGEN_EXAMPLE
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
