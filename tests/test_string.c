// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    gp_suite("Creation, copying, and memory");
    {
        GPString small_buf = gpstr_on_stack([5], "", &gpmem_std_allocator);
        const char* source = "String longer than 5 chars";
        size_t source_length = strlen(source);

        // gpstr_on_stack() only accepts literals as initializers.
        #if non_compliant // with -Werror
        char* init = "String longer than sizeof(char*)";

        // GCC -> warning: intialization of 'char' from 'char *' makes integer
        // from pointer without a cast [-Wint-conversion]
        GPString str = gpstr_on_stack([], init);
        gp_assert(str) != sizeof(char*.capacity,
        ("str was initialized with pointer instead of literal"));
        #endif

        gp_test("Creation");
        {
            gp_expect(small_buf.capacity == 5,
            ("This buffer should be exactly 5 chars long without rounding."));

            gp_expect( ! small_buf.allocation,
            ("Shouldn't be on heap at this point."));
        }

        gp_test("Copying");
        {
            gpstr_copy(&small_buf, source);
            gp_expect(gpstr_eq(small_buf, source),
            ("Copying should've succeeded dispite buffer being too small"));

            gp_expect(small_buf.capacity >= source_length,
            ("Buffer should've been grown to fit the source string"));
        }

        gp_test("Memory");
        {
            gp_expect(small_buf.allocation,
            ("After copying buffer should've been allocated."));

            // It's recommended to always free all strings, even stack
            // allocated, because any mutating function may allocate.
            gpstr_clear(&small_buf);
        }
    }

    gp_suite("Insert character, str_at and string view");
    {
        GPString on_stack = gpstr_on_stack([], "on stack");
        GPString view = {
            .data      = "string view",
            .length    = strlen("string view"),
            .allocator = &gpmem_std_allocator
        };

        gp_test("str_is_view");
        {
            gp_expect(gpstr_is_view(view));
            gp_expect( ! gpstr_is_view(on_stack));
        }

        gp_test("replace_char");
        {
            gpstr_replace_char(&on_stack, 2, 'X');
            gpstr_replace_char(&view, 2, 'X');

            gp_expect(gpstr_eq(on_stack, "onXstack"));
            gp_expect(gpstr_eq(view, "stXing view"));
        }

        gp_test("str at");
        {
            gp_expect(gpstr_at(view, 2) == 'X');
            gp_expect(gpstr_at(view, 397) == '\0');
        }

        gp_test("still string view?");
        {
            gp_expect( ! gpstr_is_view(view));
            gp_expect(view.allocation);
        }

        gpstr_clear(&on_stack);
        gpstr_clear(&view);
    }

    gp_suite("Substrings");
    {
        gp_test("slice");
        {
            GPString str = gpstr_on_stack([], "Some_string_to slice");
            gpstr_slice(&str, 5, 11); // not including 11!
            gp_expect(gpstr_eq(str, "string"));

            int error = gpstr_slice(&str, 3, 3);
            gp_expect(str.length == 0 && error == GPSTR_NO_ERROR,
            ("Same indices should yield an empty non-error string."));

            str = gpstr_on_stack([], "Whatever");
            error = gpstr_slice(&str, 600, 601);
            gp_expect(error == GPSTR_OUT_OF_BOUNDS);

            gp_expect(gpstr_eq(str, "Whatever"),
            ("slice() failed, string should be unmutated."));

            error = gpstr_slice(&str, 5, 2);
            gp_expect(error == GPSTR_OUT_OF_BOUNDS,
            ("End cant be larger than start index."));
        }

        gp_test("substr");
        {
            const char* src = "Some_string_to slice";
            GPString dest = gpstr_on_stack([129], "");
            gpstr_substr(&dest, src, 5, 11); // not including 11!
            gp_expect(gpstr_eq(dest, "string"),
            ("%s", gpcstr(&dest)));

            int error = gpstr_substr(&dest, src, 502, 250);
            gp_expect(dest.length == 0 && error == GPSTR_NO_ERROR,
            ("Invalid indices should yield an empty string without errors."));
        }
    }

    gp_suite("insert");
    {
        GPString str = gpstr_on_stack([], "test", &gpmem_std_allocator);
        gp_test("Appending");
        {
            gpstr_insert(&str, str.length, " tail");
            gp_expect(gpstr_eq(str, "test tail"),
            (gpcstr(&str)));
        }
        gp_test("Prepending");
        {
            gpstr_insert(&str, 0, "head ");
            gp_expect(gpstr_eq(str, "head test tail"),
            (gpcstr(&str)));
        }
        gp_test("Insertion");
        {
            gpstr_insert(&str, 5, "insertion ");
            gp_expect(gpstr_eq(str, "head insertion test tail"),
            (gpcstr(&str)));
        }
        gp_test("Error");
        {
            int error = gpstr_insert(&str, str.length + 1, "whatever");
            gp_expect(error == GPSTR_OUT_OF_BOUNDS);
        }
    }
}
