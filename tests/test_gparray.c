// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include <gpc/gparray.h>

int main(void)
{
    gp_suite("Memory");
    {
        gp_test("Stack Allocation");
        {
            GPArrayBuffer(int, 8) buffer;
            GPArray(int) arr = gp_arr_buffered(int, &buffer, 1, 2, 3, 4, 5, 6, 7, 8);
            gp_expect(gp_arr_length(arr) == 8);
            gp_expect(gp_arr_capacity(arr) == 8);
            int carr[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
            gp_expect(memcmp(arr, carr, 8*sizeof(int)) == 0);

            // Safe to delete (no-op)
            gp_arr_delete(arr);
        }

        gp_test("Arrays on Arenas");
        {
            GPArena* arena = gp_arena_new(NULL, 0);
            const size_t INIT_CAPACITY = 8;
            const size_t RESERVE_CAPACITY = INIT_CAPACITY + 1;
            GPArray(int) arr = gp_arr_new(&arena->base, INIT_CAPACITY, sizeof arr[0]);
            const int* init_pos = arr;
            gp_expect(gp_arr_capacity(arr) == INIT_CAPACITY);
            gp_arr_reserve(&arr, RESERVE_CAPACITY);
        }
    }

    // TODO rest of the missing tests
}
