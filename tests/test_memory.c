// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include "../src/memory.c"
#include <pthread.h>

#if NDEBUG // Memory tests require functionality only available in debug mode
int main(void) { return 0; }
#else

static bool is_free(void*_ptr)
{
    uint8_t* ptr = _ptr;
    for (size_t i = 0; i < GP_ALLOC_ALIGNMENT; i++) if (ptr[i] != 0xFF)
        return false;
    return true;
}

void* test1(void*_)
{
    (void)_;
    void* ps[8] = {}; // Dummy objects
    gp_suite("Scope allocator");
    { // The scopes created by the scope allocator are not in any way tied to
      // the C scoping operator {}. Here we use them just to empahize that
      // scopes are nonetheless lexical.
        gp_test("Basic usage");
        {
            // Use tiny 1 byte scopes to first: demostrate that the arenas can
            // hold objects that do not fit in them, and second: force all
            // pointers to point into their own memory space so the testing
            // allocator can mark them as freed. Normally you would use larger
            // values or 0 for some default value.
            GPAllocator* scope = gp_begin(1);
            ps[0] = gp_mem_alloc(scope, 8);
            {
                GPAllocator* scope = gp_begin(1);
                ps[1] = gp_mem_alloc(scope, 8);
                {
                    GPAllocator* scope = gp_begin(1);
                    ps[2] = gp_mem_alloc(scope, 8);
                    // whoops, forgot to end_scope()
                }
                {
                    GPAllocator* scope = gp_begin(1);
                    ps[3] = gp_mem_alloc(scope, 8);
                    gp_end(scope);
                }
                {
                    GPAllocator* scope = gp_begin(1);
                    ps[4] = gp_mem_alloc(scope, 8);
                    // Sanity check
                    gp_expect( ! is_free(ps[0]));
                    gp_expect( ! is_free(ps[1]));
                    gp_expect( ! is_free(ps[2]));
                    gp_expect(   is_free(ps[3]));
                    gp_expect( ! is_free(ps[4]));
                    // whoops, forgot to end_scope()
                }
                gp_end(scope);
            }
            // We can allocate as much as we like, gp_end(scope) will free all.
            ps[5] = gp_mem_alloc(scope, 8);
            ps[6] = gp_mem_alloc(scope, 8);
            ps[7] = gp_mem_alloc(scope, 8);

            gp_end(scope); // this also ends the forgotten inner scopes.

            for (size_t i = 0; i < sizeof ps / sizeof*ps; i++)
                gp_expect(is_free(ps[i]));
        }
        // TODO how ignoring scopes being lexically scoped will fail and mention GPArena
    }
    return NULL;
}

void* test2(void*_)
{
    (void)_;
    gp_suite("Thread cleaning it's scopes");
    return NULL;
}

void* test3(void*_)
{
    (void)_;
    gp_suite("Arena allocator");
    return NULL;
}

// Testing allocator
// Check the code below main() for definitions and how to write a custom allocator.
const GPAllocator* new_test_allocator(void);
void delete_test_allocator(const GPAllocator*);

int main(void)
{
    // Override malloc() based allocator for testing purposes. This affects all
    // heap allocations in the library and is only possible if NDEBUG is defined.
    gp_heap = new_test_allocator();

    pthread_t tests[3];
    pthread_create(&tests[0], NULL, test1, NULL);
    //pthread_create(&tests[1], NULL, test2, NULL);
    //pthread_create(&tests[2], NULL, test3, NULL);

    pthread_join(tests[0], NULL);
    // pthread_join(tests[1], NULL);
    // pthread_join(tests[2], NULL);

    // Make Valgrind shut up.
    delete_test_allocator(gp_heap);
}

// ----------------------------------------------------------------------------
// Custom allocators

// Every allocator is REQUIRED to have alloc() and dealloc() where alloc()
// should NOT return NULL in ANY CIRCUMSTANCE. Not being able to return NULL may
// seem like a big limitation, but it makes reasoning about NULL massively
// simpler. All pointers returned by alloc() also MUST be aligned to
// GP_ALLOC_ALIGNMENT boundary. dealloc() is REQUIRED to handle NULL arguments.
static void* test_alloc(const GPAllocator*, size_t) GP_NONNULL_ARGS_AND_RETURN;
static void  test_dealloc(const GPAllocator* optional, void* optional_block);

