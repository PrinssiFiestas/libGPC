// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/io.c"

int main(void)
{
    // Tiny arena to put address sanitizer to work
    GPArena _arena = gp_arena_new(1);
    _arena.growth_coefficient = 0.0;
    GPAllocator* arena = (GPAllocator*)&_arena;

    gp_suite("File IO");
    {
        const char* f_contents = "yeah\nsecond line\nblah";
        gp_test("file size");
        {
            // The first character in mode string is used to determine actual
            // mode passed to fopen() to be consistent with gp_str_file(). Also
            // checks for occurrence of '+' and 'x' for text. Default is binary.
            FILE* f = gp_file_open("gp_io_test_file.txt", "write");
            gp_assert(f != NULL);
            fwrite(f_contents, 1, strlen(f_contents), f);
            gp_file_close(f);

            GPStat s;
            gp_expect(gp_stat(&s, "gp_io_test_file.txt") == 0);
            gp_expect((size_t)s.st_size == strlen(f_contents),
                s.st_size, f_contents);

            gp_assert(remove("gp_io_test_file.txt") == 0);
        }

        gp_test("Read line");
        {
            FILE* f;
            gp_assert(f = tmpfile());
            GPString str = gp_str_on_stack(arena, 1, "");
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
            fclose(f);
        }

        gp_test("Read until");
        {
            FILE* f;
            gp_assert(f = tmpfile());
            f_contents = "fooDELIMbarDELIMbloink";
            GPString str = gp_str_on_stack(arena, 1, "");
            while (gp_file_read_until(&str, f, "DELIM"))
            {
                const char* delim_pos = strstr(f_contents, "DELIM");
                const size_t segment_length = delim_pos != NULL ?
                    delim_pos - f_contents + strlen("DELIM")
                  : strlen(f_contents);

                gp_expect(gp_str_equal(str, f_contents, segment_length), str);
                f_contents += segment_length;
            }
            fclose(f);
        }

        gp_test("Read word");
        {
            FILE* f;
            gp_assert(f = tmpfile());
            f_contents = "Here\t is some" GP_WHITESPACE "words. Yeah.";
            GPString contents = gp_str_new(arena, 1, f_contents);
            GPString str = gp_str_on_stack(arena, 1, "");
            while (gp_file_read_strip(&str, f, NULL)) // NULL defaults to GP_WHITESPACE
            {
                GPString segment = gp_str_on_stack(NULL, 64, "");
                size_t pos = gp_str_find_first_of(contents, GP_WHITESPACE, 0);
                gp_str_slice(&segment, contents, 0, pos);
                gp_str_trim(&segment, GP_WHITESPACE, 'l' |'r');

                gp_expect(gp_str_equal(str, segment, gp_str_length(segment)));

                gp_str_slice(&contents, NULL, pos, gp_str_length(contents));
                gp_str_trim(&contents, GP_WHITESPACE, 'l');
            }
            fclose(f);
        }
    }

    gp_suite("Printing");
    {
        // No asserts here, uncomment exit(1) to see output.
        gp_print  (1,2,3,"Hello\n");
        gp_println(1, 2, 3, "Hello");

        // exit(1);
    }

    // Shut up Valgrind
    gp_arena_delete(&_arena);
}
