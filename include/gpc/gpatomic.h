// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_ATOMIC_INCLUDED
#define GP_ATOMIC_INCLUDED 1

#include <gpc/gpthread.h>

/** @defgroup atomic Atomics
 * ```c
 * #include <gpc/gpatomic.h>
 * ```
 * This module provides portable atomic integers regardless of C/C++ dialect or
 * compiler. All functionality in this module can be used even in strict C99
 * and even if the compiler has no support whatsoever for atomics. In that
 * case, locking atomics are used, however, most compilers do support atomics
 * possibly as an extension to C99, so locks are rarely used.
 *
 * C11 standard allows the compiler to insert locks with atomic types. This
 * means that you cannot expect `sizeof(_Atomic(inN_t))` to equal to `sizeof(intN_t)`.
 * This can cause ABI incompatibilities for different compilers even on the
 * same target platform. In our implementation, even when locks are used, this
 * will not happen. We guarantee that `sizeof(GPAtomicIntN)` is always to equal
 * to `sizeof(intN_t)`.
 *
 * Our API is based on [C11 atomics API](https://en.cppreference.com/c/atomic)
 * with the following changes:
 *
 * - We currently only support 8, 16, 32, and 64 bit fixed width integers,
 *   although we also provide typedefs for size and pointer types. Atomic flag
 *   is not provided, it's requirements make it impossible to implement
 *   portably and loosening up the requirements would make it redundant.
 * - Our types are strongly typed (unions with `nonatomic` field instead of
 *   typedefs inspired by [turf](https://github.com/preshing/turf)). This is
 *   the only sensible way of making sure that the semantics of our types
 *   remain consistent across platforms.
 * - In addition of type generic functions, we also export all functions as
 *   @ref GP_ALWAYS_INLINE functions with `_i8`, `_u8`, `_i16`, `_u16`, `_i32`,
 *   `_u32`, `_i64`, and `_u64` suffixes for all of our fixed width types
 *   respectively. We also provide macros with `_uz` and `_iz` suffixes that
 *   take `GPAtomicSize/GPAtomicUIntPtr` and `GPAtomicPtrDiff/GPAtomicIntPtr`
 *   arguments respectively. These are recommended be used with compilers that
 *   do not provide enough generic utilities to implement the generic functions,
 *   however, the most widely used compilers do support generic functions. Only
 *   generic functions have dedicated documentation for brevity, non-generics
 *   only differ by having stricter and more explicit typing.
 * - Our types are guaranteed to have the same size as their underlying type.
 *   The compiler will never allocate a mutex with the value.
 * - Runtime `atomic_is_lock_free()` currently unavailable. However, we do have
 *   `GP_ATOMIC_TYPE_LOCK_FREE` macros, which is what you want most of the time.
 * - Removed `kill_dependency()`. It is only needed for `memory_order_consume`,
 *   which is [deprecated by C++26](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3475r1.pdf).
 *
 * Strict C99 (no `_Generic` and no @ref GP_TYPEOF) doesn't provide enough
 * utilities to implement correct return type for generic functions. Therefore,
 * maximum portability applications or code that require strict C99 has to cast
 * the return value of most generic functions to desired type if the size of
 * the return type doesn't match the size of the expression.
 *
 * ```c
 * extern GPAtomicUInt8 x;
 *
 * void foo(void)
 * {
 *     // WRONG: x will sign extend even though
 *     // unsigned.
 *     unsigned u32 = gp_atomic_load(&x);
 *
 *     // Correct: Explicit cast to target type
 *     // ensures proper zero extension. This
 *     // is the recommended practice.
 *     u32 = (uint8_t)gp_atomic_load(&x);
 *
 *     // Ok: Assigning to a type with the same
 *     // size. Signedness does not matter due
 *     // to two's complement. Slightly more bug
 *     // prone, so explicit cast recommended
 *     // anyway.
 *     int8_t i8 = gp_atomic_load(&x);
 * }
 * ```
 *
 * The exception is variants of `gp_compare_exchange()`, which always return a
 * `bool` and `gp_atomic_store()` that doesn't return anything.
 * @{
 */

#ifdef GP_DOXYGEN
/** Use MSVC experimental C11 atomics.
 *
 * Should be defined per header file inclusion if needed.
 *
 * Define this to enable the [experimental C11 atomics on MSVC](https://devblogs.microsoft.com/cppblog/c11-atomics-in-visual-studio-2022-version-17-5-preview-2/).
 * This enables @ref GP_ATOMIC types. Note that our implementation uses MSVC
 * intrinsics, so this is not necessary for lock free programming.
 */
#define GP_USE_MSVC_EXPERIMENTAL_ATOMICS

/** Atomic fixed width unsigned integers.
 *
 * Replace `N` with 8, 16, 32, or 64.
 */
typedef union GPAtomicUIntN
{
    /** Non-atomic access.
     *
     * Available in all platforms. Obviously since accessing the value using
     * this is not atomic, this completely defeats the purpose of atomics, so you
     * should use @ref gp_atomic_load() and @ref gp_atomic_store() for accesses
     * and our other functions for operations. However, this can be still useful
     * for debugging.
     */
    uintN_t nonatomic;

    /** Atomic access.
     *
     * Available if @ref GP_HAS_ATOMIC_TYPES is defined. Using this defeats the
     * portability gains of our portable atomics, so using @ref GP_ATOMIC
     * should be preferred to be explicit about the limited portability.
     * However, this can be still useful for debugging and interfacing with
     * standard atomics.
     */
    _Atomic uintN_t atomic;
} GPAtomicUIntN;

/** Atomic fixed width signed integers.
 *
 * Replace `N` with 8, 16, 32, or 64.
 */
typedef union GPAtomicIntN
{
    /** Non-atomic access.
     *
     * Available in all platforms. Obviously since accessing the value using
     * this is not atomic, this completely defeats the purpose of atomics, so you
     * should use @ref gp_atomic_load() and @ref gp_atomic_store() for accesses
     * and our other functions for operations. However, this can be still useful
     * for debugging.
     */
    intN_t nonatomic;

    /** Atomic access.
     *
     * Available if @ref GP_HAS_ATOMIC_TYPES is defined. Using this defeats the
     * portability gains of our portable atomics, so using @ref GP_ATOMIC
     * should be preferred to be explicit about the limited portability.
     * However, this can be still useful for debugging and interfacing with
     * standard atomics.
     */
    _Atomic intN_t atomic;
} GPAtomicIntN;

/** Atomic `size_t`.
 *
 * `N` is 32 or 64 depending on target platform.
 */
typedef GPAtomicUIntN GPAtomicSize;

/** Atomic `ptrdiff_t`.
 *
 * `N` is 32 or 64 depending on target platform.
 */
typedef GPAtomicIntN GPAtomicPtrDiff;

/** Atomic `uintptr_t`.
 *
 * `N` is 32 or 64 depending on target platform.
 */
typedef GPAtomicUIntN GPAtomicUIntPtr;

/** Atomic `intptr_t`.
 *
 * `N` is 32 or 64 depending on target platform.
 */
typedef GPAtomicIntN GPAtomicIntPtr;

/** Check if C11/C++11 atomic types available.
 *
 * For major compilers, at the time of writing, this is defined when using GCC
 * or Clang with C11/C++11. With MSVC, this is only defined when using C++11.
 * MSVC currently only has experimental support for C11 atomics. If you want to
 * use the MSVC experimental atomics, you must use the `/experimental:c11atomics`
 * compiler flag and define @ref GP_USE_MSVC_EXPERIMENTAL_ATOMICS.
 */
#define GP_HAS_ATOMIC_TYPES 1

/** Check if atomic intrinsics or builtins available.
 *
 * Currently defined if using GCC, Clang, MSVC, or Tiny C regardless of language
 * standard. For other compilers, defined if standard atomics are available.
 */
#define GP_HAS_ATOMIC_INTRINSICS 1

/** Check if no atomics available.
 *
 * Defined if neither @ref GP_HAS_ATOMIC_TYPES nor @ref GP_HAS_ATOMIC_INTRINSICS
 * is defined. In such case, locks are used to implement atomics when compiling
 * this library. However, if this library has been compiled with a compiler that
 * does support atomics and is linked with an application that is compiled using
 * a compiler that doesn't support atomics, then no locks are used regardless of
 * this macro. The less capable compiler will just call atomic functions
 * exported by the more capable compiler. However, in such case, the memory
 * ordering will always be @ref GP_MEMORY_ORDER_SEQ_CST and the memory ordering
 * parameter of explicit functions is ignored.
 */
#define GP_USE_LOCKS_FOR_ATOMICS 1

/** Atomic type.
 *
 * `_Atomic(T)` if C11, `std::atomic<T>` if C++11. Only defined if @ref GP_HAS_ATOMIC_TYPES
 * is defined.
 */
#define GP_ATOMIC(...) _Atomic(__VA_ARGS__)

/** Family of macros to indicate whether the given type is lock-free.
 *
 * Replace `TYPE` with `INT8`, `INT16`, `INT32`, or `INT64` for @ref GPAtomicIntN
 * and @ref GPAtomicUIntN. Replace `TYPE` with `POINTER` for @ref GPAtomicSize,
 * @ref GPAtomicPtrDiff, @ref GPAtomicIntPtr, and @ref GPAtomicUIntPtr.
 *
 * Defined to 2 if operations are always lock-free, 1 if operations are
 * sometimes lock-free, 0 operations are never lock-free. Not defined if cannot
 * determine, but that only happens for the very most obscure targets.
 *
 * Almost all of these are defined to at least 1 if compiler supports atomics
 * in any shape or form. If the compiler does not support atomics in any shape
 * or form (usually strict C99 without atomic builtins), then all of these are
 * 0, however, all major compilers do have some atomics support.
 *
 * These may have different values with different compilers even on the same
 * target platform.
 */
#define GP_ATOMIC_TYPE_LOCK_FREE /* unspecified */

/** Indicates that `GPAtomicInt8` and `GPAtomicUInt8` is lock-free.
 *
 * See @ref GP_ATOMIC_TYPE_LOCK_FREE for possible values and their meanings.
 *
 * On most modern systems, this is defined to 2. Most notable exception is GCC
 * on ARM32. GCC still compiles to ARMv6 by default on ARM32, where this is
 * defined to 1. As of 2026, ARMv6 is essentially dead, so we recommend
 * compiling with at least `-march=armv7` if using GCC on ARM32. This would
 * promote the value to 2.
 */
#define GP_ATOMIC_INT8_LOCK_FREE /* unspecified */

/** Indicates that `GPAtomicInt16` and `GPAtomicUInt16` is lock-free.
 *
 * @copydetails GP_ATOMIC_INT8_LOCK_FREE
 */
#define GP_ATOMIC_INT16_LOCK_FREE /* unspecified */

/** Indicates that `GPAtomicInt32` and `GPAtomicUInt32` is lock-free.
 *
 * See @ref GP_ATOMIC_TYPE_LOCK_FREE for possible values and their meanings.
 *
 * Can be expected to be defined to 2 on most modern systems with major compilers.
 */
#define GP_ATOMIC_INT32_LOCK_FREE /* unspecified */

/** Indicates that `GPAtomicInt64` and `GPAtomicUInt64` is lock-free.
 *
 * See @ref GP_ATOMIC_TYPE_LOCK_FREE for possible values and their meanings.
 *
 * Can be expected to be defined to 2 on most 64-bit systems with major compilers.
 * Can be commonly any of 0, 1, or 2 on 32-bit systems.
 */
#define GP_ATOMIC_INT64_LOCK_FREE /* unspecified */

/** Indicates that `GPAtomicSize` and `GPAtomicPtrDiff`, `GPAtomicIntPtr` and `GPAtomicUIntPtr` is lock-free.
 *
 * @copydetails GP_ATOMIC_INT32_LOCK_FREE
 */
#define GP_ATOMIC_POINTER_LOCK_FREE /* unspecified */

/** Non-generic atomic functions.
 *
 * All generic functions have non-generic counterparts with explicit types.
 * For example, a generic call like `gp_atomic_store(&x, y)`, where `x` is of
 * type `GPAtomicInt16`, is equivalent to `gp_atomic_store_i16(&x, y)`. These
 * are necessary for compilers that do not support enough generic functionality
 * to implement the generic ones, however this is not an issue with most
 * compilers. These can be also useful when stricter and more explicit typing is
 * desired.
 *
 * We will only have operation specific documentation for generic macros for
 * brevity.
 *
 * `OP` is any operation we support, which would be one of `store`, `load`,
 * `exchange`, `fetch_add`, `fetch_sub`, `fetch_or`, `fetch_xor`, `fetch_and`,
 * `compare_exchange_strong`, or `compare_exchange_weak`. The memory order
 * explicit variants are currently not available.
 *
 * `T` is any of `intN_t` or `uintN_t` where `N` is any of 8, 16, 32, or 64. For
 * all `T`, there are functions with a `SUFFIX` of `_iN` or `_uN`. There are also
 * macros with `_uz` and `_iz` suffixes that alias appropriate sized and signed
 * function, which can be used with `GPAtomicSize`, `GPAtomicPtrDiff`, `GPAtomicUIntPtr`,
 * and `GPAtomicIntPtr` depending on signedness.
 *
 * @return
 *
 * Most functions like `gp_fetch_add()` return the previous value of @a destination
 * as a non-atomic type. However, `gp_atomic_store()` returns `void` and
 * `gp_atomic_compare_exchange_weak/strong()` returns a boolean to indicate
 * whether @a destination matched the expected argument.
 */
