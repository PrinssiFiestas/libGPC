// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    gpc_suite("Creation, copying, and memory");
    {
        GPString small_buf = gpstr_on_stack("", 5);
        GPString source    = gpstr_on_stack("String longer than 5 chars");

        // gpstr_on_stack() only accepts literals as initializers.
        #if non_compliant
        char* init = "Whatever";
        GPString str = gpstr_on_stack(init);
        #endif

        gpc_test("Creation");
        {
            gpc_expect(small_buf.capacity == 5,
            ("This buffer should be exactly 5 chars long without rounding."));

            gpc_expect( ! small_buf.is_allocated,
            ("Shouldn't be on heap at this point."));
        }

        gpc_test("Copying");
        {
            gpstr_copy(&small_buf, source);
            gpc_expect(gpstr_eq(small_buf, source),
            ("Copying should've succeeded dispite buffer being too small"));

            gpc_expect(small_buf.capacity >= source.length,
            ("Buffer should've been grown to fit the source string"));
        }

        gpc_test("Memory");
        {
            gpc_expect(small_buf.is_allocated,
            ("After copying buffer should've been allocated."));

            // It's recommended to always free all strings, even stack
            // allocated, because any mutating function may allocate.
            gpstr_clear(&small_buf);
            gpstr_clear(&source); // Stack allocated but OK!
        }
    }

    gpc_suite("Insert character, str_at and string view");
    {
        GPString on_stack = gpstr_on_stack("on stack");
        GPString view = gpstr("string view");

        gpc_test("str_is_view");
        {
            gpc_expect(gpstr_is_view(view));
            gpc_expect( ! gpstr_is_view(on_stack));
        }

        gpc_test("replace_char");
        {
            gpstr_replace_char(&on_stack, 2, 'X');
            gpstr_replace_char(&view, 2, 'X');

            gpc_expect(gpstr_eq(on_stack, gpstr("onXstack")));
            gpc_expect(gpstr_eq(view, gpstr("stXing view")));
        }

        gpc_test("str at");
        {
            gpc_expect(gpstr_at(view, 2) == 'X');
            gpc_expect(gpstr_at(view, 397) == '\0');
        }

        gpc_test("still string view?");
        {
            gpc_expect( ! gpstr_is_view(view));
            gpc_expect(view.is_allocated);
        }
    }

    // ------------------------ TEST INTERNALS

    gpc_suite("Reading and writing allocation offset amount");
    {
        const size_t whatever = 4;
        gpc_test("Small offset");
        {
            GPString s = gpstr_on_stack("1345message");
            const size_t new_offset = 5;
            gpstr_slice(&s, new_offset, whatever);
            gpc_expect(gpc_l_capacity(s) == new_offset, ("%llu", gpc_l_capacity(s)));
        }
        gpc_test("Large offset");
        {
            const size_t more_than_char_max = (unsigned short)(-1) * 2;
            char* buf = malloc(more_than_char_max);
            GPString s = { buf,
                .length   = more_than_char_max - 1,
                .capacity = more_than_char_max - 1
            };
            gpstr_slice(&s, UCHAR_MAX + 1, whatever);
            gpc_expect(gpc_l_capacity(s) == UCHAR_MAX + 1, ("%llu", gpc_l_capacity(s)));
            free(buf); // to be pedantic
        }
    }
}
