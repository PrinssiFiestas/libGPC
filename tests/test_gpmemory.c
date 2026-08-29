// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include <gpc/gpmemory.h>

// TODO migrate the old tests (or rewrite them as well).

int main(void)
{
    #ifdef GP_HAS_SANITIZER
    gp_suite("ASan Poisoning");
    {
        gp_test("Basic Poisoning");
        {
            int64_t arr[8] = {0};
            GP_ASSERT_CRASH // TODO
                (void)0;

            gp_asan_poison(arr, sizeof arr);
            GP_ASSERT_CRASH
                arr[0]++;
            GP_ASSERT_CRASH
                arr[7]++;

            gp_asan_unpoison(arr, sizeof arr);
            for (size_t i = 0; i < gp_countof(arr); i++)
                gp_assert(arr[i] == 0);
        }

        gp_test("Alignment");
        {
            // Address sanitizer requires 8-byte alignment for poisoning.
            union { uint64_t align; unsigned char bytes[16]; } mem = {0};

            // Partial poisoning does nothing.
            gp_asan_poison(mem.bytes + 1, 8);
            for (size_t i = 0; i < sizeof mem; i++)
                mem.bytes[i]++;

            gp_asan_poison(mem.bytes, sizeof mem);
            GP_ASSERT_CRASH
                mem.bytes[0]++;
            GP_ASSERT_CRASH
                mem.bytes[7]++;
            GP_ASSERT_CRASH
                mem.bytes[8]++;
            GP_ASSERT_CRASH
                mem.bytes[15]++;

            // Partial unpoison will unpoison the whole 8-byte region.
            gp_asan_unpoison(mem.bytes, 1);
            gp_asan_unpoison(mem.bytes + 15, 1);
            for (size_t i = 0; i < 16; i++)
                gp_assert(mem.bytes[i] == 0);
        }
    } // gp_suite("ASan Poisoning")
    #endif
}
