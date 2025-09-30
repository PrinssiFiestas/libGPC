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
#include <gpc/breakpoint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define GP_DUMMY_BOOL_ASSIGN
#else // suppress some warnings allowing things like gp_assert(p = malloc(1))
#define GP_DUMMY_BOOL_ASSIGN (bool){0} =
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// On failures, gp_assert() and gp_expect() print formatted information about
// their arguments to standard error. First, the boolean expression passed as
// first argument is printed with the location of the assertion. Example:
/*
    gp_expect(1 + 1 == 3);  // Prints
                            // Expectation 1 + 1 == 3 FAILED in line xx file yy.
*/
// The exact message of the first line might be slightly different, but it will
// be along those lines.
//     Next, information about additionally passed arguments will be printed in
// form "argument = evaluated_argument". Example:
/*
    gp_expect(false, 1 + 1, my_int_var); // Prints
                            // Expectation false FAILED in line xx file yy.c.
                            // 1 + 1 = 2
                            // my_int_var = -39
*/
// If not C++, format strings can be passed for custom formatting. In C99, these
// are required. A string literal without format specifiers is considered a note
// and will be printed without additional formatting. Example:
/*
    gp_expect(0, "My non-formatted note.", "%x", 127); // Prints
                            // Expectation 0 FAILED in line xx file yy.c.
                            // My non-formatted note.
                            // 127 = 7f
*/
// If the format string starts with a opening brace and optionally space, they
// will be added to the evaluated value as well. This makes printing structs and
// arrays nicer. Example:
/*
    gp_expect(0,
        "{ %s, %zu }", s.str, s.size,
        "[%i, %i, %i, %i]", arr[0], arr[1], arr[2], arr[3]); // Prints
                            // Expectation 0 FAILED in line xx file yy.c.
                            // { s.str, s.size } = { "blah", 4 }
                            // [arr[0], arr[1], arr[2], arr[3]] = [2, 7, 9, 4]
*/

/** Fatal assertion.
 * @return true if condition is true. If condition is false prints fail message,
 * marks current test and suite (if running tests) as failed, and exits program.
 */
#define gp_assert(/* bool condition, variables*/...) \
    (GP_DUMMY_BOOL_ASSIGN (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), GP_DEBUG_BREAKPOINT_TRAP, exit(1), false))

/** Non-fatal assertion.
 * @return true if condition is true. If condition is false prints fail message,
 * marks current test and suite (if running tests) as failed, and returns false.
 */
#define gp_expect(/* bool condition, variables*/...) \
    (GP_DUMMY_BOOL_ASSIGN (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), false))

#ifndef NDEBUG
/** Fatal assertion that can be disabled.
 * @return true if condition is true. If condition is false prints fail message,
 * marks current test and suite (if running tests) as failed, and exits program.
 */
#define gp_db_assert(/* bool condition, variables*/...) \
    (GP_DUMMY_BOOL_ASSIGN (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), GP_DEBUG_BREAKPOINT_TRAP, exit(1), false))

/** Non-fatal assertion that can be disabled.
 * @return true if condition is true. If condition is false prints fail message,
 * marks current test and suite (if running tests) as failed, and returns false.
 */
#define gp_db_expect(/* bool condition, variables*/...) \
    (GP_DUMMY_BOOL_ASSIGN (GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL(__VA_ARGS__), false))

#else // always return true without evaluating condition
static inline bool gp_dummy_bool(bool _) { return _; } // prevent -Wunused-value
#define gp_db_assert(...) gp_dummy_bool(sizeof(GP_1ST_ARG(__VA_ARGS__)))
#define gp_db_expect(...) gp_dummy_bool(sizeof(GP_1ST_ARG(__VA_ARGS__)))
#endif

/** Control flow assertion.
 * Portably assert that control flow never reaches at a given point. The
 * assertion is fatal in debug builds, otherwise undefined behavior is invoked,
 * which may be used by the compiler for better optimizations like dead code
 * elimination.
 */
#ifndef NDEBUG
#define GP_UNREACHABLE(...) \
do { \
    bool unreachable = 0; \
    gp_db_assert(unreachable, __VA_ARGS__); \
    __builtin_unreachable(); \
} while (0)
#elif __GNUC__
#define GP_UNREACHABLE(...) __builtin_unreachable()
#elif _MSC_VER
#define GP_UNREACHABLE(...) __assume(0)
#else
#define GP_UNREACHABLE(...) do { bool unreachable = 0; gp_assert(unreachable, __VA_ARGS__); } while (0)
#endif

/** Start test.
 * Subsequent calls starts a new test ending the last one. If name
 * is NULL last test will be ended without starting a new test. Calling with
 * NULL when test is not running does nothing.
 */
void gp_test(const char* name);

/** Start suite.
 * Subsequent calls starts a new suite ending the last one. If
 * name is NULL last suite will be ended without starting a new suite. Calling
 * with NULL when suite is not running does nothing. Also ends last test.
 */
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
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
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

#define GP_CURSOR_BACK_CPP(N) "\033[" #N "D"
static inline void gp_fail_internal_cpp(
    const char*const condition,
    const char*const file,
    const int line,
    const char*const func,
    std::string vars)
{
    vars.insert(0, "\"" GP_CURSOR_BACK_CPP(1));
    const char*const cstr = vars.c_str();
    const GPPrintable ps[2] = {{condition, GP_TYPE_INT}, {cstr, GP_TYPE_CHAR_PTR}};
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
