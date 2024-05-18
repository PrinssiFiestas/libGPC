// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/hashmap.c"
#include <gpc/assert.h>

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
        // TODO just test that this matches hash map for documentation.
        // Then, test the internals more troughoutly.
    }

    // ------------------------------------------------------------------------
    // TODO internal tests for 128 bit stuff
}
