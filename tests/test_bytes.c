// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/bytes.c"
#include <gpc/assert.h>
#include <string.h>

// GPString depends on big part of bytes module so we only test some bytes
// specific stuff here.

int main(void)
{
    gp_suite("Printing");
    {
        char str[16];
        size_t str_length;
        gp_test("print");
        {
            const char* cstr = "1+1=2";
            str_length = gp_bytes_print(str, 1, (char)'+', 1, "=", 1 + 1);
            gp_expect(gp_bytes_equal(str, str_length, cstr, strlen(cstr)));
        }

        gp_test("println");
        {
            str_length = gp_bytes_println(str, 1, 2, 3);
            const char* cstr = "1 2 3\n";
            gp_expect(gp_bytes_equal(str, str_length, cstr, strlen(cstr)));
        }

        gp_test("n");
        {
            char str[4];
            size_t ret = gp_bytes_n_print(str, sizeof str, "blah blah blah");
            gp_expect(gp_bytes_equal(str, sizeof str, "blah", 4),
                "%.4s", str);
            gp_expect(ret == strlen("blah blah blah"));
        }
    }
}
