// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    #if 0
    gpc_suite("Creation, copying, and memory");
    {
        gpc_String small_buf = gpc_str_on_stack("", 5);
        gpc_String source    = gpc_str_on_stack("String longer than 5 chars");

        gpc_String on_heap      = gpc_str_new("on heap!", 0);
        gpc_String on_heap_long = gpc_str_new(source.cstr, 50);

        // gpc_str_on_stack() only accepts literals as initializers.
        #if non_compliant
        char* init = "Whatever";
        gpc_String str = gpc_str_on_stack(init);
        #endif

        gpc_test("Creation");
        {
            gpc_expect(small_buf.capacity == 5,
            ("This buffer should be exactly 5 chars long without rounding."));

            gpc_expect( ! small_buf.allocation,
            ("Shouldn't be on heap at this point."));

            gpc_expect(
                on_heap.capacity == gpc_next_power_of_2(strlen("on heap!")),
                ("Buffer size should be rounded up."));
            gpc_expect(on_heap_long.capacity == gpc_next_power_of_2(50));
        }

        gpc_test("Copying");
        {
            gpc_str_copy(&small_buf, source);
            gpc_expect(gpc_str_equal(small_buf, source),
            ("Copying should've succeeded dispite buffer being too small"));

            gpc_expect(small_buf.capacity >= source.length,
            ("Buffer should've been grown to fit the source string"));
        }

        gpc_test("Memory");
        {
            gpc_expect(small_buf.allocation,
            ("After copying buffer should've been allocated."));

            // Freeing memory with free() or gpc_str_free() is safe even with
            // stack allocated strings given that they have been initialized.
            // It's recommended to always free all strings, even stack
            // allocated, because any mutating function may allocate.
            free(small_buf.allocation);
            free(source.allocation); // Stack allocated but OK!
            gpc_str_free(&on_heap);
            gpc_str_free(&on_heap_long);
            gpc_String empty_str = {0};

            gpc_expect(memcmp(&on_heap, &empty_str, sizeof(gpc_String)) == 0,
            ("gpc_str_free() should empty all string contents."));
        }
    }
    #endif
}
