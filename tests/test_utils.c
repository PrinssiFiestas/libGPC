// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/utils.c"
#include <time.h>

int main(void)
{
    gpc_suite("next_power_of_2");
    {
        gpc_test("Zero");
        {
            size_t npo0 = gpc_next_power_of_2(0);
            gpc_expect(npo0 == 1);
        }

        gpc_test("Non-power of 2 rounding");
        {
            size_t npo5 = gpc_next_power_of_2(5);
            gpc_expect(npo5 == 8);
        }

        gpc_test("Power of 2");
        {
            size_t npo2_6 = gpc_next_power_of_2(1 << 6);
            gpc_expect(npo2_6 == 1 << 7, ("Should be the NEXT power of 2."));
        }

        gpc_test("Overflow protection");
        {
            size_t very_large_number = SIZE_MAX - 16;
            size_t npo_max = gpc_next_power_of_2(very_large_number);
            gpc_expect(npo_max == SIZE_MAX,
            ("Will lead to infinite loop without protection."));
        }
    }

    gpc_suite("mem_eq sanity check");
    {
        gpc_test("if memories are equal");
            gpc_expect(gpc_mem_eq("blah", "blah", sizeof("blah")));
    }

    gpc_suite("random nummber generation");
    {
        gpc_g_random_seed((uint64_t)time(NULL));
        gpc_test("range");
        {
            for (int i = 0; i < 32; i++)
            {
                int32_t n = gpc_g_random_range(4, 7);
                gpc_assert(4 <= n && n <= 7);
                n = gpc_g_random_range(-12, -3);
                gpc_assert(-12 <= n && n <= -3);
                n = gpc_g_random_range(-3, 3);
                gpc_assert(-3 <= n && n <= 3);
            }
        }
    }
}
