// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include <gpc/io.h>
#include "../src/memory.c"
#include "../src/thread.h"

// Testing allocator. Does not free but marks memory as freed instead.
// Check below main() for definitions and how to write custom allocators.
GPAllocator* new_test_allocator(void);
void delete_test_allocator(GPAllocator*);

static bool is_free(void*_ptr)
{
    gp_assert(_ptr);
    uint8_t* ptr = _ptr;
    ASAN_UNPOISON_MEMORY_REGION(ptr, GP_ALLOC_ALIGNMENT);
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

            // This could be used for better informed estimation for gp_begin(),
            // but now we'll just test that it's not empty.
            size_t scope_size = gp_end(scope);
            gp_assert(scope_size > 0);

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

        #if __GNUC__ || _MSC_VER
        gp_test("Magic scope");
        {
            const int dummy_var = 0;
            (void)dummy_var;
            GP_BEGIN
                // Shadowing to demonstrate that we are indeed in another {} scope
                const int dummy_var = 1;
                (void)dummy_var;

                // GP_BEGIN created scope allocator
                int* p = gp_mem_alloc(scope, sizeof*p);
                *p = 7;

                gp_defer(scope, gp_suite, NULL); // also end suite when exiting scope

                // Exiting scope using any control structure is ok. gp_end() is
                // guaranteed to be called on scope exit for the scope allocator
                // of the current scope.
                return (GPThreadResult)0;
            GP_END
        }
        #endif
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
            const size_t block_size = 64;
            // Using a tiny arena forces it to allocate each objects separately.
            // This allows the testing allocator to mark freed objects on
            // rewind.
            GPArena arena = {0};
            gp_arena_init(&arena, 1);
            arena.growth_coefficient = 1.;
            char* ps[4] = {0};
            for (size_t i = 0; i < 4; i++) {
                ps[i] = gp_mem_alloc((GPAllocator*)&arena, block_size);
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
                "ps[2] should be considered as freed despite not freed from the "
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
    GPAllocator* original_heap = gp_heap;
    GPAllocator* test_allocator = new_test_allocator();
    gp_heap = test_allocator;

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

    GPArena shared_arena = {.is_shared = (void*)1 };
    gp_arena_init(&shared_arena, 0);
    for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
        gp_thread_create(&tests[i], test_shared, &shared_arena);
    for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
        gp_thread_join(tests[i], NULL);
    gp_arena_delete(&shared_arena);


    gp_heap = original_heap; // put sanitizers back to work

    gp_suite("Other stuff");
    {
        gp_test("Scratch allocator");
        {
            for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
                gp_thread_create(&tests[i], test_scratch, NULL);
            for (size_t i = 0; i < sizeof tests / sizeof*tests; i++)
                gp_thread_join(tests[i], NULL);
        }

        gp_test("Arena reset");
        {
            GPArena arena = {0};
            gp_arena_init(&arena, 8);
            void* arena_start = gp_mem_alloc((GPAllocator*)&arena, 0);
            for (size_t i = 0; i < 32; ++i)
                (void*){0} = gp_mem_alloc((GPAllocator*)&arena, 32);
            gp_arena_reset(&arena);
            gp_assert(arena_start == gp_mem_alloc((GPAllocator*)&arena, 0));
            gp_arena_delete(&arena);
        }

        gp_test("Alignment");
        {
            #define ALIGNMENT 256 // must be a power of 2!
            typedef struct aligned_data {
                #if __STDC_VERSION__ >= 201112L
                _Alignas(ALIGNMENT)
                #endif
                uint8_t data[ALIGNMENT];
            } AlignedData;

            AlignedData* data = gp_mem_alloc_aligned(gp_heap, sizeof*data, sizeof*data);
            gp_expect((uintptr_t)data == gp_round_to_aligned((uintptr_t)data, sizeof*data));
            gp_expect((uintptr_t)data % ALIGNMENT == 0);
            memset(data, 0, sizeof*data);
            gp_mem_dealloc(gp_heap, data);

            GPArena arena = {.max_size = 1};
            gp_arena_init(&arena, 1);
            data = gp_mem_alloc_aligned((GPAllocator*)&arena, sizeof*data, sizeof*data);
            gp_expect((uintptr_t)data == gp_round_to_aligned((uintptr_t)data, sizeof*data));
            gp_expect((uintptr_t)data == gp_round_to_aligned((uintptr_t)(arena.head + 1), sizeof*data));
            gp_expect((uintptr_t)data % ALIGNMENT == 0);
            memset(data, 0, sizeof*data);
            gp_arena_delete(&arena);
        }

        gp_test("Virtual Arena");
        {
            GPVirtualArena va;
            const size_t huge_size = 1024*1024*1024;
            gp_assert(gp_virtual_init(&va, huge_size) != NULL);

            char* buffer = gp_mem_alloc((GPAllocator*)&va, huge_size);

            // Physical memory is only used now
            buffer[0] = 'x';
            buffer[huge_size - 1] = 'x';

            gp_virtual_rewind(&va, buffer);
            gp_expect(va.position == va.start, "Arena pointer should be resetted");
            gp_expect(buffer[huge_size - 1] == 'x', "Memory should remain untouched");

            // Faster than gp_mem_alloc(), which is used for polymorphism.
            buffer = gp_virtual_alloc(&va, huge_size, GP_ALLOC_ALIGNMENT);

            gp_virtual_reset(&va); // physical memory also freed
            gp_expect(va.position == va.start, "Arena pointer should be resetted");
            #if !_WIN32
            gp_expect(buffer[huge_size - 1] == '\0',
                "Physical freed, so access should zero memory");
            #endif

            gp_virtual_delete(&va);
        }
    } // gp_suite("Other stuff")

    delete_test_allocator(test_allocator);
}

// ----------------------------------------------------------------------------
// Custom allocators

// Every allocator is REQUIRED to have alloc() and dealloc() where alloc()
// should NOT return NULL in ANY CIRCUMSTANCE. Not being able to return NULL may
// seem like a big limitation, but it makes reasoning about NULL massively
// simpler. All pointers returned by alloc() also MUST be aligned to
// GP_ALLOC_ALIGNMENT boundary. dealloc() is REQUIRED to handle NULL arguments.
static void* test_alloc(GPAllocator*, size_t, size_t) GP_NONNULL_ARGS_AND_RETURN;
static void  test_dealloc(GPAllocator* optional, void* optional_block);

// While struct gp_allocator could be used by itself, your allocator probably
// needs some data of its own. We are going to write an arena-like allocator.
typedef struct test_allocator
{
    // Inherit from GPAllocator by making the FIRST member a GPAllocator. This
    // allows safe and well defined upcasting.
    GPAllocator base;

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
GPAllocator* new_test_allocator(void)
{
    gp_mutex_init(&test_allocator_mutex);
    TestAllocator* allocator = malloc(1000 * (1 << 10));
    if (allocator == NULL)
        abort();
    memset(allocator, 0xBE, 1000 * (1 << 10)); // magic byte 0xBE for debugging
    gp_assert(allocator != NULL);
    *allocator = (TestAllocator) {
        .base  = {.alloc = test_alloc, .dealloc = test_dealloc },
        .free_block = (uint8_t*)allocator + gp_round_to_aligned(sizeof*allocator, GP_ALLOC_ALIGNMENT)
    };
    return (GPAllocator*)allocator;
}

// Normally, arena running out of memory should be handled somehow. GPArena and
// the scope allocator does this by creating new arenas where the size is
// guranteed to fit the size argument of alloc(). Here we will keep things
// simple for testing purposes and omit handling out of memory case.
static void* test_alloc(GPAllocator*_allocator, size_t size, size_t alignment)
{
    gp_mutex_lock(&test_allocator_mutex);

    // Downcast to orignial type
    TestAllocator* allocator = (TestAllocator*)_allocator;

    void* block =
        (uint8_t*)gp_round_to_aligned((uintptr_t)allocator->free_block, alignment)
        + alignment
        - sizeof size;

    // Store the block size to itself so the block can be later marked as free.
    memcpy(block, &size, sizeof size);

    allocator->free_block = (uint8_t*)block + sizeof size + size;

    gp_mutex_unlock(&test_allocator_mutex);

    // block points to its size. We don't want to return that but return the
    // memory right next to it instead.
    return (uint8_t*)block + sizeof size;
}

// Not actually deallocates, but marks pointer as free for testing purposes.
static void test_dealloc(GPAllocator* allocator, void*_block)
{
    if (!allocator || !_block)
        return;

    gp_mutex_lock(&test_allocator_mutex);

    uint8_t* block = _block;

    size_t block_size;
    ASAN_UNPOISON_MEMORY_REGION(block - sizeof block_size, sizeof block_size);
    memcpy(&block_size, block - sizeof block_size, sizeof block_size);
    ASAN_UNPOISON_MEMORY_REGION(block, block_size);

    // Mark the block as free.
    memset(
        block - sizeof block_size,
        0xFF,
        sizeof block_size + block_size);

    gp_mutex_unlock(&test_allocator_mutex);
}

static void private_delete_test_allocator(TestAllocator* allocator)
{
    gp_mutex_destroy(&test_allocator_mutex);
    free((void*)allocator);
}

void delete_test_allocator(GPAllocator* allocator)
{
    private_delete_test_allocator((TestAllocator*)allocator);
}
