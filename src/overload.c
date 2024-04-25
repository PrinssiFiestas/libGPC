// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/overload.h>

extern inline bool gp_is_unsigned(const GPType T);
extern inline bool gp_is_integer (const GPType T);
extern inline bool gp_is_floating(const GPType T);
extern inline bool gp_is_pointer (const GPType T);

size_t gp_sizeof(const GPType T)
{
    switch (T)
    {
        case GP_CHAR:
        case GP_SIGNED_CHAR:
        case GP_UNSIGNED_CHAR:
            return sizeof(char);
        case GP_SHORT:
        case GP_UNSIGNED_SHORT:
            return sizeof(short);
        case GP_BOOL:
            return sizeof(bool);
        case GP_INT:
        case GP_UNSIGNED:
            return sizeof(int);
        case GP_LONG:
        case GP_UNSIGNED_LONG:
            return sizeof(long);
        case GP_LONG_LONG:
        case GP_UNSIGNED_LONG_LONG:
            return sizeof(long long);
        case GP_FLOAT:
            return sizeof(float);
        case GP_DOUBLE:
            return sizeof(double);
        case GP_CHAR_PTR:
        case GP_STRING:
        case GP_PTR:
            return sizeof(char*);
    }
    return 0; // shut up compiler
}
