// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// First write everything before any #includes from every source file to an
// #ifdef X_IMPLEMENTATION block that comes before everything else. This makes
// sure that things like #define Y_IMPLEMENTATION and #define _GNU_SOURCE
// come before any header.
//
// Header files have to be written next. Every time an #include directive
// references a local header file, it has to be inlined recursively. Private
// headers (headers in src/) has to be in X_IMPLEMENTATION block to hide
// implementation specific possibly C-only headers from user. Note that no
// assumptions can be made about conditional compilation which means that if the
// first occurrence of #include <mylib/mymodule.h> is in an #if block, it will
// be inlined in that #if block essentialy removing it if that #if condition
// evaluates to false!
//
// After all header files have been written, write the rest from all of the
// source files in an #ifdef X_IMPLEMENTATION block. #include directives
// referring to local headers have to be filtered out or inlined if not already.

#include "../include/gpc/string.h"
#include "../include/gpc/io.h"
#include "../include/gpc/array.h"
#include "../include/gpc/assert.h"
#include "../include/gpc/terminal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#if _WIN32
#define stat _stat
#endif

static FILE*        out;
static const char*  out_path;
static const char*  version_number;
static GPString     implementation;
static GPAllocator* gmem;
static GPString     line;

typedef struct file
{
    char*  name;
    size_t name_length;
    char*  include_dir; // NULL for src/
    FILE*  fp;
} File;

static GPArray(char*)include_paths;
static GPArray(File) headers;
static GPArray(File) sources;

// It makes no sense to keep corrupted generated header if something goes
// fatally wrong, so remove it.
#define Assert(...) \
do { \
    if ( ! gp_expect(__VA_ARGS__)) { \
        fclose(out); \
        remove(out_path); \
        GP_BREAKPOINT; \
        exit(EXIT_FAILURE); \
    } \
} while(0)

static void init_globals(void)
{
    gmem = &gp_arena_new(NULL, 1 << 15)->base;

    gp_assert(out = fopen(out_path, "wb"), strerror(errno));

    include_paths = gp_arr_new(gmem, sizeof*include_paths, 16);
    headers       = gp_arr_new(gmem, sizeof*headers, 64);
    sources       = gp_arr_new(gmem, sizeof*sources, 64);
    line          = gp_str_new(gmem, 1024, "");

    implementation = gp_str_new(gmem, 32, "");
    { // e.g. gpc.h -> GPC_IMPLEMENTATION
        const char* out_name;
        if ((out_name = strrchr(out_path, '/')) != NULL)
            out_name += strlen("/");
        else
            out_name = out_path;
        gp_str_copy(&implementation, out_name, strlen(out_name));
        gp_str_slice(&implementation, NULL, 0, gp_str_find_first(implementation, ".", strlen("."), 0));
        gp_str_to_upper(&implementation);
        gp_str_append(&implementation, "_IMPLEMENTATION", strlen("_IMPLEMENTATION"));
    }

    DIR* dir;
    Assert(dir = opendir("include/"), strerror(errno));
    for (struct dirent* entry = readdir(dir); entry != NULL; entry = readdir(dir))
    {
        if (entry->d_name[0] == '.') // ignore ".", "..", and hidden directories
            continue;

        char* buf = gp_mem_alloc(gmem, strlen(entry->d_name) + sizeof"/");
        strcat(strcpy(buf, entry->d_name), "/");
        include_paths = gp_arr_push(sizeof*include_paths, include_paths, &buf);
    }
    closedir(dir);

    GPString full_path = gp_str_on_stack(gmem, 256, "include/");
    for (size_t i = 0; i < gp_arr_length(include_paths); i++)
    {
        gp_str_slice(&full_path, NULL, 0, strlen("include/")); // clean last iteration
        gp_str_append(&full_path, include_paths[i], strlen(include_paths[i]));

        Assert(dir = opendir(gp_cstr(full_path)), strerror(errno));

        for (struct dirent* entry = readdir(dir); entry != NULL; entry = readdir(dir))
        {
            if (entry->d_name[0] == '.') // ignore ".", "..", and hidden files
                continue;
            const char* file_extension = strchr(entry->d_name, '.');
            if (file_extension[1] != 'h')
                continue;

            gp_str_slice(&full_path, NULL, 0, strlen("include/") + strlen(include_paths[i]));
            gp_str_append(&full_path, entry->d_name, strlen(entry->d_name));

            File header = {
                .name = strcpy(gp_mem_alloc(gmem, strlen(entry->d_name) + sizeof""), entry->d_name),
                .name_length = strlen(entry->d_name),
                .include_dir = include_paths[i],
                .fp          = fopen(gp_cstr(full_path), "rb")
            };
            Assert(header.fp != NULL, strerror(errno));

            headers = gp_arr_push(sizeof*headers, headers, &header);
        }

        closedir(dir);
    }
    Assert(dir = opendir("src/"), strerror(errno));
    gp_str_copy(&full_path, "src/", strlen("src/"));
    for (struct dirent* entry = readdir(dir); entry != NULL; entry = readdir(dir))
    {
        if (entry->d_name[0] == '.') // ignore ".", "..", and hidden files
            continue;
        const char* file_extension = strchr(entry->d_name, '.');
        if (file_extension[1] != 'c' && file_extension[1] != 'h')
            continue;

        gp_str_slice(&full_path, NULL, 0, strlen("src/"));
        gp_str_append(&full_path, entry->d_name, strlen(entry->d_name));
        File source = {
            .name = strcpy(gp_mem_alloc(gmem, strlen(entry->d_name) + sizeof""), entry->d_name),
            .name_length = strlen(entry->d_name),
            .fp          = fopen(gp_cstr(full_path), "rb")
        };
        Assert(source.fp != NULL, strerror(errno));

        if (file_extension[1] == 'c')
            sources = gp_arr_push(sizeof*sources, sources, &source);
        else
            headers = gp_arr_push(sizeof*headers, headers, &source);
    }
    closedir(dir);
}

