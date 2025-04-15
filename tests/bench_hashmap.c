// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// TODO reference keys are 64 bits, our's are 128 bits, implement and try 64 as well.

// TODO get rid of hashing, why would we measure that? Implement other TODOs first though.

#define GPC_IMPLEMENTATION
#include "../build/gpc.h"
#include <x86intrin.h>
#include "reference_hashmap.h"

#define MEASURE(RESULT, FUNC) do { \
    T t_rdtsc = __rdtsc(); \
      t_rdtsc = __rdtsc() - t_rdtsc; \
    T t0 = __rdtsc(); \
    FUNC; \
    T t1 = __rdtsc(); \
    RESULT += t1 - t0 - t_rdtsc; \
} while (0)

typedef volatile uint64_t T;

size_t hash(const char* key)
{
    if (sizeof(size_t) == sizeof(uint64_t))
        return gp_bytes_hash64(key, strlen(key));
    return gp_bytes_hash32(key, strlen(key));
}

int main(int argc, char** argv)
{
    size_t elements_length = 2048;
    if (argc >= 2)
        elements_length = atoi(argv[1]);
    uint64_t seed = time(NULL);
    if (argc >= 3)
        seed = gp_bytes_hash64(argv[2], strlen(argv[2]));

    gp_println("Number of elements:", elements_length, "\nSeed:", seed);

    // We mostly care about small strings
    const size_t KEY_SIZE = 7 + sizeof"";

    GPRandomState rs = gp_new_random_state(seed);
    char* keys = gp_alloc(gp_scratch_arena(), (KEY_SIZE + 1) * elements_length); // +1 just in case
    uint32_t values = gp_new(gp_scratch_arena(), uint32_t, elements_length);
    for (size_t i = 0; i < elements_length * KEY_SIZE / sizeof(uint32_t); ++i)
        ((uint32_t*)keys)[i] = gp_random(&rs);
    for (size_t i = 0; i < elements_length + 1; ++i)
        keys[i * KEY_SIZE - sizeof""] = '\0';
    for (size_t i = 0; i < elements_length; ++i)
        values[i] = gp_random(&rs);

    #if 1 // use arena, possibly unfair
    GPDictionary(uint32_t) dict_gp = gp_dict(gp_scratch_arena(), uint32_t);
    #else // use heap
    GPDictionary(uint32_t) dict_gp = gp_dict(gp_heap, uint32_t);
    #endif
    HASHMAP(char, uint32_t) dict_reference;
    hashmap_init(&dict_reference, hash, strcmp);

    // ------------------------------------------------------------------------
    // Measurements and validation

    T gp_put_time = 0;
    MEASURE(gp_put_time,
        for (size_t i = 0; i < elements_length; ++i)
            gp_put(&dict_gp, keys + i * KEY_SIZE, strlen(keys + i * KEY_SIZE), values[i]);
    );

    T reference_put_time = 0;
    MEASURE(reference_put_time,
        for (size_t i = 0; i < elements_length; ++i)
            hashmap_put(&dict_reference, keys + i * KEY_SIZE, values + i);
    );

    T gp_get_time = 0;
    T reference_get_time = 0;
    for (size_t i = 0; i < elements_length; ++i) {
        uint32_t gp_value;
        uint32_t reference_value;
        MEASURE(gp_get_time,
            gp_value = *gp_get(dict_gp, keys + i * KEY_SIZE, strlen(keys + i * KEY_SIZE)));
        MEASURE(reference_get_time,
            reference_value = *hashmap_get(&dict_reference, keys + i * KEY_SIZE));

        // Use fetched values to validate and to prevent compiler from optimizing out
        gp_assert(gp_value == reference_value);
    }

    // ------------------------------------------------------------------------
    // Results

    double put_ratio = (double)gp_put_time/reference_put_time;
    double get_ratio = (double)gp_get_time/reference_get_time;

    gp_println(
        "\nGPDictionary put time:", gp_put_time,
        "\nReference put time:", reference_put_time,
        "\nRatio:", put_ratio,
        "\n"
        "\nGPDictionary get time:", gp_get_time,
        "\nReference get time:", reference_get_time,
        "\nRatio:", get_ratio,
        "\n"
        "\nGPDictionary total time:", gp_put_time + gp_get_time,
        "\nReference total time:", reference_put_time + reference_get_time,
        "\nRatio:", (double)(gp_put_time + gp_get_time) / (reference_put_time + reference_get_time),
        "\nGeometric mean:", sqrt(put_ratio * get_ratio));
    hashmap_cleanup(&dict_reference);
    gp_dict_delete(dict_gp);
}

#include "reference_hashmap.c"
