// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/hashmap.c"
#include <gpc/assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct my_type
{
    char* data;
    size_t length;
    bool is_destroyed;
} MyType;

MyType create(const char* init)
{
    return (MyType){ strcpy(malloc(strlen(init) + 1), init), strlen(init), false };
}

// Really takes MyType* as argument but void* avoids awkward func pointer cast
void destroy(void*_obj)
{
    MyType* obj = _obj;
    free(obj->data);
    obj->is_destroyed = true;
}

int main(void)
{
    // Tiny arena to put address sanitizer to work
    GPArena* _arena = gp_arena_new(NULL, 1);
    _arena->growth_factor = 0.0; // TODO document this
    GPAllocator* arena = (GPAllocator*)_arena;

    gp_suite("Hash map");
    {
        gp_test("FNV_1a Hash");
        {
            // https://fnvhash.github.io/fnv-calculator-online/
            const char* str = "I am the Walrus.";
            gp_assert(gp_bytes_hash32(str,  strlen(str)) == 0x249f7959);
            gp_assert(gp_bytes_hash64(str,  strlen(str)) == 0x7a680bab8c51fa39);
            gp_assert(gp_uint128_equal(gp_bytes_hash128(str, strlen(str)), gp_uint128(0x67dc4bcbf73fe4e5, 0xb72b80a0168bcee1)));
        }

        const char* key = "This is my key!";
        gp_test("Pointer elements");
        {
            // Maps store pointers by default.
            GPHashMap* cstr_map = gp_hash_map_new(arena, NULL);

            gp_expect(gp_hash_map_get(cstr_map, key, strlen(key)) == NULL,
                "No elements yet");

            const char* cstr = "I am the element!";
            gp_hash_map_put(cstr_map, key, strlen(key), cstr);
            gp_expect(gp_hash_map_get(cstr_map, key, strlen(key)) == cstr);

            gp_hash_map_remove(cstr_map, key, strlen(key));
            gp_expect(gp_hash_map_get(cstr_map, key, strlen(key)) == NULL,
                "Element removed.");
        }

        gp_test("Integers as pointer elements");
        {
            GPHashMap* int_map = gp_hash_map_new(arena, NULL);

            gp_expect(gp_hash_map_get(int_map, key, strlen(key)) == NULL,
                "No elements yet");

            // Putting and getting integers as pointers by using casts can save
            // memory.
            gp_hash_map_put(int_map, key, strlen(key), (void*)7);
            gp_expect((intptr_t)gp_hash_map_get(int_map, key, strlen(key)) == 7);

            gp_hash_map_remove(int_map, key, strlen(key));

            // However, now the return value of get() can not be used to check
            // if element is removed. The element could just be set to 0.
            gp_expect(gp_hash_map_get(int_map, key, strlen(key)) == NULL,
                "Element removed??");
        }

        gp_test("Destructor");
        {
            // Since elements are pointers by default, free() is a valid
            // destructor.
            GPHashMap* cstr_map = gp_hash_map_new(
                arena, &(GPMapInitializer){.destructor = free });

            gp_expect(gp_hash_map_get(cstr_map, key, strlen(key)) == NULL,
                "No elements yet");

            const char cstr_init[] = "C-string on heap!";
            char* cstr = strcpy(malloc(sizeof cstr_init), cstr_init);
            gp_hash_map_put(cstr_map, key, strlen(key), cstr);
            gp_expect(gp_hash_map_get(cstr_map, key, strlen(key)) == cstr);

            gp_hash_map_remove(cstr_map, key, strlen(key));

            gp_expect(gp_hash_map_get(cstr_map, key, strlen(key)) == NULL,
                "Element removed.");

            // The hash maps in the tests above omitted cleanup since they were
            // allocated in the arena. Here, the objects themselves are pointers
            // to objects in heap so cleanup is essential to make sure that
            // free() is being called for all elements even when the map itself
            // is in arena.
            gp_hash_map_delete(cstr_map);
        }

        gp_test("Sized elements");
        {
            // Providing a size for elements tells the map to store the actual
            // elements in the map, not pointers to them. This is especially
            // useful with types that don't fit to sizeof(void*) so they do not
            // need a separate allocation.
            GPHashMap* map = gp_hash_map_new(
                arena, &(GPMapInitializer){
                    .element_size = sizeof(MyType), .destructor = destroy });

            const char* key1 = "Key to first object";
            const char* key2 = "Key to second object";
            // The scoping operator {} is used to emphasize that init_values are
            // copied to appropriate slot in map and are not being used after.
            {
                MyType init_values = create("Some stuff");
                gp_hash_map_put(map, key1, strlen(key1), &init_values);

                // Other way of storing elemets is to use the returned pointer.
                init_values = create("Some other stuff");
                MyType* elem = gp_hash_map_put(map, key2, strlen(key2), NULL);
                *elem = init_values;
            }
            MyType* obj1 = gp_hash_map_get(map, key1, strlen(key1));
            gp_expect(memcmp(obj1->data, "Some stuff", obj1->length) == 0);
            gp_expect( ! obj1->is_destroyed);
            MyType* obj2 = gp_hash_map_get(map, key2, strlen(key2));
            gp_expect(memcmp(obj2->data, "Some other stuff", obj2->length) == 0);
            gp_expect( ! obj2->is_destroyed);

            gp_hash_map_remove(map, key1, strlen(key1));
            gp_expect(gp_hash_map_get(map, key1, strlen(key1)) == NULL,
                "Element removed.");

            // Again, size given in map initializer means that the objects are
            // stored in the map so there is no use-after-free here. However,
            // the destructor has been called and any store using key1 will
            // of course overwrite whatever obj1 is pointing to.
            gp_expect(obj1->is_destroyed);

            gp_hash_map_delete(map);

            // No use after free here since the map is stored in the arena.
            // However, destructor for all objects have been run.
            gp_expect(obj2->is_destroyed);
        }
    }

    gp_suite("Non-hashed map");
    {
        GPMapInitializer init = {.destructor = free };
        GPMap* map = gp_map_new(gp_global_heap, &init);

        int* elem_25 = malloc(sizeof(int));
        int* elem_67 = malloc(sizeof(int));
        *elem_25 = 25;
        *elem_67 = 67;
        GPUInt128 key_25 = gp_uint128(0,0);
        GPUInt128 key_67 = gp_uint128(0,0);
        *gp_uint128_lo_addr(&key_25) = 3;
        *gp_uint128_hi_addr(&key_25) = 0;
        *gp_uint128_lo_addr(&key_67) = 3;
        *gp_uint128_hi_addr(&key_67) = 9;

        gp_map_put(map, key_25, elem_25);
        gp_map_put(map, key_67, elem_67);
        gp_expect(*(int*)gp_map_get(map, key_25) == *elem_25);
        gp_expect(*(int*)gp_map_get(map, key_67) == *elem_67);

        gp_map_delete(map);
    }

    // -------------------------------------------
    // Internal tests

    gp_suite("Pointers");
    {
        gp_test("Validity after collision");
        {
            GPMapInitializer init = {.element_size = sizeof(int) };
            GPMap* map = gp_map_new(gp_global_heap, &init);

            GPUInt128 key1 = gp_uint128(0,0);
            GPUInt128 key2 = gp_uint128(0,0);
            *gp_uint128_lo_addr(&key1) = 3;
            *gp_uint128_hi_addr(&key1) = 0;
            *gp_uint128_lo_addr(&key2) = 3;
            *gp_uint128_hi_addr(&key2) = 9;

            gp_map_put(map, key1, &(int){ 111 });
            int* elem1 = gp_map_get(map, key1);
            gp_expect(*elem1 == 111);

            gp_map_put(map, key2, &(int){ 222 });
            gp_expect(gp_map_get(map, key1) == elem1); // This failed in old impl

            gp_map_delete(map);
        }

        gp_test("Double free");
        {
            GPMapInitializer init = {.destructor = free };
            GPMap* map = gp_map_new(arena, &init);

            GPUInt128 key1 = gp_uint128(0,0);
            GPUInt128 key2 = gp_uint128(0,0);
            *gp_uint128_lo_addr(&key1) = 3;
            *gp_uint128_hi_addr(&key1) = 0;
            *gp_uint128_lo_addr(&key2) = 3;
            *gp_uint128_hi_addr(&key2) = 9;

            gp_map_put(map, key1, malloc(sizeof(int)));
            gp_map_put(map, key2, malloc(sizeof(int)));

            gp_map_remove(map, key1); // free once
            gp_map_delete(map);       // are we freeing again?
        }
    }
    #if __GNUC__ && __SIZEOF_INT128__
    gp_suite("Uint128");
    {
        gp_test("Shift key");
        {
            GPUInt128 x = gp_uint128(59318, 86453012);
            size_t shift = 8;
            gp_expect(gp_s_shift_key(x, shift).u128 == x.u128 >> 3);
        }
    }
    #endif

    gp_arena_delete(_arena);
}
