// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ASSERT_INCLUDED
#define GPC_ASSERT_INCLUDED 1

#include "overload.h"
#include "attributes.h"
#include <stdbool.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Aborts execution and prints failure message to stderr if condition == false,
// returns true otherwise.
#define /* bool */ gpc_assert(condition,...) \
((condition) ? true : gpc_fatal(#condition, __VA_ARGS__))

// Prints failure message to stderr and returns false if condition == false,
// returns true otherwise.
#define /* bool */ gpc_expect(condition,...) \
((condition) ? true : gpc_fail(#condition, __VA_ARGS__))

// Aborts execution and prints failure message to stderr.
#define gpc_fatal(const_char_ptr_condition,...) \
_gpc_fatal(const_char_ptr_condition, __VA_ARGS__)

// Marks current test and suite as failed and prints failure message to stderr
#define gpc_fail(const_char_ptr_condition,...) \
_gpc_fail(const_char_ptr_condition, __VA_ARGS__)

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test. Calling with
// NULL when test is not running does nothing.
void gpc_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite. Calling
// with NULL when suite is not running does nothing.
void gpc_suite(const char* name);

// Optional explicit end of all testing and report results. If this
// function is not called explicitly, it will be called when main() returns.
void gpc_end_testing(void);

// Call this with false if your terminal or output doesn't support colours.
void gpc_enable_color(bool);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
// ----------------------------------------------------------------------------

#define _gpc_fatal(const_char_ptr_condition, ...) \
_gpc_failure(1, __FILE__, __LINE__, __func__, const_char_ptr_condition, GPC_PROCESS_ALL_BUT_1ST(_GPC_STRFY_AND_PASS, GPC_COMMA, __VA_ARGS__))

#define _gpc_fail(const_char_ptr_condition, ...) \
_gpc_failure(0, __FILE__, __LINE__, __func__, const_char_ptr_condition, GPC_PROCESS_ALL_BUT_1ST(_GPC_STRFY_AND_PASS, GPC_COMMA, __VA_ARGS__))

#define _GPC_STRFY_AND_PASS(X) (_gpc_assert_push_var_name(#X) GPC_COMMA(X) X)
void _gpc_assert_push_var_name(const char* var_name);
bool _gpc_failure(bool aborting, const char* file, int line, const char* func, const char* condition, const char* formats, ...);

#endif // GPC_ASSERT_INCLUDED
