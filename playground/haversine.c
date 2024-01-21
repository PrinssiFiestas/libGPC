#include <stdio.h>
#include <stdlib.h>
#include <gpc/string.h>
#include <gpc/utils.h>
#include <gpc/assert.h>
#include <gpc/memory.h>

#define MAX_PATH 256

int main(int argc, char* argv[])
{
    if (argc == 1)
        return fprintf(stderr, "No input\n");

    size_t fields = (size_t)atol(argv[1]);
    GPString out_file_name = gpstr_on_stack([MAX_PATH], "data_");
    gpstr_insert(&out_file_name, out_file_name.length, gpstr(argv[1]));
    gpstr_insert(&out_file_name, out_file_name.length, gpstr("_flex.json"));

    FILE* out_json = fopen(gpcstr(out_file_name), "w");
    gp_assert(out_json != NULL);

    const GPString json_header = gpstr("{\"pairs\":[\n");

    const size_t extra_braces_etc = sizeof("]\n\n}\n\n\nwhatever");
    #define FIELD_LENGTH (4 * sizeof("\t{ 'x0':25.57578843257872548372787548,  }\n"))
    const size_t full_length = json_header.length + fields * FIELD_LENGTH + extra_braces_etc;

    GPString json_contents = { malloc(full_length) };

    gpstr_copy(&json_contents, json_header);
    while (fields--)
    {
        GPString buf = gpstr_on_stack([FIELD_LENGTH], "");
        gpstr_interpolate(&buf, "\t{ "
              "\"x0\":", 180.* gp_g_frandom(), ", \"y0\":", 90.* gp_g_frandom(),
            ", \"x1\":", 180.* gp_g_frandom(), ", \"y1\":", 90.* gp_g_frandom(),
        " },\n");
        gpstr_insert(&json_contents, json_contents.length, buf);
    }
    gpstr_trim(&json_contents, ",\n", 'r');
    gpstr_insert(&json_contents, json_contents.length, gpstr("]\n}"));

    puts(gpcstr(json_contents));
    fwrite(json_contents.data, json_contents.length, 1, out_json);

    free(json_contents.data);
    fclose(out_json);
}
