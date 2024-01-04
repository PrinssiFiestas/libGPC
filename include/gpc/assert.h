// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file assert.h
 * @brief Unit testing
 */

#ifndef GPASSERT_INCLUDED
#define GPASSERT_INCLUDED 1

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
#define /* bool */ gp_assert(/* bool condition, */...) \
((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true : (gp_fatal(__VA_ARGS__), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define /* bool */ gp_expect(/* bool condition, */...) \
((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true : (gp_fail(__VA_ARGS__), false))

// Tests and suites are thread safe if running C11 or higher. Otherwise tests
// and suites started in one thread ends another.

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test. Calling with
// NULL when test is not running does nothing.
void gp_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite. Calling
// with NULL when suite is not running does nothing.
void gp_suite(const char* name);

// Optional explicit end of all testing and report results. If this
// function is not called explicitly, it will be called when main() returns.
void gp_end_testing(void);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

//
#define GP_GEN_VAR_INFO(FORMAT, VAR) gp_generate_var_info(#VAR, FORMAT, VAR)

// To be used in GP_PROCESS_ALL()
#define GP_GENERATE_VAR_INFO_INDIRECT(VAR) GP_GENERATE_VAR_INFO VAR

#if __STDC_VERSION__ >= 201112L
#define GP_GEN_VAR_INFO_AUTO_FMT(VAR) gp_generate_var_info(#VAR, GP_GET_FORMAT(VAR), VAR)
#define GP_GENERATE_VAR_INFO(...) \
GP_OVERLOAD2(__VA_ARGS__, GP_GEN_VAR_INFO, GP_GEN_VAR_INFO_AUTO_FMT)(__VA_ARGS__)
#else
#define GP_GEN_VAR_INFO_NO_FMT(LITERAL) gp_generate_var_info(#LITERAL, " ")
#define GP_GENERATE_VAR_INFO(...) \
GP_OVERLOAD2(__VA_ARGS__, GP_GEN_VAR_INFO, GP_GEN_VAR_INFO_NO_FMT)(__VA_ARGS__)
#endif

// Prints fail message, marks current test and suite (if running tests) as
// failed, and exits program.
#define gp_fatal(...) \
gp_failure(1, __FILE__, __LINE__, __func__, GP_COUNT_ARGS(__VA_ARGS__), GP_STRFY_1ST_ARG(__VA_ARGS__), GP_PROCESS_ALL_BUT_1ST(GP_GENERATE_VAR_INFO_INDIRECT, GP_COMMA, __VA_ARGS__))

// Prints fail message and marks current test and suite (if running tests) as
// failed.
#define gp_fail(...) \
gp_failure(0, __FILE__, __LINE__, __func__, GP_COUNT_ARGS(__VA_ARGS__), GP_STRFY_1ST_ARG(__VA_ARGS__), GP_PROCESS_ALL_BUT_1ST(GP_GENERATE_VAR_INFO_INDIRECT, GP_COMMA, __VA_ARGS__))

char* gp_generate_var_info(const char* var_name, const char* format, /* T var */...) GP_PRINTF(2, 3);

void gp_failure(bool aborting, const char* file, int line, const char* func, size_t arg_count, const char* condition, ...);

#endif // GPASSERT_INCLUDED
