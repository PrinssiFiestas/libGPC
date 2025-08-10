// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define FUZZ_COUNT 128

#if __GNUC__ && __SIZEOF_INT128__

// Force int128.h implementations to not use __[u]int128_t, we want to test
// against that.
#undef __SIZEOF_INT128__
#define GP_TEST_INT128 1

#include "../src/int128.c"
#include <gpc/assert.h>
#include <gpc/io.h>
#include <gpc/utils.h>

#if _WIN32 && !__cplusplus // write result to file that we can use for MSVC tests
#define WRITE_RESULT(...) do { \
    GP_TYPEOF(__VA_ARGS__) _result = (__VA_ARGS__); \
    fwrite(&_result, sizeof _result, 1, msvc_test_file); \
} while(0)
#else
#define WRITE_RESULT(...) ((void)0)
#endif

#define EXPECT_EQ(A, B, ...) \
    (WRITE_RESULT(B), gp_expect((A) == (B)__VA_OPT__(,)__VA_ARGS__))

#define ASSERT_EQ(A, B, ...) \
    (WRITE_RESULT(B), gp_assert((A) == (B)__VA_OPT__(,)__VA_ARGS__))

static GPRandomState g_rs;

GPUint128 uint128_random(void)
{
    GPUint128 y;
    uint32_t data[4];
    for (size_t i = 0; i < 4; ++i)
        data[i] = gp_random(&g_rs);
    memcpy(&y, data, sizeof data);
    return y;
}

GPInt128 int128_random(void)
{
    GPUint128 u = uint128_random();
    GPInt128 y;
    memcpy(&y, &u, sizeof y);
    return y;
}

GPInt128 int128_random_positive(void)
{
    GPInt128 y = int128_random();
    *gp_int128_hi_addr(&y) &= INT64_MAX;
    return y;
}

GPInt128 int128_random_negative(void)
{
    GPInt128 y = int128_random();
    *gp_int128_hi_addr(&y) |= INT64_MIN;
    return y;
}

