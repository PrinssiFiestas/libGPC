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

// Copy n characters from src to dest and return a pointer to the end of the
// copied string in dest. src is assumed to be null terminated if n < 0.
void gpc_str_push(char *restrict *dest, const char* restrict src, ptrdiff_t n)
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

char* gpc_generate_var_info(const char* var_name, const char* format, ...)
{
    va_list arg, arg_;
    va_start(arg, format);
    va_copy(arg_, arg);

    char* var_info = NULL;

    size_t modified_format_length = strlen(GPC_BRIGHT_GREEN GPC_RESET_TERMINAL) + strlen(var_name) + strlen(" = \'\'") + strlen(GPC_BRIGHT_WHITE_BG GPC_RESET_TERMINAL) + strlen(format) + sizeof("\n");
    char* modified_format = malloc(modified_format_length);
    if (modified_format == NULL)
    {
        perror("malloc() failed on gpc_generate_var_info().");
        goto cleanup;
    }

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

    if (type == 'c' || type == 'C')
        gpc_str_push(&p,  "\'", -1);
    else if (type == 's' || type == 'S' || type == 'Z')
        gpc_str_push(&p, "\"", -1);

    gpc_str_push(&p, GPC_RESET_TERMINAL, -1);
    gpc_str_push(&p, format_specifier_end, -1);
    gpc_str_push(&p, "\n", -1);

    size_t var_info_length = (size_t)vsnprintf(NULL, 0, modified_format, arg_);
    var_info = malloc(var_info_length + sizeof('\0'));
    if (var_info == NULL)
    {
        perror("malloc() failed in gpc_generate_var_info().");
        goto cleanup;
    }

    vsprintf(var_info, modified_format, arg);
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
    fprintf(stderr,
            "Condition " GPC_RED"%s"GPC_RESET_TERMINAL
            " in %s " GPC_WHITE_BG GPC_BLACK "line %i" GPC_RESET_TERMINAL" %s "
            GPC_RED"[FAILED]" GPC_RESET_TERMINAL"\n",
            condition, file, line, func);

    va_list va_args;
    va_start(va_args, condition);

    while (arg_count--)
    {
        char* arg_info = va_arg(va_args, char*);
        fputs(arg_info, stderr);
        free(arg_info);
    }
    fputs("\n", stderr);

    if (aborting)
        exit(EXIT_FAILURE);

    va_end(va_args);
}
