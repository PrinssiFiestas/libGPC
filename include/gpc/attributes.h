// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ATTRIBUTES_H
#define GPC_ATTRIBUTES_H

// TODO check for C23 once it comes out for [[nodiscard]] and typeof()

// Are these checks needed now that I removed -pedantic?
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wattributes"
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif
#define GPC_NODISCARD __attribute__((__warn_unused_result__))
#else
#define GPC_NODISCARD
#endif

// ----------------------------------------------------------------------------

#ifdef _MSC_VER
#define GPC_THREAD_LOCAL __declspec(thread)
#else
#define GPC_THREAD_LOCAL _Thread_local
#endif

// ----------------------------------------------------------------------------

#if defined(__MINGW32__) && !defined(__clang__)
#define GPC_LONG_DOUBLE double
#define GPC_LG_FORMAT "%g"
#else
#define GPC_LONG_DOUBLE long double
#define GPC_LG_FORMAT "%Lg"
#endif

// ----------------------------------------------------------------------------

#if defined(__GNUC__)

// Use for pointer validation in function arguments with GCC and Clang. Example:
// void foo(int pointer[GPC_STATIC 1]);
#define GPC_STATIC static

#define GPC_TYPEOF(X) typeof(X)
#define GPC_CAST_TO_TYPEOF(X) (typeof(X))

#else

#define GPC_STATIC
#define GPC_TYPEOF(X)
#define GPC_CAST_TO_TYPEOF(X)

#endif // defined(__GNUC__)

#endif // GPC_ATTRIBUTES_H
