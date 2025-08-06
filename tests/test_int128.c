// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

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
        u128.gnu = 1;
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

            EXPECT_EQ(gp_uint128_not(ua).gnu, ~ua.gnu);
            EXPECT_EQ(gp_uint128_not(ub).gnu, ~ub.gnu);
            EXPECT_EQ(gp_int128_not(ia).gnu,  ~ia.gnu);
            EXPECT_EQ(gp_int128_not(ib).gnu,  ~ib.gnu);

            EXPECT_EQ(gp_uint128_and(ua, ub).gnu, ua.gnu & ub.gnu);
            EXPECT_EQ(gp_int128_and(ia, ib).gnu,  ia.gnu & ib.gnu);

            EXPECT_EQ(gp_uint128_or(ua, ub).gnu, ua.gnu | ub.gnu);
            EXPECT_EQ(gp_int128_or(ia, ib).gnu,  ia.gnu | ib.gnu);

            EXPECT_EQ(gp_uint128_xor(ua, ub).gnu, ua.gnu ^ ub.gnu);
            EXPECT_EQ(gp_int128_xor(ia, ib).gnu,  ia.gnu ^ ib.gnu);
        }

        // Note: n larger or equal to 128 is undefined.
        gp_test("<< >>"); for (uint8_t n = 0; n < 128; ++n)
        {
            ua = uint128_random();
            ia = int128_random();

            // Shifting big signed numbers left cannot be represented in
            // __int128_t, which is undefined, use mask to limit the size of
            // the numbers.
            GPInt128 mask = {.gnu = GP_INT128_MAX.gnu >> n };

            ASSERT_EQ(gp_uint128_shift_left(ua, n).gnu,  ua.gnu << n, "%hhu", n);
            ASSERT_EQ(gp_uint128_shift_right(ua, n).gnu, ua.gnu >> n, "%hhu", n);
            ASSERT_EQ(gp_int128_shift_right(ia, n).gnu,  ia.gnu >> n, "%hhu", n);
            ASSERT_EQ(
                gp_int128_shift_left(gp_int128_and(ia, mask), n).gnu,
                (ia.gnu & mask.gnu) << n,
                "%hhu", n);
        }
    } // gp_suite("Bitwise operators");

    gp_suite("Arithmetic");
    {
        gp_test("Unsigned +");
        {
            // No carry
            ua = gp_uint128(0x1, 0x123456789ABCDEF0);
            ub = gp_uint128(0x0, 0xFEDCBA9876543210);
            EXPECT_EQ(gp_uint128_add(ua, ub).gnu, ua.gnu + ub.gnu);

            // Carry propagation
            ua = gp_uint128(0, UINT64_MAX);
            ub = gp_uint128(0, 1);
            gp_expect(gp_uint128_add(ua, ub).gnu ==  gp_uint128(1, 0).gnu);
            EXPECT_EQ(gp_uint128_add(ua, ub).gnu, ua.gnu + ub.gnu);

            // Overflow
            ua = GP_UINT128_MAX;
            ub = uint128_random();
            EXPECT_EQ(gp_uint128_add(ua, ub).gnu, ua.gnu + ub.gnu);
        }

        gp_test("Signed +");
        {
            // No carry
            ia = gp_int128(0x1, 0x123456789ABCDEF0);
            ib = gp_int128(0x0, 0xFEDCBA9876543210);
            EXPECT_EQ(gp_int128_add(ia, ib).gnu, ia.gnu + ib.gnu);

            // Carry propagation
            ia = gp_int128(0, UINT64_MAX);
            ib = gp_int128(0, 1);
            gp_expect(gp_int128_add(ia, ib).gnu == gp_int128(1, 0).gnu);
            EXPECT_EQ(gp_int128_add(ia, ib).gnu, ia.gnu + ib.gnu);

            // Overflow is undefined

            // Positive + negative carry ((UINT64_MAX+1) + -1 == UINT64_MAX)
            ia = gp_int128(1, 0);
            ib = gp_int128(-1, -1);
            gp_expect(gp_int128_add(ia, ib).gnu == gp_int128(0, UINT64_MAX).gnu);
            EXPECT_EQ(gp_int128_add(ia, ib).gnu, ia.gnu + ib.gnu);

            // Negative + negative (-1 + -1 == -2)
            ia = gp_int128(-1, -1);
            ib = gp_int128(-1, -1);
            gp_expect(gp_int128_add(ia, ib).gnu == gp_int128(-1, -2).gnu);
            EXPECT_EQ(gp_int128_add(ia, ib).gnu, ia.gnu + ib.gnu);
        }

        gp_test("+- fuzz"); for (size_t fuzz_count = 0; fuzz_count < 128; ++fuzz_count)
        { // test basic addition/subtraction and large numbers with mixed signs
            ua = uint128_random();
            ub = uint128_random();
            ia = int128_random();
            ib = int128_random();

            ASSERT_EQ(gp_uint128_add(ua, ub).gnu, ua.gnu + ub.gnu, "%zu", fuzz_count);
            ASSERT_EQ(gp_uint128_sub(ua, ub).gnu, ua.gnu - ub.gnu, "%zu", fuzz_count);

            // Again, overflow is UB!
            if (ib.gnu >= 0 && ia.gnu <= GP_INT128_MAX.gnu - ib.gnu)
                ASSERT_EQ(gp_int128_add(ia, ib).gnu, ia.gnu + ib.gnu, "%zu", fuzz_count);
            if (ib.gnu  < 0 && ia.gnu >= GP_INT128_MIN.gnu - ib.gnu)
                ASSERT_EQ(gp_int128_add(ia, ib).gnu, ia.gnu + ib.gnu, "%zu", fuzz_count);
            if (ib.gnu >= 0 && ia.gnu >= GP_INT128_MIN.gnu + ib.gnu)
                ASSERT_EQ(gp_int128_sub(ia, ib).gnu, ia.gnu - ib.gnu, "%zu", fuzz_count);
            if (ib.gnu  < 0 && ia.gnu <= GP_INT128_MAX.gnu + ib.gnu)
                ASSERT_EQ(gp_int128_sub(ia, ib).gnu, ia.gnu - ib.gnu, "%zu", fuzz_count);
        }
    } // gp_suite("Arithmetic");

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