// While struct gp_allocator could be used by itself, your allocator probably
// needs some data of its own. We are going to write an arena-like allocator.
typedef struct test_allocator
{
    // Inherit from GPAllocator by making the FIRST member a GPAllocator. This
    // allows safe and well defined upcasting.
    GPAllocator allocator;

    // This will be returned by test_alloc().
    void* free_block;
} TestAllocator;

// Since test_allocator inherits from GPAllocator, it could be used like so:
/*
    // Upcasting to GPAllocator* is safe; test_allocator inherits from it.
    void* my_data = gp_mem_alloc((GPAllocator*)&test_allocator, BLOCK_SIZE);
    // do something with my_data
    gp_mem_dealloc((GPAllocator*)&test_allocator, my_data);
*/
// However, in this case a constructor is needed. By returning a GPAllocator*
// instead of TestAllocator*, we are also privatizing TestAllocator* specific
// functionality while also removing the need for ugly upcasts from user.
const GPAllocator* new_test_allocator(void)
{
    // Create an arena with capacity of 100 KB
    TestAllocator* allocator = calloc(100, 1 << 10);
    gp_assert(allocator != NULL);
    *allocator = (TestAllocator) {
        .allocator  = {.alloc = test_alloc, .dealloc = test_dealloc },
        .free_block = (uint8_t*)allocator + gp_round_to_aligned(sizeof*allocator)
    };
    return (const GPAllocator*)allocator;
}

// Since we are overriding gp_heap and tests are threaded, a mutex is necessary.
static pthread_mutex_t test_allocator_mutex = PTHREAD_MUTEX_INITIALIZER;

// Normally, arena running out of memory should be handled somehow. GPArena and
// the scope allocator does this by creating new arenas where the size is
// guranteed to fit the size argument of alloc(). Here we will keep things
// simple for testing purposes and omit handling out of memory case.
static void* test_alloc(const GPAllocator*_allocator, size_t size)
{
    pthread_mutex_lock(&test_allocator_mutex);

    // Downcast to orignial type
    TestAllocator* allocator = (TestAllocator*)_allocator;

    void* block = allocator->free_block;

    // Store the block size to itself so the block can be later marked as free.
    memcpy(block, &size, sizeof size);

    // Remember aligment!
    allocator->free_block = (uint8_t*)block
      + gp_round_to_aligned(sizeof size)
      + gp_round_to_aligned(size);

    pthread_mutex_unlock(&test_allocator_mutex);

    // block points to its size. We don't want to return that but return the
    // memory right next to it instead.
    return (uint8_t*)block + gp_round_to_aligned(sizeof size);
}

// Not actually deallocates, but marks pointer as free for testing purposes.
static void test_dealloc(const GPAllocator* allocator, void*_block)
{
    if (!allocator || !_block)
        return;

    pthread_mutex_lock(&test_allocator_mutex);

    uint8_t* block = _block;

    size_t block_size;
    memcpy(&block_size, block - gp_round_to_aligned(sizeof block_size), sizeof block_size);

    // Mark the block as free.
    memset(
        block - gp_round_to_aligned(sizeof block_size),
        0xFF,
        gp_round_to_aligned(sizeof block_size) + gp_round_to_aligned(block_size));

    pthread_mutex_unlock(&test_allocator_mutex);
}

static void private_delete_test_allocator(TestAllocator* allocator)
{
    pthread_mutex_destroy(&test_allocator_mutex);
    free((void*)allocator);
}

void delete_test_allocator(const GPAllocator* allocator)
{
    private_delete_test_allocator((TestAllocator*)allocator);
}

#endif // NDEBUG

#if 0 // -------------- OLD -------------------------

// #include <threads.h>

// Not many assertions here, address sanitizer does half of the work, manual
// debugging does the other half.
// TODO override heap allocator for proper tests.

int foo(void*_)
{
    (void)_;
    GPAllocator* scope = gp_begin(0);
    {
        GPAllocator* scope = gp_begin(0);
        {
            GPAllocator* scope = gp_begin(0);
            {
                GPAllocator* scope = gp_begin(0);
                gp_end(scope);
            }
            gp_end(scope);
        }
        gp_end(scope);
    }
    gp_end(scope);

    return 0;
}

