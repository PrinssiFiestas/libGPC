// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/utils.c"
#include <stdint.h>

int main(void)
{
    gpc_suite("next_power_of_2");
    {
        gpc_test("Zero");
        {
            size_t npo0 = gpc_next_power_of_2(0);
            gpc_expect(npo0 == 1, (GPC_SIZE_T_FORMAT, npo0));
        }

        gpc_test("Non-power of 2 rounding");
        {
            size_t npo5 = gpc_next_power_of_2(5);
            gpc_expect(npo5 == 8, (GPC_SIZE_T_FORMAT, npo5));
        }

        gpc_test("Power of 2");
        {
            size_t npo2_6 = gpc_next_power_of_2(1 << 6);
            gpc_expect(npo2_6 == 1 << 7,
            (GPC_SIZE_T_FORMAT " Should be the NEXT power of 2", npo2_6),
            (GPC_SIZE_T_FORMAT, (size_t)(1 << 7)));
        }

        gpc_test("Overflow protection");
        {
            size_t very_large_number = SIZE_MAX - 16;
            size_t npo_max = gpc_next_power_of_2(very_large_number);
            gpc_expect(npo_max == SIZE_MAX,
            ("Will lead to infinite loop without protection."));
        }
    }
}
