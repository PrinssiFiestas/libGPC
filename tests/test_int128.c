// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define FUZZ_COUNT 128

#if __GNUC__ && __SIZEOF_INT128__ && !__clang__

// Force int128.h implementations to not use GPTIInt/GPTIUint, we want to test
// against that. This is very hacky and finicky, a potential bug might be that
// we end up testing GPTIInt implementations against GPTIInt always passing the
// tests, so make sure to manually break a test to see that they still work on
// changes!
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

GPUInt128 uint128_random(GPRandomState* rs)
{
    GPUInt128 y;
    uint32_t data[4];
    for (size_t i = 0; i < 4; ++i)
        data[i] = gp_random(rs);
    memcpy(&y, data, sizeof data);
    return y;
}

GPInt128 int128_random(GPRandomState* rs)
{
    GPUInt128 u = uint128_random(rs);
    GPInt128 y;
    memcpy(&y, &u, sizeof y);
    return y;
}

GPInt128 int128_random_positive(GPRandomState* rs)
{
    GPInt128 y = int128_random(rs);
    *gp_int128_hi_addr(&y) &= INT64_MAX;
    return y;
}

GPInt128 int128_random_negative(GPRandomState* rs)
{
    GPInt128 y = int128_random(rs);
    *gp_int128_hi_addr(&y) |= INT64_MIN;
    return y;
}

