// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include "../src/memory.c"
#include <threads.h>

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
    GPAlc* scope = gp_begin(0);
    gp_defer(scope, to_defer, (void*)-1);
    gp_defer(scope, to_defer, (void*) 0);
    gp_defer(scope, to_defer, (void*) 1);
    gp_defer(scope, to_defer, (void*)-1);
    {
        GPAlc* scope = gp_begin(0);
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

    thrd_t t;
    thrd_create(&t, foo, NULL);

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

    thrd_join(t, NULL);

    thrd_create(&t, bar, NULL);
    thrd_join(t, NULL);
}
