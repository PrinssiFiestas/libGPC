// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/io.c"

int main(void)
{
    gp_suite("File IO");
    {
        gp_test("file size");
        {
            FILE* f = fopen("gp_io_test_file.txt", "w");
            gp_assert(f != NULL);
            const char* f_contents = "yeah\nsecond line\nblah";
            fwrite(f_contents, 1, strlen(f_contents), f);
            fclose(f);

            GPStat s;
            gp_expect(gp_stat(&s, "gp_io_test_file.txt") == 0);
            gp_expect((size_t)s.st_size == strlen(f_contents),
                s.st_size, f_contents);

            gp_assert(f = fopen("gp_io_test_file.txt", "r"));
            GPString str = gp_str_on_stack(gp_heap, 1, "");
            while (gp_file_read_line(&str, f))
            {
                size_t line_length = 0;
                while (f_contents[line_length] != '\n' && f_contents[line_length] != '\0')
                    line_length++;
                if (f_contents[line_length] != '\0')
                    line_length += strlen("\n");

                gp_expect(gp_str_equal(str, f_contents, line_length), str);
                f_contents += line_length;
            }

            gp_str_delete(str);
            fclose(f);

            gp_assert(remove("gp_io_test_file.txt") == 0);
        }
    }

    gp_suite("Printing");
    {
        gp_print  (1,2,3,"Hello\n");
        gp_println(1, 2, 3, "Hello");

        // No asserts here, uncomment to test printing
        // gp_assert(0);
    }
}
