// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define FUZZ_COUNT 4096

#if (__GNUC__ && __SIZEOF_INT128__) || _MSC_VER

// Force int128.h implementations to not use gp_tetra_int_t, we want to test against
// that. This is very hacky and finicky, a potential bug might be that we end up
// testing gp_tetra_int_t implementations against gp_tetra_int_t always passing the
// tests, so make sure to manually break a test to see that they still work on
// changes!
#  if __GNUC__
#    define GP_TEST_INT128 1
#  endif

#include "../src/int128.c"
#include <gpc/assert.h>
#include <gpc/io.h>
#include <gpc/utils.h>
#include <gpc/terminal.h>

#if __GNUC__

#  if _WIN32 && !__cplusplus// write result to file that we can use for MSVC tests
#    define WRITE_RESULT(A) \
        fwrite(&(gp_tetra_uint_t){A}, sizeof(A), 1, msvc_test_file)
#  else
#    define WRITE_RESULT(...) ((void)0)
#  endif // !__cplusplus

#  define expect_eq128(A, B, ...) \
      (WRITE_RESULT(B), gp_expect(gp_i128(A).i128 == (gp_tetra_int_t)(B)__VA_OPT__(,)__VA_ARGS__))

#  define assert_eq128(A, B, ...) \
      (WRITE_RESULT(B), gp_assert(gp_i128(A).i128 == (gp_tetra_int_t)(B)__VA_OPT__(,)__VA_ARGS__))

#else // MSVC

#include <stdarg.h>
bool confirm_result(FILE* msvc_test_file, ...)
{
    va_list arg;
    va_start(arg, msvc_test_file);
    GPUInt128 u0, u1 = va_arg(arg, GPUInt128);
    fread(&u0, sizeof u0, 1, msvc_test_file);
    va_end(arg);
    return memcmp(&u0, &u1, sizeof u0) == 0;
}
#  define expect_eq128(A, ...) \
      gp_expect(confirm_result(msvc_test_file, A))

#  define assert_eq128(A, ...) \
      gp_assert(confirm_result(msvc_test_file, A))

