// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// First write everything before any #includes from every source file to an
// #ifdef X_IMPLEMENTATION block that comes before everything else. This makes
// sure that things like #define STB_Y_IMPLEMENTATION and #define _GNU_SOURCE
// come before any header. Then start inlining every local #include.
//
// If there is any header files left after inlining, those have to be written
// next. These could be header files that only contain static inline functions
// or macros or whatever that may be part of the library, but not used by the
// library.
//
// After all header files have been written, write the rest from all of the
// source files in an #ifdef X_IMPLEMENTATION block.

#include <gpc/string.h>
#include <gpc/io.h>
#include <gpc/array.h>
#include <gpc/assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#if !_MSC_VER
#include <dirent.h>
#else
#define stat _stat
// TODO use this on MSVC
// https://github.com/tronkko/dirent/blob/master/include/dirent.h
#endif

static FILE* out = NULL;
static const char* out_name = "gpc.h";
static GPArena garena;
static const GPAllocator*const gmem = (GPAllocator*)&garena;

typedef struct file
{
    char* name;
    size_t name_length;
    FILE* fp;
    bool is_inlined;
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
        remove(out_name); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

static void init_globals(void)
{
    garena = gp_arena_new(1 << 30);
    garena.growth_coefficient = .25;

    gp_assert(out = fopen(out_name, "w"), strerror(errno));

    include_paths = gp_arr_new(gmem, sizeof*include_paths, 16);
    headers = gp_arr_new(gmem, sizeof*headers, 64);
    sources = gp_arr_new(gmem, sizeof*headers, 64);

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
        gp_str_slice(&full_path, NULL, 0, strlen("include/"));
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
            gp_println(full_path);

            File header = {
                .name = strcpy(gp_mem_alloc(gmem, strlen(entry->d_name) + sizeof""), entry->d_name),
                .name_length = strlen(entry->d_name),
                .fp          = fopen(gp_cstr(full_path), "r")
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
        if (file_extension[1] != 'c' && file_extension[1] != 'C')
            continue;

        gp_str_slice(&full_path, NULL, 0, strlen("src/"));
        gp_str_append(&full_path, entry->d_name, strlen(entry->d_name));
        gp_println(full_path);
        File source = {
            .name = strcpy(gp_mem_alloc(gmem, strlen(entry->d_name) + sizeof""), entry->d_name),
            .name_length = strlen(entry->d_name),
            .fp          = fopen(gp_cstr(full_path), "r")
        };
        Assert(source.fp != NULL, strerror(errno));

        sources = gp_arr_push(sizeof*sources, sources, &source);
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
        if ((path = strstr(entry->d_name, "license")) ||
            (path = strstr(entry->d_name, "LICENSE")) ||
            (path = strstr(entry->d_name, "License")))
            break;
    }

    if (path != NULL)
    {
        FILE* license;
        Assert(license = fopen(path, "r"), strerror(errno));
        GPString line = gp_str_on_stack(gmem, 128, "");
        gp_file_println(out, "/*");

        while (gp_file_read_line(&line, license))
            gp_file_print(out, " * ", line);

        gp_file_println(out, "\n */\n\n");
        fclose(license);
        if (gp_str_allocation(line)) // pedantic free
            gp_arena_rewind(&garena, gp_str_allocation(line));
    }
    closedir(dir);
}

int main(void)
{
    init_globals();
    write_license();
    fputs("/*\n"
        " * This file has been generated. The original code may have gone trough heavy\n"
        " * restructuring, so some parts of this file might be confusing to read.\n"
        " */\n\n\n",
        out);
}
