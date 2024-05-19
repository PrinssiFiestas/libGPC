// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/hashmap.c"
#include <gpc/assert.h>
#include <stdlib.h>

int main(void)
{
    gp_suite("Hash map");
    {
        GPHashMap* map = gp_hash_map_new(
            &gp_heap, &(GPMapInitializer){ sizeof(int) });

        const char* key = "This is my key!";
        gp_expect(gp_hash_map_get(map, key, strlen(key)) == NULL,
            "No elements yet");

        gp_hash_map_set(map, key, strlen(key), &(int){3});
        gp_expect(*(int*)gp_hash_map_get(map, key, strlen(key)) == 3);

        gp_hash_map_remove(map, key, strlen(key));
        gp_expect(gp_hash_map_get(map, key, strlen(key)) == NULL,
            "Element removed!");

        gp_hash_map_delete(map);
    }

    gp_suite("Non-hashed map");
    {
        GPMapInitializer init = {.destructor = free };
        GPMap* map = gp_map_new(&gp_heap, &init);

        int* elem_25 = malloc(sizeof(int));
        int* elem_67 = malloc(sizeof(int));
        *elem_25 = 25;
        *elem_67 = 67;
        GPUint128 key_25 = {0};
        GPUint128 key_67 = {0};
        *gp_u128_lo(&key_25) = 3;
        *gp_u128_hi(&key_25) = 0;
        *gp_u128_lo(&key_67) = 3;
        *gp_u128_hi(&key_67) = 9;

        gp_map_set(map, key_25, elem_25);
        gp_map_set(map, key_67, elem_67);
        gp_expect(*(int*)gp_map_get(map, key_25) == *elem_25);
        gp_expect(*(int*)gp_map_get(map, key_67) == *elem_67);

        gp_map_delete(map);
    }

    // -------------------------------------------
    // Internal tests

    #if __GNUC__
    gp_suite("Uint128");
    {
        gp_test("Shift key");
        {
            GPUint128 x  = {{.hi = 59318, .lo = 86453012}};
            size_t shift = 8;
            gp_expect(gp_shift_key(x, shift).u128 == x.u128 >> 3);
        }
    }
    #endif
}