int main(void)
{
    gp_suite("Endianness"); // the very prerequisite for anything
    {
        uint16_t u16 = 1;
        uint8_t* p = (uint8_t*)&u16;
        GPUint128 u128;
        u128.u128 = 1;
        gp_assert(gp_uint128_hi(u128) == 0);
        gp_assert(gp_uint128_lo(u128) == 1);
        if (gp_is_big_endian()) {
            gp_assert( ! gp_is_little_endian());
            gp_assert(p[0] == 0);
            gp_assert(p[1] == 1);
        }
        if (gp_is_little_endian()) {
            gp_assert( ! gp_is_big_endian());
            gp_assert(p[0] == 1);
            gp_assert(p[1] == 0);
        }
    } // gp_suite("Endianness");

    #if _WIN32
    FILE* msvc_test_file = gp_file_open("../build/gnu_int128_result.bin", "write");
    gp_assert(msvc_test_file != NULL, strerror(errno));
    #endif

    // Seed global random state with date
    {
        time_t t = time(NULL);
        struct tm* tm = gmtime(&t);
        gp_assert(tm != NULL);
        g_rs = gp_random_state(tm->tm_yday + 1000*tm->tm_year);
    }

    GPUint128 ua, ub;
    GPInt128  ia, ib;

    gp_suite("Bitwise operators");
    {
        gp_test("~ & | ^"); // trivial, but good sanity checks
        {
            ua = uint128_random();
            ub = uint128_random();
            ia = int128_random();
            ib = int128_random();

            EXPECT_EQ(gp_uint128_not(ua).u128, ~ua.u128);
            EXPECT_EQ(gp_uint128_not(ub).u128, ~ub.u128);
            EXPECT_EQ(gp_int128_not(ia).i128,  ~ia.i128);
            EXPECT_EQ(gp_int128_not(ib).i128,  ~ib.i128);

            EXPECT_EQ(gp_uint128_and(ua, ub).u128, ua.u128 & ub.u128);
            EXPECT_EQ(gp_int128_and(ia, ib).i128,  ia.i128 & ib.i128);

            EXPECT_EQ(gp_uint128_or(ua, ub).u128, ua.u128 | ub.u128);
            EXPECT_EQ(gp_int128_or(ia, ib).i128,  ia.i128 | ib.i128);

            EXPECT_EQ(gp_uint128_xor(ua, ub).u128, ua.u128 ^ ub.u128);
            EXPECT_EQ(gp_int128_xor(ia, ib).i128,  ia.i128 ^ ib.i128);
        }

        // Note: n larger or equal to 128 is undefined.
        gp_test("<< >>"); for (uint8_t n = 0; n < 128; ++n)
        {
            ua = uint128_random();
            ia = int128_random();

            // Shifting big signed numbers left cannot be represented in
            // __int128_t, which is undefined, use mask to limit the size of
            // the numbers.
            GPInt128 mask = {.i128 = GP_INT128_MAX.i128 >> n };

            ASSERT_EQ(gp_uint128_shift_left(ua, n).u128,  ua.u128 << n, "%hhu", n);
            ASSERT_EQ(gp_uint128_shift_right(ua, n).u128, ua.u128 >> n, "%hhu", n);
            ASSERT_EQ(gp_int128_shift_right(ia, n).i128,  ia.i128 >> n, "%hhu", n);
            ASSERT_EQ(
                gp_int128_shift_left(gp_int128_and(ia, mask), n).i128,
                (ia.i128 & mask.i128) << n,
                "%hhu", n);
        }
    } // gp_suite("Bitwise operators");

    gp_suite("Addition & Subtraction");
    {
        gp_test("Unsigned +-");
        {
            // Carry propagation
            ua = gp_uint128(0, UINT64_MAX);
            ub = gp_uint128(0, 1);
            gp_expect(gp_uint128_add(ua, ub).u128 ==  gp_uint128(1, 0).u128);
            EXPECT_EQ(gp_uint128_add(ua, ub).u128, ua.u128 + ub.u128);
            ua = gp_uint128(1, 0);
            gp_expect(gp_uint128_sub(ua, ub).u128 == gp_uint128(0, UINT64_MAX).u128);
            EXPECT_EQ(gp_uint128_sub(ua, ub).u128, ua.u128 - ub.u128);

            // Overflow
            ua = GP_UINT128_MAX;
            ub = uint128_random();
            EXPECT_EQ(gp_uint128_add(ua, ub).u128, ua.u128 + ub.u128);
            ua = gp_uint128(0, 0);
            EXPECT_EQ(gp_uint128_sub(ua, ub).u128, ua.u128 - ub.u128);
        }

        gp_test("Signed +-");
        {
            // Carry propagation
            ia = gp_int128(0, UINT64_MAX);
            ib = gp_int128(0, 1);
            gp_expect(gp_int128_add(ia, ib).i128 == gp_int128(1, 0).i128);
            EXPECT_EQ(gp_int128_add(ia, ib).i128, ia.i128 + ib.i128);
            ia = gp_int128(1, 0);
            gp_expect(gp_int128_sub(ia, ib).i128 == gp_int128(0, UINT64_MAX).i128);
            EXPECT_EQ(gp_int128_sub(ia, ib).i128, ia.i128 - ib.i128);

            // Overflow is undefined

            // Positive + negative carry ((UINT64_MAX+1) + -1 == UINT64_MAX)
            ia = gp_int128(1, 0);
            ib = gp_int128(-1, -1);
            gp_expect(gp_int128_add(ia, ib).i128 == gp_int128(0, UINT64_MAX).i128);
            EXPECT_EQ(gp_int128_add(ia, ib).i128, ia.i128 + ib.i128);

            // Negative + negative (-1 + -1 == -2)
            ia = gp_int128(-1, -1);
            ib = gp_int128(-1, -1);
            gp_expect(gp_int128_add(ia, ib).i128 == gp_int128(-1, -2).i128);
            EXPECT_EQ(gp_int128_add(ia, ib).i128, ia.i128 + ib.i128);
        }

        gp_test("+- fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        { // test basic addition/subtraction and large numbers with mixed signs
            ua = uint128_random();
            ub = uint128_random();
            ia = int128_random();
            ib = int128_random();

            ASSERT_EQ(gp_uint128_add(ua, ub).u128, ua.u128 + ub.u128, "%zu", fuzz_count);
            ASSERT_EQ(gp_uint128_sub(ua, ub).u128, ua.u128 - ub.u128, "%zu", fuzz_count);

            // Again, overflow is UB!
            if (ib.i128 >= 0 && ia.i128 <= GP_INT128_MAX.i128 - ib.i128)
                ASSERT_EQ(gp_int128_add(ia, ib).i128, ia.i128 + ib.i128, "%zu", fuzz_count);
            if (ib.i128  < 0 && ia.i128 >= GP_INT128_MIN.i128 - ib.i128)
                ASSERT_EQ(gp_int128_add(ia, ib).i128, ia.i128 + ib.i128, "%zu", fuzz_count);
            if (ib.i128 >= 0 && ia.i128 >= GP_INT128_MIN.i128 + ib.i128)
                ASSERT_EQ(gp_int128_sub(ia, ib).i128, ia.i128 - ib.i128, "%zu", fuzz_count);
            if (ib.i128  < 0 && ia.i128 <= GP_INT128_MAX.i128 + ib.i128)
                ASSERT_EQ(gp_int128_sub(ia, ib).i128, ia.i128 - ib.i128, "%zu", fuzz_count);
        }
    } // gp_suite("Addition & Subtraction");

    gp_suite("Multiplication");
    {
        gp_test("Multiply 64-bit integers to 128-bit integer");
        {
            gp_expect(gp_uint128_mul64(UINT64_MAX, 2).u128 == gp_uint128(1, UINT64_MAX - 1).u128);
            EXPECT_EQ(gp_uint128_mul64(UINT64_MAX, 2).u128, (__uint128_t)UINT64_MAX * 2);

            gp_expect(gp_uint128_mul64(UINT64_MAX, UINT64_MAX).u128 == gp_uint128(-2, 1).u128);
            EXPECT_EQ(gp_uint128_mul64(UINT64_MAX, UINT64_MAX).u128, (__uint128_t)UINT64_MAX * UINT64_MAX);

            ua = uint128_random();
            EXPECT_EQ(gp_uint128_mul64(ua.u64[0], ua.u64[1]).u128, (__uint128_t)ua.u64[0] * ua.u64[1],
                "%llx", ua.u64[0], "%llx", ua.u64[1]);
        }

        size_t overflow_count = 0;

        gp_test("Unsigned fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        {
            // Absolutely massive numbers, practically always overflow
            ua = uint128_random();
            ub = uint128_random();
            ASSERT_EQ(gp_uint128_mul(ua, ub).u128, ua.u128 * ub.u128, "%zu", fuzz_count);

            // Huge numbers, should overflow sometimes
            ua = gp_uint128_and(ua, gp_uint128(0x00000005FFFFFFFF, 0xFFFFFFFFFFFFFFFF));
            ub = gp_uint128(0, gp_random(&g_rs));
            ASSERT_EQ(gp_uint128_mul(ua, ub).u128, ua.u128 * ub.u128, "%zu", fuzz_count);
            overflow_count += ua.u128 >= GP_UINT128_MAX.u128 / ub.u128;
        }
        gp_println("\toverflow ratio: %g", (double)overflow_count/FUZZ_COUNT); // ≈ 0.5

        gp_test("Signed fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        {
            // Bitwise operations used to limit the size of numbers to prevent
            // overflow.

            // Positive * negative
            ia = gp_int128_and(int128_random_positive(), gp_int128(1, 0xFFFFFFFFFFFFFFFF));
            ib = gp_int128(-1, 0xFFFFFFFF00000000 | gp_random(&g_rs));
            ASSERT_EQ(gp_int128_mul(ia, ib).i128, ia.i128 * ib.i128, "%zu", fuzz_count);

            // Negative * positive
            ia = gp_int128_or(int128_random_negative(), gp_int128(-2, 0));
            ib = gp_int128(0, gp_random(&g_rs));
            ASSERT_EQ(gp_int128_mul(ia, ib).i128, ia.i128 * ib.i128, "%zu", fuzz_count);

            // Negative * negative
            ia = gp_int128_or(int128_random_negative(), gp_int128(-2, 0));
            ib = gp_int128(-1, 0xFFFFFFFF00000000 | gp_random(&g_rs));
            ASSERT_EQ(gp_int128_mul(ia, ib).i128, ia.i128 * ib.i128, "%zu", fuzz_count);
        }
    } // gp_suite("Multiplication");

    #if _WIN32 // pedantic close
    gp_file_close(msvc_test_file);
    #endif
}

#elif _MSC_VER && _M_X64 // mostly test that intrinsics are used correctly

#include "../src/int128.c"
#include <gpc/io.h>

int main(void)
{
    // TODO maybe we could store the results of GNU tests to a file, read the
    // file here (if exists), and compare against it. It would work nicely with
    // `make test_all`
}

#else
int main(void) { return 0; }
#endif // __GNUC__ && __SIZEOF_INT128__
