// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/utils.c"
#include <stdio.h>
#include <time.h>

int main(void)
{
    gp_suite("Next Power of 2");
    {
        gp_test("Zero");
        {
            size_t npo0 = gp_next_power_of_2(0);
            gp_expect(npo0 == 1);
        }

        gp_test("Non-power of 2 rounding");
        {
            size_t npo5 = gp_next_power_of_2(5);
            gp_expect(npo5 == 8);
        }

        gp_test("Power of 2");
        {
            size_t npo2_6 = gp_next_power_of_2(1 << 6);
            gp_expect(npo2_6 == 1 << 7, "Should be the NEXT power of 2.");
        }
    }

    gp_suite("Random Nummber Generation");
    {
        // No need to test gp_random(), it is just a trivial wrapper to
        // pcg32_random_r(), returns random number in range [0, UINT32_MAX].

        GPRandomState s = gp_random_state((uint64_t)time(NULL));
        gp_test("Range");
        {
            for (size_t i = 0; i < 2048; ++i)
            {
                int32_t n = gp_random_range(&s, 4, 7);
                gp_assert(4 <= n && n < 7, "%w32i", n);
                n = gp_random_range(&s, -12, -3);
                gp_assert(-12 <= n && n < -3, "%w32i", n);
                n = gp_random_range(&s, -3, 3);
                gp_assert(-3 <= n && n < 3, "%w32i", n);
                // power of 2 ranges: works just the same but better optimized.
                n = gp_random_range(&s, 0, 16);
                gp_assert(0 <= n && n < 16);
            }
        }

        // Testing RNG results is flaky by definition, so this test is disabled
        // by default. However, it is useful for debugging, which is why this
        // should not be deleted.
        #ifdef GP_FLAKY_COIN_FLIP_TEST
        gp_test("Coin flip");
        {
            const size_t num_trials = 1000;
            size_t heads = 0;
            size_t tails = 0;
            size_t changes = 0;
            uint32_t previous = gp_random_range(&s, 0, 2);

            for (size_t i = 0; i < num_trials; ++i) {
                uint32_t n = gp_random_range(&s, 0, 2);
                heads += n == 0;
                tails += n == 1;
                changes += n == previous;
                previous = n;
                gp_assert(n <= 1);
            }
            gp_assert(heads + tails == num_trials);
            gp_expect(gp_approx((double)heads/tails, 1., .05), heads, tails);
            gp_expect(gp_approx((double)changes/num_trials, .5, .05), changes);
        }
        #endif
    } // gp_suite("Random Number Generation");

    gp_suite("min(), max()");
    {
        gp_test("as_signed()");
        {
            char bufs[5][4] = {0};
            // Using snprintf() for GCC format type checking. This confirms that
            // the return type is indeed the arguments signed equivalent.
            sprintf(bufs[0], "%hhi", gp_as_signed((unsigned char     )-1));
            sprintf(bufs[1], "%hi" , gp_as_signed((unsigned short    )-1));
            sprintf(bufs[2], "%i"  , gp_as_signed((unsigned int      )-1));
            sprintf(bufs[3], "%li" , gp_as_signed((unsigned long     )-1));
            sprintf(bufs[4], "%lli", gp_as_signed((unsigned long long)-1));
            gp_expect(strcmp(bufs[0], "-1") == 0);
            gp_expect(strcmp(bufs[1], "-1") == 0);
            gp_expect(strcmp(bufs[2], "-1") == 0);
            gp_expect(strcmp(bufs[3], "-1") == 0);
            gp_expect(strcmp(bufs[4], "-1") == 0);
        }

        gp_test("Basic min() and max()");
        {
            #if MIXED_SIGNEDNESS_MIN_MAX_WILL_NOT_COMPILE
            // GNUC will issue a lengthy error message about failing static
            // assertion, the readable message is at the end of that message.
            // Otherwise a cryptic message about negative array size is given.
            gp_expect(gp_min(-1, 1u));
            gp_expect(gp_max(-1u, 1));
            #endif

            gp_expect(gp_min(-1, -2) == -2);
            gp_expect(gp_max(-1u, 1llu) == UINT_MAX);
        }

        gp_test("Signed imin() and imax()");
        {
            // This time mixed signs are ok, however, both of the arguments will
            // be interpreted as signed.
            gp_expect(gp_imin(-1, 1u) == -1);
            gp_expect(gp_imin(0u, 1u - 7u) == -6);
            gp_expect(gp_imax(0u, 1u - 7u) == 0);
            gp_expect(gp_imax(SIZE_MAX/* (size_t)-1 */, 3) == 3);

            // Sanity checks
            gp_expect(gp_imin( 3,  9) ==  3);
            gp_expect(gp_imax( 3,  9) ==  9);
            gp_expect(gp_imin( 3, -9) == -9);
            gp_expect(gp_imax( 3, -9) ==  3);
            gp_expect(gp_imin(-3, -9) == -9);
            gp_expect(gp_imax(-3, -9) == -3);
        }
    } // gp_suite("min(), max()");
}