int main(void)
{
    return 0; // TODO TODO TODO TODO TODO

    gp_suite("Endianness"); // the very prerequisite for anything
    {
        uint16_t u16 = 1;
        uint8_t* p = (uint8_t*)&u16;
        GPUInt128 u128;
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
    #  if __GNUC__
    FILE* msvc_test_file = gp_file_open("../build/gnu_int128_result.bin", "write");
    #  else // MSVC
    FILE* msvc_test_file = gp_file_open("../build/gnu_int128_result.bin", "read");
    #  endif
    gp_assert(msvc_test_file != NULL, strerror(errno));
    #endif

    static GPRandomState rs; // Seed random state with date
    {
        time_t t = time(NULL);
        struct tm* tm = gmtime(&t);
        gp_assert(tm != NULL);
        rs = gp_random_state(tm->tm_yday + 1000*tm->tm_year);
    }

    GPUInt128 ua, ub;
    GPInt128  ia, ib;

    gp_suite("Comparisons");
    {
        gp_test("== and !=");
        {
            do { // a REALLY REALLY REALLY REEEALLY pedantic loop to prevent overflow
                ua = ub = uint128_random(&rs);
                ia = ib = int128_random_positive(&rs);
            } while (gp_uint128_equal(ua, GP_UINT128_MAX) || gp_int128_equal(ia, GP_INT128_MAX));
            gp_assert(gp_uint128_equal(ua, ub));
            gp_assert(gp_int128_equal(ia, ib));

            ua = gp_uint128_add(ua, gp_uint128(0, 1));
            ia = gp_int128_add(ia, gp_int128(0, 1));
            gp_assert(gp_uint128_not_equal(ua, ub));
            gp_assert(gp_int128_not_equal(ia, ib));
        }

        gp_test("<, <=, >, and >=");
        {
            gp_assert( ! gp_uint128_less_than(ua, ub));
            gp_assert( ! gp_uint128_less_than_equal(ua, ub));
            gp_assert(   gp_uint128_greater_than(ua, ub));
            gp_assert(   gp_uint128_greater_than_equal(ua, ub));
            ua = gp_uint128_sub(ua, gp_uint128(0, 1)); // equal again
            gp_assert( ! gp_uint128_less_than(ua, ub));
            gp_assert(   gp_uint128_less_than_equal(ua, ub));
            gp_assert( ! gp_uint128_greater_than(ua, ub));
            gp_assert(   gp_uint128_greater_than_equal(ua, ub));

            gp_assert( ! gp_int128_less_than(ia, ib));
            gp_assert( ! gp_int128_less_than_equal(ia, ib));
            gp_assert(   gp_int128_greater_than(ia, ib));
            gp_assert(   gp_int128_greater_than_equal(ia, ib));
            ia = gp_int128_sub(ia, gp_int128(0, 1)); // equal again
            gp_assert( ! gp_int128_less_than(ia, ib));
            gp_assert(   gp_int128_less_than_equal(ia, ib));
            gp_assert( ! gp_int128_greater_than(ia, ib));
            gp_assert(   gp_int128_greater_than_equal(ia, ib));

            ia = ib = gp_int128_negate(ia);
            gp_assert( ! gp_int128_less_than(ia, ib));
            gp_assert(   gp_int128_less_than_equal(ia, ib));
            gp_assert( ! gp_int128_greater_than(ia, ib));
            gp_assert(   gp_int128_greater_than_equal(ia, ib));
            ia = gp_int128_sub(ia, gp_int128(0, 1));
            gp_assert(   gp_int128_less_than(ia, ib));
            gp_assert(   gp_int128_less_than_equal(ia, ib));
            gp_assert( ! gp_int128_greater_than(ia, ib));
            gp_assert( ! gp_int128_greater_than_equal(ia, ib));

            ia = int128_random_negative(&rs);
            ib = int128_random_positive(&rs);
            if ( ! gp_int128_equal(ia, ib)) { // STUPIDLY PEDANTIC check, skip if ia==ib==0
                gp_assert(   gp_int128_less_than(ia, ib));
                gp_assert(   gp_int128_less_than_equal(ia, ib));
                gp_assert( ! gp_int128_greater_than(ia, ib));
                gp_assert( ! gp_int128_greater_than_equal(ia, ib));
            }
        }
    } // gp_suite("Comparisons");

    gp_suite("Bitwise operators");
    {
        gp_test("~ & | ^"); // trivial, but good sanity checks
        {
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            ia = int128_random(&rs);
            ib = int128_random(&rs);

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
            ua = uint128_random(&rs);
            ia = int128_random(&rs);

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
            ub = uint128_random(&rs);
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
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            ia = int128_random(&rs);
            ib = int128_random(&rs);

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
        gp_test("Negation");
        {
            ua = gp_uint128(0, 0);
            ia = gp_int128(0, 0);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(0, 0)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(0, 0)));
            EXPECT_EQ(gp_uint128_negate(ua).u128, -ua.u128);
            EXPECT_EQ(gp_int128_negate(ia).i128, -ia.i128);

            ua = gp_uint128(0, UINT64_MAX);
            ia = gp_int128(0, UINT64_MAX);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(UINT64_MAX, 1)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(-1, 1)));
            EXPECT_EQ(gp_uint128_negate(ua).u128, -ua.u128);
            EXPECT_EQ(gp_int128_negate(ia).i128, -ia.i128);

            ua = gp_uint128(UINT64_MAX, 0);
            ia = gp_int128(-1, 0);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(1, 0)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(1, 0)));
            EXPECT_EQ(gp_uint128_negate(ua).u128, -ua.u128);
            EXPECT_EQ(gp_int128_negate(ia).i128, -ia.i128);

            ua = gp_uint128(UINT64_MAX, UINT64_MAX);
            ia = gp_int128(-1, -1);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(0, 1)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(0, 1)));
            EXPECT_EQ(gp_uint128_negate(ua).u128, -ua.u128);
            EXPECT_EQ(gp_int128_negate(ia).i128, -ia.i128);

            ua = uint128_random(&rs);
            ia = int128_random(&rs);
            EXPECT_EQ(gp_uint128_negate(ua).u128, -ua.u128);
            EXPECT_EQ(gp_int128_negate(ia).i128, -ia.i128);
        }

        gp_test("Multiply 64-bit unsigned integers to 128-bit unsigned integer");
        {
            uint64_t a = gp_random(&rs), b = gp_random(&rs);
            gp_expect(gp_uint128_less_than(gp_uint128_mul64(a, b), gp_uint128(1, 0)));
            EXPECT_EQ(gp_uint128_mul64(a, b).u128, (__uint128_t)a * b);

            gp_expect(gp_uint128_mul64(UINT64_MAX, 2).u128 == gp_uint128(1, UINT64_MAX - 1).u128);
            EXPECT_EQ(gp_uint128_mul64(UINT64_MAX, 2).u128, (__uint128_t)UINT64_MAX * 2);

            gp_expect(gp_uint128_mul64(UINT64_MAX, UINT64_MAX).u128 == gp_uint128(-2, 1).u128);
            EXPECT_EQ(gp_uint128_mul64(UINT64_MAX, UINT64_MAX).u128, (__uint128_t)UINT64_MAX * UINT64_MAX);

            ua = uint128_random(&rs);
            EXPECT_EQ(gp_uint128_mul64(ua.little_endian.lo, ua.little_endian.hi).u128, (__uint128_t)ua.little_endian.lo * ua.little_endian.hi,
                "%llx", ua.little_endian.lo, "%llx", ua.little_endian.hi);
        }

        gp_test("Multiply 64-bit signed integers to 128-bit signed integer");
        {
            int64_t a = gp_random(&rs), b = gp_random(&rs);
            EXPECT_EQ(gp_int128_mul64(a,   b).i128, (__int128_t)a  *  b);
            EXPECT_EQ(gp_int128_mul64(-a,  b).i128, (__int128_t)-a *  b);
            EXPECT_EQ(gp_int128_mul64(a,  -b).i128, (__int128_t)a  * -b);
            EXPECT_EQ(gp_int128_mul64(-a, -b).i128, (__int128_t)-a * -b);
            a <<= 30;
            EXPECT_EQ(gp_int128_mul64(a,   b).i128, (__int128_t)a  *  b);
            EXPECT_EQ(gp_int128_mul64(-a,  b).i128, (__int128_t)-a *  b);
            EXPECT_EQ(gp_int128_mul64(a,  -b).i128, (__int128_t)a  * -b);
            EXPECT_EQ(gp_int128_mul64(-a, -b).i128, (__int128_t)-a * -b);
            a >>= 30; b <<= 30;
            EXPECT_EQ(gp_int128_mul64(a,   b).i128, (__int128_t)a  *  b);
            EXPECT_EQ(gp_int128_mul64(-a,  b).i128, (__int128_t)-a *  b);
            EXPECT_EQ(gp_int128_mul64(a,  -b).i128, (__int128_t)a  * -b);
            EXPECT_EQ(gp_int128_mul64(-a, -b).i128, (__int128_t)-a * -b);
            a <<= 30;
            EXPECT_EQ(gp_int128_mul64(a,   b).i128, (__int128_t)a  *  b);
            EXPECT_EQ(gp_int128_mul64(-a,  b).i128, (__int128_t)-a *  b);
            EXPECT_EQ(gp_int128_mul64(a,  -b).i128, (__int128_t)a  * -b);
            EXPECT_EQ(gp_int128_mul64(-a, -b).i128, (__int128_t)-a * -b);
        }

        size_t overflow_count = 0;

        gp_test("Unsigned fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        {
            // Absolutely massive numbers, practically always overflow
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            ASSERT_EQ(gp_uint128_mul(ua, ub).u128, ua.u128 * ub.u128, "%zu", fuzz_count);

            // Huge numbers, should overflow sometimes
            ua = gp_uint128_and(ua, gp_uint128(0x00000005FFFFFFFF, 0xFFFFFFFFFFFFFFFF));
            ub = gp_uint128(0, gp_random(&rs));
            ASSERT_EQ(gp_uint128_mul(ua, ub).u128, ua.u128 * ub.u128, "%zu", fuzz_count);
            overflow_count += ua.u128 >= GP_UINT128_MAX.u128 / ub.u128;
        }
        gp_println("\toverflow ratio: %g", (double)overflow_count/FUZZ_COUNT); // ≈ 0.5

        gp_test("Signed fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        {
            // Bitwise operations used to limit the size of numbers to prevent
            // overflow.

            // Positive * negative
            ia = gp_int128_and(int128_random_positive(&rs), gp_int128(1, 0xFFFFFFFFFFFFFFFF));
            ib = gp_int128(-1, 0xFFFFFFFF00000000 | gp_random(&rs));
            ASSERT_EQ(gp_int128_mul(ia, ib).i128, ia.i128 * ib.i128, "%zu", fuzz_count);

            // Negative * positive
            ia = gp_int128_or(int128_random_negative(&rs), gp_int128(-2, 0));
            ib = gp_int128(0, gp_random(&rs));
            ASSERT_EQ(gp_int128_mul(ia, ib).i128, ia.i128 * ib.i128, "%zu", fuzz_count);

            // Negative * negative
            ia = gp_int128_or(int128_random_negative(&rs), gp_int128(-2, 0));
            ib = gp_int128(-1, 0xFFFFFFFF00000000 | gp_random(&rs));
            ASSERT_EQ(gp_int128_mul(ia, ib).i128, ia.i128 * ib.i128, "%zu", fuzz_count);
        }
    } // gp_suite("Multiplication");

    gp_suite("Division/modulus");
    {
        GPUInt128 u64s;
        GPUInt128 remainder;

        gp_test("0X/0X");
        {
            u64s = uint128_random(&rs);
            ua = gp_uint128(0, u64s.little_endian.lo);
            ub = gp_uint128(0, u64s.little_endian.hi);
            EXPECT_EQ(gp_uint128_divmod(ua, ub, &remainder).u128, ua.u128 / ub.u128);
            EXPECT_EQ(remainder.u128, ua.u128 % ub.u128);
        }

        gp_test("0X/XX");
        {
            ua = gp_uint128(0, gp_uint128_lo(uint128_random(&rs)));
            ub = uint128_random(&rs);
            EXPECT_EQ(gp_uint128_divmod(ua, ub, &remainder).u128, ua.u128 / ub.u128);
            EXPECT_EQ(remainder.u128, ua.u128 % ub.u128);
        }

        gp_test("X0/X0");
        {
            u64s = uint128_random(&rs);
            ua = gp_uint128(u64s.little_endian.lo, 0);
            ub = gp_uint128(u64s.little_endian.hi, 0);
            EXPECT_EQ(gp_uint128_divmod(ua, ub, &remainder).u128, ua.u128 / ub.u128);
            EXPECT_EQ(remainder.u128, ua.u128 % ub.u128);
        }

        gp_test("XX/X0");
        {
            ua = uint128_random(&rs);
            ub = gp_uint128(gp_uint128_hi(uint128_random(&rs)), 0);
            EXPECT_EQ(gp_uint128_divmod(ua, ub, &remainder).u128, ua.u128 / ub.u128);
            EXPECT_EQ(remainder.u128, ua.u128 % ub.u128);
        }

        gp_test("XX/0X");
        {
            ua = uint128_random(&rs);
            ub = gp_uint128(0, gp_uint128_lo(uint128_random(&rs)));
            EXPECT_EQ(gp_uint128_divmod(ua, ub, &remainder).u128, ua.u128 / ub.u128);
            EXPECT_EQ(remainder.u128, ua.u128 % ub.u128);
        }

        gp_test("XX/XX");
        {
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            EXPECT_EQ(gp_uint128_divmod(ua, ub, &remainder).u128, ua.u128 / ub.u128);
            EXPECT_EQ(remainder.u128, ua.u128 % ub.u128);
        }

        // Signed div/mod uses gp_uint128_divmod under the hood, just sanity
        // check for correct sign handling.
        gp_test("Sign");
        {
            ia = int128_random_positive(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_positive(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_idiv(ia, ib).i128 >= 0);
            EXPECT_EQ(gp_int128_idiv(ia, ib).i128, ia.i128 / ib.i128);
            EXPECT_EQ(gp_int128_imod(ia, ib).i128, ia.i128 % ib.i128);

            ia = int128_random_negative(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_positive(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_idiv(ia, ib).i128  < 0);
            EXPECT_EQ(gp_int128_idiv(ia, ib).i128, ia.i128 / ib.i128);
            EXPECT_EQ(gp_int128_imod(ia, ib).i128, ia.i128 % ib.i128);

            ia = int128_random_positive(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_negative(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_idiv(ia, ib).i128  < 0);
            EXPECT_EQ(gp_int128_idiv(ia, ib).i128, ia.i128 / ib.i128);
            EXPECT_EQ(gp_int128_imod(ia, ib).i128, ia.i128 % ib.i128);

            ia = int128_random_negative(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_negative(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_idiv(ia, ib).i128 >= 0);
            EXPECT_EQ(gp_int128_idiv(ia, ib).i128, ia.i128 / ib.i128);
            EXPECT_EQ(gp_int128_imod(ia, ib).i128, ia.i128 % ib.i128);
        }
    } // gp_suite("Division/modulus");

    #ifdef __SIZEOF_INT128__
    gp_suite("Float conversions");
    {
        // https://github.com/m-ou-se/floatconv/blob/main/src/test.rs
        static const GPTetraUInt u128s[] = {
            0, 1, 2, 3, 1234,
            GP_TETRA_UINT_MAX, // Overflows the mantissa, should increment the exponent (which will be odd).
            GP_TETRA_UINT_MAX / 2, // Overflows the mantissa, should increment the exponent (which will be even).
            0x400000000000000, // Exact match, no rounding
            0x400000000000022, // Round to closest (up)
            0x400000000000012, // Round to closest (down)
            0x8000000000000C, // Tie, round to even (up)
            0x80000000000004, // Tie, round to even (down)
            // Round to closest (up), with tie-breaking bit further than 64 bits away.
            ((GPTetraUInt)0x8000000000000400 << 64) | 0x0000000000000001,
            // Round to closest (down), with 1-bit in 63rd position (which should be insignificant).
            ((GPTetraUInt)0x8000000000000000 << 64) | 0x8000000000000000,
            // Round to closest (down), with 1-bits in all insignificant positions.
            ((GPTetraUInt)0x80000000000003FF << 64) | 0xFFFFFFFFFFFFFFFF,
            // Mantissa of 2*52 bits, with last 32 bits set.
            ((GPTetraUInt)0x10000000000      << 64) | 0x00000000FFFFFFFF,
            // Mantissa of 2*52 bits, with bit 23 set.
            ((GPTetraUInt)0x10000000000      << 64) | 0x0000000000800000,
            // Mantissa of 2*52 bits, with last 23 bits set.
            ((GPTetraUInt)0x10000000000      << 64) | 0x00000000007FFFFF,
            // Mantissa of 128-32 bits, with last 24 bits set.
            ((GPTetraUInt)0x100000000        << 64) | 0x0000000000FFFFFF,
            (GPTetraUInt)1 << 127,
            (GPTetraUInt)2 << 126,
            (GPTetraUInt)3 << 126,
            (GPTetraUInt)1 << 64,
            (GPTetraUInt)1 << 63,
            (GPTetraUInt)1 << 54,
            (GPTetraUInt)1 << 53,
            (GPTetraUInt)1 << 52,
            (GPTetraUInt)1 << 51,
            ((GPTetraUInt)1 << 54) - 1,
            ((GPTetraUInt)1 << 53) - 1,
            ((GPTetraUInt)1 << 52) - 1,
            ((GPTetraUInt)1 << 51) - 1,
            ((GPTetraUInt)1 << 54) + 1,
            ((GPTetraUInt)1 << 53) + 1,
            ((GPTetraUInt)1 << 52) + 1,
            ((GPTetraUInt)1 << 51) + 1,
            UINT64_MAX,
            (GPTetraUInt)UINT64_MAX << 64,
            (GPTetraUInt)UINT64_MAX << 63,
            (GPTetraUInt)UINT64_MAX << 53,
            (GPTetraUInt)UINT64_MAX << 52,
            (GPTetraUInt)UINT64_MAX << 51,
            (GPTetraUInt)(UINT64_MAX >> 13) << 64,
            (GPTetraUInt)(UINT64_MAX >> 13) << 63,
            (GPTetraUInt)(UINT64_MAX >> 13) << 53,
            (GPTetraUInt)(UINT64_MAX >> 13) << 52,
            (GPTetraUInt)(UINT64_MAX >> 13) << 51,
            (GPTetraUInt)(UINT64_MAX >> 12) << 64,
            (GPTetraUInt)(UINT64_MAX >> 12) << 63,
            (GPTetraUInt)(UINT64_MAX >> 12) << 53,
            (GPTetraUInt)(UINT64_MAX >> 12) << 52,
            (GPTetraUInt)(UINT64_MAX >> 12) << 51,
            (GPTetraUInt)(UINT64_MAX >> 11) << 64,
            (GPTetraUInt)(UINT64_MAX >> 11) << 63,
            (GPTetraUInt)(UINT64_MAX >> 11) << 53,
            (GPTetraUInt)(UINT64_MAX >> 11) << 52,
            (GPTetraUInt)(UINT64_MAX >> 11) << 51,
            GP_TETRA_UINT_MAX - (GP_TETRA_UINT_MAX >> 24),
            GP_TETRA_UINT_MAX - (GP_TETRA_UINT_MAX >> 23),
            GP_TETRA_UINT_MAX - (GP_TETRA_UINT_MAX >> 22)
        };
        (void)u128s;
    } // gp_suite("Float conversions");
    #endif // __SIZEOF_INT128__

    #if _WIN32 // pedantic close
    gp_file_close(msvc_test_file);
    #endif
}

#else
int main(void) { return 0; }
#endif // __GNUC__ && __SIZEOF_INT128__
