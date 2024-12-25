// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include <gpc/io.h>
#include "../src/memory.c"
#include "../src/thread.h"

// Testing allocator. Does not free but marks memory as freed instead.
// Check below main() for definitions and how to write custom allocators.
const GPAllocator* new_test_allocator(void);
void delete_test_allocator(void);

static bool is_free(void*_ptr)
{
    gp_assert(_ptr);
    uint8_t* ptr = _ptr;
    #ifdef __SANITIZE_ADDRESS__
    ASAN_UNPOISON_MEMORY_REGION(ptr, GP_ALLOC_ALIGNMENT);
    #endif
    for (size_t i = 0; i < GP_ALLOC_ALIGNMENT; i++) if (ptr[i] != 0xFF)
        return false;
    return true;
}

static void deferred_dealloc(void* p)
{
    gp_mem_dealloc(gp_heap, p);
}

static GPThreadResult test0(void*_)
{
    (void)_;
    void* ps[8] = {0}; // Dummy objects

    gp_suite("Scope allocator");
    { // The scopes created by the scope allocator are not in any way tied to
      // the C scoping operator {}. Here we use them just to empahize that
      // scopes are lexical.
        gp_test("Basic usage");
        {
            // Use tiny 1 byte scopes to demostrate that the arenas can hold
            // objects that do not fit in them. Normally you would use larger
            // values or 0 for some default value.
            GPAllocator* scope = gp_begin(1);
            ps[0] = gp_mem_alloc(scope, 64);
            {
                GPAllocator* scope = gp_begin(1);
                ps[1] = gp_mem_alloc(scope, 64);
                {
                    GPAllocator* scope = gp_begin(1);
                    ps[2] = gp_mem_alloc(scope, 64);
                    // whoops, forgot to end(scope)
                }
                {
                    GPAllocator* scope = gp_begin(1);
                    ps[3] = gp_mem_alloc(scope, 64);
                    gp_end(scope);
                }
                {
                    GPAllocator* scope = gp_begin(1);
                    ps[4] = gp_mem_alloc(scope, 64);
                    // Sanity check
                    gp_expect( ! is_free(ps[0]));
                    gp_expect( ! is_free(ps[1]));
                    gp_expect( ! is_free(ps[2]));
                    gp_expect(   is_free(ps[3]));
                    gp_expect( ! is_free(ps[4]));
                    // whoops, forgot to end(scope)
                }
                gp_end(scope); // this also ends the "forgotten" inner scopes.
            }
            // We can allocate as much as we like, gp_end(scope) will free all.
            ps[5] = gp_mem_alloc(scope, 64);
            ps[6] = gp_mem_alloc(scope, 64);
            ps[7] = gp_mem_alloc(scope, 64);

            gp_end(scope);

            for (size_t i = 0; i < sizeof ps / sizeof*ps; i++)
                gp_expect(is_free(ps[i]));
        }

        gp_test("Defer");
        {
            GPAllocator* scope = gp_begin(0);

            // Note: using heap memory this way makes no sense, the scope
            // allocator could be used directly so no deferred free needed. In
            // fact, it is unsafe to defer free since reallocating p1 or p2
            // would cause double free. We only use the heap now for unit
            // testing and demonstration purposes.

            void* p1 = gp_mem_alloc(gp_heap, 64);
            gp_scope_defer(scope, deferred_dealloc, p1);
            void* p2 = gp_mem_alloc(gp_heap, 64);
            gp_defer(scope, deferred_dealloc, p2);
            FILE* f = tmpfile();
            #if WARNING
            // pi does not type check to arg of fclose which would be unsafe. Good!
            int* pi = gp_mem_alloc(gp_heap, sizeof*pi);
            gp_defer(scope, gp_file_close, pi);
            #else
            gp_defer(scope, gp_file_close, f);
            #endif

            gp_expect( ! is_free(p1));
            gp_expect( ! is_free(p2));

            gp_end(scope);

            gp_expect(is_free(p1));
            gp_expect(is_free(p2));
        }
    }
    gp_suite(NULL);

    return (GPThreadResult)0;
}

static void* test1_ps[4] = {0};

static GPThreadResult test1(void*_)
{
    (void)_;
    GPAllocator* scope0 = gp_begin(0);
    GPAllocator* scope1 = gp_begin(0);
    GPAllocator* scope2 = gp_begin(0);
    GPAllocator* scope3 = gp_begin(0);
    test1_ps[0] = gp_mem_alloc(scope0, 8);
    test1_ps[1] = gp_mem_alloc(scope1, 8);
    test1_ps[2] = gp_mem_alloc(scope2, 8);
    test1_ps[3] = gp_mem_alloc(scope3, 8);

    return (GPThreadResult)0;
} // All scopes will be cleaned when threads terminate

static GPThreadResult test2(void*_)
{
    (void)_;
    gp_suite("Arena allocator");
    {
        gp_test("Basic usage");
        {
            // Using a tiny arena forces it to allocate each objects separately.
            // This allows the testing allocator to mark freed objects on
            // rewind.
            GPArena arena = gp_arena_new(1);
            arena.growth_coefficient = 1.;
            void* ps[4] = {0};
            for (size_t i = 0; i < 4; i++) {
                ps[i] = gp_mem_alloc((GPAllocator*)&arena, 64);
                strcpy(ps[i], &"abcd"[i]);
            }
            // Sanity test
            for (size_t i = 0; i < 4; i++)
                gp_assert( ! is_free(ps[i]));

            gp_arena_rewind(&arena, ps[2]);
            gp_expect( ! is_free(ps[0]));
            gp_expect( ! is_free(ps[1]));
            gp_expect( ! is_free(ps[2]), "Not from heap, but from arena!");
            gp_expect(   is_free(ps[3]));

            void* overwriting_pointer = gp_mem_alloc((GPAllocator*)&arena, 4);
            strcpy(overwriting_pointer, "XXX");
            gp_assert(strcmp(ps[2], "XXX") == 0,
                "ps[2] should be considered as freed dispite not freed from the "
                "heap! Here it got overwritten since the arena reused it's "
                "memory.", (char*)ps[2]);

            gp_arena_delete(&arena);

            gp_expect(is_free(ps[0]));
            gp_expect(is_free(ps[1]));
        }
    }
    return (GPThreadResult)0;
}

