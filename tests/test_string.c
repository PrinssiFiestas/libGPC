// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    gpc_suite("Creation, copying, and memory");
    {
        gpc_String small_buf = gpc_str_on_stack("", 5);
        gpc_String source    = gpc_str_on_stack("String longer than 5 chars");

        // gpc_str_on_stack() only accepts literals as initializers.
        #if non_compliant
        char* init = "Whatever";
        gpc_String str = gpc_str_on_stack(init);
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
            gpc_str_copy(&small_buf, source);
            gpc_expect(gpc_str_eq(small_buf, source),
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
            gpc_str_clear(&small_buf);
            gpc_str_clear(&source); // Stack allocated but OK!
        }
    }

    gpc_suite("Insert character, str_at and string view");
    {
        gpc_String on_stack = gpc_str_on_stack("on stack");
        gpc_String view = gpc_str("string view");

        gpc_test("str_is_view");
        {
            gpc_expect(gpc_str_is_view(view));
            gpc_expect( ! gpc_str_is_view(on_stack));
        }

        gpc_test("insert_char");
        {
            gpc_str_insert_char(&on_stack, 2, 'X');
            gpc_str_insert_char(&view, 2, 'X');

            gpc_expect(gpc_str_eq(on_stack, gpc_str("onXstack")));
            gpc_expect(gpc_str_eq(view, gpc_str("stXing view")));
        }

        gpc_test("str at");
        {
            gpc_expect(gpc_str_at(view, 2) == 'X');
            gpc_expect(gpc_str_at(view, 397) == '\0');
        }

        gpc_test("still string view?");
        {
            gpc_expect( ! gpc_str_is_view(view));
            gpc_expect(view.is_allocated);
        }
    }
}
