// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include <gpc/io.h>
#include "../src/array.c"

#define arr_assert_eq(ARR, CARR, CARR_LENGTH) do { \
    gp_expect(gp_arr_length(ARR) == (CARR_LENGTH)); \
    for (size_t _gp_i = 0; _gp_i < (CARR_LENGTH); ++_gp_i) \
        if (!gp_expect((ARR)[_gp_i] == (CARR)[_gp_i])) \
            printf("i = %zu\n", _gp_i); \
} while (0)

#define CARR_LEN(CARR) (sizeof(CARR) / sizeof*(CARR))

int main(void)
{
    gp_suite("Memory");
    {
        gp_test("Arrays on stack");
        {
            // Create array that can hold 4 elements
            GPArray(int) arr = gp_arr_on_stack(gp_heap, 4, int);
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
            array_mem.header = (GPArrayHeader) {
                .capacity  = 2048,
                .allocator = gp_heap // optional if 2048 is not exceeded
            };
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
            // 256 * sizeof(int) is NOT capable of holding an array with 256 ints. // TODO ok this settles it: change arrays to use power of 2 blocks!
            GPAllocator* scope = gp_begin(256 * sizeof(int));

            const size_t INIT_CAPACITY = 8;
            const size_t RESERVE_CAPACITY = INIT_CAPACITY + 1;
            GPArray(int) arr = gp_arr_new(scope, sizeof*arr, INIT_CAPACITY);
            const int* init_pos = arr;
            gp_expect(gp_arr_capacity(arr) == INIT_CAPACITY);
            arr = gp_arr_reserve(sizeof*arr, arr, RESERVE_CAPACITY); // Extend arr memory
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

            // Repeated memory block extension test on virtual arena
            GPVirtualArena va;
            arr = gp_arr_new(gp_virtual_init(&va, 4*4096), sizeof arr[0], INIT_CAPACITY);
            init_pos = arr;
            gp_expect(gp_arr_capacity(arr) == INIT_CAPACITY);
            arr = gp_arr_reserve(sizeof arr[0], arr, RESERVE_CAPACITY); // Extend arr memory
            gp_expect(gp_arr_capacity(arr) > INIT_CAPACITY
                && arr == init_pos,"Arenas should know how to extend memory of "
                                   "lastly created objects so arr is not moved.");
            gp_virtual_delete(&va);
        }
    } // gp_suite("Memory");

    gp_suite("Array manipulation");
    {
        GPAllocator* scope = gp_begin(0);

        gp_test("Copy slice");
        {
            GPArray(int) arr = gp_arr_on_stack(NULL, 64, int);
            const int carr[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

            // Always reassign destination array to self in case of reallocation!
            arr = gp_arr_slice(sizeof*arr, arr, carr, 1, 6);
            const int carr2[] = { 1, 2, 3, 4, 5 };
            arr_assert_eq(arr, carr2, CARR_LEN(carr2));
            arr = gp_arr_clear(arr);
            gp_assert(gp_arr_length(arr) == 0);
        }

        gp_test("Mutating slice");
        {
            GPArray(int) arr = gp_arr_on_stack(NULL, 64, int, 0, 1, 2, 3, 4, 5 );
            arr = gp_arr_slice(sizeof*arr, arr, NULL, 2, 5);
            const int carr[] = { 2, 3, 4 };
            arr_assert_eq(arr, carr, CARR_LEN(carr));
        }

        gp_test("Push and pop");
        {
            GPArray(int) arr = gp_arr_new(scope, sizeof*arr, 4);
            arr = gp_arr_push(sizeof*arr, arr, &(int){3});
            arr = gp_arr_push(sizeof*arr, arr, &(int){6});
            gp_expect(arr[0] == 3);
            gp_expect(arr[1] == 6);
            gp_expect(gp_arr_length(arr) == 2);
            gp_expect(*(int*)gp_arr_pop(sizeof*arr, arr) == 6);
            gp_expect(*(int*)gp_arr_pop(sizeof*arr, arr) == 3);
            gp_expect(gp_arr_length(arr) == 0);

            // Undefined, arr length is 0, don't do this!
            // gp_arr_pop(sizeof*arr, arr);
        }

        gp_test("Append, insert, and remove");
        {
            GPArray(int) arr = gp_arr_new(scope, sizeof*arr, 4);
            arr = gp_arr_append(sizeof*arr, arr, (int[]){1,2,3}, 3);
            arr_assert_eq(arr, ((int[]){1,2,3}), 3);
            arr = gp_arr_append(sizeof*arr, arr, (int[]){4,5,6}, 3);
            arr_assert_eq(arr, ((int[]){1,2,3,4,5,6}), 6);
            arr = gp_arr_insert(sizeof*arr, arr, 3, (int[]){0,0}, 2);
            arr_assert_eq(arr, ((int[]){1,2,3,0,0,4,5,6}), 8);
            arr = gp_arr_erase(sizeof*arr, arr, 3, 2);
            arr_assert_eq(arr, ((int[]){1,2,3,4,5,6}), 6);
        }

        gp_test("Null termination");
        {
            GPArray(char*) arr = gp_arr_new(scope, sizeof arr[0], 8);
            for (size_t i = 0; i < 8; ++i)
                arr = gp_arr_push(sizeof arr[0], arr, &"dummy string");
            arr = gp_arr_null_terminate(sizeof arr[0], arr);
            gp_expect(gp_arr_length(arr) == 8, "Null termination shouldn't change array length");
            gp_expect(arr[gp_arr_length(arr)] == NULL);
        }

        gp_test("Map, fold, foldr, filter");
        {
            void increment(void* out, const void* in);

            GPArray(int) arr  = gp_arr_on_stack(scope, 2, int);
            GPArray(int) arr2 = gp_arr_on_stack(scope, 4, int, 1, 2, 3, 4);
            arr = gp_arr_map(sizeof*arr, arr, arr2, gp_arr_length(arr2), increment);
            arr_assert_eq(arr, ((int[]){2, 3, 4, 5}), 4);

            arr = gp_arr_map(sizeof*arr, arr, NULL, 0, increment);
            arr_assert_eq(arr, ((int[]){3, 4, 5, 6}), 4);

            void* sum(void* accumulator, const void* element);
            gp_expect(*(int*)gp_arr_fold(sizeof*arr, arr, &(int){0}, sum)
                == 0 + 3 + 4 + 5 + 6);

            void* append(void* accumulator, const void* element);
            GPArray(const char*) arr_cstr = gp_arr_on_stack(scope, 4, const char*,
                "I", "am", "the", "Walrus");
            char* result = gp_arr_foldr(sizeof*arr_cstr, arr_cstr, NULL, append);
            gp_expect(strcmp(result, "Walrus the am I ") == 0, result);

            bool even(const void* element);
            arr2 = gp_arr_filter(sizeof*arr2, arr2, arr, gp_arr_length(arr), even);
            arr_assert_eq(arr2, ((int[]){4, 6}), 2);

            bool more_than_5(const void* element);
            arr2 = gp_arr_filter(sizeof*arr2, arr2, NULL, 0, more_than_5);
            arr_assert_eq(arr2, ((int[]){6}), 1);
        }
        gp_end(scope);
    }
}

void increment(void* out, const void* in) { *(int*)out = *(int*)in + 1; }
void* sum(void* y, const void* x) { *(int*)y += *(int*)x; return y; }
void* append(void* result, const void*_element)
{
    const char* element = *(const char**)_element;
    const size_t length = result != NULL ? strlen(result) : 0;
    result = gp_mem_realloc(
        gp_last_scope(), result, length, length + strlen(element) + sizeof" ");
    ((char*)result)[length] = '\0';
    return strcat(strcat(result, element), " ");
}
bool even(const void* element) { return !(*(int*)element % 2); }
bool more_than_5(const void* element) { return *(int*)element > 5; }