#endif // __GNUC__

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
    gp_suite("Endianness"); // the very prerequisite for anything
    {
        uint16_t u16 = 1;
        uint8_t* p = (uint8_t*)&u16;
        #if __GNUC__
        GPUInt128 u128;
        u128.u128 = 1;
        gp_assert(gp_uint128_hi(u128) == 0);
        gp_assert(gp_uint128_lo(u128) == 1);
        #endif
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
    #  if __GNUC__ && !__cplusplus
    FILE* msvc_test_file = gp_file_open("./build/gnu_int128_result.bin", "write");
    gp_assert(msvc_test_file != NULL, strerror(errno));
    #  else // MSVC
    FILE* msvc_test_file = gp_file_open("./build/gnu_int128_result.bin", "read");
    if (msvc_test_file == NULL) {
        gp_println(GP_YELLOW
            "[WARNING] could not open MSVC test file in test_int128.c, skipping GPInt128 tests."
            GP_RESET_TERMINAL);
        exit(0);
    }
    #  endif
    #endif

    // WARNING: do NOT conditionally compile any uses of random numbers, MSVC
    // tests expect the same set of random numbers that was given to GCC.
    static GPRandomState rs; // Seed random state with date
    {
        time_t t = time(NULL);
        struct tm* tm = gmtime(&t);
        gp_assert(tm != NULL);
        rs = gp_random_state(tm->tm_yday + 1000*tm->tm_year);
    }

    gp_suite("Leading/trailing zeroes"); // internal test, but prerequisite
    {
        uint64_t u64;

        gp_random_bytes(&rs, &u64, sizeof u64);
        gp_test("Leading zeroes"); for (size_t n = 0; n < 64; ++n)
        {
            if ((u64 >> n) == 0)
                continue;
            #if __GNUC__
            gp_assert((int)gp_leading_zeros_u64(u64 >> n) == (int)__builtin_clzll(u64 >> n),
                "%w64X", u64,
                n,
                gp_leading_zeros_u64(u64 >> n),
                __builtin_clzll(u64 >> n));
            #endif
            gp_assert(gp_leading_zeros_u64(u64 >> n) >= n);
            gp_random_bytes(&rs, &u64, sizeof u64);
        }

        gp_random_bytes(&rs, &u64, sizeof u64);
        gp_test("Trailing zeroes"); for (size_t n = 0; n < 64; ++n)
        {
            if ((u64 << n) == 0)
                continue;
            #if __GNUC__
            gp_assert((int)gp_trailing_zeros_u64(u64 << n) == (int)__builtin_ctzll(u64 << n),
                "%w64X", u64,
                n,
                gp_trailing_zeros_u64(u64 >> n),
                __builtin_ctzll(u64 >> n));
            #endif
            gp_assert(gp_trailing_zeros_u64(u64 << n) >= n, "%zu", n);
            gp_random_bytes(&rs, &u64, sizeof u64);
        }
    } // gp_suite("Leading/trailing zeroes");

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

            expect_eq128(gp_uint128_not(ua), ~ua.u128);
            expect_eq128(gp_uint128_not(ub), ~ub.u128);
            expect_eq128(gp_int128_not(ia),  ~ia.i128);
            expect_eq128(gp_int128_not(ib),  ~ib.i128);

            expect_eq128(gp_uint128_and(ua, ub), ua.u128 & ub.u128);
            expect_eq128(gp_int128_and(ia, ib),  ia.i128 & ib.i128);

            expect_eq128(gp_uint128_or(ua, ub), ua.u128 | ub.u128);
            expect_eq128(gp_int128_or(ia, ib),  ia.i128 | ib.i128);

            expect_eq128(gp_uint128_xor(ua, ub), ua.u128 ^ ub.u128);
            expect_eq128(gp_int128_xor(ia, ib),  ia.i128 ^ ib.i128);
        }

        // Note: n larger or equal to 128 is undefined.
        gp_test("<< >>"); for (uint8_t n = 0; n < 128; ++n)
        {
            ua = uint128_random(&rs);
            ia = int128_random(&rs);

            // Shifting big signed numbers left cannot be represented in
            // __int128_t, which is undefined, use mask to limit the size of
            // the numbers.
            GPInt128 mask = gp_int128_shift_right(GP_INT128_MAX, n);

            assert_eq128(gp_uint128_shift_left(ua, n),  ua.u128 << n, "%hhu", n);
            assert_eq128(gp_uint128_shift_right(ua, n), ua.u128 >> n, "%hhu", n);
            assert_eq128(gp_int128_shift_right(ia, n),  ia.i128 >> n, "%hhu", n);
            assert_eq128(
                gp_int128_shift_left(gp_int128_and(ia, mask), n),
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
            gp_expect(gp_uint128_equal(gp_uint128_add(ua, ub), gp_uint128(1, 0)));
            expect_eq128(gp_uint128_add(ua, ub), ua.u128 + ub.u128);
            ua = gp_uint128(1, 0);
            gp_expect(gp_uint128_equal(gp_uint128_sub(ua, ub), gp_uint128(0, UINT64_MAX)));
            expect_eq128(gp_uint128_sub(ua, ub), ua.u128 - ub.u128);

            // Overflow
            ua = GP_UINT128_MAX;
            ub = uint128_random(&rs);
            expect_eq128(gp_uint128_add(ua, ub), ua.u128 + ub.u128);
            ua = gp_uint128(0, 0);
            expect_eq128(gp_uint128_sub(ua, ub), ua.u128 - ub.u128);
        }

        gp_test("Signed +-");
        {
            // Carry propagation
            ia = gp_int128(0, UINT64_MAX);
            ib = gp_int128(0, 1);
            gp_expect(gp_int128_equal(gp_int128_add(ia, ib), gp_int128(1, 0)));
            expect_eq128(gp_int128_add(ia, ib), ia.i128 + ib.i128);
            ia = gp_int128(1, 0);
            gp_expect(gp_int128_equal(gp_int128_sub(ia, ib), gp_int128(0, UINT64_MAX)));
            expect_eq128(gp_int128_sub(ia, ib), ia.i128 - ib.i128);

            // Overflow is undefined

            // Positive + negative carry ((UINT64_MAX+1) + -1 == UINT64_MAX)
            ia = gp_int128(1, 0);
            ib = gp_int128(-1, -1);
            gp_expect(gp_int128_equal(gp_int128_add(ia, ib), gp_int128(0, UINT64_MAX)));
            expect_eq128(gp_int128_add(ia, ib), ia.i128 + ib.i128);

            // Negative + negative (-1 + -1 == -2)
            ia = gp_int128(-1, -1);
            ib = gp_int128(-1, -1);
            gp_expect(gp_int128_equal(gp_int128_add(ia, ib), gp_int128(-1, -2)));
            expect_eq128(gp_int128_add(ia, ib), ia.i128 + ib.i128);
        }

        gp_test("+- fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        { // test basic addition/subtraction and large numbers with mixed signs
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            ia = int128_random(&rs);
            ib = int128_random(&rs);

            assert_eq128(gp_uint128_add(ua, ub), ua.u128 + ub.u128, "%zu", fuzz_count);
            assert_eq128(gp_uint128_sub(ua, ub), ua.u128 - ub.u128, "%zu", fuzz_count);

            // Again, overflow is UB!
            if (gp_i128_greater_than_equal(ib, 0) &&
                gp_i128_less_than_equal(ia, gp_i128_sub(GP_INT128_MAX, ib)))
                assert_eq128(gp_int128_add(ia, ib), ia.i128 + ib.i128, "%zu", fuzz_count);
            if (gp_i128_less_than(ib, 0) &&
                gp_i128_greater_than_equal(ia, gp_i128_sub(GP_INT128_MIN, ib)))
                assert_eq128(gp_int128_add(ia, ib), ia.i128 + ib.i128, "%zu", fuzz_count);
            if (gp_i128_greater_than_equal(ib, 0) &&
                gp_i128_greater_than_equal(ia, gp_i128_add(GP_INT128_MIN, ib)))
                assert_eq128(gp_int128_sub(ia, ib), ia.i128 - ib.i128, "%zu", fuzz_count);
            if (gp_i128_less_than(ib, 0) &&
                gp_i128_less_than_equal(ia, gp_i128_add(GP_INT128_MAX, ib)))
                assert_eq128(gp_int128_sub(ia, ib), ia.i128 - ib.i128, "%zu", fuzz_count);
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
            expect_eq128(gp_uint128_negate(ua), -ua.u128);
            expect_eq128(gp_int128_negate(ia), -ia.i128);

            ua = gp_uint128(0, UINT64_MAX);
            ia = gp_int128(0, UINT64_MAX);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(UINT64_MAX, 1)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(-1, 1)));
            expect_eq128(gp_uint128_negate(ua), -ua.u128);
            expect_eq128(gp_int128_negate(ia), -ia.i128);

            ua = gp_uint128(UINT64_MAX, 0);
            ia = gp_int128(-1, 0);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(1, 0)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(1, 0)));
            expect_eq128(gp_uint128_negate(ua), -ua.u128);
            expect_eq128(gp_int128_negate(ia), -ia.i128);

            ua = gp_uint128(UINT64_MAX, UINT64_MAX);
            ia = gp_int128(-1, -1);
            gp_expect(gp_uint128_equal(gp_uint128_negate(ua), gp_uint128(0, 1)));
            gp_expect(gp_int128_equal(gp_int128_negate(ia), gp_int128(0, 1)));
            expect_eq128(gp_uint128_negate(ua), -ua.u128);
            expect_eq128(gp_int128_negate(ia), -ia.i128);

            ua = uint128_random(&rs);
            ia = int128_random(&rs);
            expect_eq128(gp_uint128_negate(ua), -ua.u128);
            expect_eq128(gp_int128_negate(ia), -ia.i128);
        }

        gp_test("Multiply 64-bit unsigned integers to 128-bit unsigned integer");
        {
            uint64_t a = gp_random(&rs), b = gp_random(&rs);
            gp_expect(gp_uint128_less_than(gp_uint128_mul64(a, b), gp_uint128(1, 0)));
            expect_eq128(gp_uint128_mul64(a, b), (__uint128_t)a * b);

            gp_expect(gp_uint128_equal(
                gp_uint128_mul64(UINT64_MAX, 2), gp_uint128(1, UINT64_MAX - 1)));
            expect_eq128(gp_uint128_mul64(UINT64_MAX, 2), (__uint128_t)UINT64_MAX * 2);

            gp_expect(gp_uint128_equal(
                gp_uint128_mul64(UINT64_MAX, UINT64_MAX), gp_uint128(-2, 1)));
            expect_eq128(gp_uint128_mul64(UINT64_MAX, UINT64_MAX), (__uint128_t)UINT64_MAX * UINT64_MAX);

            ua = uint128_random(&rs);
            expect_eq128(gp_uint128_mul64(ua.little_endian.lo, ua.little_endian.hi), (__uint128_t)ua.little_endian.lo * ua.little_endian.hi,
                "%llx", ua.little_endian.lo, "%llx", ua.little_endian.hi);
        }

        gp_test("Multiply 64-bit signed integers to 128-bit signed integer");
        {
            int64_t a = gp_random(&rs), b = gp_random(&rs);
            expect_eq128(gp_int128_mul64(a,   b), (__int128_t)a  *  b);
            expect_eq128(gp_int128_mul64(-a,  b), (__int128_t)-a *  b);
            expect_eq128(gp_int128_mul64(a,  -b), (__int128_t)a  * -b);
            expect_eq128(gp_int128_mul64(-a, -b), (__int128_t)-a * -b);
            a <<= 30;
            expect_eq128(gp_int128_mul64(a,   b), (__int128_t)a  *  b);
            expect_eq128(gp_int128_mul64(-a,  b), (__int128_t)-a *  b);
            expect_eq128(gp_int128_mul64(a,  -b), (__int128_t)a  * -b);
            expect_eq128(gp_int128_mul64(-a, -b), (__int128_t)-a * -b);
            a >>= 30; b <<= 30;
            expect_eq128(gp_int128_mul64(a,   b), (__int128_t)a  *  b);
            expect_eq128(gp_int128_mul64(-a,  b), (__int128_t)-a *  b);
            expect_eq128(gp_int128_mul64(a,  -b), (__int128_t)a  * -b);
            expect_eq128(gp_int128_mul64(-a, -b), (__int128_t)-a * -b);
            a <<= 30;
            expect_eq128(gp_int128_mul64(a,   b), (__int128_t)a  *  b);
            expect_eq128(gp_int128_mul64(-a,  b), (__int128_t)-a *  b);
            expect_eq128(gp_int128_mul64(a,  -b), (__int128_t)a  * -b);
            expect_eq128(gp_int128_mul64(-a, -b), (__int128_t)-a * -b);
        }

        gp_test("Unsigned fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        {
            // Absolutely massive numbers, practically always overflow
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            assert_eq128(gp_uint128_mul(ua, ub), ua.u128 * ub.u128, "%zu", fuzz_count);

            // Huge numbers, should overflow sometimes
            ua = gp_uint128_and(ua, gp_uint128(0x00000005FFFFFFFF, 0xFFFFFFFFFFFFFFFF));
            ub = gp_uint128(0, gp_random(&rs));
            assert_eq128(gp_uint128_mul(ua, ub), ua.u128 * ub.u128, "%zu", fuzz_count);
        }

        gp_test("Signed fuzz"); for (size_t fuzz_count = 0; fuzz_count < FUZZ_COUNT; ++fuzz_count)
        {
            // Bitwise operations used to limit the size of numbers to prevent
            // overflow.

            // Positive * negative
            ia = gp_int128_and(int128_random_positive(&rs), gp_int128(1, 0xFFFFFFFFFFFFFFFF));
            ib = gp_int128(-1, 0xFFFFFFFF00000000 | gp_random(&rs));
            assert_eq128(gp_int128_mul(ia, ib), ia.i128 * ib.i128, "%zu", fuzz_count);

            // Negative * positive
            ia = gp_int128_or(int128_random_negative(&rs), gp_int128(-2, 0));
            ib = gp_int128(0, gp_random(&rs));
            assert_eq128(gp_int128_mul(ia, ib), ia.i128 * ib.i128, "%zu", fuzz_count);

            // Negative * negative
            ia = gp_int128_or(int128_random_negative(&rs), gp_int128(-2, 0));
            ib = gp_int128(-1, 0xFFFFFFFF00000000 | gp_random(&rs));
            assert_eq128(gp_int128_mul(ia, ib), ia.i128 * ib.i128, "%zu", fuzz_count);
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
            expect_eq128(gp_uint128_divmod(ua, ub, &remainder), ua.u128 / ub.u128);
            expect_eq128(remainder, ua.u128 % ub.u128);
        }

        gp_test("0X/XX");
        {
            ua = gp_uint128(0, gp_uint128_lo(uint128_random(&rs)));
            ub = uint128_random(&rs);
            expect_eq128(gp_uint128_divmod(ua, ub, &remainder), ua.u128 / ub.u128);
            expect_eq128(remainder, ua.u128 % ub.u128);
        }

        gp_test("X0/X0");
        {
            u64s = uint128_random(&rs);
            ua = gp_uint128(u64s.little_endian.lo, 0);
            ub = gp_uint128(u64s.little_endian.hi, 0);
            expect_eq128(gp_uint128_divmod(ua, ub, &remainder), ua.u128 / ub.u128);
            expect_eq128(remainder, ua.u128 % ub.u128);
        }

        gp_test("XX/X0");
        {
            ua = uint128_random(&rs);
            ub = gp_uint128(gp_uint128_hi(uint128_random(&rs)), 0);
            expect_eq128(gp_uint128_divmod(ua, ub, &remainder), ua.u128 / ub.u128);
            expect_eq128(remainder, ua.u128 % ub.u128);
        }

        gp_test("XX/0X");
        {
            ua = uint128_random(&rs);
            ub = gp_uint128(0, gp_uint128_lo(uint128_random(&rs)));
            expect_eq128(gp_uint128_divmod(ua, ub, &remainder), ua.u128 / ub.u128);
            expect_eq128(remainder, ua.u128 % ub.u128);
        }

        gp_test("XX/XX");
        {
            ua = uint128_random(&rs);
            ub = uint128_random(&rs);
            expect_eq128(gp_uint128_divmod(ua, ub, &remainder), ua.u128 / ub.u128,
                "%w128X", ua, "%w128X", ub,
                "%w128X", gp_uint128_divmod(ua, ub, &remainder),
                "%w128X", ua.u128 / ub.u128);
            expect_eq128(remainder, ua.u128 % ub.u128);
        }

        // Signed div/mod uses gp_uint128_divmod under the hood, just sanity
        // check for correct sign handling.
        gp_test("Sign");
        {
            ia = int128_random_positive(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_positive(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_greater_than_equal(
                gp_int128_idiv(ia, ib), gp_int128(0, 0)));
            expect_eq128(gp_int128_idiv(ia, ib), ia.i128 / ib.i128);
            expect_eq128(gp_int128_imod(ia, ib), ia.i128 % ib.i128);

            ia = int128_random_negative(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_positive(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_less_than(gp_int128_idiv(ia, ib), gp_int128(0, 0)));
            expect_eq128(gp_int128_idiv(ia, ib), ia.i128 / ib.i128);
            expect_eq128(gp_int128_imod(ia, ib), ia.i128 % ib.i128);

            ia = int128_random_positive(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_negative(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_less_than(gp_int128_idiv(ia, ib), gp_int128(0, 0)));
            expect_eq128(gp_int128_idiv(ia, ib), ia.i128 / ib.i128);
            expect_eq128(gp_int128_imod(ia, ib), ia.i128 % ib.i128);

            ia = int128_random_negative(&rs);
            do {
                ib = gp_int128_shift_right(int128_random_negative(&rs), gp_random_range(&rs, 64, 127));
            } while (gp_int128_equal(ib, gp_int128(0, 0)));
            gp_expect(gp_int128_greater_than_equal(gp_int128_idiv(ia, ib), gp_int128(0, 0)));
            expect_eq128(gp_int128_idiv(ia, ib), ia.i128 / ib.i128);
            expect_eq128(gp_int128_imod(ia, ib), ia.i128 % ib.i128);
        }
    } // gp_suite("Division/modulus");

    #ifdef __SIZEOF_INT128__
    gp_suite("Float conversions"); // https://github.com/m-ou-se/floatconv/blob/main/src/test.rs
    {
        static const gp_tetra_uint_t u128s[] = {
            0, 1, 2, 3, 1234,
            GP_TETRA_UINT_MAX, // Overflows the mantissa, should increment the exponent (which will be odd).
            GP_TETRA_UINT_MAX / 2, // Overflows the mantissa, should increment the exponent (which will be even).
            0x400000000000000, // Exact match, no rounding
            0x400000000000022, // Round to closest (up)
            0x400000000000012, // Round to closest (down)
            0x8000000000000C, // Tie, round to even (up)
            0x80000000000004, // Tie, round to even (down)
            // Round to closest (up), with tie-breaking bit further than 64 bits away.
            ((gp_tetra_uint_t)0x8000000000000400 << 64) | 0x0000000000000001,
            // Round to closest (down), with 1-bit in 63rd position (which should be insignificant).
            ((gp_tetra_uint_t)0x8000000000000000 << 64) | 0x8000000000000000,
            // Round to closest (down), with 1-bits in all insignificant positions.
            ((gp_tetra_uint_t)0x80000000000003FF << 64) | 0xFFFFFFFFFFFFFFFF,
            // Mantissa of 2*52 bits, with last 32 bits set.
            ((gp_tetra_uint_t)0x10000000000      << 64) | 0x00000000FFFFFFFF,
            // Mantissa of 2*52 bits, with bit 23 set.
            ((gp_tetra_uint_t)0x10000000000      << 64) | 0x0000000000800000,
            // Mantissa of 2*52 bits, with last 23 bits set.
            ((gp_tetra_uint_t)0x10000000000      << 64) | 0x00000000007FFFFF,
            // Mantissa of 128-32 bits, with last 24 bits set.
            ((gp_tetra_uint_t)0x100000000        << 64) | 0x0000000000FFFFFF,
            (gp_tetra_uint_t)1 << 127,
            (gp_tetra_uint_t)2 << 126,
            (gp_tetra_uint_t)3 << 126,
            (gp_tetra_uint_t)1 << 64,
            (gp_tetra_uint_t)1 << 63,
            (gp_tetra_uint_t)1 << 54,
            (gp_tetra_uint_t)1 << 53,
            (gp_tetra_uint_t)1 << 52,
            (gp_tetra_uint_t)1 << 51,
            ((gp_tetra_uint_t)1 << 54) - 1,
            ((gp_tetra_uint_t)1 << 53) - 1,
            ((gp_tetra_uint_t)1 << 52) - 1,
            ((gp_tetra_uint_t)1 << 51) - 1,
            ((gp_tetra_uint_t)1 << 54) + 1,
            ((gp_tetra_uint_t)1 << 53) + 1,
            ((gp_tetra_uint_t)1 << 52) + 1,
            ((gp_tetra_uint_t)1 << 51) + 1,
            UINT64_MAX,
            (gp_tetra_uint_t)UINT64_MAX << 64,
            (gp_tetra_uint_t)UINT64_MAX << 63,
            (gp_tetra_uint_t)UINT64_MAX << 53,
            (gp_tetra_uint_t)UINT64_MAX << 52,
            (gp_tetra_uint_t)UINT64_MAX << 51,
            (gp_tetra_uint_t)(UINT64_MAX >> 13) << 64,
            (gp_tetra_uint_t)(UINT64_MAX >> 13) << 63,
            (gp_tetra_uint_t)(UINT64_MAX >> 13) << 53,
            (gp_tetra_uint_t)(UINT64_MAX >> 13) << 52,
            (gp_tetra_uint_t)(UINT64_MAX >> 13) << 51,
            (gp_tetra_uint_t)(UINT64_MAX >> 12) << 64,
            (gp_tetra_uint_t)(UINT64_MAX >> 12) << 63,
            (gp_tetra_uint_t)(UINT64_MAX >> 12) << 53,
            (gp_tetra_uint_t)(UINT64_MAX >> 12) << 52,
            (gp_tetra_uint_t)(UINT64_MAX >> 12) << 51,
            (gp_tetra_uint_t)(UINT64_MAX >> 11) << 64,
            (gp_tetra_uint_t)(UINT64_MAX >> 11) << 63,
            (gp_tetra_uint_t)(UINT64_MAX >> 11) << 53,
            (gp_tetra_uint_t)(UINT64_MAX >> 11) << 52,
            (gp_tetra_uint_t)(UINT64_MAX >> 11) << 51,
            GP_TETRA_UINT_MAX - (GP_TETRA_UINT_MAX >> 24),
            GP_TETRA_UINT_MAX - (GP_TETRA_UINT_MAX >> 23),
            GP_TETRA_UINT_MAX - (GP_TETRA_UINT_MAX >> 22)
        };
        gp_test("GPUint128 <-> float/double"); for (
            size_t i = 0; i < sizeof u128s/sizeof u128s[0]; ++i)
        {
            GPUInt128 u = gp_u128(u128s[i]);
            gp_assert(gp_f32_uint128(u) == (float)u128s[i]);
            gp_assert(gp_f64_uint128(u) == (double)u128s[i]);

            // isinf((float)u128s[i]) fails to detect inf for whatever reason,
            // converting it first trough a variable fixes this. Compiler bug?
            float  f32 = u128s[i];
            double f64 = u128s[i];
            if ( ! isinf(f32))
                gp_assert(gp_uint128_f32(f32).u128 == (gp_tetra_uint_t)f32);
            if ( ! isinf(f64))
                gp_assert(gp_uint128_f64(f64).u128 == (gp_tetra_uint_t)f64);
        }
    } // gp_suite("Float conversions");
    #endif // __SIZEOF_INT128__

    #if _WIN32 // pedantic close
    gp_file_close(msvc_test_file);
    #endif
}

#else
int main(void) { return 0; }
#endif // __GNUC__ && __SIZEOF_INT128__
