// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GP_NO_FORMAT_STRING_CHECK // "%w128i"

#include <gpc/assert.h>
#include <gpc/terminal.h>
#include <gpc/string.h>
#include <gpc/utils.h>
#include <gpc/thread.h>
#include "common.h"
#include <printf/printf.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h> // GetModuleFileNameA(), SetConsoleMode(), CaptureStackBackTrace()
#else
#include <execinfo.h> // backtrace()
#endif

static GP_MAYBE_THREAD_LOCAL const char* gp_current_test  = NULL;
static GP_MAYBE_THREAD_LOCAL const char* gp_current_suite = NULL;
static GP_MAYBE_THREAD_LOCAL bool gp_test_failed  = false;
static GP_MAYBE_THREAD_LOCAL bool gp_suite_failed = false;
static GP_MAYBE_ATOMIC uint32_t gp_test_count    = 0;
static GP_MAYBE_ATOMIC uint32_t gp_suite_count   = 0;
static GP_MAYBE_ATOMIC uint32_t gp_tests_failed  = 0;
static GP_MAYBE_ATOMIC uint32_t gp_suites_failed = 0;
static GPThreadOnce gp_tests_once  = GP_THREAD_ONCE_INIT;
static GPThreadOnce gp_colors_once = GP_THREAD_ONCE_INIT;
#define GP_FAILED_STR GP_RED          "[FAILED]" GP_RESET_TERMINAL
#define GP_PASSED_STR GP_BRIGHT_GREEN "[PASSED]" GP_RESET_TERMINAL
static const char* prog_name = "";

// ----------------------------------------------------------------------------
// Implementations for gp_suite(), gp_test(), and relevant

void gp_end_testing(void)
{
    if (gp_test_count + gp_suite_count == 0)
        return;

    gp_test(NULL);
    gp_suite(NULL);

    pf_printf("Finished testing%s%s\n", *prog_name ? " in " : ".", prog_name);
    pf_printf("A total of %u tests ran in %u suites\n", gp_test_count, gp_suite_count);

    if (gp_tests_failed || gp_suites_failed)
        pf_fprintf(stderr,
            GP_RED "%u tests failed and %u suites failed!" GP_RESET_TERMINAL "\n",
            gp_tests_failed, gp_suites_failed);
    else
        pf_printf(GP_BRIGHT_GREEN "Passed all tests!" GP_RESET_TERMINAL "\n");

    puts("---------------------------------------------------------------");

    if (gp_tests_failed || gp_suites_failed)
        exit(EXIT_FAILURE);

    // Prevent redundant reporting at exit.
    gp_test_count    = 0;
    gp_suite_count   = 0;
    gp_tests_failed  = 0;
    gp_suites_failed = 0;
}

void gp_enable_terminal_colors(void)
{
    // ASNI escape sequences may not be enabled by default.
    #if _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(console, &mode);
    SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #endif
}

static void gp_init_testing(void)
{
    gp_thread_once(&gp_colors_once, gp_enable_terminal_colors);

    #if (__GNUC__ && __linux__) || BSD
    extern const char* __progname;
    prog_name = __progname;
    #elif _WIN32
    static char prog_name_buf[MAX_PATH] = "";
    size_t length = GetModuleFileNameA(NULL, prog_name_buf, MAX_PATH);

    bool valid_ascii = 0 < length && length < MAX_PATH;
    for (size_t i = 0; i < length && valid_ascii; i++)
        valid_ascii = ~prog_name_buf[i] & 0x80;
    if (valid_ascii) {
        const char* trimmed = strrchr(prog_name_buf, '\\');
        prog_name = trimmed ? trimmed + sizeof"\\"-sizeof"" : prog_name_buf;
    }
    #endif

    puts("---------------------------------------------------------------");
    pf_printf("Starting tests%s%s\n\n", *prog_name ? " in " : "", prog_name);
    atexit(gp_end_testing);
}

