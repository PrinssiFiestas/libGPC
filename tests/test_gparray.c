// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include <gpc/gparray.h>
#include <stdio.h>

#define arr_assert_eq(ARR, CARR, CARR_LENGTH, ...) do { \
    gp_expect(gp_arr_length(ARR) == (CARR_LENGTH)); \
    for (size_t _gp_i = 0; _gp_i < (CARR_LENGTH); ++_gp_i) \
        if (!gp_expect((ARR)[_gp_i] == (CARR)[_gp_i]__VA_OPT__(,) __VA_ARGS__)) \
            printf("i = %zu\n", _gp_i); \
} while (0)

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
            gp_expect(gp_arr_capacity(arr) > INIT_CAPACITY
                && (uintptr_t)arr == (uintptr_t)init_pos,
                "Arenas should know how to extend memory of "
                "lastly created objects so arr is not moved.");

            void* new_object= gp_mem_alloc(&arena->base, 1); (void)new_object;
            gp_arr_reserve(&arr, 32);
            gp_expect((uintptr_t)arr != (uintptr_t)init_pos,
                      "arr can not extend since it would overwrite new object.");
            const int*const new_pos = arr;
            gp_arr_reserve(&arr, 64);
            gp_expect((uintptr_t)arr == (uintptr_t)new_pos,
                      "After reallocation arr is lats element so it can be extended.");

            gp_arr_reserve(&arr, 256);
            gp_expect((uintptr_t)arr != (uintptr_t)new_pos,
                      "arr did not fit in arena node so it should be reallocated.");

            gp_arena_delete(arena);

            // TODO contiguous arena tests
        }

        #ifdef GP_HAS_SANITIZER
        gp_test("Poisoning");
        {
            GPArray(uint64_t) arr = gp_arr_new(gp_heap, 16, sizeof arr[0]);
            GP_ASSERT_CRASH
                arr[0] = 0;
            GP_ASSERT_CRASH
                arr[15] = 0;

            gp_arr_reserve(&arr, 4);
            for (size_t i = 0; i < 4; i++)
                arr[i] = 0;
            GP_ASSERT_CRASH
                arr[5] = 0;

            gp_arr_delete(arr);
        }
        #endif

        // TODO will poison boundaries mess up this test? If yes, use contiguous array instead.
        gp_test("Alignment");
        {
            GPArena* arena = gp_arena_new(NULL, 1 << 16);
            const size_t BLOCK_SIZE = 1 << 8;

            // Align arena pointer.
            char* align = arena->base.alloc(
                &arena->base,
                NULL, 0,
                BLOCK_SIZE, BLOCK_SIZE,
                true, &(size_t){0});

            GPArray(char) arr = gp_arr_new_aligned(
                &arena->base, BLOCK_SIZE, sizeof arr[0], BLOCK_SIZE);

            gp_expect(align + 2 * BLOCK_SIZE == arr);
            gp_expect(align + BLOCK_SIZE == gp_arr_allocation(arr));

            gp_arr_delete(arr);

            gp_expect(gp_mem_alloc(&arena->base, 0) == align + BLOCK_SIZE);
        }

        // TODO test gp_arr_recycle() and gp_arr_finalize()
    } // gp_suite("Memory");

    gp_suite("Array Manipulation");
    {
        GPArena init = { .growth_factor = 1.0 };
        GPAllocator* alc = &gp_arena_new(NULL, 16)->base;

        gp_test("Copy Slice");
        {
            GPArrayBuffer(int, 64) buffer;
            GPArray(int) arr = gp_arr_buffered(int, &buffer);
            const int carr[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

            gp_arr_slice(&arr, carr, 1, 6);
            const int carr2[] = { 1, 2, 3, 4, 5 };
            arr_assert_eq(arr, carr2, gp_countof(carr2));
        }

        gp_test("Mutating Slice");
        {
            GPArrayBuffer(int, 64) buffer;
            GPArray(int) arr = gp_arr_buffered(int, &buffer);
            gp_arr_copy(&arr, ((int[]){ 0, 1, 2, 3, 4, 5 }), 5);
            gp_arr_slice(&arr, NULL, 2, 5);
            const int carr[] = { 2, 3, 4 };
            arr_assert_eq(arr, carr, gp_countof(carr));
        }

        gp_test("Push and Pop");
        {
            GPArray(int) arr = gp_arr_new(alc, 4, sizeof arr[0]);
            gp_arr_push(&arr, (int){3});
            *gp_arr_push(&arr) = (int){6};
            gp_expect(arr[0] == 3);
            gp_expect(arr[1] == 6);
            gp_expect(gp_arr_length(arr, int) == 2);
            gp_expect(*(int*)gp_arr_pop(&arr) == 6);
            gp_expect(*gp_arr_pop(&arr, int) == 3);
        }

        gp_test("Append, Insert, and Erase");
        {
            GPArray(int) arr = gp_arr_new(alc, 4, sizeof arr[0]);
            int* ret;
            ret = gp_arr_append(&arr, ((int[]){1,2,3}), 3);
            gp_expect(ret == arr);
            arr_assert_eq(arr, ((int[]){1,2,3}), 3);

            ret = gp_arr_append(&arr, ((int[]){4,5,6}), 3);
            gp_expect(ret == arr + 3);
            arr_assert_eq(arr, ((int[]){1,2,3,4,5,6}), 3);

            ret = gp_arr_insert(&arr, 3, ((int[]){0,0}), 2);
            gp_expect(ret == arr + 3);
            arr_assert_eq(arr, ((int[]){1,2,3,0,0,4,5,6}), 8);

            ret = gp_arr_erase(&arr, 3, 2);
            gp_expect(ret == arr + 3);
            arr_assert_eq(arr, ((int[]){1,2,3,4,5,6}), 6);
        }
    } // gp_suite("Array Manipulation");
}