static void write_license(void)
{
    DIR* dir;
    Assert(dir = opendir("."), strerror(errno));
    const char* path;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if ((path = strstr(entry->d_name, "license")) != NULL ||
            (path = strstr(entry->d_name, "LICENSE")) != NULL ||
            (path = strstr(entry->d_name, "License")) != NULL)
            break;
    }
    if (path == entry->d_name)
    {
        FILE* license;
        Assert(license = fopen(path, "rb"), strerror(errno));
        gp_file_println(out, "/*");

        while (gp_file_read_line(&line, license))
            gp_file_print(out, " * ", line);

        gp_file_println(out, "\n */\n");
        fclose(license);
    }
    closedir(dir);
}

static bool no_match(const GPString line, const size_t i)
{
    return i == GP_NOT_FOUND || line[i].c == '\n' ||
        (line[i].c == '/' && line[i + 1].c == '/');
}

static size_t find_multiline_comment_end(
    const GPString line, const size_t start, bool* is_in_multiline_comment)
{
    Assert(*is_in_multiline_comment);
    if (start == GP_NOT_FOUND)
        return GP_NOT_FOUND;

    const size_t pos = gp_str_find_first(line, "*/", strlen("*/"), start);
    if (pos != GP_NOT_FOUND) {
        *is_in_multiline_comment = false;
        return pos + strlen("*/");
    }
    return GP_NOT_FOUND;
}

static size_t find_include_end(GPString, size_t, bool*);

// #include "blah.h"
// Returns ^ that index
static size_t find_include_directive(
    const GPString line, const size_t i, bool* is_in_multiline_comment)
{
    if (no_match(line, i))
        return GP_NOT_FOUND;

    if (*is_in_multiline_comment)
        return find_include_directive(
            line,
            find_multiline_comment_end(line, i, is_in_multiline_comment),
            is_in_multiline_comment);

    if (line[i].c == '#')
        return find_include_end(line, i + 1, is_in_multiline_comment);

    if (line[i].c == '/' && line[i + 1].c == '*') {
        *is_in_multiline_comment = true;
        return find_include_directive(line, i + strlen("/*"), is_in_multiline_comment);
    }
    if (line[i].c != ' ' && line[i].c != '\t')
        return GP_NOT_FOUND;

    return find_include_directive(line, i + 1, is_in_multiline_comment);
}

