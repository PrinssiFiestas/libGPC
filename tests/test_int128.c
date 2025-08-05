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

#if _WIN32 // write result to file that we can use for MSVC tests
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
        gp_test("~ & | ^");
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

        gp_test("<< >>");
        {
            // Note: n larger or equal to 128 is undefined.
            for (uint8_t n = 0; n < 128; ++n) {
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
        }
    }

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
