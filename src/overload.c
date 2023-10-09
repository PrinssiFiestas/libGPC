// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/overload.h"

extern inline bool gpc_isUnsigned(const enum gpc_Type T);
extern inline bool gpc_isInteger(const enum gpc_Type T);
extern inline bool gpc_isFloating(const enum gpc_Type T);
extern inline bool gpc_isPointer(const enum gpc_Type T);

size_t gpc_sizeof(const enum gpc_Type T)
{
    switch (T)
    {
        case GPC_UNSIGNED_CHAR:      return sizeof(unsigned char);
        case GPC_UNSIGNED_SHORT:     return sizeof(unsigned short);
        case GPC_UNSIGNED:           return sizeof(unsigned);
        case GPC_UNSIGNED_LONG:      return sizeof(unsigned long);
        case GPC_UNSIGNED_LONG_LONG: return sizeof(unsigned long long);
        case GPC_BOOL:               return sizeof(bool);
        case GPC_CHAR:               return sizeof(char);
        case GPC_SHORT:              return sizeof(short);
        case GPC_INT:                return sizeof(int);
        case GPC_LONG:               return sizeof(long);
        case GPC_LONG_LONG:          return sizeof(long long);
        case GPC_FLOAT:              return sizeof(float);
        case GPC_DOUBLE:             return sizeof(double);
        case GPC_CHAR_PTR:           return sizeof(char*);
        case GPC_PTR:                return sizeof(void*);
    }
    return 0;
}
