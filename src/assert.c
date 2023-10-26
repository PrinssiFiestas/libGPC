// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include <stdio.h>
#include <stdlib.h>

bool _gpc_assert_fail(
    bool aborting,
    const char* a_var_name,
    const char* operator,
    const char* b_var_name,
    const char* a_evaluated,
    const char* b_evaluated,
    const char* additional_message)
{
    fprintf(stderr, "%s %s %s evaluated to %s %s %s\n%s\n",
        a_var_name, operator, b_var_name,
        a_evaluated, operator, b_evaluated,
        additional_message);
    if (aborting)
        exit(EXIT_FAILURE);
    return false;
}

// ----------------------------------------------------------------------------

#if defined(__GNUC__) && (__STDC_VERSION__ >= 201112L)

#include <stdarg.h>

// Not very DRY but macrofying any more than this kills debuggability
#define GET_VAL(T) \
    va_list arg; \
    va_start(arg, buf); \
    T val = va_arg(arg, T); \
    va_end(arg);
const char* gpc_strfyb(char* buf, ...)
{
    GET_VAL(int);
    sprintf(buf, "%s", val ? "true" : "false");
    return (const char*)(buf);
}
const char* gpc_strfyi(char* buf, ...)
{
    GET_VAL(long long);
    sprintf(buf, "%lli", val);
    return (const char*)(buf);
}
const char* gpc_strfyu(char* buf, ...)
{
    GET_VAL(unsigned long long);
    sprintf(buf, "%llu", val);
    return (const char*)(buf);
}
const char* gpc_strfyf(char* buf, ...)
{
    GET_VAL(double);
    sprintf(buf, "%g", val);
    return (const char*)(buf);
}
const char* gpc_strfyc(char* buf, ...)
{
    GET_VAL(int);
    sprintf(buf, "\'%c\'", val);
    return (const char*)(buf);
}
const char* gpc_strfyC(char* buf, ...)
{
    GET_VAL(int);
    sprintf(buf, "0x%x", val);
    return (const char*)(buf);
}
const char* gpc_strfyp(char* buf, ...)
{
    GET_VAL(void*);
    sprintf(buf, "%p", val);
    return (const char*)(buf);
}
#undef GET_VAL

#endif // defined(__GNUC__) && (__STDC_VERSION__ >= 201112L)