void gp_test(const char* name)
{
    gp_thread_once(&gp_tests_once, gp_init_testing);

    // End current test
    if (gp_current_test != NULL)
    {
        const char* indent = gp_current_suite == NULL ? "" : "\t";
        if (gp_test_failed) {
            gp_tests_failed++;
            pf_fprintf(stderr,
            "%s" GP_FAILED_STR " test " GP_CYAN "%s" GP_RESET_TERMINAL "\n", indent, gp_current_test);
        } else {
            pf_printf(
            "%s" GP_PASSED_STR " test " GP_CYAN "%s" GP_RESET_TERMINAL "\n", indent, gp_current_test);
        }
        gp_current_test = NULL;
    }
    // Start new test
    if (name != NULL) {
        // No starting message cluttering output
        gp_current_test = name;
        gp_test_failed  = false;
        gp_test_count++;
    }
}

void gp_suite(const char* name)
{
    gp_thread_once(&gp_tests_once, gp_init_testing);
    gp_test(NULL); // End current test

    // End current suite
    if (gp_current_suite != NULL)
    {
        if (gp_suite_failed) {
            gp_suites_failed++;
            pf_fprintf(stderr, GP_FAILED_STR " suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n\n", gp_current_suite);
        } else {
            pf_printf(GP_PASSED_STR " suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n\n", gp_current_suite);
        }
        gp_current_suite = NULL;
    }

    // Start new suite
    if (name != NULL)
    {
        pf_printf("Starting suite " GP_CYAN "%s" GP_RESET_TERMINAL "\n", name);

        gp_current_suite = name;
        gp_suite_failed  = false;
        gp_suite_count++;
    }
}

// ----------------------------------------------------------------------------
// Implementations for gp_assert() and gp_expect()

// TODO C99 has no reflection, better to skip printing non-formatted variables
// than print garbage. This improves portability too, it's okay to leave
// assertions using reflection in.

