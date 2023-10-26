// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ASSERT_INCLUDED
#define GPC_ASSERT_INCLUDED 1

#include "overload.h"
#include <stdbool.h>
#include <string.h>

// ----------------------------------------------------------------------------
//
//          CORE API
//
// ----------------------------------------------------------------------------

// Returns true if assertion is true. Aborts program and prints failure message
// along with optional additional message if assertion is false. Note that the
// logical operator can be given as an argument. As an example, use
// gpc_assert(x, <, y) with commas instead of gpc_assert(x < y) for more
// informative failure messages. Additional failure message can be added as the
// last argument e.g. gpc_assert(x, <, y, "x should be smaller than y!").
#define gpc_assert(...) GPC_OVERLOAD4(__VA_ARGS__, \
    gpc_bool_assert_cmp_with_msg, \
    gpc_bool_assert_cmp_wout_msg, \
    gpc_bool_assert_with_msg, \
    gpc_bool_assert_wout_msg)(__VA_ARGS__)

#define gpc_bool_assert_cmp_with_msg(T_a, operator, T_b, const_char_ptr_message)\
    _gpc_bool_assert_cmp_with_msg(T_a, operator, T_b, const_char_ptr_message)

#define gpc_bool_assert_cmp_wout_msg(T_a, operator, T_b) \
    _gpc_bool_assert_cmp_wout_msg(T_a, operator, T_b)

#define gpc_bool_assert_with_msg(bool_expression, const_char_ptr_message) \
    _gpc_bool_assert_with_msg(bool_a, const_char_ptr_message)

#define gpc_bool_assert_wout_msg(bool_expression) \
    _gpc_bool_assert_wout_msg(bool_expression)

// Returns true if assertion is true. Returns false and prints failure message
// along with optional additional message if assertion is false. Note that the
// logical operator can be given as an argument. As an example, use
// gpc_expect(x, <, y) with commas instead of gpc_expect(x < y) for more
// informative failure messages. Additional failure message can be added as the
// last argument e.g. gpc_expect(x, <, y, "x should be smaller than y!").
#define gpc_expect(...) GPC_OVERLOAD4(__VA_ARGS__, \
    gpc_bool_expect_cmp_with_msg, \
    gpc_bool_expect_cmp_wout_msg, \
    gpc_bool_expect_with_msg, \
    gpc_bool_expect_wout_msg)(__VA_ARGS__)

#define gpc_assert_str(...) GPC_OVERLOAD4(__VA_ARGS__, \
    gpc_bool_assert_str_cmp_with_msg, \
    gpc_bool_assert_str_cmp_wout_msg, \
    OVERLOAD_2_ARGS_NOT_DEFINED, \
    OVERLOAD_1_ARGS_NOT_DEFINED)(__VA_ARGS__)

#define gpc_expect_str(...) GPC_OVERLOAD4(__VA_ARGS__, \
    gpc_bool_expect_str_cmp_with_msg, \
    gpc_bool_expect_str_cmp_wout_msg, \
    OVERLOAD_2_ARGS_NOT_DEFINED, \
    OVERLOAD_1_ARGS_NOT_DEFINED)(__VA_ARGS__)

// Starts test. Subsequent calls starts a new test ending the last one. If name
// is NULL last test will be ended without starting a new test.
void gpc_test(const char* name);

// Starts suite. Subsequent calls starts a new suite ending the last one. If
// name is NULL last suite will be ended without starting a new suite.
void gpc_suite(const char* name);

// Optional explicit end of all testing and report results. If this
// function is not called explicitly, it will be called when main() returns.
void gpc_end_testing(void);

// ----------------------------------------------------------------------------
//
//          END OF CORE API
//
// ----------------------------------------------------------------------------

#if defined(__GNUC__) && (__STDC_VERSION__ >= 201112L)
// _Generic, typeof, and brace enclosed compound statements allow pretty
// printing and macros without multiple side effects.

// Compiler can give warnings like comparing different signed integers. This
// macro helps by printing warning message that includes expression
// T_a operator T_b exactly as the user wrote it. The expression is never
// evaluated.
#define _COMPILER_COMPARISON_DIAGNOSTICS(T_a, operator, T_b) \
    (false ? T_a operator T_b : (void)0)

#define _gpc_bool_assert_cmp_with_msg(T_a, operator, T_b, message) \
({ \
    _COMPILER_COMPARISON_DIAGNOSTICS(T_a, operator, T_b); \
    typeof(T_a) _a = (T_a); \
    typeof(T_b) _b = (T_b); \
    char bufs[24][2]; \
    _a operator _b ? true : _gpc_assert_fail( \
        true, \
        #T_a, \
        #operator, \
        #T_b, \
        _GPC_ASSERT_STRFY(bufs[1], _a), \
        _GPC_ASSERT_STRFY(bufs[2], _b), \
        message); \
})

// Store formatted VAR in BUF and return VAR. VAR will be promoted if not bool.
// The casts fix GCC converting -1 to UINT_MAX when passed as variadic arg to
// strfyi(). They have to be pointer sized to prevent compiler complaints with
// pointers.
#define _GPC_ASSERT_STRFY(BUF, VAR)                                 \
    _Generic(VAR,                                                   \
        bool:               gpc_strfyb((BUF), (VAR)),               \
        short:              gpc_strfyi((BUF), (ptrdiff_t)(VAR)),    \
        int:                gpc_strfyi((BUF), (ptrdiff_t)(VAR)),    \
        long:               gpc_strfyi((BUF), (ptrdiff_t)(VAR)),    \
        long long:          gpc_strfyi((BUF), (ptrdiff_t)(VAR)),    \
        unsigned short:     gpc_strfyu((BUF), (uintptr_t)(VAR)),    \
        unsigned int:       gpc_strfyu((BUF), (uintptr_t)(VAR)),    \
        unsigned long:      gpc_strfyu((BUF), (uintptr_t)(VAR)),    \
        unsigned long long: gpc_strfyu((BUF), (uintptr_t)(VAR)),    \
        float:              gpc_strfyf((BUF), (VAR)),               \
        double:             gpc_strfyf((BUF), (VAR)),               \
        char:               gpc_strfyc((BUF), (VAR)),               \
        unsigned char:      gpc_strfyC((BUF), (VAR)),               \
        char*:              gpc_strfyp((BUF), (VAR)),               \
        const char*:        gpc_strfyp((BUF), (VAR)),               \
        default:            gpc_strfyp((BUF), (VAR))) // pointer

// var is variadic to get around type system when using _Generic in
//_GPC_ASSERT_STRFY
const char* gpc_strfyb(char* buf,/*bool var*/...);
const char* gpc_strfyi(char* buf,/*long long var*/...);
const char* gpc_strfyu(char* buf,/*unsigned long long var*/...);
const char* gpc_strfyf(char* buf,/*double var*/...);
const char* gpc_strfyc(char* buf,/*char var*/...);
const char* gpc_strfyC(char* buf,/*char var*/...);
const char* gpc_strfyp(char* buf,/*void* var*/...);

#else // less detailed assertion failures

#endif // Compiler specific implementations

bool _gpc_assert_fail(
    bool aborting,
    const char* a_var_name,
    const char* operator,
    const char* b_var_name,
    const char* a_evaluated,
    const char* b_evaluated,
    const char* additional_message);

#endif // GPC_ASSERT_INCLUDED
