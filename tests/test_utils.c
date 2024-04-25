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

        gp_test("Overflow protection");
        {
            size_t very_large_number = SIZE_MAX - 16;
            size_t npo_max = gp_next_power_of_2(very_large_number);
            gp_expect(npo_max == SIZE_MAX,
            ("Will lead to infinite loop without protection."));
        }
    }

    gp_suite("mem_eq sanity check");
    {
        gp_test("if memories are equal");
            gp_expect(gp_mem_equal("blah", "blah", sizeof("blah")));
    }

    gp_suite("Clip range");
    {
        gp_test("Clipping");
        {
            // TODO
        }
    }

    gp_suite("random nummber generation");
    {
        gp_g_random_seed((uint64_t)time(NULL));
        gp_test("range");
        {
            for (int i = 0; i < 32; i++)
            {
                int32_t n = gp_g_random_range(4, 7);
                gp_assert(4 <= n && n <= 7);
                n = gp_g_random_range(-12, -3);
                gp_assert(-12 <= n && n <= -3);
                n = gp_g_random_range(-3, 3);
                gp_assert(-3 <= n && n <= 3);
            }
        }
    }
}
