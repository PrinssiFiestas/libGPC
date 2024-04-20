// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include <gpc/terminal.h>
#include <gpc/utils.h>
#include <printf/printf.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

static GP_THREAD_LOCAL const char* gp_current_test  = NULL;
static GP_THREAD_LOCAL const char* gp_current_suite = NULL;
static GP_THREAD_LOCAL bool gp_test_failed  = false;
static GP_THREAD_LOCAL bool gp_suite_failed = false;
static GP_ATOMIC unsigned gp_test_count    = 0;
static GP_ATOMIC unsigned gp_suite_count   = 0;
static GP_ATOMIC unsigned gp_tests_failed  = 0;
static GP_ATOMIC unsigned gp_suites_failed = 0;
static GP_ATOMIC bool gp_initialized_testing = false;
#define GP_FAILED_STR GP_RED          "[FAILED]" GP_RESET_TERMINAL
#define GP_PASSED_STR GP_BRIGHT_GREEN "[PASSED]" GP_RESET_TERMINAL

// ----------------------------------------------------------------------------
// Implementations for gp_suite(), gp_test(), and relevant

void gp_end_testing(void)
{
    if (gp_test_count + gp_suite_count == 0)
        return;

    gp_test(NULL);
    gp_suite(NULL);

    printf("A total of %u tests ran in %u suites\n", gp_test_count, gp_suite_count);

    if (gp_tests_failed || gp_suites_failed)
        fprintf(stderr,
            GP_RED "%u tests failed and %u suites failed!" GP_RESET_TERMINAL "\n",
            gp_tests_failed, gp_suites_failed);
    else
        printf(GP_BRIGHT_GREEN "Passed all tests!" GP_RESET_TERMINAL "\n");

    // Prevent redundant reporting at exit. Also user may want to restart tests.
    gp_test_count    = 0;
    gp_suite_count   = 0;
    gp_tests_failed  = 0;
    gp_suites_failed = 0;
    gp_initialized_testing = false;
}

static void gp_init_testing(void)
{
    if ( ! gp_initialized_testing)
    {
        printf("\tStarting tests...\n\n");
        atexit(gp_end_testing);
        gp_initialized_testing = true;
    }
}

void gp_test(const char* name)
{
    gp_init_testing();

    // End current test
    if (gp_current_test != NULL)
    {
        const char* indent = gp_current_suite == NULL ? "" : "\t";
        if (gp_test_failed) {
            gp_tests_failed++;
            fprintf(stderr,
            "%s" GP_FAILED_STR " test " GP_CYAN "%s" GP_RESET_TERMINAL "\n", indent, gp_current_test);
        } else {
            printf(
            "%s" GP_PASSED_STR " test " GP_CYAN "%s" GP_RESET_TERMINAL "\n", indent, gp_current_test);
        }

        gp_current_test = NULL;
    }

    // Start new test
    if (name != NULL)
    {
        // No starting message cluttering output

        gp_current_test = name;
        gp_test_failed = false;
        gp_test_count++;
    }
}

void gp_suite(const char* name)
{
    gp_init_testing();
    gp_test(NULL); // End current test

    // End current suite
    if (gp_current_suite != NULL)
    {
        if (gp_suite_failed) {
            gp_suites_failed++;
            fprintf(stderr, GP_FAILED_STR " suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n\n", gp_current_suite);
        } else {
            printf(GP_PASSED_STR " suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n\n", gp_current_suite);
        }
        gp_current_suite = NULL;
    }

    // Start new suite
    if (name != NULL)
    {
        printf("Starting suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n", name);

        gp_current_suite = name;
        gp_suite_failed = false;
        gp_suite_count++;
    }
}

// ----------------------------------------------------------------------------
// Implementations for gp_assert(), gp_expect(), and relevant

