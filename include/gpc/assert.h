// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**
 * @file assert.h
 * @brief Unit testing
 */

#ifndef GP_ASSERT_INCLUDED
#define GP_ASSERT_INCLUDED 1

#include <gpc/utils.h>
#include <gpc/bytes.h>
#include <gpc/overload.h>
#include <gpc/attributes.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define GP_DUMMY_BOOL_ASSIGN
#else // suppress some warnings
#define GP_DUMMY_BOOL_ASSIGN (bool){0} =
#endif

#ifndef GP_USER_ASSERT_EXIT
#define GP_USER_ASSERT_EXIT exit
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and exits program.
#define gp_assert(/* bool condition, variables*/...) \
    (GP_DUMMY_BOOL_ASSIGN (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), GP_USER_ASSERT_EXIT(1), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define gp_expect(/* bool condition, variables*/...) \
    (GP_DUMMY_BOOL_ASSIGN (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
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

#define GP_FAIL(...) \
    gp_fail_internal( \
        __FILE__, \
        __LINE__, \
        __func__, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)
//
void gp_fail_internal(
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const GPPrintable* objs,
    ...);

#ifdef __cplusplus
} // extern "C"

static inline void gp_fail_internal_cpp(
    const char*const condition,
    const char*const file,
    const int line,
    const char*const func,
    std::string vars)
{
    vars.insert(0, "\"" GP_CURSOR_BACK(1));
    const char*const cstr = vars.c_str();
    const GPPrintable ps[2] = {{condition, GP_INT}, {cstr, GP_CHAR_PTR}};
    gp_fail_internal(file, line, func, 2, ps, 0, cstr);
}

#define GP_STREAM_VAR_INFO(VAR) #VAR " = " << (VAR)
#define GP_STREAM_INSERT_VAR(...) << "\n" <<
#undef GP_FAIL
#define GP_FAIL(...) \
    gp_fail_internal_cpp( \
        "", \
        __FILE__, \
        __LINE__, \
        __func__, \
        (std::ostringstream() << \
            GP_PROCESS_ALL_ARGS(GP_STREAM_VAR_INFO, GP_STREAM_INSERT_VAR, __VA_ARGS__) \
        ).str())
#endif // __cplusplus

#endif // GP_ASSERT_INCLUDED
