// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/bytes.c"
#include <gpc/assert.h>

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

    gp_suite("To X");
    {
        char str[] = "hellö";
        size_t length = strlen(str);

        gp_test("To upper");
        {
            gp_bytes_to_upper(str, length);
            gp_expect(gp_bytes_equal(str, length, "HELLö", length));
        }

        gp_test("To lower");
        {
            gp_bytes_to_lower(str, length);
            gp_expect(gp_bytes_equal(str, length, "hellö", length));
        }

        gp_test("Equal case");
        {
            gp_expect(gp_bytes_equal_case("heLlo", 5, "HEllo", 5));
        }

        gp_test("To valid ASCII");
        {
            size_t non_ascii_pos;
            gp_expect( ! gp_bytes_is_valid_ascii(str, length, &non_ascii_pos));
            gp_expect(non_ascii_pos == (size_t)(strstr(str, "ö") - str));

            length = gp_bytes_to_valid(str, length, "X");
            gp_expect(gp_bytes_equal(str, length, "hellX", strlen("hellX")));
        }
    }

    gp_suite("ASCII examination");
    {
        gp_test("Find first of");
        {
            const char* haystack = " \t\r\nblah";
            gp_expect(
                gp_bytes_find_first_of(haystack, strlen(haystack), "abcd", 0)
                == strcspn(haystack, "abcd"));
            gp_expect(
                gp_bytes_find_first_of(haystack, strlen(haystack), GP_ASCII_WHITESPACE, 4)
                == GP_NOT_FOUND);
        }

        gp_test("Find first not of");
        {
            const char* haystack = " \t\r\nblah";
            gp_expect(
                gp_bytes_find_first_not_of(haystack, strlen(haystack), GP_ASCII_WHITESPACE, 0)
                == strspn(haystack, "\n\r\t "));
            gp_expect(
                gp_bytes_find_first_not_of(haystack, strlen(haystack), "hlab", 4)
                == GP_NOT_FOUND);
        }
    }
}
