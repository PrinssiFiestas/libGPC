// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file assert.h
 * @brief Unit testing
 */

#ifndef GP_ASSERT_INCLUDED
#define GP_ASSERT_INCLUDED 1

#include "overload.h"
#include "attributes.h"
#include <stdbool.h>
void exit(int status);

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and exits program.
#define gp_assert(/* bool condition, variables*/...) \
    ((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), exit(1), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define gp_expect(/* bool condition, variables*/...) \
    ((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), false))

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test. Calling with
// NULL when test is not running does nothing.
void gp_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite. Calling
// with NULL when suite is not running does nothing. Also ends last test.
void gp_suite(const char* name);

// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------

// Optional explicit end of all testing and report results. If this
// function is not called explicitly, it will be called when main() returns.
void gp_end_testing(void);

#ifndef GP_STRING_INCLUDED
//
struct GPPrintable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum GPType type;

    // Actual data is in pr_cstr_fail_internal() variadic args.
};
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#endif

#define GP_FAIL(...) \
    gp_fail_internal( \
        __FILE__, \
        __LINE__, \
        __func__, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (struct GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)
//
void gp_fail_internal(
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const struct GPPrintable* objs,
    ...);

#endif // GP_ASSERT_INCLUDED