void gp_fail_internal(
    int aborting,
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const struct GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    if (gp_current_test != NULL)
    {
        gp_test_failed = true;
        func = gp_current_test;
    }
    if (gp_current_suite != NULL)
    {
        gp_suite_failed = true;
        if (gp_current_test == NULL)
            func = gp_current_suite;
    }

    const char* condition = objs[0].identifier;
    // 1st arg is just condition, ignore that.
    if (gp_sizeof(objs[0].type) == sizeof(uint64_t))
        (void)va_arg(args.list, uint64_t);
    else // 32 bits due to arg promotion
        (void)va_arg(args.list, uint32_t);

    fprintf(stderr,
        "Condition " GP_RED "%s" GP_RESET_TERMINAL
        " in %s " GP_WHITE_BG GP_BLACK "line %i" GP_RESET_TERMINAL " %s "
        GP_FAILED_STR "\n",
        condition, file, line, func);

    char* buf = NULL;
    size_t buf_capacity = 0;
    for (size_t i = 1; i < arg_count; i++)
    {
        if (objs[i].identifier[0] == '\"')
        {
            const char* fmt = va_arg(args.list, char*);
            size_t fmt_spec_count = 0;
            char* fmt_spec = NULL;
            const char* l_braces = "([{<";
            const char* r_braces = ")]}>";
            const char* brace = strchr(l_braces, fmt[0]);

            for (const char* c = fmt; (c = strchr(c, '%')) != NULL; c++)
            {
                if (c[1] == '%') {
                    c++;
                } else {
                    fmt_spec_count++;
                    fmt_spec = strpbrk(c, "csdioxXufFeEgGp");
                    if (fmt_spec == NULL) {
                        fprintf(stderr, "Invalid format specifier \"%s\".", fmt);
                        continue;
                    }
                }
            }
            size_t printed = 0;
            if (fmt_spec_count == 0) // user comment
            {
                fprintf(stderr, "%s\n", fmt);
                continue;
            }
            else if (fmt_spec_count == 1)
            {
                fprintf(stderr,
                    GP_BRIGHT_WHITE "%s" GP_RESET_TERMINAL " = ",
                    objs[i + 1/*0 is fmt so next one*/].identifier);

                // Color and opening quote if string or char
                if (*fmt_spec == 'c') // character
                    fprintf(stderr, GP_YELLOW);
                else if (*fmt_spec == 's') // string
                    fprintf(stderr, GP_BRIGHT_RED);
                else if (strchr("dibBouxX", *fmt_spec)) // integer
                    fprintf(stderr, GP_BRIGHT_BLUE);
                else if (strchr("fFeEgG", *fmt_spec)) // floating point
                    fprintf(stderr, GP_BRIGHT_MAGENTA);
                else if (*fmt_spec == 'p') // pointer
                    fprintf(stderr, GP_BLUE);
            }
            else
            {
                if (brace != NULL) {
                    fputc(*brace, stderr);
                    printed++;
                    if (fmt[1] == ' ') {
                        fputc(' ', stderr);
                        printed++;
                    }
                }
                for (size_t j = 0; j < fmt_spec_count - 1; j++)
                    printed += fprintf(stderr,"%s, ",objs[i + 1 + j].identifier);
                printed += fprintf(stderr, "%s", objs[i + fmt_spec_count].identifier);

                if (brace != NULL) {
                    if (fmt[1] == ' ') {
                        printed++;
                        fputc(' ', stderr);
                    }
                    size_t brace_i = brace - l_braces;
                    fputc(r_braces[brace_i], stderr);
                    printed++;
                }
                fprintf(stderr, GP_RESET_TERMINAL " = " GP_BRIGHT_CYAN);
                printed += strlen(" = ");
            }

            size_t required_capacity = pf_vsnprintf(NULL, 0, fmt, args.list)+1;
            if (required_capacity >= buf_capacity) {
                buf = realloc(
                    buf, buf_capacity = gp_next_power_of_2(required_capacity));
            }
            if (printed + required_capacity > 120)
                fprintf(stderr, "\n\t");

            pf_vsnprintf_consuming(buf, buf_capacity, fmt, &args);
            fprintf(stderr, "%s", buf);

            fprintf(stderr, GP_RESET_TERMINAL "\n");

            i += fmt_spec_count;
            continue;
        } // end if string literal

        fprintf(stderr,
            GP_BRIGHT_WHITE "%s" GP_RESET_TERMINAL " = ", objs[i].identifier);

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                fprintf(stderr,
                    GP_YELLOW "\'%c\'", (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                fprintf(stderr, GP_BRIGHT_BLUE "%u", va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                fprintf(stderr,
                    GP_BRIGHT_BLUE "%lu", va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                fprintf(stderr,
                    GP_BRIGHT_BLUE "%llu", va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                fprintf(stderr, va_arg(args.list, int) ? "true" : "false");
                break;

            case GP_SHORT:
            case GP_INT:
                fprintf(stderr, GP_BRIGHT_BLUE "%i", va_arg(args.list, int));
                break;

            case GP_LONG:
                fprintf(stderr, GP_BRIGHT_BLUE "%li", va_arg(args.list, long));
                break;

            case GP_LONG_LONG:
                fprintf(stderr, GP_BRIGHT_BLUE "%lli", va_arg(args.list, long long));
                break;

            double f;
            case GP_FLOAT:
            case GP_DOUBLE:
                f = va_arg(args.list, double);
                fprintf(stderr, GP_BRIGHT_MAGENTA "%g", f);
                if (f - (int64_t)f == f/* whole number */&&
                    (int64_t)f < 100000) { // not printed using %e style
                    fprintf(stderr, ".0");
                } break;

            const char* char_ptr;
            case GP_CHAR_PTR:
                char_ptr = va_arg(args.list, char*);
                if (char_ptr != NULL)
                    fprintf(stderr, GP_BRIGHT_RED "\"%s\"", char_ptr);
                else
                    fprintf(stderr, GP_BRIGHT_RED "(null)");
                break;

            case GP_PTR:
                fprintf(stderr, GP_BLUE "%p", va_arg(args.list, void*));
                break;
        }
        fprintf(stderr, GP_RESET_TERMINAL "\n");
    } // end for args

    if (aborting)
        exit(EXIT_FAILURE);
    free(buf);
    va_end(_args);
    va_end(args.list);
}


















// OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
// ----------------------------------------------------------------------------
// Implementations for gp_assert(), gp_expect(), and relevant

// Copy n characters from src to dest and return a pointer to the end of the
// copied string in dest. src is assumed to be null terminated if n < 0.
static void gp_str_push(char *restrict *dest, const char* restrict src, ptrdiff_t n)
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

char* gp_generate_var_info(const char* var_name, const char* format, /* T var */...)
{
    va_list arg, arg_;
    va_start(arg, format);
    va_copy(arg_, arg);

    char* var_info = NULL;

    size_t modified_format_buf_len = strlen(var_name)
        + strlen(GP_BRIGHT_GREEN GP_RESET_TERMINAL)    // color for var_name
        + strlen(" = \'\'") // added quotes if arg is char or char*
        + strlen(GP_BRIGHT_WHITE_BG GP_RESET_TERMINAL) // color for arg value
        + strlen(format) + sizeof("\n");

    char* modified_format = malloc(modified_format_buf_len);
    if (modified_format == NULL)
    {
        perror("malloc() failed on gp_generate_var_info().");
        goto cleanup;
    }

    // Only a string literal passed, requires C11 at the moment
    if (var_name[0] == '\"')
    {
        // Allocation and copying is only required because we need to remove
        // quotes and the returned pointer will be freed by gp_failure().
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
        gp_str_push(&p, GP_BRIGHT_WHITE, -1);
        gp_str_push(&p, var_name, -1);
        gp_str_push(&p, GP_RESET_TERMINAL, -1);
        gp_str_push(&p, " = ", -1);

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
        gp_str_push(&p, format, format_specifier - format);

        // Color and opening quote if string or char
        if (type == 'c' || type == 'C') // character
            gp_str_push(&p, GP_YELLOW "\'", -1);
        else if (type == 's' || type == 'S' || type == 'Z') // string
            gp_str_push(&p, GP_BRIGHT_RED "\"", -1);
        else if (strchr("dibBouxX", type)) // integer
            gp_str_push(&p, GP_BRIGHT_BLUE, -1);
        else if (strchr("fFeEgG", type)) // floating point
            gp_str_push(&p, GP_BRIGHT_MAGENTA, -1);
        else if (type == 'p') // pointer
            gp_str_push(&p, GP_BLUE, -1);

        gp_str_push(&p, format_specifier, format_specifier_end - format_specifier);

        // Closing quote
        if (type == 'c' || type == 'C')
            gp_str_push(&p,  "\'", -1);
        else if (type == 's' || type == 'S' || type == 'Z')
            gp_str_push(&p, "\"", -1);

        // Reset color
        gp_str_push(&p, GP_RESET_TERMINAL, -1);

        // Additional notes after format specifier
        gp_str_push(&p, format_specifier_end, -1);

        gp_str_push(&p, "\n", -1);
    }

    int var_info_length = (size_t)vsnprintf(NULL, 0, modified_format, arg_);
    if (var_info_length < 0)
    {
        perror("vsprintf() failed in gp_generate_var_info()!");
        goto cleanup;
    }

    if ((var_info = malloc((size_t)var_info_length + sizeof('\0'))) == NULL)
    {
        perror("malloc() failed in gp_generate_var_info().");
        goto cleanup;
    }

    if (vsprintf(var_info, modified_format, arg) < 0)
    {
        perror("vsprintf() failed in gp_generate_var_info()!");
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

void gp_failure(
    bool aborting,
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const char* condition,
    ...)
{
    if (gp_current_test != NULL)
    {
        gp_test_failed = true;
        func = gp_current_test;
    }
    if (gp_current_suite != NULL)
    {
        gp_suite_failed = true;
        if (gp_current_test == NULL)
            func = gp_current_suite;
    }

    fprintf(stderr,
            "Condition " GP_RED"%s"GP_RESET_TERMINAL
            " in %s " GP_WHITE_BG GP_BLACK "line %i" GP_RESET_TERMINAL" %s "
            GP_RED"[FAILED]" GP_RESET_TERMINAL"\n",
            condition, file, line, func);

    va_list va_args;
    va_start(va_args, condition);

    // Ignore dummy parameter required by GP_PROCESS_ALL_BUT_1ST()
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
