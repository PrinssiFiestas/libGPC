// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include <gpc/io.h>
#include <gpc/array.h> // note: source included after the tests so we can get access to shadowing macros

#define arr_assert_eq(ARR, CARR, CARR_LENGTH, ...) do { \
    gp_expect(gp_arr_length(ARR) == (CARR_LENGTH)); \
    for (size_t _gp_i = 0; _gp_i < (CARR_LENGTH); ++_gp_i) \
        if (!gp_expect((ARR)[_gp_i] == (CARR)[_gp_i]__VA_OPT__(,) __VA_ARGS__)) \
            printf("i = %zu\n", _gp_i); \
} while (0)

#define CARR_LEN(CARR) (sizeof(CARR) / sizeof*(CARR))

#ifndef GP_NO_TYPE_SAFE_MACRO_SHADOWING
#define FPTR2PTR(...) __VA_ARGS__
#else
#define FPTR2PTR(...) GP_FPTR_TO_VOIDPTR(__VA_ARGS__)
#endif

int main(void)
{
    gp_suite("Memory");
    {
        gp_test("Arrays on stack");
        {
            // Create array that can hold 4 elements
            GPArrayBuffer(int, 4) buffer;
            // Passing an allocator makes the array reallocateable
            GPArray(int) arr = gp_arr_buffered(int, gp_global_heap, &buffer);

            gp_expect(gp_arr_allocation(arr) == NULL,
                "Stack allocated arrays should not have a allocation.");

            GPArrayBuffer(int, 8) buffer2;
            GPArray(int) arr2 = gp_arr_buffered(int, NULL, &buffer2, 1, 2, 3, 4, 5, 6, 7, 8);
            gp_expect(gp_arr_length(arr2) == 8);
            gp_expect(
                !memcmp(arr2, (int[]){ 1, 2, 3, 4, 5, 6, 7, 8 }, 8*sizeof(int)));

            // Copying past capacity is safe; arr uses gp_global_heap to reallocate.
            gp_arr_copy(sizeof*arr, &arr, arr2, gp_arr_length(arr2));
            arr_assert_eq(arr, arr2, gp_arr_length(arr2));

            // First array is on gp_global_heap, don't forget to deallocate!
            gp_arr_delete(arr);

            // The second array is on stack and no allocator provided so no need
            // to delete, but it is safe to do it anyway.
            gp_arr_delete(arr2);
        }

        gp_test("Arrays on arenas/scopes");
        {
            // Here using the scope allocator, but everything in this example
            // applies to GPArena as well.

            // Note: arrays and arenas have metadata so an arena with size
            // 256 * sizeof(int) is NOT capable of holding an array with 256 ints.
            GPScope* scope = gp_begin(256 * sizeof(int));

            const size_t INIT_CAPACITY = 8;
            const size_t RESERVE_CAPACITY = INIT_CAPACITY + 1;
            GPArray(int) arr = gp_arr_new(sizeof(int), &scope->base, INIT_CAPACITY);
            const int* init_pos = arr;
            gp_expect(gp_arr_capacity(arr) == INIT_CAPACITY);
            gp_arr_reserve(sizeof*arr, &arr, RESERVE_CAPACITY); // Extend arr memory
            gp_expect(gp_arr_capacity(arr) > INIT_CAPACITY
                && arr == init_pos,"Arenas should know how to extend memory of "
                                   "lastly created objects so arr is not moved.");

            void* new_object = gp_mem_alloc(&scope->base, 1); (void)new_object;
            gp_arr_reserve(sizeof*arr, &arr, 32);
            gp_expect(arr != init_pos,
                "arr can nott extend since it would overwrite new_object.");
            const int*const new_pos = arr;
            gp_arr_reserve(sizeof*arr, &arr, 64);
            gp_expect(arr == new_pos,
                "After reallocation arr is last element so it can be extended.");

            gp_arr_reserve(sizeof*arr, &arr, 256);
            gp_expect(arr != new_pos,
                "arr did not fit in arena so it should've been reallocated.");

            // No need to delete arr or dealloc new_object, they live in scope.
            gp_end(scope);

            // Repeated memory block extension test on virtual arena
            GPContiguousArena* ca = gp_carena_new(4*4096);
            arr = gp_arr_new(sizeof arr[0], &ca->base, INIT_CAPACITY);
            init_pos = arr;
            gp_expect(gp_arr_capacity(arr) == INIT_CAPACITY);
            gp_arr_reserve(sizeof arr[0], &arr, RESERVE_CAPACITY); // Extend arr memory
            gp_expect(gp_arr_capacity(arr) > INIT_CAPACITY
                && arr == init_pos,"Arenas should know how to extend memory of "
                                   "lastly created objects so arr is not moved.");
            gp_carena_delete(ca);
        }
    } // gp_suite("Memory");

    gp_suite("Array manipulation");
    {
        GPScope* scope = gp_begin(0);

        gp_test("Copy slice");
        {
            GPArrayBuffer(int, 64) buffer;
            GPArray(int) arr = gp_arr_buffered(int, NULL, &buffer);
            const int carr[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

            // Always reassign destination array to self in case of reallocation!
            gp_arr_slice(sizeof*arr, &arr, carr, 1, 6);
            const int carr2[] = { 1, 2, 3, 4, 5 };
            arr_assert_eq(arr, carr2, CARR_LEN(carr2));
        }

        gp_test("Mutating slice");
        {
            GPArrayBuffer(int, 64) buffer;
            GPArray(int) arr = gp_arr_buffered(int, NULL, &buffer);
            gp_arr_copy(sizeof(int), &arr, ((int[]){ 0, 1, 2, 3, 4, 5 }), 5);
            gp_arr_slice(sizeof*arr, &arr, NULL, 2, 5);
            const int carr[] = { 2, 3, 4 };
            arr_assert_eq(arr, carr, CARR_LEN(carr));
        }

        gp_test("Push and pop");
        {
            GPArray(int) arr = gp_arr_new(sizeof(int), &scope->base, 4);
            gp_arr_push(sizeof*arr, &arr, &(int){3});
            gp_arr_push(sizeof*arr, &arr, &(int){6});
            gp_expect(arr[0] == 3);
            gp_expect(arr[1] == 6);
            gp_expect(gp_arr_length(arr) == 2);
            gp_expect(*(int*)gp_arr_pop(sizeof(int), &arr) == 6);
            gp_expect(*(int*)gp_arr_pop(sizeof(int), &arr) == 3);
            gp_expect(gp_arr_length(arr) == 0);

            // Undefined, arr length is 0, don't do this!
            // gp_arr_pop(sizeof*arr, arr);
        }

        gp_test("Append, insert, and remove");
        {
            // Note that type safe macro aliasing means that parenthesis are
            // required around compound literals.

            GPArray(int) arr = gp_arr_new(sizeof(int), &scope->base, 4);
            gp_arr_append(sizeof*arr, &arr, ((int[]){1,2,3}), 3);
            arr_assert_eq(arr, ((int[]){1,2,3}), 3);
            gp_arr_append(sizeof*arr, &arr, ((int[]){4,5,6}), 3);
            arr_assert_eq(arr, ((int[]){1,2,3,4,5,6}), 6);
            gp_arr_insert(sizeof*arr, &arr, 3, ((int[]){0,0}), 2);
            arr_assert_eq(arr, ((int[]){1,2,3,0,0,4,5,6}), 8);
            gp_arr_erase(sizeof*arr, &arr, 3, 2);
            arr_assert_eq(arr, ((int[]){1,2,3,4,5,6}), 6);
        }

        gp_test("Null termination");
        {
            GPArray(char*) arr = gp_arr_new(sizeof(char*), &scope->base, 8);
            for (size_t i = 0; i < 8; ++i)
                gp_arr_push(sizeof arr[0], &arr, &(char*){"dummy string"});
            gp_arr_null_terminate(sizeof arr[0], &arr);
            gp_expect(gp_arr_length(arr) == 8, "Null termination shouldn't change array length");
            gp_expect(arr[gp_arr_length(arr)] == NULL);
        }

        gp_test("Map, fold, foldr, filter");
        {
            void increment(int* out, const int* in);

            GPArrayBuffer(int, 2) arr_buf;
            GPArrayBuffer(int, 4) arr2_buf;
            GPArray(int) arr  = gp_arr_buffered(int, &scope->base, &arr_buf);
            GPArray(int) arr2 = gp_arr_buffered(int, &scope->base, &arr2_buf, 1, 2, 3, 4);
            gp_arr_map(sizeof arr[0], &arr, arr2, gp_arr_length(arr2), FPTR2PTR(increment));
            arr_assert_eq(arr, ((int[]){2, 3, 4, 5}), 4);

            gp_arr_map(sizeof arr[0], &arr, NULL, 0, FPTR2PTR(increment));
            arr_assert_eq(arr, ((int[]){3, 4, 5, 6}), 4);

            void* sum(void* accumulator, const void* element);
            gp_expect(*(int*)gp_arr_fold(sizeof arr[0], arr, &(int){0}, FPTR2PTR(sum))
                == 0 + 3 + 4 + 5 + 6);

            void* append(void* accumulator, const void* element);
            GPArrayBuffer(const char*, 4) cstr_buffer;
            GPArray(const char*) arr_cstr = gp_arr_buffered(const char*, &scope->base, &cstr_buffer,
                "I", "am", "the", "Walrus");
            char* result = gp_arr_foldr(sizeof*arr_cstr, arr_cstr, NULL, FPTR2PTR(append));
            gp_expect(strcmp(result, "Walrus the am I ") == 0, result);

            bool even(const int* element);
            gp_arr_filter(sizeof arr2[0], &arr2, arr, gp_arr_length(arr), FPTR2PTR(even));
            arr_assert_eq(arr2, ((int[]){4, 6}), 2);

            bool more_than_5(const void* element);
            gp_arr_filter(sizeof arr2[0], &arr2, NULL, 0, FPTR2PTR(more_than_5));
            arr_assert_eq(arr2, ((int[]){6}), 1);
        }
        gp_end((GPScope*)scope);
    } // gp_suite("Array manipulation");

    gp_suite("Truncating Arrays");
    {
        gp_test("Truncation");
        {
            GPArrayBuffer(int, 4) buf;
            // NULL allocator makes array truncating.
            GPArray(int) arr = gp_arr_buffered(int, NULL, &buf);
            int* arr_ptr = arr;

            gp_expect(gp_arr_reserve(sizeof(int), &arr, 4) == 0, "No reallocation needed.");
            gp_expect(gp_arr_reserve(sizeof(int), &arr, 7) == 7-4,
                "Cannot reallocate truncating array, the difference of requested "
                "capacity and actual capacity returned.");
            gp_expect(arr == arr_ptr, "Truncating array will never reallocate.");

            gp_expect(gp_arr_copy(sizeof(int), &arr, ((int[]){1, 2}), 2) == 0);
            arr_assert_eq(arr, ((int[]){1, 2}), 2);
            gp_expect(gp_arr_copy(sizeof(int), &arr, ((int[]){4, 5, 6, 7, 8, 9, 10}), 7) == 7-4);
            arr_assert_eq(arr, ((int[]){4, 5, 6, 7}), 4);

            gp_expect(gp_arr_slice(sizeof(int), &arr, ((int[]){0, 1, 2, 3, 4, 5, 6, 7}), 1, 6) == (6-1)-4);
            arr_assert_eq(arr, ((int[]){1, 2, 3, 4}), 4);

            gp_arr_set(arr)->length--;
            gp_expect(gp_arr_push(sizeof(int), &arr, &(int){1}) == false);
            arr_assert_eq(arr, ((int[]){1, 2, 3, 1}), 4);
            gp_expect(gp_arr_push(sizeof(int), &arr, &(int){9}) == true);
            arr_assert_eq(arr, ((int[]){1, 2, 3, 1}), 4);

            gp_arr_set(arr)->length = 1;
            gp_expect(gp_arr_append(sizeof(int), &arr, ((int[]){1, 2}), 2) == 0);
            arr_assert_eq(arr, ((int[]){1, 1, 2}), 3);
            gp_expect(gp_arr_append(sizeof(int), &arr, ((int[]){1, 2, 3, 4, 5}), 5) == 3+5-4);
            arr_assert_eq(arr, ((int[]){1, 1, 2, 1}), 4);

            gp_arr_set(arr)->length = 3;
            gp_expect(gp_arr_insert(sizeof(int), &arr, 2, ((int[]){3, 3, 3}), 3) == 3+3-4);
            gp_println(gp_arr_length(arr));
            arr_assert_eq(arr, ((int[]){1, 1, 3, 3}), 4);

            gp_expect(gp_arr_null_terminate(sizeof(int), &arr) == NULL);

            void increment(int* out, const int* in);
            gp_expect(gp_arr_map(sizeof(int), &arr, ((int[]){5, 4, 3, 2, 1, 0}), 6, FPTR2PTR(increment)) == 6-4);
            arr_assert_eq(arr, ((int[]){6, 5, 4, 3}), 4);

            gp_expect(arr == arr_ptr, "Again, a truncating array will never reallocate.");
        }
    } // gp_suite("Truncating Arrays");
}

void increment(int* out, const int* in) { *out = *in + 1; }
void* sum(void* y, const void* x) { *(int*)y += *(int*)x; return y; }
void* append(void* result, const void*_element)
{
    const char* element = *(const char**)_element;
    const size_t length = result != NULL ? strlen(result) : 0;
    result = gp_mem_reserve(
        &gp_last_scope()->base, result, length, length + strlen(element) + sizeof" ");
    ((char*)result)[length] = '\0';
    return strcat(strcat(result, element), " ");
}
bool even(const int* element) { return !(*element % 2); }
bool more_than_5(const void* element) { return *(int*)element > 5; }

#include "../src/array.c"
