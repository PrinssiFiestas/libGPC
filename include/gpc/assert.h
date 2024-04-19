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
#define/* bool */ gp_assert(/* bool condition, variables*/...) \
((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true : (gp_fatal(__VA_ARGS__), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define/* bool */ gp_expect(/* bool condition, variables*/...) \
((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true : (gp_fail(__VA_ARGS__), false))

// Tests and suites are thread safe if running C11 or higher. Otherwise tests
// and suites started in one thread ends another.

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test. Calling with
// NULL when test is not running does nothing.
void gp_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite. Calling
// with NULL when suite is not running does nothing. Also ends last test.
void gp_suite(const char* name);

#ifdef GP_TESTS // define inline tests

// EXPERIMENTAL
// Requires GNUC. Use with GP_TEST_INS(). Check tests/test_assert.c for docs.
#define GP_TEST_OUT(VAR, PROC, ...)                              \
    typeof(VAR) _gp_##VAR[] = { VAR, __VA_ARGS__};               \
    int _gp_##VAR##_i = 1;                                       \
    bool _gp_##VAR##_run = false;                                \
    _gp_##VAR##_test:                                            \
    if (_gp_##VAR##_i == 0) {                                    \
        _gp_##VAR##_i = -1;                                      \
        goto _gp_##VAR##_test_end;                               \
    }                                                            \
    if (_gp_##VAR##_run) {                                       \
        typeof(VAR) VAR##_EXPECTED = _gp_##VAR[_gp_##VAR##_i++]; \
        if (GP_IS_PRIMITIVE(VAR))                                \
            gp_assert(PROC, (VAR), (VAR##_EXPECTED));            \
        else                                                     \
            gp_assert(PROC);                                     \
        if (_gp_##VAR##_i == GP_COUNT_ARGS(__VA_ARGS__) + 1)     \
            _gp_##VAR##_i = 0;                                   \
    }

// EXPERIMENTAL
// Requires GNUC. Use with GP_TEST_OUT(). Check tests/test_assert.c for docs
#define GP_TEST_INS(VAR, ...)                                         \
    GP_PROCESS_ALL_ARGS(GP_PASS_TO_DECL_ARR, GP_DUMP, __VA_ARGS__)    \
    {                                                                 \
        int _gp_index = _gp_##VAR##_i;                                \
        GP_PROCESS_ALL_ARGS(GP_PASS_TO_SET_INS, GP_DUMP, __VA_ARGS__) \
    }                                                                 \
    VAR = _gp_##VAR[0];                                               \
    _gp_##VAR##_test_end:                                             \
    for (_gp_##VAR##_run = 1; _gp_##VAR##_i != -1; ({ goto _gp_##VAR##_test; }))

#else // don't waste performance on production

#define GP_TEST_OUT(...)
#define GP_TEST_INS(...)

#endif // GP_TESTS

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

//
struct GPPrintable
{
    // Created with #. If var_name[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum GPType type;

    // Actual data is in pr_cstr_print_internal() variadic args.
};
#ifndef GP_PRINTABLE
#define GP_PRINTABLE(X) { #X, GP_TYPE(X) }
#endif

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and exits program.
#define/* bool */ gp_assert_NEW(/* bool condition, variables*/...) \
((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true : (GP_FAIL(1,__VA_ARGS__), false))

// Returns true if condition is true. If condition is false prints fail message,
// marks current test and suite (if running tests) as failed, and returns false.
#define/* bool */ gp_expect_NEW(/* bool condition, variables*/...) \
((bool){0} = (GP_1ST_ARG(__VA_ARGS__)) ? true : (GP_FAIL(0,__VA_ARGS__), false))

#define GP_FAIL(IS_FATAL, ...) \
    gp_fail_internal( \
        IS_FATAL, \
        __FILE__, \
        __LINE__, \
        __func__, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (struct GPPrintable[]) \
            { GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) }, \
        __VA_ARGS__)

void gp_fail_internal(
    int aborting,
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const struct GPPrintable* objs,
    ...) GP_NONNULL_ARGS();
















// TODO get rid of old stuff

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

// TEMP
char* gp_generate_var_info(const char* var_name, const char* format, /* T var */...);

void gp_failure(bool aborting, const char* file, int line, const char* func, size_t arg_count, const char* condition, ...);

#define GP_DECL_ARR(NAME, ...) typeof(NAME) _gp_##NAME[] = { NAME, __VA_ARGS__ };
#define GP_PASS_TO_DECL_ARR(ARGS) GP_DECL_ARR ARGS
#define GP_SET_INS(NAME, UNUSED...) NAME = _gp_##NAME[_gp_index];
#define GP_PASS_TO_SET_INS(ARGS) GP_SET_INS ARGS

#define GP_IS_PRIMITIVE(VAR)  \
_Generic(VAR,                 \
    bool:               true, \
    short:              true, \
    int:                true, \
    long:               true, \
    long long:          true, \
    unsigned short:     true, \
    unsigned int:       true, \
    unsigned long:      true, \
    unsigned long long: true, \
    float:              true, \
    double:             true, \
    char:               true, \
    unsigned char:      true, \
    char*:              true, \
    const char*:        true, \
    void*:              true, \
    default:            false)
#endif // GPASSERT_INCLUDED
