// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../include/gpc/terminal.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define GPC_NOT_RUNNING NULL
static GPC_THREAD_LOCAL const char* gpc_current_test  = GPC_NOT_RUNNING;
static GPC_THREAD_LOCAL const char* gpc_current_suite = GPC_NOT_RUNNING;
static GPC_THREAD_LOCAL bool gpc_test_failed  = false;
static GPC_THREAD_LOCAL bool gpc_suite_failed = false;
static GPC_ATOMIC unsigned gpc_test_count    = 0;
static GPC_ATOMIC unsigned gpc_suite_count   = 0;
static GPC_ATOMIC unsigned gpc_tests_failed  = 0;
static GPC_ATOMIC unsigned gpc_suites_failed = 0;
static GPC_ATOMIC bool gpc_initialized_testing = false;
#define GPC_FAILED_STR GPC_RED          "[FAILED]" GPC_RESET_TERMINAL
#define GPC_PASSED_STR GPC_BRIGHT_GREEN "[PASSED]" GPC_RESET_TERMINAL

// ----------------------------------------------------------------------------
// Implementations for gpc_suite(), gpc_test(), and relevant

void gpc_end_testing(void)
{
    if (gpc_test_count + gpc_suite_count == 0)
        return;

    gpc_test(NULL);
    gpc_suite(NULL);

    printf("A total of %u tests ran in %u suites\n", gpc_test_count, gpc_suite_count);

    if (gpc_tests_failed || gpc_suites_failed)
        fprintf(stderr, GPC_RED "%u tests failed and %u suites failed!\n", gpc_tests_failed, gpc_suites_failed);
    else
        printf(GPC_BRIGHT_GREEN "Passed all tests!" GPC_RESET_TERMINAL "\n");

    // Prevent redundant reporting at exit. Also user may want to restart tests.
    gpc_test_count    = 0;
    gpc_suite_count   = 0;
    gpc_tests_failed  = 0;
    gpc_suites_failed = 0;
    gpc_initialized_testing = false;
}

static void gpc_init_testing(void)
{
    if ( ! gpc_initialized_testing)
    {
        printf("\tStarting tests...\n\n");
        atexit(gpc_end_testing);
        gpc_initialized_testing = true;
    }
}

void gpc_test(const char* name)
{
    gpc_init_testing();

    // End current test
    if (gpc_current_test != GPC_NOT_RUNNING)
    {
        const char* indent = gpc_current_suite == GPC_NOT_RUNNING ? "" : "\t";
        if (gpc_test_failed)
        {
            gpc_tests_failed++;
            fprintf(stderr, "%s" GPC_FAILED_STR " test " GPC_CYAN "%s" GPC_RESET_TERMINAL "\n", indent, gpc_current_test);
        }
        else
            printf("%s" GPC_PASSED_STR " test " GPC_CYAN "%s" GPC_RESET_TERMINAL "\n", indent, gpc_current_test);

        gpc_current_test = GPC_NOT_RUNNING;
    }

    // Start new test
    if (name != NULL)
    {
        // No starting message cluttering output

        gpc_current_test = name;
        gpc_test_failed = false;
        gpc_test_count++;
    }
}

void gpc_suite(const char* name)
{
    gpc_init_testing();

    // End current suite
    if (gpc_current_suite != GPC_NOT_RUNNING)
    {
        if (gpc_suite_failed)
        {
            gpc_suites_failed++;
            fprintf(stderr, GPC_FAILED_STR " suite " GPC_CYAN "%s" GPC_RESET_TERMINAL "\n\n", gpc_current_suite);
        }
        else
            printf(GPC_PASSED_STR " suite " GPC_CYAN "%s" GPC_RESET_TERMINAL "\n\n", gpc_current_suite);

        gpc_current_suite = GPC_NOT_RUNNING;
    }

    // Start new suite
    if (name != NULL)
    {
        printf("Starting suite " GPC_CYAN "%s" GPC_RESET_TERMINAL "\n", name);

        gpc_current_suite = name;
        gpc_suite_failed = false;
        gpc_suite_count++;
    }
}

// ----------------------------------------------------------------------------
// Implementations for gpc_assert(), gpc_expect(), and relevant

// Copy n characters from src to dest and return a pointer to the end of the
// copied string in dest. src is assumed to be null terminated if n < 0.
static void gpc_str_push(char *restrict *dest, const char* restrict src, ptrdiff_t n)
{
    if (n < 0)
    {
        strcpy(*dest, src);
        *dest += strlen(src);
    }
    else
    {
        strncpy(*dest, src, (size_t)n);
        *dest += n;
    }
}

