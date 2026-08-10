// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// TODO gp_expect() should only log on debug builds?

#ifndef GP_ASSERT_INCLUDED
#define GP_ASSERT_INCLUDED 1

#include <gpc/gptarget.h>
#include <gpc/gptypes.h>
#include <gpc/gpbreakpoint.h>
#include <gpc/gppreprocessor.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup assert Testing and Assertions
/// @code
/// #include <gpc/gpassert.h>
/// @endcode
/// This module provides macros for assertions and functions for unit testing.
/// All assertion macros can be used for unit testing as well as any other code.
///
/// On failures, assertion macros print formatted information about their
/// arguments to standard error. First, the boolean expression passed as first
/// argument is printed with the location of the assertion. For example,
/// @code
/// gp_expect(1 + 1 == 3);
/// @endcode
/// may print
/// @code
/// file.c line 10 in main
/// Condition 1 + 1 == 3 [FAILED]
/// @endcode
/// The exact message may change and should not be relied upon.
///
/// Information about additionally passed arguments will be printed in form
/// `argument = evaluated_argument`. Example:
/// @code
/// const char* my_string = "some characters";
/// int my_int_var = 0;
/// float my_float_var = 3.14;
/// gp_expect(my_int_var + 1 == 3,
///     my_int_var,
///     my_float_var,
///     my_int_var + 1,
///     my_string + 5);
/// @endcode
/// may print
/// @code
/// file.c line 16 in main
/// Condition my_int_var + 1 == 3 [FAILED]
/// my_int_var = 0
/// my_float_var = 3.14
/// my_int_var + 1 = 1
/// my_string + 5 = "characters"
/// @endcode
///
/// If not C++, format strings can be passed for custom formatting. In C99,
/// these are required. A string literal without format specifiers is considered
/// a note and will be printed without additional formatting. Example:
/// @code
/// gp_expect(0, "Note: this is a literal.", "%x", 127);
/// @endcode
/// may print
/// @code
/// file.c line 17 in main
/// Condition 0 [FAILED]
/// Note: this is a literal.
/// 127 = 7f
/// @endcode
/// The formats available are the same ones supported by @ref gp_str_print(),
/// which is based on GNU C Library `printf()`. Format strings are not type
/// checked, so make sure to double check their correctness when used.
///
/// If the format string starts with a opening brace and optionally space, they
/// will be added to the evaluated value as well. This makes printing structs and
/// arrays nicer. Example:
/// @code
/// gp_expect(0,
///     "{ %s, %zu }", s.str, s.size,
///     "[%i, %i, %i, %i]", arr[0], arr[1], arr[2], arr[3]);
/// @endcode
/// may print
/// @code
/// file.c line 18 in main
/// Condition 0 [FAILED]
/// { s.str, s.size } = { "blah", 4 }
/// [arr[0], arr[1], arr[2], arr[3]] = [2, 7, 4, 9]
/// @endcode
/// @{

/** Fatal assertion.
 *
 * If condition is false, prints fail message, marks current test and suite (if
 * running tests) as failed, and exits program. Unlike standard `assert()`, this
 * macro cannot be disabled. Use an explicit `#if` block or @ref gp_assume()
 * instead if performance is an issue.
 *
 * @return `true` if condition is true, will not return otherwise.
 */
#define gp_assert(/* bool condition, variables */...) \
    (gp_pass_bool(GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL_MESSAGE(__VA_ARGS__), GP_DEBUG_BREAKPOINT_TRAP, exit(1), false))

/** Non-fatal assertion.
 *
 * If condition is false, prints fail message, marks current test and suite (if
 * running tests) as failed without ending execution. This is mostly useful when
 * unit testing, application code will rarely use this.
 *
 * @return condition casted to `bool`.
 */
#define gp_expect(/* bool condition, variables */...) \
    (gp_pass_bool(GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL_MESSAGE(__VA_ARGS__), false))

