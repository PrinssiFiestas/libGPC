// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include <stdio.h>
#include <stdlib.h>

static size_t _gpc_arg_count = 0;
static const char* _gpc_assert_var_names[GPC_MAX_ARGUMENTS] = {};

void _gpc_assert_push_var_name(const char* var_name)
{
    _gpc_assert_var_names[_gpc_arg_count++] = var_name;
}

bool _gpc_failure(
    bool aborting,
    const char* file,
    int line,
    const char* func,
    const char* condition,
    const char* formats,
    ...)
{
    fprintf(stderr, "Condition %s in %s %i %s [FAILED]\n",
            condition, file, line, func);

    (void)formats;

    if (aborting)
        exit(EXIT_FAILURE);
    return false;
}