static size_t find_include_end(
    const GPString line, const size_t i, bool* is_in_multiline_comment)
{
    if (no_match(line, i))
        return GP_NOT_FOUND;

    if (*is_in_multiline_comment)
        return find_include_end(
            line,
            find_multiline_comment_end(line, i, is_in_multiline_comment),
            is_in_multiline_comment);

    if (memcmp(line + i, "include", strlen("include")) == 0)
        return i + strlen("include");

    if (line[i].c == '/' && line[i + 1].c == '*') {
        *is_in_multiline_comment = true;
        return find_include_end(line, i + strlen("/*"), is_in_multiline_comment);
    }
    if (line[i].c != ' ' && line[i].c != '\t')
        return GP_NOT_FOUND;

    return find_include_end(line, i + 1, is_in_multiline_comment);
}

static void write_sources_until_include(void)
{
    gp_file_println(out, "#ifdef", implementation, "\n");
    for (File* source = sources; source < sources + gp_arr_length(sources); source++)
    {
        gp_file_println(out, "/* * * * * * *\n *", source->name, "\n */\n");

        bool is_in_multiline_comment = false;
        while (true)
        {
            fpos_t line_start;
            Assert(fgetpos(source->fp, &line_start) == 0, strerror(errno));
            if ( ! gp_file_read_line(&line, source->fp))
                break;

            if (find_include_directive(line, 0, &is_in_multiline_comment) != GP_NOT_FOUND)
            {
                Assert(fsetpos(source->fp, &line_start) == 0, strerror(errno));
                break;
            }
            gp_file_print(out, line);
        }
    }
    gp_file_println(out, "\n#endif /*", implementation, "*/\n");
}

static size_t find_header(
    const char* name, const size_t name_length, const File* file, const char* include_dir)
{
    size_t i = 0;
    for (;; i++)
    {
        if (i >= gp_arr_length(headers)) {
            gp_file_print(stderr, GP_YELLOW "[WARNING]" GP_RESET_TERMINAL,
                " Could not inline\n" GP_YELLOW "#include ");
            if (file->include_dir == NULL)
                gp_file_print(stderr, "\"%.*s", name_length, name, "\"");
            else
                gp_file_print(stderr, "<%.*s", name_length, name, ">");
            gp_file_println(stderr, GP_RESET_TERMINAL);
            gp_file_println(stderr, "in %.*s", file->name_length, file->name);
            gp_file_println(stderr, "File %.*s", name_length, name, "not found.");
            return GP_NOT_FOUND;
        }
        if (headers[i].include_dir == include_dir &&
            headers[i].name_length == name_length &&
            memcmp(name, headers[i].name, name_length) == 0)
            break;
    }
    return i;
}

static size_t find_header_from_include_paths(
    const char* name, size_t name_length, const File* file)
{
    const size_t slash = gp_bytes_find_first(name, name_length, "/", strlen("/"), 0);
    if (slash == GP_NOT_FOUND)
        return GP_NOT_FOUND;
    const size_t path_length = slash + strlen("/");

    size_t _i;
    for (_i = 0; _i < gp_arr_length(include_paths); _i++)
    {
        if (gp_bytes_equal(name, path_length, include_paths[_i], strlen(include_paths[_i])))
            break;
    }
    if (_i >= gp_arr_length(include_paths))
        return GP_NOT_FOUND;

    name        += strlen(include_paths[_i]);
    name_length -= strlen(include_paths[_i]);

    size_t i = 0;
    for (;; i++)
    {
        if (i >= gp_arr_length(headers)) {
            gp_file_print(stderr, GP_YELLOW "[WARNING]" GP_RESET_TERMINAL,
                " Could not inline\n" GP_BRIGHT_CYAN "#include ");
            if (file->include_dir == NULL)
                gp_file_print(stderr, "\"%.*s", name_length, name, "\"");
            else
                gp_file_print(stderr, "<%.*s", name_length, name, ">");
            gp_file_println(stderr, GP_RESET_TERMINAL);
            gp_file_println(stderr, "in %.*s", file->name_length, file->name);
            gp_file_println(stderr, "File %.*s", name_length, name, "not found.");
            return GP_NOT_FOUND;
        }
        if (headers[i].include_dir == include_paths[_i] &&
            gp_bytes_equal(headers[i].name, headers[i].name_length, name, name_length))
            break;
    }
    return i;
}

