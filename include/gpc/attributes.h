/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_ATTRIBUTES_H
#define GPC_ATTRIBUTES_H

// TODO check for C23 once it comes out for [[nodiscard]]
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#define GPC_NODISCARD __attribute__((nodiscard))
#else
#define GPC_NODISCARD
#endif

#endif // GPC_ATTRIBUTES_H