T_or_void_or_bool gp_atomic_OP_SUFFIX(GPAtomicT* destination, T operands...);

/** Atomically store a value to an atomic type.
 *
 * Atomically stores @a T__value to the value pointed by @a GPAtomicTPtr__a.
 */
#  define gp_atomic_store(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomically load a value from an atomic type.
 *
 * @return the value pointed by @a GPAtomicTPtr__a as a non-atomic type.
 */
#  define gp_atomic_load(GPAtomicTPtr__a) /* unspecified */

/** Atomically swap the value of an atomic type.
 *
 * Atomically stores @a T__value to the value pointed by @a GPAtomicTPtr__a.
 *
 * @return old value of @a GPAtomicTPtr__a.
 */
#  define gp_atomic_exchange(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomically add value to an atomic type.
 *
 * Atomically adds @a T__value to the value of @a GPAtomicTPtr__a and assigns the
 * result to @a GPAtomicTPtr__a.
 *
 * @return old value of @a GPAtomicTPtr__a.
 */
#  define gp_atomic_fetch_add(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomically subtracts value from an atomic type.
 *
 * Atomically subtracts @a T__value from the value of @a GPAtomicTPtr__a and
 * assigns the result to @a GPAtomicTPtr__a.
 *
 * @return old value of @a GPAtomicTPtr__a.
 */
#  define gp_atomic_fetch_sub(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomically perform a bit-wise OR operation on an atomic type.
 *
 * Atomically performs a bit-wise OR to @a T__value and @a GPAtomicTPtr__a, and
 * assigns result to @a GPAtomicTPtr__a.
 *
 * @return old value of @a GPAtomicTPtr__a.
 */
#  define gp_atomic_fetch_or(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomically perform a bit-wise XOR operation on an atomic type.
 *
 * Atomically performs a bit-wise XOR to @a T__value and @a GPAtomicTPtr__a, and
 * assigns result to @a GPAtomicTPtr__a.
 *
 * @return old value of @a GPAtomicTPtr__a.
 */
#  define gp_atomic_fetch_xor(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomically perform a bit-wise AND operation on an atomic type.
 *
 * Atomically performs a bit-wise AND to @a T__value and @a GPAtomicTPtr__a, and
 * assigns result to @a GPAtomicTPtr__a.
 *
 * @return old value of @a GPAtomicTPtr__a.
 */
#  define gp_atomic_fetch_and(GPAtomicTPtr__a, T__value) /* unspecified */

/** Atomic compare exchange (CAS).
 *
 * A compare exchange operation (in literature commonly referred as [compare-and-swap](https://en.wikipedia.org/wiki/Compare-and-swap) or **CAS**)
 * compares value pointed by @a GPAtomicTPtr__a to the value pointed by @a TPtr__Expected
 * and assigns @a T__desired to @a GPAtomicTPtr__a if @a GPAtomicTPtr__a matched
 * @a TPtr__Expected. If the values did not match, then @a GPAtomicTPtr__a is
 * copied to @a TPtr__Expected.
 *
 * For CAS loops, you probably want the weak variant
 * @ref gp_atomic_compare_exchange_weak() instead.
 *
 * @return `true` if swap successful (@a GPAtomicTPtr__a matched @a TPtr__expected),
 * `false` otherwise.
 */
#  define gp_atomic_compare_exchange_strong(GPAtomicTPtr__a, TPtr__expected, T__desired) /* unspecified */

/** Sometimes slightly faster atomic compare exchange (CAS).
 *
 * Like @ref gp_atomic_compare_exchange_strong(), except the weak variant is
 * allowed to fail spuriously. This can give better performance in [CAS loops](https://preshing.com/20150402/you-can-do-any-kind-of-atomic-read-modify-write-operation/).
 */
#  define gp_atomic_compare_exchange_weak(GPAtomicTPtr__a, TPtr__expected, T__desired) /* unspecified */

/** Generic functions with explicit memory ordering.
 *
 * All generic functions have counterparts with `_explicit` suffix that take
 * an additional @a MEMORY_ORDER parameter, which is one of @ref gp_memory_order_t
 * constants. All the functions without the `_explicit` suffix are equivalent of
 * calling this with @a MEMORY_ORDER equal to @ref GP_MEMORY_ORDER_SEQ_CST.
 *
 * We will only have operation specific documentation for generic macros for
 * brevity.
 *
 * `OP` is any operation we support, which would be one of `store`, `load`,
 * `exchange`, `fetch_add`, `fetch_sub`, `fetch_or`, `fetch_xor`, `fetch_and`,
 * `compare_exchange_strong`, or `compare_exchange_weak`.
 *
 * Some implementations ignore the memory ordering parameter. In such case, the
 * memory ordering will always be sequentially consistent.
 */
#define gp_atomic_OP_explicit(ARGS..., MEMORY_ORDER) /* unspecified */

/** CPU and compiler memory fence.
 *
 * Almost nobody has ever any reason to use this. If you do, then you already
 * know what this is and where to use.
 */
#  define gp_atomic_thread_fence(MEMORY_ORDER) /* unspecified */

/** Compiler memory fence.
 *
 * Almost nobody has ever any reason to use this. If you do, then you already
 * know what this is and where to use.
 */
#  define gp_atomic_signal_fence(MEMORY_ORDER) /* unspecified */

#endif // GP_DOXYGEN

//------------------------------------------------------------------------------
// Includes and implementation switch macros

/// @cond
#if (__STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)) \
    || defined(GP_USE_MSVC_EXPERIMENTAL_ATOMICS) || defined(__TINYC__)
#  include <stdatomic.h>
#  define GP_USE_C11_ATOMICS 1
#  define GP_USE_STD_ATOMICS 1
#  define GP_HAS_ATOMIC_TYPES 1
#  define GP_HAS_ATOMIC_INTRINSICS 1
#  define GP_ATOMIC(...) _Atomic(__VA_ARGS__)
#elif __cplusplus >= 201103L
#  include <atomic>
#  define GP_USE_CPP_ATOIMCS 1
#  define GP_USE_STD_ATOMICS 1
#  define GP_HAS_ATOMIC_TYPES 1
#  define GP_HAS_ATOMIC_INTRINSICS 1
#  define GP_ATOMIC(...) std::atomic<__VA_ARGS__>
#elif defined(_MSC_VER)
#  include <intrin.h>
#  define GP_USE_INTERLOCKED_INTRINSICS 1
#  define GP_HAS_ATOMIC_INTRINSICS 1
#elif defined(__GNUC__)
#  define GP_USE_ATOMIC_BUILTINS 1
#  define GP_HAS_ATOMIC_INTRINSICS 1
#else
#  define GP_USE_LOCKS_FOR_ATOMICS 1
#endif

//------------------------------------------------------------------------------
// Is lock free

#if SIZE_MAX != UINTPTR_MAX || PTRDIFF_MAX != INTPTR_MAX
#error Portability assumption `sizeof(size_t) == sizeof(void*) && sizeof(ptrdiff_t) == sizeof(void*)` failed.
#endif

#if defined(GP_USE_STD_ATOMICS)
#  define GP_ATOMIC_INT8_LOCK_FREE    ATOMIC_CHAR_LOCK_FREE
#  define GP_ATOMIC_INT16_LOCK_FREE   ATOMIC_SHORT_LOCK_FREE
#  define GP_ATOMIC_INT32_LOCK_FREE   ATOMIC_INT_LOCK_FREE
#  define GP_ATOMIC_INT64_LOCK_FREE   ATOMIC_LLONG_LOCK_FREE
#  define GP_ATOMIC_POINTER_LOCK_FREE ATOMIC_POINTER_LOCK_FREE
#  define GP_ATOMIC_FLAG_LOCK_FREE    2
#elif defined(GP_USE_INTERLOCKED_INTRINSICS)
// Based on the "Requirements" table here: https://learn.microsoft.com/en-us/cpp/intrinsics/interlockedexchangeadd-intrinsic-functions?view=msvc-170
#  define GP_ATOMIC_INT8_LOCK_FREE    2
#  define GP_ATOMIC_INT16_LOCK_FREE   2
#  define GP_ATOMIC_INT32_LOCK_FREE   2
#  ifndef GP_TARGET_ARCH_X86 // ARM32, ARM64, x64
#    define GP_ATOMIC_INT64_LOCK_FREE 2
#  else // 64-bit intrinsics not available, we implemented our own with locks
#    define GP_ATOMIC_INT64_LOCK_FREE 0
#  endif
#  define GP_ATOMIC_POINTER_LOCK_FREE 2
#  define GP_ATOMIC_FLAG_LOCK_FREE    2
#elif defined(GP_HAS_ATOMIC_INTRINSICS)
#  if SIZE_MAX == UINT64_MAX
#    define GP_ATOMIC_INT8_LOCK_FREE    2
#    define GP_ATOMIC_INT16_LOCK_FREE   2
#    define GP_ATOMIC_INT32_LOCK_FREE   2
#    define GP_ATOMIC_INT64_LOCK_FREE   2
#    define GP_ATOMIC_POINTER_LOCK_FREE 2
#    define GP_ATOMIC_FLAG_LOCK_FREE    2
#  elif defined(__GCC_ATOMIC_POINTER_LOCK_FREE) // clang defines these too
#    define GP_ATOMIC_INT8_LOCK_FREE    __GCC_ATOMIC_CHAR_LOCK_FREE
#    define GP_ATOMIC_INT16_LOCK_FREE   __GCC_ATOMIC_SHORT_LOCK_FREE
#    define GP_ATOMIC_INT32_LOCK_FREE   __GCC_ATOMIC_INT_LOCK_FREE
#    define GP_ATOMIC_INT64_LOCK_FREE   __GCC_ATOMIC_LLONG_LOCK_FREE
#    define GP_ATOMIC_POINTER_LOCK_FREE __GCC_ATOMIC_POINTER_LOCK_FREE
#    define GP_ATOMIC_FLAG_LOCK_FREE    2
   // #else could not determine.
#  endif
#else // using mutex implementation
#  define GP_ATOMIC_INT8_LOCK_FREE    0
#  define GP_ATOMIC_INT16_LOCK_FREE   0
#  define GP_ATOMIC_INT32_LOCK_FREE   0
#  define GP_ATOMIC_INT64_LOCK_FREE   0
#  define GP_ATOMIC_POINTER_LOCK_FREE 0
#  define GP_ATOMIC_FLAG_LOCK_FREE    0
#endif

/// @endcond
//------------------------------------------------------------------------------
// Types

/** Sometimes atomic type.
 *
 * Expands to `_Atomic(T)` or `std::atomic<T>` if supported or just non-atomic
 * `T` if not supported. Support can be checked by checking if @ref GP_HAS_ATOMIC_TYPES
 * is defined
 *
 * This can be used as a simple hack to quickly improve thread safety of
 * existing code. This can be useful for debugging and prototyping, but you
 * should prefer to use the other portable atomic functions from our @ref atomic
 * module.
 */
#ifdef GP_GOT_ATOMIC_TYPES
#  define GP_MAYBE_ATOMIC(...) GP_ATOMIC(__VA_ARGS__)
#else
#  define GP_MAYBE_ATOMIC(...) GP_TYPEOF_TYPE(__VA_ARGS__)
#endif

 /** Memory order constraints.
 *
 * For the explanation of these, see [memory_order](https://en.cppreference.com/c/atomic/memory_order).
 *
 * These are ignored when atomics are not supported. In such case, backing
 * mutex is used, which has sequentially-consistent ordering.
 */
typedef enum gp_memory_order_t
{
    GP_MEMORY_ORDER_RELAXED, ///< Relaxed memory ordering.
    GP_MEMORY_ORDER_CONSUME, ///< Provided for compatibility, [not recommended to use](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3475r1.pdf).
    GP_MEMORY_ORDER_ACQUIRE, ///< Acquire memory ordering.
    GP_MEMORY_ORDER_RELEASE, ///< Release memory ordering.
    GP_MEMORY_ORDER_ACQ_REL, ///< Both acquire and release memory ordering.
    GP_MEMORY_ORDER_SEQ_CST, ///< Sequentially consistent memory ordering.
    #define GP_MEMORY_ORDER_LENGTH 6 ///< Number of `enum gp_memory_order_t` constants.
} gp_memory_order_t;
// Implementation detail: Values/order of these enum constants are not specified
// by the standard, but practical implementations (including Visual Studio)
// match GNU equivalent macros. These values are the only ones that make sense:
// 0 is relaxed and increasing the makes it stricter.

