// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include "../src/memory.c"

// Not many assertions here, address sanitizer does half of the work, manual
// debugging does the other half.

int main(void)
{
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
}
