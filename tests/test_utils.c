// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/utils.c"
#include <time.h>

int main(void)
{
    gp_suite("next_power_of_2");
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
            gp_expect(npo2_6 == 1 << 7, ("Should be the NEXT power of 2."));
        }
    }

    gp_suite("Bounds checking");
    {
        gp_test("Basic tests");
        {
            size_t start = 5; // non-inclusive
            size_t end   = 7; // inclusive
            gp_expect(gp_check_bounds(&start, &end, 7));

            start = 8;
            end   = 8;
            gp_expect( ! gp_check_bounds(&start, &end, 999),
                "end should be at least 1 more than start to not clip");
            gp_expect(start == 7, start);

            start = 5;
            end   = 5;
            gp_expect( ! gp_check_bounds(&start, NULL, 5));
            gp_expect(   gp_check_bounds(NULL,   &end, 5));

            start = 6;
            end   = 9;
            gp_expect( ! gp_check_bounds(&start, &end, 4));
            gp_expect(end == 4 && start == 3, start, end);
        }
        gp_test("Zero limit");
        {
            size_t start = 0;
            size_t end   = 0;
            gp_expect( ! gp_check_bounds(&start, NULL, 0));
            gp_expect(start == 0, "Can't make smaller than 0");

            gp_expect(gp_check_bounds(NULL, &end, 0), end);

            gp_expect( ! gp_check_bounds(&start, &end, 0));
            gp_expect(start == 0 && end == 0, start, end,
                "Can't have a valid range with 0 limit");
        }
    }

    gp_suite("random nummber generation");
    {
        GPRandomState s = gp_new_random_state((uint64_t)time(NULL));
        gp_test("range");
        {
            for (int i = 0; i < 32; i++)
            {
                int32_t n = gp_random_range(&s, 4, 7);
                gp_assert(4 <= n && n <= 7);
                n = gp_random_range(&s, -12, -3);
                gp_assert(-12 <= n && n <= -3);
                n = gp_random_range(&s, -3, 3);
                gp_assert(-3 <= n && n <= 3);
            }
        }
    }
}