void gp_fail_internal(
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const GPPrintable* objs,
    ...)
{
    gp_thread_once(&gp_colors_once, gp_enable_terminal_colors);

    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    bool in_main = strcmp(func, "main") == 0;
    bool is_internal = strncmp(func, "gp_", 3) == 0;
    if (gp_current_test != NULL) {
        gp_test_failed = true;
        func = gp_current_test;
    }
    if (gp_current_suite != NULL) {
        gp_suite_failed = true;
        if (gp_current_test == NULL)
            func = gp_current_suite;
    }

    const char* condition = objs[0].identifier;
    switch (gp_sizeof(objs[0].type)) {
    case  1: (void)va_arg(args.list, gp_promoted_arg_int8_t ); break;
    case  2: (void)va_arg(args.list, gp_promoted_arg_int16_t); break;
    case  4: (void)va_arg(args.list, gp_promoted_arg_int32_t); break;
    case  8: (void)va_arg(args.list, gp_promoted_arg_int64_t); break;
    case 16: (void)va_arg(args.list, GPInt128               ); break;
    default : GP_UNREACHABLE("Invalid GPPrintable passed to gp_assert()");
    }

    const char* indent = gp_current_test != NULL ? "\t" : "";

    if ( ! is_internal) // user assertion
        pf_fprintf(stderr,
            "%s%s " GP_WHITE_BG GP_BLACK "line %i" GP_RESET_TERMINAL
            " in " GP_CYAN "%s" GP_RESET_TERMINAL "\n"
            "%sCondition " GP_RED "%s " GP_FAILED_STR "\n",
            indent, file, line, func, indent, condition);
    else // do not distract the user with internal file/line information
        pf_fprintf(stderr,
            "%sCondition " GP_RED "%s " GP_FAILED_STR " in " GP_CYAN "%s" GP_RESET_TERMINAL "\n",
            indent, condition, func);

    char* buf = NULL;
    size_t buf_capacity = 0;
    for (size_t i = 1; i < arg_count; i++)
    {
        fputs(indent, stderr);
        if (objs[i].identifier[0] == '\"')
        {
            const char* fmt = va_arg(args.list, char*);
            size_t fmt_spec_count = 0;
            size_t asterisk_count = 0;
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
                    fmt_spec = strpbrk(c, GP_FORMAT_SPECIFIERS);
                    for (const char* _c = c; _c < fmt_spec; _c++) if (*_c == '*')
                        asterisk_count++;

                    if (fmt_spec == NULL) {
                        pf_fprintf(stderr, "Invalid format specifier \"%s\".", fmt);
                        continue;
                    }
                }
            }
            size_t printed = 0;
            if (fmt_spec_count == 0) // user comment
            {
                pf_fprintf(stderr, "%s\n", fmt);
                continue;
            }
            else if (fmt_spec_count == 1 && asterisk_count == 0)
            {
                // Static analyzer false positive for buffer overflow
                if (i + 1 >= arg_count)
                    GP_UNREACHABLE("");

                pf_fprintf(stderr,
                    GP_BRIGHT_WHITE "%s" GP_RESET_TERMINAL " = ",
                    objs[i + 1/*0 is fmt so next one*/].identifier);

                // Color and opening quote if string or char
                if (*fmt_spec == 'c') // character
                    pf_fprintf(stderr, GP_YELLOW);
                else if (*fmt_spec == 's' || *fmt_spec == 'S') // string
                    pf_fprintf(stderr, GP_BRIGHT_RED);
                else if (strchr("dibBouxX", *fmt_spec)) // integer
                    pf_fprintf(stderr, GP_BRIGHT_BLUE);
                else if (strchr("fFeEgG", *fmt_spec)) // floating point
                    pf_fprintf(stderr, GP_BRIGHT_MAGENTA);
                else if (*fmt_spec == 'p') // pointer
                    pf_fprintf(stderr, GP_BLUE);
            }
            else
            {
                if (brace != NULL) {
                    fputc(*brace, stderr);
                    ++printed;
                    if (fmt[1] == ' ') {
                        fputc(' ', stderr);
                        ++printed;
                    }
                }
                const char* _fmt = fmt; // must detect and skip asterisks
                for (size_t j = 0; j < fmt_spec_count + asterisk_count - 1; )
                {
                    while (true) {
                        _fmt = strchr(_fmt, '%');
                        assert(_fmt); // loop should've ended after processing all specs
                        if (_fmt[1] != '%')
                            break;
                    }
                    const char* spec = strpbrk(_fmt, GP_FORMAT_SPECIFIERS);
                    for (const char* _c = _fmt; _c < spec; _c++) if (*_c == '*')
                        ++j;
                    if (j >= fmt_spec_count + asterisk_count - 1)
                        break;
                    printed += pf_fprintf(stderr,"%s, ",objs[i + 1 + j].identifier);
                    ++j;
                    ++_fmt;
                }
                printed += pf_fprintf(stderr,
                    "%s", objs[i + fmt_spec_count + asterisk_count].identifier);

                if (brace != NULL) {
                    if (fmt[1] == ' ') {
                        printed++;
                        fputc(' ', stderr);
                    }
                    size_t brace_i = brace - l_braces;
                    fputc(r_braces[brace_i], stderr);
                    printed++;
                }
                pf_fprintf(stderr, GP_RESET_TERMINAL " = " GP_BRIGHT_CYAN);
                printed += sizeof" = "-sizeof"";
            }

            size_t required_capacity = pf_vsnprintf(NULL, 0, fmt, args.list) + 1;
            if (required_capacity >= buf_capacity) {
                buf = realloc(
                    buf, buf_capacity = gp_next_power_of_2(required_capacity));
            }
            if (printed + required_capacity > 120)
                pf_fprintf(stderr, "\n\t");

            assert(buf);
            pf_vsnprintf_consuming(buf, buf_capacity, fmt, &args);
            pf_fprintf(stderr, "%s", buf);

            pf_fprintf(stderr, GP_RESET_TERMINAL "\n");

            i += fmt_spec_count + asterisk_count;
            continue;
        } // end if string literal

        pf_fprintf(stderr,
            GP_BRIGHT_WHITE "%s" GP_RESET_TERMINAL " = ", objs[i].identifier);

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                pf_fprintf(stderr,
                    GP_YELLOW "\'%c\'", va_arg(args.list, gp_promoted_arg_char_t));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                pf_fprintf(stderr, GP_BRIGHT_BLUE "%u", va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                pf_fprintf(stderr,
                    GP_BRIGHT_BLUE "%lu", va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                pf_fprintf(stderr,
                    GP_BRIGHT_BLUE "%llu", va_arg(args.list, unsigned long long));
                break;

            case GP_UINT128:
                pf_fprintf(stderr,
                    GP_BRIGHT_BLUE "%w128u", va_arg(args.list, GPUInt128));
                break;

            case GP_BOOL:
                pf_fprintf(stderr, va_arg(args.list, gp_promoted_arg_bool_t) ? "true" : "false");
                break;

            case GP_SHORT:
            case GP_INT:
                pf_fprintf(stderr, GP_BRIGHT_BLUE "%i", va_arg(args.list, int));
                break;

            case GP_LONG:
                pf_fprintf(stderr, GP_BRIGHT_BLUE "%li", va_arg(args.list, long));
                break;

            case GP_LONG_LONG:
                pf_fprintf(stderr, GP_BRIGHT_BLUE "%lli", va_arg(args.list, long long));
                break;

            case GP_INT128:
                pf_fprintf(stderr,
                    GP_BRIGHT_BLUE "%w128i", va_arg(args.list, GPInt128));
                break;

            double f;
            case GP_FLOAT:
            case GP_DOUBLE:
                f = va_arg(args.list, gp_promoted_arg_double_t);
                pf_fprintf(stderr, GP_BRIGHT_MAGENTA "%g", f);
                if (f - (int64_t)f == f/* whole number */&&
                    (int64_t)f < 100000) { // not printed using %e style
                    pf_fprintf(stderr, ".0");
                } break;

            #if GP_HAS_LONG_DOUBLE
            long double lf;
            case GP_LONG_DOUBLE:
                lf = va_arg(args.list, long double);
                pf_fprintf(stderr, GP_BRIGHT_MAGENTA "%Lg", lf);
                if (lf - (int64_t)lf == lf/* whole number */&&
                    (int64_t)lf < 100000) { // not printed using %e style
                    pf_fprintf(stderr, ".0");
                } break;
            #else
            case GP_LONG_DOUBLE: GP_UNREACHABLE("long double not supported.");
            #endif

            const char* char_ptr;
            case GP_CHAR_PTR:
                char_ptr = va_arg(args.list, char*);
                if (char_ptr != NULL)
                    pf_fprintf(stderr, GP_BRIGHT_RED "\"%s\"", char_ptr);
                else
                    pf_fprintf(stderr, GP_BRIGHT_RED "(null)");
                break;

            GPString str;
            case GP_STRING:
                str = va_arg(args.list, GPString);
                if (str != NULL)
                    pf_fprintf(stderr, GP_BRIGHT_RED "\"%.*s\"",
                        (int)gp_str_length(str), (char*)str);
                else
                    pf_fprintf(stderr, GP_BRIGHT_RED "(null)");
                break;

            case GP_PTR:
                pf_fprintf(stderr, GP_BLUE "%p", va_arg(args.list, void*));
                break;

            case GP_NO_TYPE:
            case GP_TYPE_LENGTH:
                GP_UNREACHABLE("");
        }
        pf_fprintf(stderr, GP_RESET_TERMINAL "\n");
    } // end for args
    fputs("\n", stderr);

    free(buf);
    va_end(_args);
    va_end(args.list);

    #if GP_HAS_SANITIZER
    if ( ! in_main)
        __sanitizer_print_stack_trace();
    #endif // GP_HAS_SANITIZER
}
