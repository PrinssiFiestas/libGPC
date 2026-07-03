// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_UTILS_INCLUDED
#define GP_UTILS_INCLUDED

#include <gpc/assert.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

GP_GNU_ATTRIB(noinline) GP_OPTIMIZE_NONE
void gp_launder_noinline(void**); ///< @private

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup utils Miscellaneous Utilities
/// @code
/// #include <gpc/utils.h>
/// @endcode
/// @{

/** Round 32-bit number up to the next power of 2.
 * Always rounds up so 0 -> 1, 1 -> 2, 2 -> 4, etc.
 */
GP_NODISCARD GP_INLINE
uint32_t gp_next_power_of_2_32(uint32_t x)
{
    #if __GNUC__ && INT_MAX == INT32_MAX // pedantic size check due to clzg() not always available
    return x == 0 ? 1 : 1 << (64 - __builtin_clz(x));
    #else
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
    #endif
}
/** Round 64-bit number up to the next power of 2.
 * Always rounds up so 0 -> 1, 1 -> 2, 2 -> 4, etc.
 */
GP_NODISCARD GP_INLINE
uint64_t gp_next_power_of_2_64(uint64_t x)
{
    #if __GNUC__ && LLONG_MAX == INT64_MAX // pedantic size check due to clzg() not always available
    return x == 0 ? 1 : 1llu << (64 - __builtin_clzll(x));
    #else
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
    #endif
}
/** Round number of type size_t up to the next power of 2.
 * Always rounds up so 0 -> 1, 1 -> 2, 2 -> 4, etc.
 */
GP_NODISCARD GP_INLINE
size_t gp_next_power_of_2(size_t x)
{
    return sizeof x == sizeof(uint32_t) ?
        gp_next_power_of_2_32(x) : gp_next_power_of_2_64(x);
}

GP_NODISCARD GP_INLINE
size_t gp_trailing_zeros_u32(uint32_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // Note: C23 stdc_trailing_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned)); // be pedantic and paranoid
    return __builtin_ctz(u); // note: generic ctz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    u &= -u;
    // u==0 is undefined with ctz(), we know it's not 0 anyway
    // size_t c = 32;
    // if (u) c--;
    size_t c = 31;
    if (u & 0x0000FFFF0000FFFF) c -= 16;
    if (u & 0x00FF00FF00FF00FF) c -=  8;
    if (u & 0x0F0F0F0F0F0F0F0F) c -=  4;
    if (u & 0x3333333333333333) c -=  2;
    if (u & 0x5555555555555555) c -=  1;
    return c;
    #endif
}

GP_NODISCARD GP_INLINE
size_t gp_trailing_zeros_u64(uint64_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // Note: C23 stdc_trailing_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned long long)); // be pedantic and paranoid
    return __builtin_ctzll(u); // note: generic ctz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    u &= -u;
    // u==0 is undefined with ctz(), we know it's not 0 anyway
    // size_t c = 64;
    // if (u) c--;
    size_t c = 63;
    if (u & 0x00000000FFFFFFFF) c -= 32;
    if (u & 0x0000FFFF0000FFFF) c -= 16;
    if (u & 0x00FF00FF00FF00FF) c -=  8;
    if (u & 0x0F0F0F0F0F0F0F0F) c -=  4;
    if (u & 0x3333333333333333) c -=  2;
    if (u & 0x5555555555555555) c -=  1;
    return c;
    #endif
}

GP_NODISCARD GP_INLINE
size_t gp_leading_zeros_u32(uint32_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // Note: C23 stdc_leading_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned)); // be pedantic and paranoid
    return __builtin_clz(u); // note: generic clz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    uint32_t v = u;
    uint32_t r;
    uint32_t shift;

    r     = (v > 0xFFFF    ) << 4; v >>= r;
    shift = (v > 0xFF      ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF       ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3       ) << 1; v >>= shift; r |= shift;
                                                r |= (v >> 1);
    return 31 - r;
    #endif
}
GP_NODISCARD GP_INLINE
size_t gp_leading_zeros_u64(uint64_t u)
{
    gp_assume(u != 0, "Invalid argument.");

    // Note: C23 stdc_leading_zeros() breaks builds, don't use it!
    #if __GNUC__ && !defined(GP_TEST_INT128)
    GP_STATIC_ASSERT(sizeof u == sizeof(unsigned long long)); // be pedantic and paranoid
    return __builtin_clzll(u); // note: generic clz() not available in older GCC
    #else // https://graphics.stanford.edu/~seander/bithacks.html
    uint64_t v = u;
    uint64_t r;
    uint64_t shift;

    r =     (v > 0xFFFFFFFF) << 5; v >>= r;
    shift = (v > 0xFFFF    ) << 4; v >>= shift; r |= shift;
    shift = (v > 0xFF      ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF       ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3       ) << 1; v >>= shift; r |= shift;
                                                r |= (v >> 1);
    return 63 - r;
    #endif
}

/** Round number up to alignment boundary.
 * @p boundary must be a power of 2.
 * @return @p x if already aligned.
 */
GP_NODISCARD GP_INLINE
uintptr_t gp_round_to_aligned(uintptr_t x, uintptr_t boundary)
{
    gp_assume((boundary & (boundary-1)) == 0, "Alignment boundary must be a power of 2.");
    return x + (boundary - 1) - ((x - 1) & (boundary - 1));
}

/** Detach pointer from it's origin.
 *
 * Remove any information about @a ptr as seen by the compiler. Most notably
 * this means that the compiler will not be able to do aliasing analysis, which
 * can be useful when debugging aliasing bugs. This can be also used for some
 * advanced optimizations that would be allowed by the CPU and the ABI, but are
 * undefined by the C implementation.
 *
 * If you don't know what you're doing, then don't use this for anything else
 * than debugging. Detaching pointer from it's origins inhibits optimizations,
 * so using it wrongly just degrades performance. Furthermore, using it to fix
 * undefined behavior (like strict aliasing violations) doesn't inherently fix
 * anything, it just confuses the compiler to make the program look valid. Only
 * use this in production code if you are well familiar with your target
 * architecture and compiler.
 *
 * @return detached @a ptr.
 */
GP_INLINE void* gp_launder(void* ptr)
{
    #if defined(__GNUC__) && !defined(__FILC__)
    __asm__ volatile("" : "+r"(ptr));
    #else // can't avoid function call overhead, sorry!
    gp_launder_noinline(&ptr);
    #endif
    return ptr;
}

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

#endif // GP_UTILS_INCLUDED
