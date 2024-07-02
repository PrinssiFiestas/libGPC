// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/unicode.c"
#include <gpc/assert.h>
#include <gpc/io.h>

// char16_t and char32_t only needed for tests. libGPC does not depend on them.
#if __STDC_VERSION__ >= 201112L
#include <uchar.h>
#endif

int main(void)
{
    // Tiny arena to put address sanitizer to work
    GPArena _arena = gp_arena_new(1);
    _arena.growth_coefficient = 0.0;
    GPAllocator* arena = (GPAllocator*)&_arena;

    gp_suite("Conversions");
    {
        #if __STDC_VERSION__ >= 201112L
        GPString utf8 = gp_str_on_stack(NULL, 32, "zß水🍌");
        GPArray(uint16_t) utf16 = gp_arr_new(arena, sizeof utf16[0], 32);

        gp_test("UTF-8 to UTF-16");
        {
            const char16_t wcs[] = u"zß水🍌";
            utf16 = gp_arr_copy(
                sizeof utf16[0], utf16, wcs, sizeof wcs / sizeof*wcs - sizeof"");
            gp_utf8_to_utf16(&utf16, utf8, gp_str_length(utf8));
            gp_expect(gp_arr_length(utf16) == sizeof wcs / sizeof*wcs - sizeof"",
                gp_arr_length(utf16), sizeof wcs / sizeof*wcs - sizeof"");
            if ( ! gp_expect(gp_bytes_equal(
                utf16, gp_arr_length(utf16) * sizeof utf16[0],
                wcs, sizeof wcs - sizeof u"")))
                for (size_t i = 0; i < gp_arr_length(utf16); i++)
                    gp_file_println(stderr, utf16[i], wcs[i]);
        }

        gp_test("UTF-16 to UTF-8");
        {
            GPString decoding = gp_str_on_stack(NULL, 32, "");
            gp_utf16_to_utf8(&decoding, utf16, gp_arr_length(utf16));
            gp_expect(gp_str_equal(utf8, decoding, gp_str_length(decoding)), utf8, decoding);
        }
        #endif
    }

    gp_arena_delete(&_arena);
}