char* gpc_generate_var_info(const char* var_name, const char* format, /* T var */...)
{
    va_list arg, arg_;
    va_start(arg, format);
    va_copy(arg_, arg);

    char* var_info = NULL;

    size_t modified_format_buf_len = strlen(var_name)
        + strlen(GPC_BRIGHT_GREEN GPC_RESET_TERMINAL)    // color for var_name
        + strlen(" = \'\'") // added quotes if arg is char or char*
        + strlen(GPC_BRIGHT_WHITE_BG GPC_RESET_TERMINAL) // color for arg value
        + strlen(format) + sizeof("\n");

    char* modified_format = malloc(modified_format_buf_len);
    if (modified_format == NULL)
    {
        perror("malloc() failed on gpc_generate_var_info().");
        goto cleanup;
    }

    // Only a string literal passed, requires C11 at the moment
    if (var_name[0] == '\"')
    {
        // Allocation and copying is only required because we need to remove
        // quotes and the returned pointer will be freed by gpc_failure().
        // Returning a struct with info if allocated and a str_view would be
        // better but we'll settle with this now and optimize later.
        size_t var_name_len = strlen(var_name);
        size_t var_info_len = var_name_len - strlen("\"\"");
        var_info = malloc(var_info_len + sizeof('\0'));
        if (var_info != NULL)
        {
            strncpy(var_info, var_name + 1, var_info_len);
            var_info[var_info_len] = '\0';
        }
        goto cleanup;
    }

    // Build the modified format
    {
        char* p = modified_format;
        gpc_str_push(&p, GPC_BRIGHT_GREEN, -1);
        gpc_str_push(&p, var_name, -1);
        gpc_str_push(&p, GPC_RESET_TERMINAL, -1);
        gpc_str_push(&p, " = ", -1);

        const char all_supported_formats[] = "dibBouxXfFeEgGaAcCsSZp";

        // Find format specifier ignoring %%
        const char* format_specifier = format;
        while ((format_specifier = strchr(format_specifier, '%'))[1] == '%')
            format_specifier += strlen("%%");
        if (format_specifier == NULL)
        {
            fprintf(stderr, "\"%s\" has no format specifier!\n", format);
            goto cleanup;
        }

        const char* format_specifier_end = strpbrk(format_specifier, all_supported_formats) + 1;
        const char type = *(format_specifier_end - 1);

        // Additional notes before format specifier
        gpc_str_push(&p, format, format_specifier - format);

        // Color and opening quote if string or char
        if (type == 'c' || type == 'C') // character
            gpc_str_push(&p, GPC_YELLOW "\'", -1);
        else if (type == 's' || type == 'S' || type == 'Z') // string
            gpc_str_push(&p, GPC_BRIGHT_RED "\"", -1);
        else if (strchr("dibBouxX", type)) // integer
            gpc_str_push(&p, GPC_BRIGHT_BLUE, -1);
        else if (strchr("fFeEgG", type)) // floating point
            gpc_str_push(&p, GPC_BRIGHT_MAGENTA, -1);
        else if (type == 'p') // pointer
            gpc_str_push(&p, GPC_BLUE, -1);

        gpc_str_push(&p, format_specifier, format_specifier_end - format_specifier);

        // Closing quote
        if (type == 'c' || type == 'C')
            gpc_str_push(&p,  "\'", -1);
        else if (type == 's' || type == 'S' || type == 'Z')
            gpc_str_push(&p, "\"", -1);

        // Reset color
        gpc_str_push(&p, GPC_RESET_TERMINAL, -1);

        // Additional notes after format specifier
        gpc_str_push(&p, format_specifier_end, -1);

        gpc_str_push(&p, "\n", -1);
    }

    int var_info_length = (size_t)vsnprintf(NULL, 0, modified_format, arg_);
    if (var_info_length < 0)
    {
        perror("vsprintf() failed in gpc_generate_var_info()!");
        goto cleanup;
    }

    if ((var_info = malloc((size_t)var_info_length + sizeof('\0'))) == NULL)
    {
        perror("malloc() failed in gpc_generate_var_info().");
        goto cleanup;
    }

    if (vsprintf(var_info, modified_format, arg) < 0)
    {
        perror("vsprintf() failed in gpc_generate_var_info()!");
        var_info = NULL;
    }
    else
        var_info[var_info_length] = '\0';

    cleanup:

    va_end(arg);
    va_end(arg_);
    free(modified_format);

    return var_info;
}

void gpc_failure(
    bool aborting,
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const char* condition,
    ...)
{
    if (gpc_current_test != GPC_NOT_RUNNING)
    {
        gpc_test_failed = true;
        func = gpc_current_test;
    }
    if (gpc_current_suite != GPC_NOT_RUNNING)
    {
        gpc_suite_failed = true;
        if (gpc_current_test == GPC_NOT_RUNNING)
            func = gpc_current_suite;
    }

    fprintf(stderr,
            "Condition " GPC_RED"%s"GPC_RESET_TERMINAL
            " in %s " GPC_WHITE_BG GPC_BLACK "line %i" GPC_RESET_TERMINAL" %s "
            GPC_RED"[FAILED]" GPC_RESET_TERMINAL"\n",
            condition, file, line, func);

    va_list va_args;
    va_start(va_args, condition);

    // Ignore dummy parameter required by GPC_PROCESS_ALL_BUT_1ST()
    (void)va_arg(va_args, void*);
    arg_count--;

    while (arg_count--)
    {
        char* arg_info = va_arg(va_args, char*);
        if (arg_info != NULL)
            fputs(arg_info, stderr);
        free(arg_info);
    }
    fputs("\n", stderr);

    if (aborting)
        exit(EXIT_FAILURE);

    va_end(va_args);
}