/** Optimized assertion.
 *
 * If @ref GP_TARGET_DEBUG is defined and condition is false, marks current test
 * and suite (if running tests) as failed, and exits program. If @ref GP_TARGET_DEBUG
 * is not defined and condition is false, then undefined behavior is invoked. In
 * practice, this means that this assertion is often optimized away completely
 * in optimized builds. This may include the condition itself when there is no
 * side effects, however, calling functions with side effects in condition is
 * safe when condition is true. Optimizing compilers and static analyzers may
 * use the condition for symbolic analysis to produce better output.
 *
 * @return `true` if condition is true, undefined behavior invoked otherwise
 * potentially optimizing the call out completely.
 */
#define gp_assume(/* bool condition, variables */...) \
    (gp_pass_bool(GP_1ST_ARG(__VA_ARGS__)) ? true :  \
        (GP_FAIL_MESSAGE(__VA_ARGS__), GP_ASSUME_FAIL, false))

/** Invoke undefined behavior.
 *
 * Equivalent to `gp_assert(false)` when @ref GP_TARGET_DEBUG is defined.
 * Otherwise, invokes undefined behavior, which may be used by the compiler for
 * better optimizations like dead code elimination.
 *
 * Arguments will be passed to @ref gp_assert() for error messages in debug
 * builds, ignored otherwise.
 */
#ifdef GP_TARGET_DEBUG
#  if defined(__GNUC__) && !defined(__cplusplus) && !defined(GP_PEDANTIC) // nicer fail message
#    define gp_unreachable(...) \
({ \
    bool unreachable = 0; \
    gp_assert(unreachable, __VA_ARGS__); \
})
#  else
#    define gp_unreachable(...) gp_assert(0, __VA_ARGS__)
#  endif
#elif __STDC_VERSION__ >= 202311L
#  define gp_unreachable(...) unreachable()
#elif defined(__GNUC__)
#  define gp_unreachable(...) __builtin_unreachable()
#elif defined(_MSC_VER)
#  define gp_unreachable(...) __assume(0)
#else
#  define gp_unreachable(...) (*(char*)0 = 0)
#endif

/** Compile-time assertion.
 *
 * @a CONDITION is a compile time expression that aborts compilation if it
 * evaluates to zero in which case @a MESSAGE (passed as a string literal) will
 * be displayed. Otherwise, does nothing.
 *
 * @a MESSAGE is optional and can be omitted. If C99, @a MESSAGE will be ignored
 * and the error message in case of failing @a CONDITION will be some cryptic
 * message about negative array size.
 */
#ifdef GP_DOXYGEN
#  define GP_STATIC_ASSERT(CONDITION, MESSAGE) _Static_assert(CONDITION, MESSAGE)
#elif __STDC_VERSION__ >= 202311L || defined(__cplusplus)
#  define GP_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#elif __STDC_VERSION__ >= 201112L || defined(__TINYC__) || defined(__COMPCERT__)
#  define GP_STATIC_ASSERT(E, ...) _Static_assert((E), ""__VA_ARGS__)
#else // C99, message will be ignored, it is just there for compatibility
#  define GP_STATIC_ASSERTION_NAME(LINE) GP_TOKEN_PASTE(_gp_static_assertion_, LINE)
#  define GP_STATIC_ASSERT(E, ...) extern char GP_STATIC_ASSERTION_NAME(__LINE__)[(E) ? 1 : -1]
#endif

/** Compile-time assertion as an expression.
 *
 * @a CONDITION is a compile time expression that aborts compilation if it
 * evaluates to zero in which case @a MESSAGE (passed as a string literal) will
 * be displayed. Otherwise, does nothing.
 *
 * Unlike @ref GP_STATIC_ASSERT, which must be used as a stand-alone statement,
 * this can be used in an expression like any other boolean value, which is
 * mostly useful for programmatic checks in function macros. This macro must be
 * used in function scope.
 *
 * @a MESSAGE is optional and can be omitted. If not GNU C, @a MESSAGE will be
 * ignored and the error message in case of failing @a CONDITION will be some
 * cryptic message about negative array size. The message is less likely to be
 * ignored with @ref GP_STATIC_ASSERT, which is why it is recommended to use it
 * instead when applicable.
 *
 * @return `true` if @a CONDITION is true, otherwise will not compile.
 */