#ifdef GP_USE_INTERLOCKED_INTRINSICS
GP_STATIC_ASSERT(sizeof(char) == sizeof(int8_t), "Expected Win32 types.");
GP_STATIC_ASSERT(sizeof(short) == sizeof(int16_t), "Expected Win32 types.");
GP_STATIC_ASSERT(sizeof(long) == sizeof(int32_t), "Expected Win32 types.");
GP_STATIC_ASSERT(sizeof(long long) == sizeof(int64_t), "Expected Win32 types.");
#endif

#ifdef GP_HAS_ATOMIC_TYPES
// Ensure consistent ABI. Compilers are allowed to insert locks to atomic types.
// We couldn't find any implementation that actually does this for standard
// integer types, but let's be explicit about the expectation and get notified
// if that happens. Now we can promise consistent ABI in docs.
GP_STATIC_ASSERT(sizeof(GP_ATOMIC(int8_t)) == sizeof(int8_t), "Detected compiler inserted lock.");
GP_STATIC_ASSERT(sizeof(GP_ATOMIC(int16_t)) == sizeof(int16_t), "Detected compiler inserted lock.");
GP_STATIC_ASSERT(sizeof(GP_ATOMIC(int32_t)) == sizeof(int32_t), "Detected compiler inserted lock.");
GP_STATIC_ASSERT(sizeof(GP_ATOMIC(int64_t)) == sizeof(int64_t), "Detected compiler inserted lock.");
#endif

