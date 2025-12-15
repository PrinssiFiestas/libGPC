// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/hashmap.c"
#include <gpc/array.h>
#include <gpc/assert.h>
#include <errno.h>

int main(void)
{
    gp_suite("Hash map");
    {
        GPMap        map = gp_map_new(sizeof(int), gp_global_heap, 0x10);
        const char* key1 = "key1";
        const char* key2 = "key2";
        uint64_t   hash1 = gp_bytes_hash(key1, strlen(key1));
        uint64_t   hash2 = gp_bytes_hash(key2, strlen(key2));
        int       value1 = 1;
        int       value2 = 2;
        int       value3 = 3;
        int*     bucket1 = NULL; // pointer to stored element in map
        int*     bucket2 = NULL; // pointer to stored element in map
        int*     bucket3 = NULL; // pointer to stored element in map

        gp_test("Put and get");
        {
            // String keys and hashes can be used interchangeably.
            bucket1 = gp_map_put(&map, key1, strlen(key1), &value1);
            gp_expect(gp_map_get(map, NULL, hash1) == bucket1);
            gp_expect(*bucket1 == value1);
            bucket2 = gp_map_put(&map, NULL, hash2, &value2);
            gp_expect(gp_map_get(map, key2, strlen(key2)) == bucket2);
            gp_expect(*bucket2 == value2);
        }

        gp_test("Iteration");
        {
            // GPMap is unordered, you shouldn't rely on the exact order.
            GPMapIterator it = gp_map_begin(map);
            if (it.value == bucket1) {
                gp_expect(it.value == bucket1);
                it = gp_map_next(it);
                gp_expect(it.value == bucket2);
            } else {
                gp_expect(it.value == bucket2);
                it = gp_map_next(it);
                gp_expect(it.value == bucket1);
            }
            it = gp_map_next(it);
            gp_expect(it.value == NULL);
        }

        gp_test("Removal");
        {
            gp_expect(gp_map_get(map, NULL, hash1) == bucket1);
            gp_expect(gp_map_remove(&map, NULL, hash1) == bucket1);
            gp_expect(gp_map_get(map, NULL, hash1) == NULL);
            gp_expect(gp_map_get(map, NULL, hash2) == bucket2);
            gp_expect(gp_map_remove(&map, NULL, hash2) == bucket2);
            gp_expect(gp_map_get(map, NULL, hash2) == NULL);
            gp_expect(gp_map_remove(&map, NULL, hash1) == NULL);
            gp_expect(gp_map_remove(&map, NULL, hash2) == NULL);
            gp_expect(gp_map_begin(map).value == NULL);
        }

        gp_test("Hard coded hashes");
        {
            // The exact values of these hashes are used to test internals. This
            // also demonstrates that you can use whatever hashing function you
            // like, as long as it never returns zero and the values are
            // statistically unique enough.

            bucket1 = gp_map_put(&map, NULL, 0x33, &value1);
            bucket2 = gp_map_put(&map, NULL, 0x03, &value2);
            gp_expect(bucket1 != bucket2);
            gp_expect(bucket1 == gp_map_get(map, NULL, 0x33));
            gp_expect(bucket2 == gp_map_get(map, NULL, 0x03));
            gp_expect(*bucket1 == value1);
            gp_expect(*bucket2 == value2);

            bucket3 = gp_map_put(&map, NULL, 0x103, &value3);
            gp_expect(bucket1 == gp_map_get(map, NULL, 0x33));
            gp_expect(bucket2 == gp_map_get(map, NULL, 0x03));
            gp_expect(bucket3 == gp_map_get(map, NULL, 0x103));
            gp_expect(*bucket1 == value1);
            gp_expect(*bucket2 == value2);
            gp_expect(*bucket3 == value3);

            // Again, don't rely on exact order, we just do this because we know
            // the how the internals work and it's convenient.
            GPMapIterator it = gp_map_begin(map);
            gp_expect(it.value == bucket3, *(int*)it.value);
            it = gp_map_next(it);
            gp_expect(it.value == bucket2, *(int*)it.value);
            it = gp_map_next(it);
            gp_expect(it.value == bucket1, *(int*)it.value);
            it = gp_map_next(it);
            gp_expect(it.value == NULL);

            gp_expect(gp_map_remove(&map, NULL, 0x03) != NULL);
            gp_expect(gp_map_get(map, NULL, 0x03)     == NULL);
            gp_expect(gp_map_remove(&map, NULL, 0x03) == NULL);
            gp_expect(gp_map_remove(&map, NULL, 0x33) != NULL);
            gp_expect(gp_map_get(map, NULL, 0x33)     == NULL);
            gp_expect(gp_map_remove(&map, NULL, 0x33) == NULL);
            gp_expect(gp_map_remove(&map, NULL, 0x103) != NULL);
            gp_expect(gp_map_get(map, NULL, 0x103)     == NULL);
            gp_expect(gp_map_remove(&map, NULL, 0x103) == NULL);
        }

        gp_test("Full depth");
        {
            // Any halfway decent hash function will not have 60 colliding bits
            // for 16 values in gazillion years, but something custom like using
            // pointer values might. This test tests some related edge cases.

            for (uint64_t i = 0; i < 16; ++i)
                gp_map_put(&map, NULL, 0x0555555555555555 | (i<<60), &(int){i});

            for (uint64_t i = 0; i < 16; ++i)
                gp_assert(
                    *(int*)gp_map_get(map, NULL, 0x0555555555555555 | (i<<60)) == (int)i,
                    i);

            bool found[16] = {false};
            GPMapIterator it = gp_map_begin(map);
            for (uint64_t i = 0; i < 16; it = gp_map_next(it), ++i)
                found[*(int*)it.value] = true;
            gp_expect(it.value == NULL);
            for (uint64_t i = 0; i < 16; ++i)
                gp_assert(found[i], i);

            for (uint64_t i = 0; i < 16; ++i)
                gp_assert(
                    gp_map_remove(&map, NULL, 0x0555555555555555 | (i<<60)), i);
            gp_expect(gp_map_begin(map).value == NULL);
        }
        gp_map_delete(map);

        gp_test("Fuzzing");
        {
            time_t t = time(NULL);
            struct tm* tm = gmtime(&t);
            gp_assert(tm != NULL, strerror(errno));
            GPRandomState rs = gp_random_state_seed(tm->tm_yday, tm->tm_year);

            // Current implementation's max is internally limited to 0x10-0x4000.
            size_t init_cap   = gp_random_bound(&rs, 0x4100);
            size_t iterations = gp_random_range(&rs, 0x1000, 0x10000);

            typedef struct key_val {
                uint64_t key;
                int      val;
            } KeyVal;
            GPArray(KeyVal) key_vals = gp_arr_new(
                sizeof key_vals[0], gp_global_heap, iterations);
            map = gp_map_new(sizeof(int), gp_global_heap, init_cap);

            // Fill elements randomly removing a random element in between.
            for (size_t i = 0; i < iterations; ++i) {
                if (gp_random_bound(&rs, 8) != 0) {
                    KeyVal kv = {.val = i, .key = gp_bytes_hash(&i, sizeof i) };
                    gp_arr_push(sizeof key_vals[0], &key_vals, &kv);
                    gp_map_put(&map, NULL, kv.key, &kv.val);
                } else {
                    size_t j = gp_random_bound(&rs, gp_arr_length(key_vals));
                    bool removed = gp_map_remove(&map, NULL, key_vals[j].key);
                    gp_assert(removed, i, j, key_vals[j].key, key_vals[j].val);
                    gp_arr_erase(sizeof key_vals[0], &key_vals, j, 1);
                }
            }

            // Check matches
            for (size_t i = 0; i < gp_arr_length(key_vals); ++i) {
                int* p = gp_map_get(map, NULL, key_vals[i].key);
                gp_assert(p != NULL, i, key_vals[i].key, key_vals[i].val);
                gp_assert(*p == key_vals[i].val, i, key_vals[i].key, key_vals[i].val);
            }

            // Check matches using iterator
            size_t length = 0;
            for (GPMapIterator it = gp_map_begin(map); it.value != NULL; it = gp_map_next(it))
            {
                size_t i = 0;
                for (; i < gp_arr_length(key_vals); ++i)
                    if (*(int*)it.value == key_vals[i].val)
                        break;
                gp_assert(
                    i < gp_arr_length(key_vals),
                    "Value not found.",
                    *(int*)it.value);
                ++length;
            }
            gp_expect(length == gp_arr_length(key_vals), length, gp_arr_length(key_vals));

            // Remove all values
            for (size_t i = 0; i < gp_arr_length(key_vals); ++i)
                gp_assert(
                    gp_map_remove(&map, NULL, key_vals[i].key),
                        i, key_vals[i].key, key_vals[i].val);
            gp_expect(gp_map_begin(map).value == NULL);

            gp_arr_delete(key_vals);
            gp_map_delete(map);
        }
    } // gp_suite("Hash map");

    gp_suite("Hashing");
    {
        gp_test("FNV_1a Hash");
        {
            // https://fnvhash.github.io/fnv-calculator-online/
            const char* str = "I am the Walrus.";
            gp_assert(gp_bytes_hash32(str,  strlen(str)) == 0x249f7959);
            gp_assert(gp_bytes_hash64(str,  strlen(str)) == 0x7a680bab8c51fa39);
            gp_assert(gp_uint128_equal(
                gp_bytes_hash128(str, strlen(str)),
                gp_uint128(0x67dc4bcbf73fe4e5, 0xb72b80a0168bcee1)));
        }
    }
}
