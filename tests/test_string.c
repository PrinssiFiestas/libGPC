// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
    gp_suite("Substrings");
    {
        gp_test("slice");
        {
            GPString str = gpstr_on_stack([], "Some_string_to slice");
            gpstr_slice(&str, 5, 11); // not including 11!
            gp_expect(gpstr_eq(str, gpstr("string")));
        }

        gp_test("substr");
        {
            GPString src = gpstr("Some_string_to slice");
            GPString dest = gpstr_on_stack([128], "");
            gpstr_substr(&dest, src, 5, 11); // not including 11!
            gp_expect(gpstr_eq(dest, gpstr("string")),
            ("%s", gpcstr(dest)));
        }
    }

    gp_suite("insert");
    {
        GPString str = gpstr_on_stack([128], "test");
        gp_test("Appending");
        {
            gpstr_insert(&str, str.length, gpstr(" tail"));
            gp_expect(gpstr_eq(str, gpstr("test tail")),
            (gpcstr(str)));
        }
        gp_test("Prepending");
        {
            gpstr_insert(&str, 0, gpstr("head "));
            gp_expect(gpstr_eq(str, gpstr("head test tail")),
            (gpcstr(str)));
        }
        gp_test("Insertion");
        {
            gpstr_insert(&str, 5, gpstr("insertion "));
            gp_expect(gpstr_eq(str, gpstr("head insertion test tail")),
            (gpcstr(str)));
        }
    }
}