static size_t find_header_index(
    const GPString line, File* file, bool* is_in_multiline_comment)
{
    const size_t include_end = find_include_directive(line, 0, is_in_multiline_comment);
    if (include_end == GP_NOT_FOUND)
        return GP_NOT_FOUND;

    size_t i = include_end;
    while (true)
    {
        if ((line[i].c == '/' && line[i + 1].c == '*') ||
            *is_in_multiline_comment)
        {
            *is_in_multiline_comment = true;
            i = find_multiline_comment_end(line, i, is_in_multiline_comment);
        }
        else if (line[i].c == '"' || line[i].c == '<')
        {
            const char* include_dir = file->include_dir;

            if (line[i++].c == '<')
            {
                if (include_dir == NULL) {
                    return find_header_from_include_paths(
                        (char*)line + i,
                        gp_str_find_first_of(line, ">", i) - i,
                        file);
                } else {
                    bool found = false;
                    size_t j = 0;
                    for (; j < gp_arr_length(include_paths); ++j) {
                        found = strncmp(gp_cstr(line) + i, include_paths[j], strlen(include_paths[j])) == 0;
                        if (found)
                            break;
                    }
                    if ( ! found)
                        return GP_NOT_FOUND;
                    include_dir = include_paths[j];
                }

                i += strlen(include_dir);
            }

            return find_header(
                (char*)line + i,
                gp_str_find_first_of(line, "\">", i) - i,
                file,
                include_dir);
        }
        else {
            i++;
        }
        Assert(i < gp_str_length(line), "Parsing error:", line);
    }
}

static void write_file(GPArray(File) files, const size_t index)
{
    File* file = files + index;
    if (file->fp == NULL)
        return;

    gp_file_println(out,
        "/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */\n"
        "/*", file->name, "*/\n");
    if ( ! file->include_dir) // private header
        gp_file_println(out, "#ifdef", implementation);

    bool is_in_multiline_comment = false;
    while (gp_file_read_line(&line, file->fp))
    {
        const size_t i = find_header_index(line, file, &is_in_multiline_comment);
        if (i == GP_NOT_FOUND) {
            gp_file_print(out, line);
        } else {
            if ( ! file->include_dir && headers[i].include_dir)
                gp_file_println(out, "\n#endif /*", implementation, "*/\n");

            write_file(headers, i);

            if ( ! file->include_dir && headers[i].include_dir)
                gp_file_println(out, "#ifdef", implementation, "\n");
        }
    }
    gp_file_println(out, "");
    if ( ! file->include_dir)
        gp_file_println(out, "#endif /*", implementation, "*/\n");
    fclose(file->fp);
    file->fp = NULL;
}

static void write_files(GPArray(File) files)
{
    for (size_t i = 0; i < gp_arr_length(files); i++)
        write_file(files, i);
}

int main(int argc, char* argv[])
{
    if (argc != 2 && argc != 3) {
        gp_file_println(stderr,
            "singleheadergen: you must provide exactly 1 output file path and optional version number.");
        return EXIT_FAILURE;
    }
    gp_enable_terminal_colors();

    out_path       = argv[1];
    version_number = argv[2];
    const size_t out_path_slash = gp_bytes_find_last(out_path, strlen(out_path), "/", 1);
    const char* out_name = out_path;
    if (out_path_slash != GP_NOT_FOUND)
        out_name += out_path_slash + strlen("/");

    init_globals();
    if (version_number != NULL)
        gp_file_println(out, "/*", out_name, version_number, "*/");
    write_license();
    fputs("/*\n"
        " * This file has been generated. The original code may have gone trough heavy\n"
        " * restructuring, so some parts of this file might be confusing to read.\n"
        " */\n"
        "\n"
        "#if __GNUC__ && !__clang__\n"
        "#pragma GCC system_header\n"
        "#endif\n\n",
        out);
    write_sources_until_include();
    write_files(headers);
    gp_file_println(out,
        "/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n"
        " *\n"
        " */\n",
        "        #ifdef", implementation, "\n"
        "/*\n"
        " *\n"
        " * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */\n");
    write_files(sources);
    gp_file_println(out, "\n#endif /*", implementation, "*/\n\n");
    fflush(out);
}
