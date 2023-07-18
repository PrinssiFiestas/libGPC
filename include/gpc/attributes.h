// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ATTRIBUTES_H
#define GPC_ATTRIBUTES_H

// TODO check for C23 once it comes out for [[nodiscard]]
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wattributes"
#define GPC_NODISCARD __attribute__((__warn_unused_result__))
#define GPC_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#else
#define GPC_NODISCARD
#define GPC_NONNULL
#endif

#ifdef _MSC_VER
#define GPC_THREAD_LOCAL __declspec(thread)
#else
#define GPC_THREAD_LOCAL _Thread_local
#endif

#if defined(__MINGW32__) && !defined(__clang__)
#define GPC_LONG_DOUBLE double
#define GPC_LG_FORMAT "%g"
#else
#define GPC_LONG_DOUBLE long double
#define GPC_LG_FORMAT "%Lg"
#endif

#endif // GPC_ATTRIBUTES_H