#ifdef GP_DOXYGEN
#  define gp_static_assert(CONDITION, MESSAGE) ({_Static_assert(CONDITION, MESSAGE); true})
#elif (defined(__GNUC__) || defined(__TINYC__)) && !defined(GP_PEDANTIC)
#  define gp_static_assert(...) \
        gp_pass_bool(({GP_STATIC_ASSERT(__VA_ARGS__); true;}))
#else
#  define gp_static_assert(E, ...) gp_pass_bool(sizeof(char[(E) ? 1 : -1]))
#endif

/** Start suite and unit testing.
 *
 * First call starts unit testing. Subsequent calls starts a new suite ending
 * the last suite and test. If name is NULL last suite will be ended without
 * starting a new suite. Calling with NULL when suite is not running does
 * nothing.
 */
GP_API void gp_suite(const char* name);

/** Start unit test.
 *
 * Subsequent calls starts a new test ending the last one. If name
 * is NULL last test will be ended without starting a new test. Calling with
 * NULL when test is not running does nothing.
 */
GP_API void gp_test(const char* name);

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

#ifdef GP_TARGET_DEBUG
#  define GP_ASSUME_FAIL GP_DEBUG_BREAKPOINT_TRAP, exit(1)
#else
#  define GP_ASSUME_FAIL gp_unreachable()
#endif

// Ignore unused value warnings
static inline bool gp_pass_bool(bool b) { return b; }

#define GP_FAIL_MESSAGE(...) \
    gp_internal_fail( \
        __FILE__, \
        __LINE__, \
        __func__, \
        GP_COUNT_ARGS(__VA_ARGS__), \
        (GPInternalReflectionData[]) \
            { {0}, GP_PROCESS_ALL_ARGS(GP_PRINTABLE, GP_COMMA, __VA_ARGS__) } + 1, \
        __VA_ARGS__)
//
typedef struct gp_internal_reflection_data
{
    // Created with #. If identifier[0] == '\"', then contains format string.
    const char* identifier;

    // Simplified specifier. If var_name is not a format string, then this is
    // used avoiding format string parsing.
    const enum gp_type_t type;

    // Actual data is in gp_str_print_internal() variadic args.
} GPInternalReflectionData;

#if GP_HAS_C11_GENERIC
#define GP_PRINTABLE(X) { #X, gp_type(X) }
#else
#define GP_PRINTABLE(X) { #X, INT_MAX - (int)(sizeof(X)) }
#endif

//
GP_API void gp_internal_fail(
    const char* file,
    int line,
    const char* func,
    size_t arg_count,
    const GPInternalReflectionData* objs,
    ...);

#ifdef __cplusplus
} // extern "C"

#include <string>

#define GP_CURSOR_BACK_CPP(N) "\033[" #N "D"
static inline void gp_internal_fail_cpp(
    const char*const condition,
    const char*const file,
    const int line,
    const char*const func,
    std::string vars)
{
    vars.insert(0, "\"" GP_CURSOR_BACK_CPP(1));
    const char*const cstr = vars.c_str();
    const GPInternalReflectionData ps[2] = {{condition, GP_TYPE_INT}, {cstr, GP_TYPE_CHAR_PTR}};
    gp_internal_fail(file, line, func, 2, ps, 0, cstr);
}

#define GP_STREAM_VAR_INFO(VAR) #VAR " = " << (VAR)
#define GP_STREAM_INSERT_VAR(...) << "\n" <<
#undef GP_FAIL_MESSAGE
#define GP_FAIL_MESSAGE(...) \
    gp_internal_fail_cpp( \
        "", \
        __FILE__, \
        __LINE__, \
        __func__, \
        (std::ostringstream() << \
            GP_PROCESS_ALL_ARGS(GP_STREAM_VAR_INFO, GP_STREAM_INSERT_VAR, __VA_ARGS__) \
        ).str())
#endif // __cplusplus

///@endcond
#endif // GP_ASSERT_INCLUDED
