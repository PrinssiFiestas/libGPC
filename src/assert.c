// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../include/gpc/terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Copy src to dest and return a pointer to the end of the copied string in dest
static char* gpc_str_push(char* dest, const char* src)
{
    while ((*dest++ = *src++));
    return dest - 1;
}

char* gpc_generate_var_info(const char* var_name, const char* format, ...)
{
    va_list arg, arg_;
    va_start(arg, format);
    va_copy(arg_, arg);

    size_t modified_format_length = strlen(var_name) + strlen(" = \'\'") + strlen(GPC_BRIGHT_WHITE_BG GPC_RESET_TERMINAL) + strlen(format) + sizeof("\n");
    char* modified_format = malloc(modified_format_length);
    if (modified_format == NULL)
    {
        perror("malloc() failed on gpc_generate_var_info().");
        goto cleanup;
    }

    char* p = modified_format;
    p = gpc_str_push(p, var_name);
    p = gpc_str_push(p, " = ");

    const char all_supported_formats[] = "dibBouxXfFeEgGaAcCsSZp";
    char type = *strpbrk(format, all_supported_formats);

    if (type == 'c' || type == 'C') // character
        p = gpc_str_push(p, GPC_YELLOW "\'");
    else if (type == 's' || type == 'S' || type == 'Z') // string
        p = gpc_str_push(p, GPC_BRIGHT_RED "\"");
    else if (strchr("dibBouxX", type)) // integer
        p = gpc_str_push(p, GPC_BRIGHT_BLUE);
    else if (strchr("fFeEgG", type)) // floating point
        p = gpc_str_push(p, GPC_BRIGHT_MAGENTA);
    else if (type == 'p') // pointer
        p = gpc_str_push(p, GPC_BLUE);

    p = gpc_str_push(p, format);

    if (type == 'c' || type == 'C')
        p = gpc_str_push(p,  "\'");
    else if (type == 's' || type == 'S' || type == 'Z')
        p = gpc_str_push(p, "\"");

    p = gpc_str_push(p, GPC_RESET_TERMINAL "\n");

    size_t var_info_length = (size_t)vsnprintf(NULL, 0, modified_format, arg_);
    char* var_info = malloc(var_info_length + sizeof('\0'));
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
