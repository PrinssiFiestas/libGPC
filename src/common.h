// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_PRINT_COMMON_INCLUDED
#define GP_PRINT_COMMON_INCLUDED

#include <gpc/overload.h>
#include <gpc/attributes.h>
#include <gpc/bytes.h>
#include <gpc/array.h>
#include <gpc/utils.h>
#include <printf/conversions.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

GPArray(void) gp_arr_reserve(
    size_t        element_size,
    GPArray(void) arr,
    size_t        capacity) GP_ARR_ATTRS();

inline size_t gp_max_digits_in(const GPType T)
{
    switch (T)
    {
        case GP_FLOAT: // promoted
        case GP_DOUBLE: // %g
            return strlen("-0.111111e-9999");

        case GP_PTR:
            return strlen("0x") + sizeof(void*) * strlen("ff");

        default: // integers https://www.desmos.com/calculator/c1ftloo5ya
            return (gp_sizeof(T) * 18)/CHAR_BIT + 2;
    }
    return 0;
}

GP_NONNULL_ARGS()
inline size_t gp_count_fmt_specs(const char* fmt)
{
    size_t i = 0;
    for (; (fmt = strchr(fmt, '%')) != NULL; fmt++)
    {
        if (fmt[1] == '%')
            fmt++;
        else // consuming more args
            i++;
    }
    return i;
}

GP_NONNULL_ARGS(3)
size_t gp_convert_va_arg(
    const size_t limit,
    void*restrict const out,
    pf_va_list*restrict const args,
    const GPType type);

GP_NONNULL_ARGS(3, 4)
size_t gp_bytes_print_objects(
    const size_t limit,
    void*restrict out,
    pf_va_list* args,
    size_t*const i,
    GPPrintable obj);

#endif // GP_PRINT_COMMON_INCLUDED
