// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/overload.h>

extern inline size_t gp_sizeof     (const GPType T);
extern inline bool   gp_is_unsigned(const GPType T);
extern inline bool   gp_is_integer (const GPType T);
extern inline bool   gp_is_floating(const GPType T);
extern inline bool   gp_is_pointer (const GPType T);

