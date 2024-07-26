// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Since the single header library is generated, sometimes weird things might
// happen with conditional compilation, because the generator can not make any
// assumptions about the preprocessor. This file just tests that it builds fine.

#define GPC_IMPLEMENTATION
#include "../build/gpc.h"
