// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include "../src/array.c"

#define arr_assert_eq(ARR, CARR, CARR_LENGTH) do { \
    typeof(ARR  )  _gp_arr1 = (ARR);  \
    typeof(*CARR)* _gp_arr2 = (CARR); \
    const size_t _gp_arr2_length = CARR_LENGTH; \
    gp_expect(gp_arr_length(_gp_arr1) == _gp_arr2_length, \
        gp_arr_length(_gp_arr1), _gp_arr2_length); \
    for (size_t _gp_i = 0; _gp_i < _gp_arr2_length; _gp_i++) \
        gp_assert(_gp_arr1[_gp_i] == _gp_arr2[_gp_i], _gp_arr1[_gp_i], _gp_arr2[_gp_i]); \
} while(0)

#define CARR_LEN(CARR) (sizeof(CARR) / sizeof*(CARR))

int main(void)
{
    gp_suite("Memory");
    {
        gp_test("Arrays on stack");
        {
            // Create array that can hold 4 elements
            GPArray(int) arr = gp_arr_on_stack(&gp_heap, 4, int);
            gp_expect(gp_arr_allocation(arr) == NULL,
                "Stack allocated arrays should not have a allocation.");

            // It's okay not to provide an allocator since this one does not grow.
            const GPArray(int) arr2 = gp_arr_on_stack(NULL, 8, int,
                1, 2, 3, 4, 5, 6, 7, 8);

            // Copying past capacity is safe; arr uses gp_heap to reallocate.
            arr = gp_arr_copy(sizeof*arr, arr, arr2, gp_arr_length(arr2));
            arr_assert_eq(arr, arr2, gp_arr_length(arr2));

            // First array is on gp_heap, don't forget to deallocate!
            gp_arr_delete(arr);

            // The second array is on stack and no allocator provided so no need
            // to delete. It is safe, but in this case compiler warns about
            // discarding const qualifier.
            #if WARNING
            gp_arr_delete(arr2);
            #endif
        }

        gp_test("Fast array on stack");
        {
            // Avoiding initialization cost of large arrays by manually creating
            // them on stack.
            struct name_is_optional_if_C11_but_C99_requires_it {
                GPArrayHeader header;
                int array[2048];
            } array_mem; // No init!
            array_mem.header.capacity  = 2048;
            array_mem.header.allocator = &gp_heap; // optional if 2048 is not exceeded
            GPArray(int) arr           = array_mem.array;

            const int carr[] = { 1, 2, 3 };
            arr = gp_arr_copy(sizeof*arr, arr, carr, CARR_LEN(carr));
            arr_assert_eq(arr, carr, CARR_LEN(carr));

            gp_arr_delete(arr);
        }

        gp_test("Arrays on arenas/scopes");
        {
            // Here using the scope allocator, but everything in this example
            // applies to GPArena as well.

            // Note: arrays and arenas have metadata so an arena with size
            // 256 * sizeof(int) is NOT capable of holding an array with 256 ints.
            GPAllocator* scope = gp_begin(256 * sizeof(int));

            const size_t INIT_CAPACITY = 8;
            GPArray(int) arr = gp_arr_new(scope, sizeof*arr, INIT_CAPACITY);
            const int*const init_pos = arr;
            gp_expect(gp_arr_capacity(arr) == INIT_CAPACITY);
            arr = gp_arr_reserve(sizeof*arr, arr, 9); // Extend arr memory
            gp_expect(gp_arr_capacity(arr) > INIT_CAPACITY
                && arr == init_pos,"Arenas should know how to extend memory of "
                                   "lastly created objects so arr is not moved.");

            void* new_object = gp_mem_alloc(scope, 1); (void)new_object;
            arr = gp_arr_reserve(sizeof*arr, arr, 32);
            gp_expect(arr != init_pos,
                "arr can nott extend since it would overwrite new_object.");
            const int*const new_pos = arr;
            arr = gp_arr_reserve(sizeof*arr, arr, 64);
            gp_expect(arr == new_pos,
                "After reallocation arr is last element so it can be extended.");

            arr = gp_arr_reserve(sizeof*arr, arr, 256);
            gp_expect(arr != new_pos,
                "arr did not fit in arena so it should've been reallocated.");

            // No need to delete arr or dealloc new_object, they live in scope.
            gp_end(scope);
        }
    }

    gp_suite("Array manipulation");
    {
        const int carr[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
        gp_test("Copy slice");
        {
            GPArray(int) arr = gp_arr_on_stack(NULL, 64, int);

            // Always reassign destination array to self in case of reallocation!
            arr = gp_arr_slice(sizeof*arr, arr, carr, 1, 6);
            const int carr[] = { 1, 2, 3, 4, 5 };
            arr_assert_eq(arr, carr, CARR_LEN(carr));
        }

        gp_test("Mutating slice");
        {
            GPArray(int) arr = gp_arr_on_stack(NULL, 64, int, 0, 1, 2, 3, 4, 5 );
            arr = gp_arr_slice(sizeof*arr, arr, NULL, 2, 5);
            const int carr[] = { 2, 3, 4 };
            arr_assert_eq(arr, carr, CARR_LEN(carr));
        }
    }
}
