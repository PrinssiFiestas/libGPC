// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    gp_suite("Creation, copying, and memory");
    {
        GPString small_buf = gpstr_on_stack("", 5);
        GPString source    = gpstr_on_stack("String longer than 5 chars");

        // gpstr_on_stack() only accepts literals as initializers.
        #if non_compliant
        char* init = "Whatever";
        GPString str = gpstr_on_stack(init);
        #endif

        gp_test("Creation");
        {
            gp_expect(gpstr_capacity(small_buf) == 5,
            ("This buffer should be exactly 5 chars long without rounding."));

            gp_expect( ! gpstr_is_allocated(small_buf),
            ("Shouldn't be on heap at this point."));
        }

        gp_test("Copying");
        {
            gpstr_copy(&small_buf, source);
            gp_expect(gpstr_eq(small_buf, source),
            ("Copying should've succeeded dispite buffer being too small"));

            gp_expect(gpstr_capacity(small_buf) >= source.length,
            ("Buffer should've been grown to fit the source string"));
        }

        gp_test("Memory");
        {
            gp_expect(gpstr_is_allocated(small_buf),
            ("After copying buffer should've been allocated."));

            // It's recommended to always free all strings, even stack
            // allocated, because any mutating function may allocate.
            gpstr_clear(&small_buf);
            gpstr_clear(&source); // Stack allocated but OK!
        }
    }

    gp_suite("Insert character, str_at and string view");
    {
        GPString on_stack = gpstr_on_stack("on stack");
        GPString view = gpstr("string view");

        gp_test("str_is_view");
        {
            gp_expect(gpstr_is_view(view));
            gp_expect( ! gpstr_is_view(on_stack));
        }

        gp_test("replace_char");
        {
            gpstr_replace_char(&on_stack, 2, 'X');
            gpstr_replace_char(&view, 2, 'X');

            gp_expect(gpstr_eq(on_stack, gpstr("onXstack")));
            gp_expect(gpstr_eq(view, gpstr("stXing view")));
        }

        gp_test("str at");
        {
            gp_expect(gpstr_at(view, 2) == 'X');
            gp_expect(gpstr_at(view, 397) == '\0');
        }

        gp_test("still string view?");
        {
            gp_expect( ! gpstr_is_view(view));
            gp_expect(gpstr_is_allocated(view));
        }
    }

    // ------------------------ TEST INTERNALS

}
