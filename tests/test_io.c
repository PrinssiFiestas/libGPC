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
            const char* f_contents = "yeah";
            fwrite(f_contents, 1, strlen(f_contents), f);
            fclose(f);

            size_t file_size = gp_file_size("gp_io_test_file.txt");
            gp_expect(file_size == strlen(f_contents),
                file_size, f_contents);
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