void to_defer(void* x)
{
    printf("%i\n", (int)(intptr_t)x);
}

int bar(void*_)
{
    (void)_;
    GPAllocator* scope = gp_begin(0);
    gp_defer(scope, to_defer, (void*)-1);
    gp_defer(scope, to_defer, (void*) 0);
    gp_defer(scope, to_defer, (void*) 1);
    gp_defer(scope, to_defer, (void*)-1);
    {
        GPAllocator* scope = gp_begin(0);
        gp_defer(scope, to_defer, (void*)123);
    }
    gp_defer(scope, to_defer, (void*) 0);
    gp_defer(scope, to_defer, (void*) 1);
    gp_defer(scope, to_defer, (void*)-1);
    gp_defer(scope, to_defer, (void*) 0);
    gp_defer(scope, to_defer, (void*) 1);

    // Whoops, forgot to gp_end(). No worries, threads clean up their scopes.
    return 0;
}

int main(void)
{
    gp_test("Memory");

    GPArena _arena = gp_arena_new(16, 2.);
    void* arena = &_arena; // get rid of ugly casts
    char* short_str = gp_alloc(arena, *short_str, strlen("Hi") + 1);
    strcpy(short_str, "Hi");

    char* my_name = gp_alloc(arena, *my_name, strlen("my name") + 1);
    strcpy(my_name, "my name");
    char* is_lore = gp_alloc(arena, *is_lore, strlen("is Lore") + 1);
    strcpy(is_lore, "is Lore");

    const char* long_str_init =
        "This is a very long stirng that should not fit to small memory blocks."
        " In that case, the allocator just reserves a dedicated block just for "
        "this string.";
    char* long_str = gp_alloc(arena, *long_str, strlen(long_str_init) + 1);
    strcpy(long_str, long_str_init);

    char* very_long_str = gp_alloc(arena, char/*this works too*/, 8 * strlen(long_str) + 1);
    very_long_str[0] = '\0';
    for (size_t i = 0; i < 8; i++)
        strcat(very_long_str, long_str);

    gp_arena_rewind(arena, is_lore);
    // long_str[0] = '\0'; // use after free
    is_lore[strlen(is_lore) - 1] = 'E'; // not freed from heap, but arena freed!
    gp_expect(_arena.head->position == is_lore);

    char* overwrite = gp_alloc(arena, *overwrite, strlen("IS LORE") + 1);
    strcpy(overwrite, "IS LORE");
    gp_expect(strcmp(is_lore, "IS LORE") == 0,
        "is_lore is freed so it should've been overwritten.");

    gp_arena_rewind(arena, NULL);
    gp_expect(_arena.head->position == short_str,
        "short_str was the first allocation.");
    gp_arena_delete(arena);

    // ------------------------------------------------------------------------
    // Scope allocator

    // thrd_t t;
    // thrd_create(&t, foo, NULL);

    char* s0 = NULL;
    char* s1 = NULL;
    char* s2 = NULL;
    {
        GPAllocator* scope0 = gp_begin(0);
        s0 = gp_alloc(scope0, *s0, 32);
        strcpy(s0, "outer");
        {
            GPAllocator* scope1 = gp_begin(0);
            s1 = gp_alloc(scope1, *s1, 32);
            strcpy(s1, "mid");
            {
                GPAllocator* scope2 = gp_begin(0);
                s2 = gp_alloc(scope2, *s2, 32);
                strcpy(s2, "inner");
                puts(s0);
                puts(s1);
                puts(s2);
                gp_end(scope2); // gp_end(scope1) and gp_end(scop0) frees this too
            }
            puts(s0);
            puts(s1);
            // puts(s2); // freed!
            gp_end(scope1); // gp_end(scope0) frees this too
        }
        puts(s0);
        // puts(s1); // freed!
        gp_end(scope0);
    }
    // puts(s0); // freed!
    // puts(s1); // freed!
    // puts(s2); // freed!

    // thrd_join(t, NULL);

    // thrd_create(&t, bar, NULL);
    // thrd_join(t, NULL);
}
#endif
