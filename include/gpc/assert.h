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

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and exits program.
#define /* bool */ gpc_assert(/* bool condition, */...) \
((bool){0} = (GPC_1ST_ARG(__VA_ARGS__)) ? true : (gpc_fatal(__VA_ARGS__), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define /* bool */ gpc_expect(/* bool condition, */...) \
((bool){0} = (GPC_1ST_ARG(__VA_ARGS__)) ? true : (gpc_fail(__VA_ARGS__), false))

// Tests and suites are thread safe if running C11 or higher. Otherwise tests
// and suites started in one thread ends another.

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

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

//
#define GPC_GEN_VAR_INFO(FORMAT, VAR) gpc_generate_var_info(#VAR, FORMAT, VAR)

// To be used in GPC_PROCESS_ALL()
#define GPC_GENERATE_VAR_INFO_INDIRECT(VAR) GPC_GENERATE_VAR_INFO VAR

#if __STDC_VERSION__ >= 201112L
#define GPC_GEN_VAR_INFO_AUTO_FMT(VAR) gpc_generate_var_info(#VAR, GPC_GET_FORMAT(VAR), VAR)
#define GPC_GENERATE_VAR_INFO(...) \
GPC_OVERLOAD2(__VA_ARGS__, GPC_GEN_VAR_INFO, GPC_GEN_VAR_INFO_AUTO_FMT)(__VA_ARGS__)
#else
#define GPC_GEN_VAR_INFO_NO_FMT(LITERAL) gpc_generate_var_info(#LITERAL, " ")
#define GPC_GENERATE_VAR_INFO(...) \
GPC_OVERLOAD2(__VA_ARGS__, GPC_GEN_VAR_INFO, GPC_GEN_VAR_INFO_NO_FMT)(__VA_ARGS__)
#endif

// Prints fail message, marks current test and suite (if running tests) as
// failed, and exits program.
#define gpc_fatal(...) \
gpc_failure(1, __FILE__, __LINE__, __func__, GPC_COUNT_ARGS(__VA_ARGS__), GPC_STRFY_1ST_ARG(__VA_ARGS__), GPC_PROCESS_ALL_BUT_1ST(GPC_GENERATE_VAR_INFO_INDIRECT, GPC_COMMA, __VA_ARGS__))

// Prints fail message and marks current test and suite (if running tests) as
// failed.
#define gpc_fail(...) \
gpc_failure(0, __FILE__, __LINE__, __func__, GPC_COUNT_ARGS(__VA_ARGS__), GPC_STRFY_1ST_ARG(__VA_ARGS__), GPC_PROCESS_ALL_BUT_1ST(GPC_GENERATE_VAR_INFO_INDIRECT, GPC_COMMA, __VA_ARGS__))

char* gpc_generate_var_info(const char* var_name, const char* format, /* T var */...) GPC_PRINTF(2, 3);
void gpc_failure(bool aborting, const char* file, int line, const char* func, size_t arg_count, const char* condition, ...);

#endif // GPC_ASSERT_INCLUDED