/// @cond
typedef union GPAtomicUInt8
{
    uint8_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(uint8_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    char _native;
    #endif
} GPAtomicUInt8;

typedef union GPAtomicUInt16
{
    uint16_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(uint16_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    short _native;
    #endif
} GPAtomicUInt16;

typedef union GPAtomicUInt32
{
    uint32_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(uint32_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    long _native;
    #endif
} GPAtomicUInt32;

typedef union GPAtomicUInt64
{
    uint64_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(uint64_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    __int64_t _native;
    #endif
} GPAtomicUInt64;

typedef union GPAtomicInt8
{
    int8_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(int8_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    char _native;
    #endif
} GPAtomicInt8;

typedef union GPAtomicInt16
{
    int16_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(int16_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    short _native;
    #endif
} GPAtomicInt16;

typedef union GPAtomicInt32
{
    int32_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(int32_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    long _native;
    #endif
} GPAtomicInt32;

typedef union GPAtomicInt64
{
    int64_t nonatomic;
    #ifdef GP_HAS_ATOMIC_TYPES
    GP_ATOMIC(int64_t) atomic;
    #endif
    #ifdef GP_USE_INTERLOCKED_INTRINSICS
    long long _native;
    #endif
} GPAtomicInt64;

#if SIZE_MAX == UINT32_MAX
typedef union GPAtomicUInt32 GPAtomicSize;
typedef union GPAtomicInt32  GPAtomicPtrDiff;
typedef union GPAtomicUInt32 GPAtomicUIntPtr;
typedef union GPAtomicInt32  GPAtomicIntPtr;
#elif SIZE_MAX == UINT64_MAX
typedef union GPAtomicUInt64 GPAtomicSize;
typedef union GPAtomicInt64  GPAtomicPtrDiff;
typedef union GPAtomicUInt64 GPAtomicUIntPtr;
typedef union GPAtomicInt64  GPAtomicIntPtr;
#else
#error Portability assumption `sizeof(size_t) == 4 || sizeof(size_t) == 8` failed.
#endif

//------------------------------------------------------------------------------
// Implementations

//-------------------------------------
// Common

#if SIZE_MAX == UINT32_MAX
#  define gp_atomic_store_uz(A, C)                      gp_atomic_store_u32(A, C)
#  define gp_atomic_load_uz(A)                          gp_atomic_load_u32(A)
#  define gp_atomic_exchange_uz(A, C)                   gp_atomic_exchange_u32(A, C)
#  define gp_atomic_fetch_add_uz(A, C)                  gp_atomic_fetch_add_u32(A, C)
#  define gp_atomic_fetch_sub_uz(A, C)                  gp_atomic_fetch_sub_u32(A, C)
#  define gp_atomic_fetch_or_uz(A, C)                   gp_atomic_fetch_or_u32(A, C)
#  define gp_atomic_fetch_xor_uz(A, C)                  gp_atomic_fetch_xor_u32(A, C)
#  define gp_atomic_fetch_and_uz(A, C)                  gp_atomic_fetch_and_u32(A, C)
#  define gp_atomic_compare_exchange_strong_uz(A, B, C) gp_atomic_compare_exchange_strong_u32(A, B, C)
#  define gp_atomic_compare_exchange_weak_uz(A, B, C)   gp_atomic_compare_exchange_weak_u32(A, B, C)
#  define gp_atomic_store_explicit_uz(A, C, MO)         gp_atomic_store_explicit_u32(A, C, MO)
#  define gp_atomic_load_explicit_uz(A, MO)             gp_atomic_load_explicit_u32(A, MO)
#  define gp_atomic_exchange_explicit_uz(A, C, MO)      gp_atomic_exchange_explicit_u32(A, C, MO)
#  define gp_atomic_fetch_add_explicit_uz(A, C, MO)     gp_atomic_fetch_add_explicit_u32(A, C, MO)
#  define gp_atomic_fetch_sub_explicit_uz(A, C, MO)     gp_atomic_fetch_sub_explicit_u32(A, C, MO)
#  define gp_atomic_fetch_or_explicit_uz(A, C, MO)      gp_atomic_fetch_or_explicit_u32(A, C, MO)
#  define gp_atomic_fetch_xor_explicit_uz(A, C, MO)     gp_atomic_fetch_xor_explicit_u32(A, C, MO)
#  define gp_atomic_fetch_and_explicit_uz(A, C, MO)     gp_atomic_fetch_and_explicit_u32(A, C, MO)
#  define gp_atomic_compare_exchange_strong_explicit_uz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_strong_explicit_u32(A, B, C, MO1, MO2)
#  define gp_atomic_compare_exchange_weak_explicit_uz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_weak_explicit_u32(A, B, C, MO1, MO2)
#  define gp_atomic_store_iz(A, C)                      gp_atomic_store_i32(A, C)
#  define gp_atomic_load_iz(A)                          gp_atomic_load_i32(A)
#  define gp_atomic_exchange_iz(A, C)                   gp_atomic_exchange_i32(A, C)
#  define gp_atomic_fetch_add_iz(A, C)                  gp_atomic_fetch_add_i32(A, C)
#  define gp_atomic_fetch_sub_iz(A, C)                  gp_atomic_fetch_sub_i32(A, C)
#  define gp_atomic_fetch_or_iz(A, C)                   gp_atomic_fetch_or_i32(A, C)
#  define gp_atomic_fetch_xor_iz(A, C)                  gp_atomic_fetch_xor_i32(A, C)
#  define gp_atomic_fetch_and_iz(A, C)                  gp_atomic_fetch_and_i32(A, C)
#  define gp_atomic_compare_exchange_strong_iz(A, B, C) gp_atomic_compare_exchange_strong_i32(A, B, C)
#  define gp_atomic_compare_exchange_weak_iz(A, B, C)   gp_atomic_compare_exchange_weak_i32(A, B, C)
#  define gp_atomic_store_explicit_iz(A, C, MO)         gp_atomic_store_explicit_i32(A, C, MO)
#  define gp_atomic_load_explicit_iz(A, MO)             gp_atomic_load_explicit_i32(A, MO)
#  define gp_atomic_exchange_explicit_iz(A, C, MO)      gp_atomic_exchange_explicit_i32(A, C, MO)
#  define gp_atomic_fetch_add_explicit_iz(A, C, MO)     gp_atomic_fetch_add_explicit_i32(A, C, MO)
#  define gp_atomic_fetch_sub_explicit_iz(A, C, MO)     gp_atomic_fetch_sub_explicit_i32(A, C, MO)
#  define gp_atomic_fetch_or_explicit_iz(A, C, MO)      gp_atomic_fetch_or_explicit_i32(A, C, MO)
#  define gp_atomic_fetch_xor_explicit_iz(A, C, MO)     gp_atomic_fetch_xor_explicit_i32(A, C, MO)
#  define gp_atomic_fetch_and_explicit_iz(A, C, MO)     gp_atomic_fetch_and_explicit_i32(A, C, MO)
#  define gp_atomic_compare_exchange_strong_explicit_iz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_strong_explicit_i32(A, B, C, MO1, MO2)
#  define gp_atomic_compare_exchange_weak_explicit_iz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_weak_explicit_i32(A, B, C, MO1, MO2)
#else // 64-bit
#  define gp_atomic_store_uz(A, C)                      gp_atomic_store_u64(A, C)
#  define gp_atomic_load_uz(A)                          gp_atomic_load_u64(A)
#  define gp_atomic_exchange_uz(A, C)                   gp_atomic_exchange_u64(A, C)
#  define gp_atomic_fetch_add_uz(A, C)                  gp_atomic_fetch_add_u64(A, C)
#  define gp_atomic_fetch_sub_uz(A, C)                  gp_atomic_fetch_sub_u64(A, C)
#  define gp_atomic_fetch_or_uz(A, C)                   gp_atomic_fetch_or_u64(A, C)
#  define gp_atomic_fetch_xor_uz(A, C)                  gp_atomic_fetch_xor_u64(A, C)
#  define gp_atomic_fetch_and_uz(A, C)                  gp_atomic_fetch_and_u64(A, C)
#  define gp_atomic_compare_exchange_strong_uz(A, B, C) gp_atomic_compare_exchange_strong_u64(A, B, C)
#  define gp_atomic_compare_exchange_weak_uz(A, B, C)   gp_atomic_compare_exchange_weak_u64(A, B, C)
#  define gp_atomic_store_explicit_uz(A, C, MO)         gp_atomic_store_explicit_u64(A, C, MO)
#  define gp_atomic_load_explicit_uz(A, MO)             gp_atomic_load_explicit_u64(A, MO)
#  define gp_atomic_exchange_explicit_uz(A, C, MO)      gp_atomic_exchange_explicit_u64(A, C, MO)
#  define gp_atomic_fetch_add_explicit_uz(A, C, MO)     gp_atomic_fetch_add_explicit_u64(A, C, MO)
#  define gp_atomic_fetch_sub_explicit_uz(A, C, MO)     gp_atomic_fetch_sub_explicit_u64(A, C, MO)
#  define gp_atomic_fetch_or_explicit_uz(A, C, MO)      gp_atomic_fetch_or_explicit_u64(A, C, MO)
#  define gp_atomic_fetch_xor_explicit_uz(A, C, MO)     gp_atomic_fetch_xor_explicit_u64(A, C, MO)
#  define gp_atomic_fetch_and_explicit_uz(A, C, MO)     gp_atomic_fetch_and_explicit_u64(A, C, MO)
#  define gp_atomic_compare_exchange_strong_explicit_uz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_strong_explicit_u64(A, B, C, MO1, MO2)
#  define gp_atomic_compare_exchange_weak_explicit_uz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_weak_explicit_u64(A, B, C, MO1, MO2)
#  define gp_atomic_store_iz(A, C)                      gp_atomic_store_i64(A, C)
#  define gp_atomic_load_iz(A)                          gp_atomic_load_i64(A)
#  define gp_atomic_exchange_iz(A, C)                   gp_atomic_exchange_i64(A, C)
#  define gp_atomic_fetch_add_iz(A, C)                  gp_atomic_fetch_add_i64(A, C)
#  define gp_atomic_fetch_sub_iz(A, C)                  gp_atomic_fetch_sub_i64(A, C)
#  define gp_atomic_fetch_or_iz(A, C)                   gp_atomic_fetch_or_i64(A, C)
#  define gp_atomic_fetch_xor_iz(A, C)                  gp_atomic_fetch_xor_i64(A, C)
#  define gp_atomic_fetch_and_iz(A, C)                  gp_atomic_fetch_and_i64(A, C)
#  define gp_atomic_compare_exchange_strong_iz(A, B, C) gp_atomic_compare_exchange_strong_i64(A, B, C)
#  define gp_atomic_compare_exchange_weak_iz(A, B, C)   gp_atomic_compare_exchange_weak_i64(A, B, C)
#  define gp_atomic_store_explicit_iz(A, C, MO)         gp_atomic_store_explicit_i64(A, C, MO)
#  define gp_atomic_load_explicit_iz(A, MO)             gp_atomic_load_explicit_i64(A, MO)
#  define gp_atomic_exchange_explicit_iz(A, C, MO)      gp_atomic_exchange_explicit_i64(A, C, MO)
#  define gp_atomic_fetch_add_explicit_iz(A, C, MO)     gp_atomic_fetch_add_explicit_i64(A, C, MO)
#  define gp_atomic_fetch_sub_explicit_iz(A, C, MO)     gp_atomic_fetch_sub_explicit_i64(A, C, MO)
#  define gp_atomic_fetch_or_explicit_iz(A, C, MO)      gp_atomic_fetch_or_explicit_i64(A, C, MO)
#  define gp_atomic_fetch_xor_explicit_iz(A, C, MO)     gp_atomic_fetch_xor_explicit_i64(A, C, MO)
#  define gp_atomic_fetch_and_explicit_iz(A, C, MO)     gp_atomic_fetch_and_explicit_i64(A, C, MO)
#  define gp_atomic_compare_exchange_strong_explicit_iz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_strong_explicit_i64(A, B, C, MO1, MO2)
#  define gp_atomic_compare_exchange_weak_explicit_iz(A, B, C, MO1, MO2) \
    atomic_compare_exchange_weak_explicit_i64(A, B, C, MO1, MO2)
#endif

//-------------------------------------
#ifdef GP_USE_C11_ATOMICS

#define gp_atomic_store(A, C)                      atomic_store(&(A)->atomic, C)
#define gp_atomic_load(A)                          atomic_load(&(A)->atomic)
#define gp_atomic_exchange(A, C)                   atomic_exchange(&(A)->atomic, C)
#define gp_atomic_fetch_add(A, C)                  atomic_fetch_add(&(A)->atomic, C)
#define gp_atomic_fetch_sub(A, C)                  atomic_fetch_sub(&(A)->atomic, C)
#define gp_atomic_fetch_or(A, C)                   atomic_fetch_or(&(A)->atomic, C)
#define gp_atomic_fetch_xor(A, C)                  atomic_fetch_xor(&(A)->atomic, C)
#define gp_atomic_fetch_and(A, C)                  atomic_fetch_and(&(A)->atomic, C)
#define gp_atomic_compare_exchange_strong(A, B, C) atomic_compare_exchange_strong(&(A)->atomic, B, C)
#define gp_atomic_compare_exchange_weak(A, B, C)   atomic_compare_exchange_weak(&(A)->atomic, B, C)
#define gp_atomic_store_explicit(A, C, MO)         atomic_store_explicit(&(A)->atomic, C, MO)
#define gp_atomic_load_explicit(A, MO)             atomic_load_explicit(&(A)->atomic, MO)
#define gp_atomic_exchange_explicit(A, C, MO)      atomic_exchange_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_add_explicit(A, C, MO)     atomic_fetch_add_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_sub_explicit(A, C, MO)     atomic_fetch_sub_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_or_explicit(A, C, MO)      atomic_fetch_or_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_xor_explicit(A, C, MO)     atomic_fetch_xor_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_and_explicit(A, C, MO)     atomic_fetch_and_explicit(&(A)->atomic, C, MO)
#define gp_atomic_compare_exchange_strong_explicit(A, B, C, MO1, MO2) \
    atomic_compare_exchange_strong_explicit(&(A)->atomic, B, C, MO1, MO2)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    atomic_compare_exchange_weak_explicit(&(A)->atomic, B, C, MO1, MO2)

GP_ALWAYS_INLINE void gp_atomic_store_i8 (GPAtomicInt8  * a, int8_t   c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_i16(GPAtomicInt16 * a, int16_t  c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_i32(GPAtomicInt32 * a, int32_t  c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_i64(GPAtomicInt64 * a, int64_t  c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u8 (GPAtomicUInt8 * a, uint8_t  c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u16(GPAtomicUInt16* a, uint16_t c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u32(GPAtomicUInt32* a, uint32_t c) { atomic_store(&a->atomic, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u64(GPAtomicUInt64* a, uint64_t c) { atomic_store(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_load_i8 (const GPAtomicInt8  * a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE int16_t  gp_atomic_load_i16(const GPAtomicInt16 * a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE int32_t  gp_atomic_load_i32(const GPAtomicInt32 * a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE int64_t  gp_atomic_load_i64(const GPAtomicInt64 * a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_load_u8 (const GPAtomicUInt8 * a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE uint16_t gp_atomic_load_u16(const GPAtomicUInt16* a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE uint32_t gp_atomic_load_u32(const GPAtomicUInt32* a) { return atomic_load(&a->atomic); }
GP_ALWAYS_INLINE uint64_t gp_atomic_load_u64(const GPAtomicUInt64* a) { return atomic_load(&a->atomic); }

GP_ALWAYS_INLINE int8_t   gp_atomic_exchange_i8 (GPAtomicInt8  * a, int8_t   c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_exchange_i16(GPAtomicInt16 * a, int16_t  c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_exchange_i32(GPAtomicInt32 * a, int32_t  c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_exchange_i64(GPAtomicInt64 * a, int64_t  c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_exchange_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_exchange_u16(GPAtomicUInt16* a, uint16_t c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_exchange_u32(GPAtomicUInt32* a, uint32_t c)
{ return atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_exchange_u64(GPAtomicUInt64* a, uint64_t c)
{ return atomic_exchange(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_add_i8 (GPAtomicInt8  * a, int8_t   c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_add_i16(GPAtomicInt16 * a, int16_t  c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_add_i32(GPAtomicInt32 * a, int32_t  c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_add_i64(GPAtomicInt64 * a, int64_t  c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_add_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_add_u16(GPAtomicUInt16* a, uint16_t c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_add_u32(GPAtomicUInt32* a, uint32_t c)
{ return atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_add_u64(GPAtomicUInt64* a, uint64_t c)
{ return atomic_fetch_add(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_sub_i8 (GPAtomicInt8  * a, int8_t   c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_sub_i16(GPAtomicInt16 * a, int16_t  c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_sub_i32(GPAtomicInt32 * a, int32_t  c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_sub_i64(GPAtomicInt64 * a, int64_t  c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_sub_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_sub_u16(GPAtomicUInt16* a, uint16_t c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_sub_u32(GPAtomicUInt32* a, uint32_t c)
{ return atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_sub_u64(GPAtomicUInt64* a, uint64_t c)
{ return atomic_fetch_sub(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_or_i8 (GPAtomicInt8  * a, int8_t   c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_or_i16(GPAtomicInt16 * a, int16_t  c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_or_i32(GPAtomicInt32 * a, int32_t  c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_or_i64(GPAtomicInt64 * a, int64_t  c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_or_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_or_u16(GPAtomicUInt16* a, uint16_t c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_or_u32(GPAtomicUInt32* a, uint32_t c)
{ return atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_or_u64(GPAtomicUInt64* a, uint64_t c)
{ return atomic_fetch_or(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_xor_i8 (GPAtomicInt8  * a, int8_t   c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_xor_i16(GPAtomicInt16 * a, int16_t  c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_xor_i32(GPAtomicInt32 * a, int32_t  c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_xor_i64(GPAtomicInt64 * a, int64_t  c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_xor_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_xor_u16(GPAtomicUInt16* a, uint16_t c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_xor_u32(GPAtomicUInt32* a, uint32_t c)
{ return atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_xor_u64(GPAtomicUInt64* a, uint64_t c)
{ return atomic_fetch_xor(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_and_i8 (GPAtomicInt8  * a, int8_t   c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_and_i16(GPAtomicInt16 * a, int16_t  c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_and_i32(GPAtomicInt32 * a, int32_t  c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_and_i64(GPAtomicInt64 * a, int64_t  c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_and_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_and_u16(GPAtomicUInt16* a, uint16_t c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_and_u32(GPAtomicUInt32* a, uint32_t c)
{ return atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_and_u64(GPAtomicUInt64* a, uint64_t c)
{ return atomic_fetch_and(&a->atomic, c); }

GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i8 (GPAtomicInt8  * a, int8_t  * b, int8_t   c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i16(GPAtomicInt16 * a, int16_t * b, int16_t  c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i32(GPAtomicInt32 * a, int32_t * b, int32_t  c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i64(GPAtomicInt64 * a, int64_t * b, int64_t  c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u8 (GPAtomicUInt8 * a, uint8_t * b, uint8_t  c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return atomic_compare_exchange_strong(&a->atomic, b, c); }

GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i8 (GPAtomicInt8  * a, int8_t  * b, int8_t   c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i16(GPAtomicInt16 * a, int16_t * b, int16_t  c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i32(GPAtomicInt32 * a, int32_t * b, int32_t  c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i64(GPAtomicInt64 * a, int64_t * b, int64_t  c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u8 (GPAtomicUInt8 * a, uint8_t * b, uint8_t  c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return atomic_compare_exchange_weak(&a->atomic, b, c); }

#define gp_atomic_thread_fence(MO) atomic_thread_fence(MO)
#define gp_atomic_signal_fence(MO) atomic_signal_fence(MO)

// ------------------------------------
#elif defined(GP_USE_CPP_ATOIMCS)

#define gp_atomic_store(A, C)                      std::atomic_store(&(A)->atomic, C)
#define gp_atomic_load(A)                          std::atomic_load(&(A)->atomic)
#define gp_atomic_exchange(A, C)                   std::atomic_exchange(&(A)->atomic, C)
#define gp_atomic_fetch_add(A, C)                  std::atomic_fetch_add(&(A)->atomic, C)
#define gp_atomic_fetch_sub(A, C)                  std::atomic_fetch_sub(&(A)->atomic, C)
#define gp_atomic_fetch_or(A, C)                   std::atomic_fetch_or(&(A)->atomic, C)
#define gp_atomic_fetch_xor(A, C)                  std::atomic_fetch_xor(&(A)->atomic, C)
#define gp_atomic_fetch_and(A, C)                  std::atomic_fetch_and(&(A)->atomic, C)
#define gp_atomic_compare_exchange_strong(A, B, C) std::atomic_compare_exchange_strong(&(A)->atomic, B, C)
#define gp_atomic_compare_exchange_weak(A, B, C)   std::atomic_compare_exchange_weak(&(A)->atomic, B, C)
#define gp_atomic_store_explicit(A, C, MO)         std::atomic_store_explicit(&(A)->atomic, C, MO)
#define gp_atomic_load_explicit(A, MO)             std::atomic_load_explicit(&(A)->atomic, MO)
#define gp_atomic_exchange_explicit(A, C, MO)      std::atomic_exchange_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_add_explicit(A, C, MO)     std::atomic_fetch_add_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_sub_explicit(A, C, MO)     std::atomic_fetch_sub_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_or_explicit(A, C, MO)      std::atomic_fetch_or_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_xor_explicit(A, C, MO)     std::atomic_fetch_xor_explicit(&(A)->atomic, C, MO)
#define gp_atomic_fetch_and_explicit(A, C, MO)     std::atomic_fetch_and_explicit(&(A)->atomic, C, MO)
#define gp_atomic_compare_exchange_strong_explicit(A, B, C, MO1, MO2) \
    std::atomic_compare_exchange_strong_explicit(&(A)->atomic, B, C, MO1, MO2)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    std::atomic_compare_exchange_weak_explicit(&(A)->atomic, B, C, MO1, MO2)

GP_ALWAYS_INLINE void gp_atomic_store_i8 (GPAtomicInt8  * a, int8_t   c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_i16(GPAtomicInt16 * a, int16_t  c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_i32(GPAtomicInt32 * a, int32_t  c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_i64(GPAtomicInt64 * a, int64_t  c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_u8 (GPAtomicUInt8 * a, uint8_t  c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_u16(GPAtomicUInt16* a, uint16_t c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_u32(GPAtomicUInt32* a, uint32_t c) {std::atomic_store(&a->atomic, c);}
GP_ALWAYS_INLINE void gp_atomic_store_u64(GPAtomicUInt64* a, uint64_t c) {std::atomic_store(&a->atomic, c);}

GP_ALWAYS_INLINE int8_t   gp_atomic_load_i8 (const GPAtomicInt8  * a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE int16_t  gp_atomic_load_i16(const GPAtomicInt16 * a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE int32_t  gp_atomic_load_i32(const GPAtomicInt32 * a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE int64_t  gp_atomic_load_i64(const GPAtomicInt64 * a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE uint8_t  gp_atomic_load_u8 (const GPAtomicUInt8 * a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE uint16_t gp_atomic_load_u16(const GPAtomicUInt16* a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE uint32_t gp_atomic_load_u32(const GPAtomicUInt32* a) {return std::atomic_load(&a->atomic);}
GP_ALWAYS_INLINE uint64_t gp_atomic_load_u64(const GPAtomicUInt64* a) {return std::atomic_load(&a->atomic);}

GP_ALWAYS_INLINE int8_t   gp_atomic_exchange_i8 (GPAtomicInt8  * a, int8_t   c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_exchange_i16(GPAtomicInt16 * a, int16_t  c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_exchange_i32(GPAtomicInt32 * a, int32_t  c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_exchange_i64(GPAtomicInt64 * a, int64_t  c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_exchange_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_exchange_u16(GPAtomicUInt16* a, uint16_t c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_exchange_u32(GPAtomicUInt32* a, uint32_t c)
{ return std::atomic_exchange(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_exchange_u64(GPAtomicUInt64* a, uint64_t c)
{ return std::atomic_exchange(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_add_i8 (GPAtomicInt8  * a, int8_t   c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_add_i16(GPAtomicInt16 * a, int16_t  c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_add_i32(GPAtomicInt32 * a, int32_t  c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_add_i64(GPAtomicInt64 * a, int64_t  c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_add_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_add_u16(GPAtomicUInt16* a, uint16_t c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_add_u32(GPAtomicUInt32* a, uint32_t c)
{ return std::atomic_fetch_add(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_add_u64(GPAtomicUInt64* a, uint64_t c)
{ return std::atomic_fetch_add(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_sub_i8 (GPAtomicInt8  * a, int8_t   c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_sub_i16(GPAtomicInt16 * a, int16_t  c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_sub_i32(GPAtomicInt32 * a, int32_t  c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_sub_i64(GPAtomicInt64 * a, int64_t  c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_sub_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_sub_u16(GPAtomicUInt16* a, uint16_t c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_sub_u32(GPAtomicUInt32* a, uint32_t c)
{ return std::atomic_fetch_sub(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_sub_u64(GPAtomicUInt64* a, uint64_t c)
{ return std::atomic_fetch_sub(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_or_i8 (GPAtomicInt8  * a, int8_t   c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_or_i16(GPAtomicInt16 * a, int16_t  c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_or_i32(GPAtomicInt32 * a, int32_t  c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_or_i64(GPAtomicInt64 * a, int64_t  c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_or_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_or_u16(GPAtomicUInt16* a, uint16_t c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_or_u32(GPAtomicUInt32* a, uint32_t c)
{ return std::atomic_fetch_or(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_or_u64(GPAtomicUInt64* a, uint64_t c)
{ return std::atomic_fetch_or(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_xor_i8 (GPAtomicInt8  * a, int8_t   c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_xor_i16(GPAtomicInt16 * a, int16_t  c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_xor_i32(GPAtomicInt32 * a, int32_t  c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_xor_i64(GPAtomicInt64 * a, int64_t  c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_xor_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_xor_u16(GPAtomicUInt16* a, uint16_t c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_xor_u32(GPAtomicUInt32* a, uint32_t c)
{ return std::atomic_fetch_xor(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_xor_u64(GPAtomicUInt64* a, uint64_t c)
{ return std::atomic_fetch_xor(&a->atomic, c); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_and_i8 (GPAtomicInt8  * a, int8_t   c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_and_i16(GPAtomicInt16 * a, int16_t  c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_and_i32(GPAtomicInt32 * a, int32_t  c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_and_i64(GPAtomicInt64 * a, int64_t  c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_and_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_and_u16(GPAtomicUInt16* a, uint16_t c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_and_u32(GPAtomicUInt32* a, uint32_t c)
{ return std::atomic_fetch_and(&a->atomic, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_and_u64(GPAtomicUInt64* a, uint64_t c)
{ return std::atomic_fetch_and(&a->atomic, c); }

GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i8 (GPAtomicInt8  * a, int8_t  * b, int8_t   c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i16(GPAtomicInt16 * a, int16_t * b, int16_t  c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i32(GPAtomicInt32 * a, int32_t * b, int32_t  c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i64(GPAtomicInt64 * a, int64_t * b, int64_t  c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u8 (GPAtomicUInt8 * a, uint8_t * b, uint8_t  c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return std::atomic_compare_exchange_strong(&a->atomic, b, c); }

GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i8 (GPAtomicInt8  * a, int8_t  * b, int8_t   c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i16(GPAtomicInt16 * a, int16_t * b, int16_t  c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i32(GPAtomicInt32 * a, int32_t * b, int32_t  c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i64(GPAtomicInt64 * a, int64_t * b, int64_t  c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u8 (GPAtomicUInt8 * a, uint8_t * b, uint8_t  c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return std::atomic_compare_exchange_weak(&a->atomic, b, c); }

#define gp_atomic_thread_fence(MO) std::atomic_thread_fence(MO)
#define gp_atomic_signal_fence(MO) std::atomic_signal_fence(MO)

// ------------------------------------
#elif defined(GP_USE_INTERLOCKED_INTRINSICS)

#  if defined(GP_TARGET_ARCH_X86) // use locks for 64-bit functions.
extern GPMutex gp_g_atomic_mutex_pool[128];
#   define GP_INTERLOCKED64_BODY(OP) \
{ \
    long long _old; \
    GPMutex* _mutex = &gp_g_atomic_mutex_pool[ \
        ((uintptr_t)a) & (countof(gp_s_atomic_mutex_pool) - 1)]; \
    gp_mutex_lock(_mutex); \
    _old = *a; \
    *a OP c; \
    gp_mutex_unlock(_mutex); \
    return _old; \
}
GP_ALWAYS_INLINE_NO_EXPORT long long gp_InterlockedExchange64(volatile long long* a, long long c)
GP_INTERLOCKED64_BODY(=)
GP_ALWAYS_INLINE_NO_EXPORT long long gp_InterlockedExchangeAdd64(volatile long long* a, long long c)
GP_INTERLOCKED64_BODY(+=)
GP_ALWAYS_INLINE_NO_EXPORT long long gp_InterlockedOr64(volatile long long* a, long long c)
GP_INTERLOCKED64_BODY(|=)
GP_ALWAYS_INLINE_NO_EXPORT long long gp_InterlockedXor64(volatile long long* a, long long c)
GP_INTERLOCKED64_BODY(^=)
GP_ALWAYS_INLINE_NO_EXPORT long long gp_InterlockedAnd64(volatile long long* a, long long c)
GP_INTERLOCKED64_BODY(&=)
#    define _InterlockedExchange64(a, c)    gp_InterlockedExchange64(a, c)
#    define _InterlockedExchangeAdd64(a, c) gp_InterlockedExchangeAdd64(a, c)
#    define _InterlockedOr64(a, c)          gp_InterlockedOr64(a, c)
#    define _InterlockedXor64(a, c)         gp_InterlockedXor64(a, c)
#    define _InterlockedAnd64(a, c)         gp_InterlockedAnd64(a, c)
long long gp_InterlockedCompareExchange64(volatile long long* a, long long b, long long c)
{
    long long old;
    GPMutex* mutex = &gp_g_atomic_mutex_pool[
        ((uintptr_t)a) & (countof(gp_s_atomic_mutex_pool) - 1)];
    gp_mutex_lock(mutex);
    old = *a;
    if (*a == c)
        *a = b;
    gp_mutex_unlock(mutex);
    return old;
}
#    define _InterlockedCompareExchange64(a, b, c) gp_InterlockedCompareExchange64(a, b, c)
#  endif // GP_TARGET_ARCH_X86

#define gp_atomic_store(A, C) ((void) \
( \
    sizeof(*(A)) == 1 ? _InterlockedExchange8 ((char     *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 2 ? _InterlockedExchange16((short    *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 4 ? _InterlockedExchange  ((long     *)&(A)->_native, (C)) : \
                        _InterlockedExchange64((long long*)&(A)->_native, (C))   \
))
GP_ALWAYS_INLINE void gp_atomic_store_i8(GPAtomicInt8* a, int8_t c)
{ _InterlockedExchange8(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u8(GPAtomicUInt8* a, uint8_t c)
{ _InterlockedExchange8(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_i16(GPAtomicInt16* a, int16_t c)
{ _InterlockedExchange16(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u16(GPAtomicUInt16* a, uint16_t c)
{ _InterlockedExchange16(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_i32(GPAtomicInt32* a, int32_t c)
{ _InterlockedExchange(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u32(GPAtomicUInt32* a, uint32_t c)
{ _InterlockedExchange(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_i64(GPAtomicInt64* a, int64_t c)
{ _InterlockedExchange64(&a->_native, c); }
GP_ALWAYS_INLINE void gp_atomic_store_u64(GPAtomicUInt64* a, uint64_t c)
{ _InterlockedExchange64(&a->_native, c); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_load(A) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_load_i8 , GPAtomicUInt8 *: gp_atomic_load_u8 , \
    GPAtomicInt16*: gp_atomic_load_i16, GPAtomicUInt16*: gp_atomic_load_u16, \
    GPAtomicInt32*: gp_atomic_load_i32, GPAtomicUInt32*: gp_atomic_load_u32, \
    GPAtomicInt64*: gp_atomic_load_i64, GPAtomicUInt64*: gp_atomic_load_u64)(A)
#else
#define gp_atomic_load(A) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedOr8 ((char     *)&(A)->_native, 0) : \
    sizeof(*(A)) == 2 ? _InterlockedOr16((short    *)&(A)->_native, 0) : \
    sizeof(*(A)) == 4 ? _InterlockedOr  ((long     *)&(A)->_native, 0) : \
                        _InterlockedOr64((long long*)&(A)->_native, 0)   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_load_i8(const GPAtomicInt8* a)
{ return _InterlockedOr8(&a->_native, 0); }
GP_ALWAYS_INLINE uint8_t gp_atomic_load_u8(const GPAtomicUInt8* a)
{ return _InterlockedOr8(&a->_native, 0); }
GP_ALWAYS_INLINE int16_t gp_atomic_load_i16(const GPAtomicInt16* a)
{ return _InterlockedOr16(&a->_native, 0); }
GP_ALWAYS_INLINE uint16_t gp_atomic_load_u16(const GPAtomicUInt16* a)
{ return _InterlockedOr16(&a->_native, 0); }
GP_ALWAYS_INLINE int32_t gp_atomic_load_i32(const GPAtomicInt32* a)
{ return _InterlockedOr(&a->_native, 0); }
GP_ALWAYS_INLINE uint32_t gp_atomic_load_u32(const GPAtomicUInt32* a)
{ return _InterlockedOr(&a->_native, 0); }
GP_ALWAYS_INLINE int64_t gp_atomic_load_i64(const GPAtomicInt64* a)
{ return _InterlockedOr64(&a->_native, 0); }
GP_ALWAYS_INLINE uint64_t gp_atomic_load_u64(const GPAtomicUInt64* a)
{ return _InterlockedOr64(&a->_native, 0); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_exchange(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_exchange_i8 , GPAtomicUInt8 *: gp_atomic_exchange_u8 , \
    GPAtomicInt16*: gp_atomic_exchange_i16, GPAtomicUInt16*: gp_atomic_exchange_u16, \
    GPAtomicInt32*: gp_atomic_exchange_i32, GPAtomicUInt32*: gp_atomic_exchange_u32, \
    GPAtomicInt64*: gp_atomic_exchange_i64, GPAtomicUInt64*: gp_atomic_exchange_u64)((A), (C))
#else
#define gp_atomic_exchange(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedExchange8 ((char     *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 2 ? _InterlockedExchange16((short    *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 4 ? _InterlockedExchange  ((long     *)&(A)->_native, (C)) : \
                        _InterlockedExchange64((long long*)&(A)->_native, (C))   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_exchange_i8(GPAtomicInt8* a, int8_t c)
{ return _InterlockedExchange8(&a->_native, c); }
GP_ALWAYS_INLINE uint8_t gp_atomic_exchange_u8(GPAtomicUInt8* a, uint8_t c)
{ return _InterlockedExchange8(&a->_native, c); }
GP_ALWAYS_INLINE int16_t gp_atomic_exchange_i16(GPAtomicInt16* a, int16_t c)
{ return _InterlockedExchange16(&a->_native, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_exchange_u16(GPAtomicUInt16* a, uint16_t c)
{ return _InterlockedExchange16(&a->_native, c); }
GP_ALWAYS_INLINE int32_t gp_atomic_exchange_i32(GPAtomicInt32* a, int32_t c)
{ return _InterlockedExchange(&a->_native, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_exchange_u32(GPAtomicUInt32* a, uint32_t c)
{ return _InterlockedExchange(&a->_native, c); }
GP_ALWAYS_INLINE int64_t gp_atomic_exchange_i64(GPAtomicInt64* a, int64_t c)
{ return _InterlockedExchange64(&a->_native, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_exchange_u64(GPAtomicUInt64* a, uint64_t c)
{ return _InterlockedExchange64(&a->_native, c); }

GP_ALWAYS_INLINE bool gp_internal_compare_exchange_i8(char* a, int8_t* b, int8_t c)
{
    char b1 = *b;
    char a1 = _InterlockedCompareExchange8(a, c, b1);
    bool result = a1 == b1;
    if ( ! result)
        *b = a1;
    return result;
}
GP_ALWAYS_INLINE bool gp_internal_compare_exchange_i16(short* a, uint16_t* b, uint16_t c)
{
    short b1 = *b;
    short a1 = _InterlockedCompareExchange16(a, c, b1);
    bool result = a1 == b1;
    if ( ! result)
        *b = a1;
    return result;
}
GP_ALWAYS_INLINE bool gp_internal_compare_exchange_i32(long* a, uint32_t* b, uint32_t c)
{
    long b1 = *b;
    long a1 = _InterlockedCompareExchange(a, c, b1);
    bool result = a1 == b1;
    if ( ! result)
        *b = a1;
    return result;
}
GP_ALWAYS_INLINE bool gp_internal_compare_exchange_i64(long long* a, uint64_t* b, uint64_t c)
{
    long long b1 = *b;
    long long a1 = _InterlockedCompareExchange64(a, c, b1);
    bool result = a1 == b1;
    if ( ! result)
        *b = a1;
    return result;
}

#define gp_atomic_compare_exchange_strong(A, B, C) \
( \
    sizeof(*(A)) == 1 ? gp_internal_compare_exchange_i8 ((char     *)&(A)->_native, (B), (C)) : \
    sizeof(*(A)) == 2 ? gp_internal_compare_exchange_i16((short    *)&(A)->_native, (B), (C)) : \
    sizeof(*(A)) == 4 ? gp_internal_compare_exchange_i32((long     *)&(A)->_native, (B), (C)) : \
                        gp_internal_compare_exchange_i64((long long*)&(A)->_native, (B), (C))   \
))
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i8(GPAtomicInt8* a, int8_t* b, int8_t c)
{ return gp_internal_compare_exchange_i8(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u8(GPAtomicUInt8* a, uint8_t* b, uint8_t c)
{ return gp_internal_compare_exchange_i8(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return gp_internal_compare_exchange_i16(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return gp_internal_compare_exchange_i16(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return gp_internal_compare_exchange_i32(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return gp_internal_compare_exchange_i32(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return gp_internal_compare_exchange_i64(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return gp_internal_compare_exchange_i64(&a->_native, b, c); }

#define gp_atomic_compare_exchange_weak(A, B, C) \
( \
    sizeof(*(A)) == 1 ? gp_internal_compare_exchange_i8 ((char     *)&(A)->_native, (B), (C)) : \
    sizeof(*(A)) == 2 ? gp_internal_compare_exchange_i16((short    *)&(A)->_native, (B), (C)) : \
    sizeof(*(A)) == 4 ? gp_internal_compare_exchange_i32((long     *)&(A)->_native, (B), (C)) : \
                        gp_internal_compare_exchange_i64((long long*)&(A)->_native, (B), (C))   \
))
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i8(GPAtomicInt8* a, int8_t* b, int8_t c)
{ return gp_internal_compare_exchange_i8(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u8(GPAtomicUInt8* a, uint8_t* b, uint8_t c)
{ return gp_internal_compare_exchange_i8(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return gp_internal_compare_exchange_i16(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return gp_internal_compare_exchange_i16(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return gp_internal_compare_exchange_i32(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return gp_internal_compare_exchange_i32(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return gp_internal_compare_exchange_i64(&a->_native, b, c); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return gp_internal_compare_exchange_i64(&a->_native, b, c); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_add(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_add_i8 , GPAtomicUInt8 *: gp_atomic_fetch_add_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_add_i16, GPAtomicUInt16*: gp_atomic_fetch_add_u16, \
    GPAtomicInt32*: gp_atomic_fetch_add_i32, GPAtomicUInt32*: gp_atomic_fetch_add_u32, \
    GPAtomicInt64*: gp_atomic_fetch_add_i64, GPAtomicUInt64*: gp_atomic_fetch_add_u64)((A), (C))
#else
#define gp_atomic_fetch_add(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedExchangeAdd8 ((char     *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 2 ? _InterlockedExchangeAdd16((short    *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 4 ? _InterlockedExchangeAdd  ((long     *)&(A)->_native, (C)) : \
                        _InterlockedExchangeAdd64((long long*)&(A)->_native, (C))   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_fetch_add_i8(GPAtomicInt8* a, int8_t c)
{ return _InterlockedExchangeAdd8(&a->_native, c); }
GP_ALWAYS_INLINE uint8_t gp_atomic_fetch_add_u8(GPAtomicUInt8* a, uint8_t c)
{ return _InterlockedExchangeAdd8(&a->_native, c); }
GP_ALWAYS_INLINE int16_t gp_atomic_fetch_add_i16(GPAtomicInt16* a, int16_t c)
{ return _InterlockedExchangeAdd16(&a->_native, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_add_u16(GPAtomicUInt16* a, uint16_t c)
{ return _InterlockedExchangeAdd16(&a->_native, c); }
GP_ALWAYS_INLINE int32_t gp_atomic_fetch_add_i32(GPAtomicInt32* a, int32_t c)
{ return _InterlockedExchangeAdd(&a->_native, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_add_u32(GPAtomicUInt32* a, uint32_t c)
{ return _InterlockedExchangeAdd(&a->_native, c); }
GP_ALWAYS_INLINE int64_t gp_atomic_fetch_add_i64(GPAtomicInt64* a, int64_t c)
{ return _InterlockedExchangeAdd64(&a->_native, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_add_u64(GPAtomicUInt64* a, uint64_t c)
{ return _InterlockedExchangeAdd64(&a->_native, c); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_sub(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_sub_i8 , GPAtomicUInt8 *: gp_atomic_fetch_sub_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_sub_i16, GPAtomicUInt16*: gp_atomic_fetch_sub_u16, \
    GPAtomicInt32*: gp_atomic_fetch_sub_i32, GPAtomicUInt32*: gp_atomic_fetch_sub_u32, \
    GPAtomicInt64*: gp_atomic_fetch_sub_i64, GPAtomicUInt64*: gp_atomic_fetch_sub_u64)((A), (C))
#else
#define gp_atomic_fetch_sub(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedExchangeAdd8 ((char     *)&(A)->_native, -(C)) : \
    sizeof(*(A)) == 2 ? _InterlockedExchangeAdd16((short    *)&(A)->_native, -(C)) : \
    sizeof(*(A)) == 4 ? _InterlockedExchangeAdd  ((long     *)&(A)->_native, -(C)) : \
                        _InterlockedExchangeAdd64((long long*)&(A)->_native, -(C))   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_fetch_sub_i8(GPAtomicInt8* a, int8_t c)
{ return _InterlockedExchangeAdd8(&a->_native, -c); }
GP_ALWAYS_INLINE uint8_t gp_atomic_fetch_sub_u8(GPAtomicUInt8* a, uint8_t c)
{ return _InterlockedExchangeAdd8(&a->_native, -c); }
GP_ALWAYS_INLINE int16_t gp_atomic_fetch_sub_i16(GPAtomicInt16* a, int16_t c)
{ return _InterlockedExchangeAdd16(&a->_native, -c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_sub_u16(GPAtomicUInt16* a, uint16_t c)
{ return _InterlockedExchangeAdd16(&a->_native, -c); }
GP_ALWAYS_INLINE int32_t gp_atomic_fetch_sub_i32(GPAtomicInt32* a, int32_t c)
{ return _InterlockedExchangeAdd(&a->_native, -c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_sub_u32(GPAtomicUInt32* a, uint32_t c)
{ return _InterlockedExchangeAdd(&a->_native, -c); }
GP_ALWAYS_INLINE int64_t gp_atomic_fetch_sub_i64(GPAtomicInt64* a, int64_t c)
{ return _InterlockedExchangeAdd64(&a->_native, -c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_sub_u64(GPAtomicUInt64* a, uint64_t c)
{ return _InterlockedExchangeAdd64(&a->_native, -c); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_or(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_or_i8 , GPAtomicUInt8 *: gp_atomic_fetch_or_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_or_i16, GPAtomicUInt16*: gp_atomic_fetch_or_u16, \
    GPAtomicInt32*: gp_atomic_fetch_or_i32, GPAtomicUInt32*: gp_atomic_fetch_or_u32, \
    GPAtomicInt64*: gp_atomic_fetch_or_i64, GPAtomicUInt64*: gp_atomic_fetch_or_u64)((A), (C))
#else
#define gp_atomic_fetch_or(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedOr8 ((char     *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 2 ? _InterlockedOr16((short    *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 4 ? _InterlockedOr  ((long     *)&(A)->_native, (C)) : \
                        _InterlockedOr64((long long*)&(A)->_native, (C))   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_fetch_or_i8(GPAtomicInt8* a, int8_t c)
{ return _InterlockedOr8(&a->_native, c); }
GP_ALWAYS_INLINE uint8_t gp_atomic_fetch_or_u8(GPAtomicUInt8* a, uint8_t c)
{ return _InterlockedOr8(&a->_native, c); }
GP_ALWAYS_INLINE int16_t gp_atomic_fetch_or_i16(GPAtomicInt16* a, int16_t c)
{ return _InterlockedOr16(&a->_native, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_or_u16(GPAtomicUInt16* a, uint16_t c)
{ return _InterlockedOr16(&a->_native, c); }
GP_ALWAYS_INLINE int32_t gp_atomic_fetch_or_i32(GPAtomicInt32* a, int32_t c)
{ return _InterlockedOr(&a->_native, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_or_u32(GPAtomicUInt32* a, uint32_t c)
{ return _InterlockedOr(&a->_native, c); }
GP_ALWAYS_INLINE int64_t gp_atomic_fetch_or_i64(GPAtomicInt64* a, int64_t c)
{ return _InterlockedOr64(&a->_native, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_or_u64(GPAtomicUInt64* a, uint64_t c)
{ return _InterlockedOr64(&a->_native, c); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_xor(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_xor_i8 , GPAtomicUInt8 *: gp_atomic_fetch_xor_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_xor_i16, GPAtomicUInt16*: gp_atomic_fetch_xor_u16, \
    GPAtomicInt32*: gp_atomic_fetch_xor_i32, GPAtomicUInt32*: gp_atomic_fetch_xor_u32, \
    GPAtomicInt64*: gp_atomic_fetch_xor_i64, GPAtomicUInt64*: gp_atomic_fetch_xor_u64)((A), (C))
#else
#define gp_atomic_fetch_xor(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedXor8 ((char     *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 2 ? _InterlockedXor16((short    *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 4 ? _InterlockedXor  ((long     *)&(A)->_native, (C)) : \
                        _InterlockedXor64((long long*)&(A)->_native, (C))   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_fetch_xor_i8(GPAtomicInt8* a, int8_t c)
{ return _InterlockedXor8(&a->_native, c); }
GP_ALWAYS_INLINE uint8_t gp_atomic_fetch_xor_u8(GPAtomicUInt8* a, uint8_t c)
{ return _InterlockedXor8(&a->_native, c); }
GP_ALWAYS_INLINE int16_t gp_atomic_fetch_xor_i16(GPAtomicInt16* a, int16_t c)
{ return _InterlockedXor16(&a->_native, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_xor_u16(GPAtomicUInt16* a, uint16_t c)
{ return _InterlockedXor16(&a->_native, c); }
GP_ALWAYS_INLINE int32_t gp_atomic_fetch_xor_i32(GPAtomicInt32* a, int32_t c)
{ return _InterlockedXor(&a->_native, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_xor_u32(GPAtomicUInt32* a, uint32_t c)
{ return _InterlockedXor(&a->_native, c); }
GP_ALWAYS_INLINE int64_t gp_atomic_fetch_xor_i64(GPAtomicInt64* a, int64_t c)
{ return _InterlockedXor64(&a->_native, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_xor_u64(GPAtomicUInt64* a, uint64_t c)
{ return _InterlockedXor64(&a->_native, c); }

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_and(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_and_i8 , GPAtomicUInt8 *: gp_atomic_fetch_and_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_and_i16, GPAtomicUInt16*: gp_atomic_fetch_and_u16, \
    GPAtomicInt32*: gp_atomic_fetch_and_i32, GPAtomicUInt32*: gp_atomic_fetch_and_u32, \
    GPAtomicInt64*: gp_atomic_fetch_and_i64, GPAtomicUInt64*: gp_atomic_fetch_and_u64)((A), (C))
#else
#define gp_atomic_fetch_and(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? _InterlockedAnd8 ((char     *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 2 ? _InterlockedAnd16((short    *)&(A)->_native, (C)) : \
    sizeof(*(A)) == 4 ? _InterlockedAnd  ((long     *)&(A)->_native, (C)) : \
                        _InterlockedAnd64((long long*)&(A)->_native, (C))   \
))
#endif
GP_ALWAYS_INLINE int8_t gp_atomic_fetch_and_i8(GPAtomicInt8* a, int8_t c)
{ return _InterlockedAnd8(&a->_native, c); }
GP_ALWAYS_INLINE uint8_t gp_atomic_fetch_and_u8(GPAtomicUInt8* a, uint8_t c)
{ return _InterlockedAnd8(&a->_native, c); }
GP_ALWAYS_INLINE int16_t gp_atomic_fetch_and_i16(GPAtomicInt16* a, int16_t c)
{ return _InterlockedAnd16(&a->_native, c); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_and_u16(GPAtomicUInt16* a, uint16_t c)
{ return _InterlockedAnd16(&a->_native, c); }
GP_ALWAYS_INLINE int32_t gp_atomic_fetch_and_i32(GPAtomicInt32* a, int32_t c)
{ return _InterlockedAnd(&a->_native, c); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_and_u32(GPAtomicUInt32* a, uint32_t c)
{ return _InterlockedAnd(&a->_native, c); }
GP_ALWAYS_INLINE int64_t gp_atomic_fetch_and_i64(GPAtomicInt64* a, int64_t c)
{ return _InterlockedAnd64(&a->_native, c); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_and_u64(GPAtomicUInt64* a, uint64_t c)
{ return _InterlockedAnd64(&a->_native, c); }

// TODO memory ordering temporarily ignored for simplicity, should implement though.

#define gp_atomic_store_explicit(A, C, MO)     gp_atomic_store(A, C)
#define gp_atomic_load_explicit(A, MO)         gp_atomic_load(A)
#define gp_atomic_exchange_explicit(A, C, MO)  gp_atomic_exchange(A, C)
#define gp_atomic_fetch_add_explicit(A, C, MO) gp_atomic_fetch_add(A, C)
#define gp_atomic_fetch_sub_explicit(A, C, MO) gp_atomic_fetch_sub(A, C)
#define gp_atomic_fetch_or_explicit(A, C, MO)  gp_atomic_fetch_or(A, C)
#define gp_atomic_fetch_xor_explicit(A, C, MO) gp_atomic_fetch_xor(A, C)
#define gp_atomic_fetch_and_explicit(A, C, MO) gp_atomic_fetch_and(A, C)
#define gp_atomic_compare_exchange_strong_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_strong(A, B, C)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_weak(A, B, C)

// NOTE: The _ReadBarrier, _WriteBarrier, and _ReadWriteBarrier compiler
// intrinsics and the MemoryBarrier macro are all deprecated.
// See https://learn.microsoft.com/en-us/cpp/intrinsics/readbarrier?view=msvc-170.
// (MemoryBarrier is heavy handed anyway on ARM64, where it uses _ARM64_BARRIER_SY).
// HOWEVER, for gp_atomic_signal_fence(), we don't really have options. MSVC at
// the time of writing doesn't even warn about using them, so we'll just do what
// LLVM does and just ignore the deprecation, see https://reviews.llvm.org/D111232?id=377545.

GP_ALWAYS_INLINE_NOEXPORT void gp_atomic_thread_fence(gp_memory_order_t mo)
{
    if (mo == GP_MEMORY_ORDER_RELAXED)
        return;

    #if defined(GP_TARGET_ARCH_X86_64)
    // Turns out that this just generates `lock or DWORD PTR [rsp], 0`, but
    // let's keep it anyway, maybe the compiler treats it slightly more
    // appropriately than `_InterlockedOr(&barrier, 0)` (it probably doesn't).
    if (mo <= GP_MEMORY_ORDER_ACQUIRE)
        _ReadBarrier();
    else if (mo <= GP_MEMORY_ORDER_RELEASE)
        _WriteBarrier();
    else
        _ReadWriteBarrier();
    if (mo == GP_MEMORY_ORDER_SEQ_CST)
        __faststorefence();
    #elif defined(GP_TARGET_ARCH_ARM64) // we'll do what MSVC does https://godbolt.org/z/za1bWGqv8
    // NOTE: Don't use `ISHST` for release, LLVM made that mistake in 2013, see
    // https://llvm.googlesource.com/llvm/+/40d0492cdea1023463a9902ee81b3c5251204039
    __dmb(mo <= GP_MEMORY_ORDER_ACQUIRE ? _ARM64_BARRIER_ISHLD : _ARM64_BARRIER_ISH);
    #elif defined(GP_TARGET_ARCH_ARM32)
    __dmb(mo <= GP_MEMORY_ORDER_ACQUIRE ? _ARM_BARRIER_ISHLD : _ARM_BARRIER_ISH);
    #else
    long barrier;
    _InterlockedOr(&barrier, 0);
    #endif
}

GP_ALWAYS_INLINE_NOEXPORT void gp_atomic_signal_fence(gp_memory_order_t mo)
{
    switch (mo) {
    case GP_MEMORY_ORDER_RELAXED: (void)0; break;
    case GP_MEMORY_ORDER_CONSUME: case GP_MEMORY_ORDER_ACQUIRE: _ReadBarrier(); break;
    case GP_MEMORY_ORDER_RELEASE: _WriteBarrier(); break;
    case GP_ATOMIC_ACQ_REL: case GP_MEMORY_ORDER_SEQ_CST: _ReadWriteBarrier(); break;
    }
}

// ------------------------------------
#elif defined(GP_USE_ATOMIC_BUILTINS)

// NOTE: using GP_MEMORY_ORDER_SEQ_CST instead of __ATOMIC_SEQ_CST, because TCC doesn't
// predefine the builtin atomic macros.

#define gp_atomic_store(A, C)     __atomic_store_n(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_load(A)         __atomic_load_n(&(A)->nonatomic, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_exchange(A, C)  __atomic_exchange_n(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_fetch_add(A, C) __atomic_fetch_add(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_fetch_sub(A, C) __atomic_fetch_sub(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_fetch_or(A, C)  __atomic_fetch_or(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_fetch_xor(A, C) __atomic_fetch_xor(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_fetch_and(A, C) __atomic_fetch_and(&(A)->nonatomic, C, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_compare_exchange_strong(A, B, C) \
    __atomic_compare_exchange_n(&(A)->nonatomic, B, C, false, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST)
#define gp_atomic_compare_exchange_weak(A, B, C) \
    __atomic_compare_exchange_n(&(A)->nonatomic, B, C, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST)

#define gp_atomic_store_explicit(A, C, MO)     __atomic_store_n(&(A)->nonatomic, C, MO)
#define gp_atomic_load_explicit(A, MO)         __atomic_load_n(&(A)->nonatomic, MO)
#define gp_atomic_exchange_explicit(A, C, MO)  __atomic_exchange_n(&(A)->nonatomic, C, MO)
#define gp_atomic_fetch_add_explicit(A, C, MO) __atomic_fetch_add(&(A)->nonatomic, C, MO)
#define gp_atomic_fetch_sub_explicit(A, C, MO) __atomic_fetch_sub(&(A)->nonatomic, C, MO)
#define gp_atomic_fetch_or_explicit(A, C, MO)  __atomic_fetch_or(&(A)->nonatomic, C, MO)
#define gp_atomic_fetch_xor_explicit(A, C, MO) __atomic_fetch_xor(&(A)->nonatomic, C, MO)
#define gp_atomic_fetch_and_explicit(A, C, MO) __atomic_fetch_and(&(A)->nonatomic, C, MO)
#define gp_atomic_compare_exchange_strong_explicit(A, B, C, MO1, MO2) \
    __atomic_compare_exchange_n(&(A)->nonatomic, B, C, false, MO1, MO2)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    __atomic_compare_exchange_n(&(A)->nonatomic, B, C, true, MO1, MO2)

GP_ALWAYS_INLINE void gp_atomic_store_i8 (GPAtomicInt8  * a, int8_t   c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_i16(GPAtomicInt16 * a, int16_t  c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_i32(GPAtomicInt32 * a, int32_t  c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_i64(GPAtomicInt64 * a, int64_t  c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_u8 (GPAtomicUInt8 * a, uint8_t  c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_u16(GPAtomicUInt16* a, uint16_t c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_u32(GPAtomicUInt32* a, uint32_t c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE void gp_atomic_store_u64(GPAtomicUInt64* a, uint64_t c)
{__atomic_store_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST);}

GP_ALWAYS_INLINE int8_t   gp_atomic_load_i8 (const GPAtomicInt8  * a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE int16_t  gp_atomic_load_i16(const GPAtomicInt16 * a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE int32_t  gp_atomic_load_i32(const GPAtomicInt32 * a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE int64_t  gp_atomic_load_i64(const GPAtomicInt64 * a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE uint8_t  gp_atomic_load_u8 (const GPAtomicUInt8 * a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE uint16_t gp_atomic_load_u16(const GPAtomicUInt16* a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE uint32_t gp_atomic_load_u32(const GPAtomicUInt32* a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}
GP_ALWAYS_INLINE uint64_t gp_atomic_load_u64(const GPAtomicUInt64* a)
{return __atomic_load_n(&a->nonatomic, GP_MEMORY_ORDER_SEQ_CST);}

GP_ALWAYS_INLINE int8_t   gp_atomic_exchange_i8 (GPAtomicInt8  * a, int8_t   c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int16_t  gp_atomic_exchange_i16(GPAtomicInt16 * a, int16_t  c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int32_t  gp_atomic_exchange_i32(GPAtomicInt32 * a, int32_t  c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int64_t  gp_atomic_exchange_i64(GPAtomicInt64 * a, int64_t  c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_exchange_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint16_t gp_atomic_exchange_u16(GPAtomicUInt16* a, uint16_t c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint32_t gp_atomic_exchange_u32(GPAtomicUInt32* a, uint32_t c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint64_t gp_atomic_exchange_u64(GPAtomicUInt64* a, uint64_t c)
{ return __atomic_exchange_n(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_add_i8 (GPAtomicInt8  * a, int8_t   c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_add_i16(GPAtomicInt16 * a, int16_t  c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_add_i32(GPAtomicInt32 * a, int32_t  c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_add_i64(GPAtomicInt64 * a, int64_t  c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_add_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_add_u16(GPAtomicUInt16* a, uint16_t c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_add_u32(GPAtomicUInt32* a, uint32_t c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_add_u64(GPAtomicUInt64* a, uint64_t c)
{ return __atomic_fetch_add(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_sub_i8 (GPAtomicInt8  * a, int8_t   c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_sub_i16(GPAtomicInt16 * a, int16_t  c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_sub_i32(GPAtomicInt32 * a, int32_t  c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_sub_i64(GPAtomicInt64 * a, int64_t  c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_sub_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_sub_u16(GPAtomicUInt16* a, uint16_t c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_sub_u32(GPAtomicUInt32* a, uint32_t c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_sub_u64(GPAtomicUInt64* a, uint64_t c)
{ return __atomic_fetch_sub(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_or_i8 (GPAtomicInt8  * a, int8_t   c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_or_i16(GPAtomicInt16 * a, int16_t  c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_or_i32(GPAtomicInt32 * a, int32_t  c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_or_i64(GPAtomicInt64 * a, int64_t  c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_or_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_or_u16(GPAtomicUInt16* a, uint16_t c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_or_u32(GPAtomicUInt32* a, uint32_t c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_or_u64(GPAtomicUInt64* a, uint64_t c)
{ return __atomic_fetch_or(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_xor_i8 (GPAtomicInt8  * a, int8_t   c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_xor_i16(GPAtomicInt16 * a, int16_t  c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_xor_i32(GPAtomicInt32 * a, int32_t  c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_xor_i64(GPAtomicInt64 * a, int64_t  c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_xor_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_xor_u16(GPAtomicUInt16* a, uint16_t c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_xor_u32(GPAtomicUInt32* a, uint32_t c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_xor_u64(GPAtomicUInt64* a, uint64_t c)
{ return __atomic_fetch_xor(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE int8_t   gp_atomic_fetch_and_i8 (GPAtomicInt8  * a, int8_t   c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int16_t  gp_atomic_fetch_and_i16(GPAtomicInt16 * a, int16_t  c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int32_t  gp_atomic_fetch_and_i32(GPAtomicInt32 * a, int32_t  c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE int64_t  gp_atomic_fetch_and_i64(GPAtomicInt64 * a, int64_t  c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint8_t  gp_atomic_fetch_and_u8 (GPAtomicUInt8 * a, uint8_t  c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint16_t gp_atomic_fetch_and_u16(GPAtomicUInt16* a, uint16_t c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint32_t gp_atomic_fetch_and_u32(GPAtomicUInt32* a, uint32_t c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE uint64_t gp_atomic_fetch_and_u64(GPAtomicUInt64* a, uint64_t c)
{ return __atomic_fetch_and(&a->nonatomic, c, GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i8 (GPAtomicInt8  * a, int8_t  * b, int8_t   c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i16(GPAtomicInt16 * a, int16_t * b, int16_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i32(GPAtomicInt32 * a, int32_t * b, int32_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_i64(GPAtomicInt64 * a, int64_t * b, int64_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u8 (GPAtomicUInt8 * a, uint8_t * b, uint8_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_strong_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, false, GP_MEMORY_ORDER_SEQ_CST,GP_MEMORY_ORDER_SEQ_CST); }

GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i8 (GPAtomicInt8  * a, int8_t  * b, int8_t   c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i16(GPAtomicInt16 * a, int16_t * b, int16_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i32(GPAtomicInt32 * a, int32_t * b, int32_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_i64(GPAtomicInt64 * a, int64_t * b, int64_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u8 (GPAtomicUInt8 * a, uint8_t * b, uint8_t  c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u16(GPAtomicUInt16* a, uint16_t* b, uint16_t c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u32(GPAtomicUInt32* a, uint32_t* b, uint32_t c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }
GP_ALWAYS_INLINE bool gp_atomic_compare_exchange_weak_u64(GPAtomicUInt64* a, uint64_t* b, uint64_t c)
{ return __atomic_compare_exchange_n(&a->nonatomic, b, c, true, GP_MEMORY_ORDER_SEQ_CST, GP_MEMORY_ORDER_SEQ_CST); }

#define gp_atomic_thread_fence(MO) __atomic_thread_fence(MO)
#define gp_atomic_signal_fence(MO) __atomic_signal_fence(MO)

//-------------------------------------
#else // use locks

// No inline functions here, this allows less capable compilers to use actual
// atomics when using the installed library that is built with the more capable
// compiler.

#define gp_atomic_store(A, C) ( \
( \
    sizeof(*(A)) == 1 ? gp_atomic_store_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_store_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_store_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_store_i64((GPAtomicInt64*)(A), (C))   \
)
GP_API void gp_atomic_store_i8 (GPAtomicInt8  *, int8_t  );
GP_API void gp_atomic_store_u8 (GPAtomicUInt8 *, uint8_t );
GP_API void gp_atomic_store_i16(GPAtomicInt16 *, int16_t );
GP_API void gp_atomic_store_u16(GPAtomicUInt16*, uint16_t);
GP_API void gp_atomic_store_i32(GPAtomicInt32 *, int32_t );
GP_API void gp_atomic_store_u32(GPAtomicUInt32*, uint32_t);
GP_API void gp_atomic_store_i64(GPAtomicInt64 *, int64_t );
GP_API void gp_atomic_store_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_load(A) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_load_i8 , GPAtomicUInt8 *: gp_atomic_load_u8 , \
    GPAtomicInt16*: gp_atomic_load_i16, GPAtomicUInt16*: gp_atomic_load_u16, \
    GPAtomicInt32*: gp_atomic_load_i32, GPAtomicUInt32*: gp_atomic_load_u32, \
    GPAtomicInt64*: gp_atomic_load_i64, GPAtomicUInt64*: gp_atomic_load_u64)(A)
#else
#define gp_atomic_load(A) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_load_i8 ((GPAtomicInt8 *)(A), 0) : \
    sizeof(*(A)) == 2 ? gp_atomic_load_i16((GPAtomicInt16*)(A), 0) : \
    sizeof(*(A)) == 4 ? gp_atomic_load_i32((GPAtomicInt32*)(A), 0) : \
                        gp_atomic_load_i64((GPAtomicInt64*)(A), 0)   \
))
#endif
GP_API int8_t   gp_atomic_load_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_atomic_load_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_atomic_load_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_atomic_load_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_atomic_load_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_atomic_load_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_atomic_load_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_atomic_load_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_exchange(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_exchange_i8 , GPAtomicUInt8 *: gp_atomic_exchange_u8 , \
    GPAtomicInt16*: gp_atomic_exchange_i16, GPAtomicUInt16*: gp_atomic_exchange_u16, \
    GPAtomicInt32*: gp_atomic_exchange_i32, GPAtomicUInt32*: gp_atomic_exchange_u32, \
    GPAtomicInt64*: gp_atomic_exchange_i64, GPAtomicUInt64*: gp_atomic_exchange_u64)((A), (C))
#else
#define gp_atomic_exchange(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_exchange_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_exchange_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_exchange_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_exchange_i64((GPAtomicInt64*)(A), (C))   \
))
#endif
GP_API int8_t   gp_atomic_exchange_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_atomic_exchange_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_atomic_exchange_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_atomic_exchange_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_atomic_exchange_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_atomic_exchange_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_atomic_exchange_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_atomic_exchange_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_add(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_add_i8 , GPAtomicUInt8 *: gp_atomic_fetch_add_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_add_i16, GPAtomicUInt16*: gp_atomic_fetch_add_u16, \
    GPAtomicInt32*: gp_atomic_fetch_add_i32, GPAtomicUInt32*: gp_atomic_fetch_add_u32, \
    GPAtomicInt64*: gp_atomic_fetch_add_i64, GPAtomicUInt64*: gp_atomic_fetch_add_u64)((A), (C))
#else
#define gp_atomic_fetch_add(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_fetch_add_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_fetch_add_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_fetch_add_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_fetch_add_i64((GPAtomicInt64*)(A), (C))   \
))
#endif
GP_API int8_t   gp_fetch_add_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_fetch_add_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_fetch_add_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_fetch_add_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_fetch_add_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_fetch_add_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_fetch_add_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_fetch_add_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_sub(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_sub_i8 , GPAtomicUInt8 *: gp_atomic_fetch_sub_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_sub_i16, GPAtomicUInt16*: gp_atomic_fetch_sub_u16, \
    GPAtomicInt32*: gp_atomic_fetch_sub_i32, GPAtomicUInt32*: gp_atomic_fetch_sub_u32, \
    GPAtomicInt64*: gp_atomic_fetch_sub_i64, GPAtomicUInt64*: gp_atomic_fetch_sub_u64)((A), (C))
#else
#define gp_atomic_fetch_sub(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_fetch_sub_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_fetch_sub_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_fetch_sub_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_fetch_sub_i64((GPAtomicInt64*)(A), (C))   \
))
#endif
GP_API int8_t   gp_fetch_sub_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_fetch_sub_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_fetch_sub_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_fetch_sub_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_fetch_sub_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_fetch_sub_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_fetch_sub_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_fetch_sub_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_or(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_or_i8 , GPAtomicUInt8 *: gp_atomic_fetch_or_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_or_i16, GPAtomicUInt16*: gp_atomic_fetch_or_u16, \
    GPAtomicInt32*: gp_atomic_fetch_or_i32, GPAtomicUInt32*: gp_atomic_fetch_or_u32, \
    GPAtomicInt64*: gp_atomic_fetch_or_i64, GPAtomicUInt64*: gp_atomic_fetch_or_u64)((A), (C))
#else
#define gp_atomic_fetch_or(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_fetch_or_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_fetch_or_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_fetch_or_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_fetch_or_i64((GPAtomicInt64*)(A), (C))   \
))
#endif
GP_API int8_t   gp_fetch_or_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_fetch_or_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_fetch_or_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_fetch_or_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_fetch_or_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_fetch_or_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_fetch_or_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_fetch_or_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_xor(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_xor_i8 , GPAtomicUInt8 *: gp_atomic_fetch_xor_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_xor_i16, GPAtomicUInt16*: gp_atomic_fetch_xor_u16, \
    GPAtomicInt32*: gp_atomic_fetch_xor_i32, GPAtomicUInt32*: gp_atomic_fetch_xor_u32, \
    GPAtomicInt64*: gp_atomic_fetch_xor_i64, GPAtomicUInt64*: gp_atomic_fetch_xor_u64)((A), (C))
#else
#define gp_atomic_fetch_xor(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_fetch_xor_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_fetch_xor_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_fetch_xor_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_fetch_xor_i64((GPAtomicInt64*)(A), (C))   \
))
#endif
GP_API int8_t   gp_fetch_xor_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_fetch_xor_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_fetch_xor_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_fetch_xor_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_fetch_xor_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_fetch_xor_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_fetch_xor_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_fetch_xor_u64(GPAtomicUInt64*, uint64_t);

#if GP_HAS_C11_GENERIC
#define gp_atomic_fetch_and(A, C) _Generic((A), \
    GPAtomicInt8 *: gp_atomic_fetch_and_i8 , GPAtomicUInt8 *: gp_atomic_fetch_and_u8 , \
    GPAtomicInt16*: gp_atomic_fetch_and_i16, GPAtomicUInt16*: gp_atomic_fetch_and_u16, \
    GPAtomicInt32*: gp_atomic_fetch_and_i32, GPAtomicUInt32*: gp_atomic_fetch_and_u32, \
    GPAtomicInt64*: gp_atomic_fetch_and_i64, GPAtomicUInt64*: gp_atomic_fetch_and_u64)((A), (C))
#else
#define gp_atomic_fetch_and(A, C) (GP_TYPEOF_CAST((A)->nonatomic) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_fetch_and_i8 ((GPAtomicInt8 *)(A), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_fetch_and_i16((GPAtomicInt16*)(A), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_fetch_and_i32((GPAtomicInt32*)(A), (C)) : \
                        gp_atomic_fetch_and_i64((GPAtomicInt64*)(A), (C))   \
))
#endif
GP_API int8_t   gp_fetch_and_i8 (GPAtomicInt8  *, int8_t  );
GP_API uint8_t  gp_fetch_and_u8 (GPAtomicUInt8 *, uint8_t );
GP_API int16_t  gp_fetch_and_i16(GPAtomicInt16 *, int16_t );
GP_API uint16_t gp_fetch_and_u16(GPAtomicUInt16*, uint16_t);
GP_API int32_t  gp_fetch_and_i32(GPAtomicInt32 *, int32_t );
GP_API uint32_t gp_fetch_and_u32(GPAtomicUInt32*, uint32_t);
GP_API int64_t  gp_fetch_and_i64(GPAtomicInt64 *, int64_t );
GP_API uint64_t gp_fetch_and_u64(GPAtomicUInt64*, uint64_t);

#define gp_atomic_compare_exchange_strong(A, B, C) \
( \
    sizeof(*(A)) == 1 ? gp_atomic_compare_exchange_strong_i8 ((GPAtomicInt8 *)(A), (B), (C)) : \
    sizeof(*(A)) == 2 ? gp_atomic_compare_exchange_strong_i16((GPAtomicInt16*)(A), (B), (C)) : \
    sizeof(*(A)) == 4 ? gp_atomic_compare_exchange_strong_i32((GPAtomicInt32*)(A), (B), (C)) : \
                        gp_atomic_compare_exchange_strong_i64((GPAtomicInt64*)(A), (B), (C))   \
)
GP_API bool gp_atomic_compare_exchange_strong_i8 (GPAtomicInt8  *, int8_t  *, int8_t  );
GP_API bool gp_atomic_compare_exchange_strong_u8 (GPAtomicUInt8 *, uint8_t *, uint8_t );
GP_API bool gp_atomic_compare_exchange_strong_i16(GPAtomicInt16 *, int16_t *, int16_t );
GP_API bool gp_atomic_compare_exchange_strong_u16(GPAtomicUInt16*, uint16_t*, uint16_t);
GP_API bool gp_atomic_compare_exchange_strong_i32(GPAtomicInt32 *, int32_t *, int32_t );
GP_API bool gp_atomic_compare_exchange_strong_u32(GPAtomicUInt32*, uint32_t*, uint32_t);
GP_API bool gp_atomic_compare_exchange_strong_i64(GPAtomicInt64 *, int64_t *, int64_t );
GP_API bool gp_atomic_compare_exchange_strong_u64(GPAtomicUInt64*, uint64_t*, uint64_t);

#define gp_atomic_compare_exchange_weak(A, B, C) gp_atomic_compare_exchange_strong(A, B, C)
GP_API bool gp_atomic_compare_exchange_weak_i8 (GPAtomicInt8  *, int8_t  *, int8_t  );
GP_API bool gp_atomic_compare_exchange_weak_u8 (GPAtomicUInt8 *, uint8_t *, uint8_t );
GP_API bool gp_atomic_compare_exchange_weak_i16(GPAtomicInt16 *, int16_t *, int16_t );
GP_API bool gp_atomic_compare_exchange_weak_u16(GPAtomicUInt16*, uint16_t*, uint16_t);
GP_API bool gp_atomic_compare_exchange_weak_i32(GPAtomicInt32 *, int32_t *, int32_t );
GP_API bool gp_atomic_compare_exchange_weak_u32(GPAtomicUInt32*, uint32_t*, uint32_t);
GP_API bool gp_atomic_compare_exchange_weak_i64(GPAtomicInt64 *, int64_t *, int64_t );
GP_API bool gp_atomic_compare_exchange_weak_u64(GPAtomicUInt64*, uint64_t*, uint64_t);

#define gp_atomic_store_explicit(A, C, MO)     gp_atomic_stor(A, C)
#define gp_atomic_load_explicit(A, MO)         gp_atomic_load(A)
#define gp_atomic_exchange_explicit(A, C, MO)  gp_atomic_exchange(A, C)
#define gp_atomic_fetch_add_explicit(A, C, MO) gp_atomic_fetch_add(A, C)
#define gp_atomic_fetch_sub_explicit(A, C, MO) gp_atomic_fetch_sub(A, C)
#define gp_atomic_fetch_or_explicit(A, C, MO)  gp_atomic_fetch_or(A, C)
#define gp_atomic_fetch_xor_explicit(A, C, MO) gp_atomic_fetch_xor(A, C)
#define gp_atomic_fetch_and_explicit(A, C, MO) gp_atomic_fetch_and(A, C)
#define gp_atomic_compare_exchange_strong_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_strong(A, B, C)
#define gp_atomic_compare_exchange_weak_explicit(A, B, C, MO1, MO2) \
    gp_atomic_compare_exchange_weak(A, B, C)

GP_ALWAYS_INLINE_NOEXPORT void gp_atomic_thread_fence(gp_memory_order_t mo)
{
    // Dummy mutex locking will definitely have a full fence. Overkill, but I
    // can't think of anything better in strict C99 right now.
    static GPMutex dummies[GP_MEMORY_ORDER_LENGTH] = {
        GP_MUTEX_INIT, GP_MUTEX_INIT, GP_MUTEX_INIT,
        GP_MUTEX_INIT, GP_MUTEX_INIT, GP_MUTEX_INIT,
    };
    gp_mutex_lock(&dummies[mo]);
    gp_mutex_unlock(&dummies[mo]);
}

// Opaque function implicit fence.
GP_GNU_ATTRIB(noinline) GP_OPTIMIZE_NONE
void gp_atomic_signal_fence_noinline();
#define gp_atomic_signal_fence(MO) gp_atomic_signal_fence_noinline()

#endif

/// @endcond
/// @}
#endif // GP_ATOMIC_INCLUDED