static GPThreadResult test_shared(void* shared_arena)
{
    (void)shared_arena;
    gp_test("Shared arena");
    {
        for (size_t i = 0; i < 1024; i++)
        {
            char* str = gp_mem_alloc(shared_arena, 32);
            strcpy(str, "Thread safe!");
            gp_assert(strcmp(str, "Thread safe!") == 0);
        }
    }
    return (GPThreadResult)0;
}

static GPThreadResult test_scratch(void*_)
{
    (void)_;
    gp_test("Scratch arena");
    {
        GPArena* scratch = gp_scratch_arena();
        char* mem = gp_mem_alloc((GPAllocator*)scratch, 64);
        strcpy(mem, "Blazing fast thread local memory!");
        gp_expect(strcmp(mem, "Blazing fast thread local memory!") == 0);

        // Do NOT delete scratch arenas! Scratch arenas get deleted
        // automatically when threads exit.
    }
    return (GPThreadResult)0;
}

int main(void)
{
    gp_heap = new_test_allocator();

    GPThread tests[3];
    gp_thread_create(&tests[0], test0, NULL);
    gp_thread_create(&tests[1], test1, NULL);
    gp_thread_create(&tests[2], test2, NULL);

    gp_thread_join(tests[0], NULL);
    gp_thread_join(tests[1], NULL);
    gp_thread_join(tests[2], NULL);

    gp_test("Thread cleaning it's scopes");
    {
        for (size_t i = 0; i < sizeof test1_ps / sizeof*test1_ps; i++)
            gp_expect(is_free(test1_ps[i]));
    }

    GPArena* shared_arena = gp_arena_new_shared(0);
    for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
        gp_thread_create(&tests[i], test_shared, shared_arena);
    for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
        gp_thread_join(tests[i], NULL);
    gp_arena_delete(shared_arena);

    for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
        gp_thread_create(&tests[i], test_scratch, NULL);
    for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
        gp_thread_join(tests[i], NULL);

    // Make Valgrind shut up.
    delete_test_allocator();
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

// We are overriding gp_heap and tests are threaded, a mutex is necessary.
static GPMutex test_allocator_mutex;

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
    gp_mutex_init(&test_allocator_mutex);
    TestAllocator* allocator = malloc(1000 * (1 << 10));
    gp_assert(allocator != NULL);
    *allocator = (TestAllocator) {
        .allocator  = {.alloc = test_alloc, .dealloc = test_dealloc },
        .free_block = (uint8_t*)allocator + gp_round_to_aligned(sizeof*allocator, GP_ALLOC_ALIGNMENT)
    };
    return (const GPAllocator*)allocator;
}

// Normally, arena running out of memory should be handled somehow. GPArena and
// the scope allocator does this by creating new arenas where the size is
// guranteed to fit the size argument of alloc(). Here we will keep things
// simple for testing purposes and omit handling out of memory case.
static void* test_alloc(const GPAllocator*_allocator, size_t size)
{
    gp_mutex_lock(&test_allocator_mutex);

    // Downcast to orignial type
    TestAllocator* allocator = (TestAllocator*)_allocator;

    void* block = allocator->free_block;

    // Store the block size to itself so the block can be later marked as free.
    memcpy(block, &size, sizeof size);

    // Remember aligment!
    allocator->free_block = (uint8_t*)block
      + gp_round_to_aligned(sizeof size, GP_ALLOC_ALIGNMENT)
      + gp_round_to_aligned(size,        GP_ALLOC_ALIGNMENT);

    gp_mutex_unlock(&test_allocator_mutex);

    // block points to its size. We don't want to return that but return the
    // memory right next to it instead.
    return (uint8_t*)block + gp_round_to_aligned(sizeof size, GP_ALLOC_ALIGNMENT);
}

// Not actually deallocates, but marks pointer as free for testing purposes.
static void test_dealloc(const GPAllocator* allocator, void*_block)
{
    if (!allocator || !_block)
        return;

    gp_mutex_lock(&test_allocator_mutex);

    uint8_t* block = _block;

    size_t block_size;
    #ifdef __SANITIZE_ADDRESS__ // arenas poison free memory, unpoison for testing
    ASAN_UNPOISON_MEMORY_REGION(block - GP_ALLOC_ALIGNMENT, GP_ALLOC_ALIGNMENT);
    #endif
    memcpy(&block_size, block - GP_ALLOC_ALIGNMENT, sizeof block_size);
    #ifdef __SANITIZE_ADDRESS__
    ASAN_UNPOISON_MEMORY_REGION(block, block_size);
    #endif

    // Mark the block as free.
    memset(
        block - GP_ALLOC_ALIGNMENT,
        0xFF,
        GP_ALLOC_ALIGNMENT + gp_round_to_aligned(block_size, GP_ALLOC_ALIGNMENT));

    gp_mutex_unlock(&test_allocator_mutex);
}

static void private_delete_test_allocator(TestAllocator* allocator)
{
    gp_mutex_destroy(&test_allocator_mutex);
    free((void*)allocator);
}

void delete_test_allocator(void)
{
    private_delete_test_allocator((TestAllocator*)gp_heap);
}
