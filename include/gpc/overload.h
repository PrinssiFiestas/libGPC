// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_OVERLOAD_INCLUDED
#define GPC_OVERLOAD_INCLUDED 1

#include <stdbool.h>
#include <stddef.h>

// Keep things sane
#define GPC_MAX_ARGUMENTS 64

// Overloading functions and macro functions by the number of arguments can be
// done with OVERLOADN() macros. First arg to OVERLOADN() is always __VA_ARGS__
// which is followed by names of functions/macros to be overloaded. The actual
// arguments also has to be given after OVERLOADN().
// Example for max 3 args below:
/*
    void func1(int arg1);
    #define MACRO2(arg1, arg2) somefunc(arg1, arg2)
    int func3(char arg1, void* arg2, const char* arg3);

    #define func(...) OVERLOAD3(__VA_ARGS__, func3, func2, func1)(__VA_ARGS__)

    int main(void)
    {
        // now func() can be called with 1-3 args.
        func(1);
        func(1, 2);
        func('1', (void*)2, "3");
    }
*/

// Helper macros

#define GPC_STRFY(A) #A
#define GPC_STRFY_1ST_ARG(A, ...) #A
#define GPC_1ST_ARG(A, ...) A
#define GPC_COMMA(...) ,
#define GPC_DUMP(...)
#define GPC_EVAL(...) __VA_ARGS__

// Arguments list can be processed with GPC_PROCESS_ALL_ARGS() macro. The first
// argument is a function or a macro that takes a single argument. This function
// processes the variadic argument list. The second argument determines a
// separator for the variadic argument list. It has to be a macro function that
// takes a variadic argument but just expands to the separator without doing
// anything to __VA_ARGS__ like GPC_COMMA() Example uses below:
/*
    int add_one(int x) { return x + 1; }
    int array[] = { GPC_PROCESS_ALL_ARGS(add_one, GPC_COMMA, 3, 4, 5) };
    // The line above expands to
    int array[] = { add_one(3), add_one(4), add_one(5) };

    #define PLUS(...) +
    int sum = GPC_PROCESS_ALL_ARGS(GPC_EVAL, PLUS, 2, 3, 4, 5);
    // The line above expands to
    int sum = 2 + 3 + 4 + 5

    // Combining the above we can get sum of squares
    double square(double x) { return x*x; }
    double sum_of_squares = GPC_LIST_ALL(square, PLUS, 3.14, .707);
    // expands to
    double sum_of_squares = square(3.14) + square(.707);
*/

// If __VA_OPT__() is needed with GPC_PROCESS_ALL_ARGS(),
// GPC_PROCESS_ALL_BUT_1ST() can be used instead. GPC_PROCESS_ALL_BUT_1ST() processes every argument that is passed to it except the first one. __VA_OPT__() can be simulated by using the first argument as a required argument making all variadic arguments optional without needing __VA_OPT__(). Example below:
/*
    int sq(int x) { return x * x; }
    #define PLUS(...) +

    // First argument required! In this case it's the format string.
    #define PRINT_SUM_OF_SQ(...) printf(GPC_PROCESS_ALL_BUT_1ST(sq, PLUS, __VA_ARGS__)

    PRINT_INCREMENTED("%i", 1, 2, 3);
    // expands to
    printf("%i", sq(1) + sq(2) + sq(3));
*/

// Use in variadic function arguments with GPC_TYPE() macro
enum gpc_Type
{
    GPC_UNSIGNED_CHAR,
    GPC_UNSIGNED_SHORT,
    GPC_UNSIGNED,
    GPC_UNSIGNED_LONG,
    GPC_UNSIGNED_LONG_LONG,
    GPC_BOOL,
    GPC_CHAR,
    GPC_SHORT,
    GPC_INT,
    GPC_LONG,
    GPC_LONG_LONG,
    GPC_FLOAT,
    GPC_DOUBLE,
    GPC_CHAR_PTR,
    GPC_PTR,
};

inline bool gpc_is_unsigned(const enum gpc_Type T) { return T <= GPC_UNSIGNED_LONG_LONG; }
inline bool gpc_is_integer(const enum gpc_Type T) { return T <= GPC_LONG_LONG; }
inline bool gpc_is_floating(const enum gpc_Type T) { return GPC_FLOAT <= T && T <= GPC_DOUBLE; }
inline bool gpc_is_pointer(const enum gpc_Type T) { return GPC_CHAR_PTR <= T && T <= GPC_PTR; }

size_t gpc_sizeof(const enum gpc_Type T);

#define GPC_TYPE(VAR)                           \
_Generic(VAR,                                   \
    bool:               GPC_BOOL,               \
    short:              GPC_SHORT,              \
    int:                GPC_INT,                \
    long:               GPC_LONG,               \
    long long:          GPC_LONG_LONG,          \
    unsigned short:     GPC_UNSIGNED_LONG,      \
    unsigned int:       GPC_UNSIGNED,           \
    unsigned long:      GPC_UNSIGNED_LONG,      \
    unsigned long long: GPC_UNSIGNED_LONG_LONG, \
    float:              GPC_FLOAT,              \
    double:             GPC_DOUBLE,             \
    char:               GPC_CHAR,               \
    unsigned char:      GPC_UNSIGNED_CHAR,      \
    char*:              GPC_CHAR_PTR,           \
    const char*:        GPC_CHAR_PTR,           \
    default:            GPC_PTR)

#define GPC_GET_FORMAT(VAR)     \
_Generic(VAR,                   \
    bool:               "%i",   \
    short:              "%hi",  \
    int:                "%i",   \
    long:               "%li",  \
    long long:          "%lli", \
    unsigned short:     "%hu",  \
    unsigned int:       "%u",   \
    unsigned long:      "%lu",  \
    unsigned long long: "%llu", \
    float:              "%g",   \
    double:             "%g",   \
    char:               "%c",   \
    unsigned char:      "%x",   \
    char*:              "%s",   \
    const char*:        "%s",   \
    default:            "%p")

#define GPC_IF_IS_NUMBER(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) _Generic(VAR,       \
short: EXPR_IF_TRUE, unsigned short: EXPR_IF_TRUE, int: EXPR_IF_TRUE,          \
unsigned: EXPR_IF_TRUE, long: EXPR_IF_TRUE, unsigned long: EXPR_IF_TRUE,       \
long long: EXPR_IF_TRUE, unsigned long long: EXPR_IF_TRUE, float: EXPR_IF_TRUE,\
double: EXPR_IF_TRUE, long double: EXPR_IF_TRUE, default: EXPR_IF_FALSE)

#define GPC_IF_IS_CHAR(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) _Generic(VAR, \
char: EXPR_IF_TRUE, unsigned char: EXPR_IF_TRUE, default: EXPR_IF_FALSE)

#define GPC_IF_IS_NUMERIC(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) _Generic(VAR, \
_Bool: EXPR_IF_TRUE, default: GPC_IF_IS_NUMBER(VAR, EXPR_IF_TRUE,         \
GPC_IF_IS_CHAR(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE)))

// Returns number of arguments
#define GPC_COUNT_ARGS(...) GPC_OVERLOAD64(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56,\
55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,     \
33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,     \
11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

// ----------------------------------------------------------------------------
// Gory script generated internals below. You have been warned.

#define GPC_PROCESS_ALL_ARGS(FUNC, SEPARATOR, ...) GPC_OVERLOAD64(__VA_ARGS__, 	\
GPC_PROC64, GPC_PROC63, GPC_PROC62, GPC_PROC61, GPC_PROC60, GPC_PROC59, GPC_PROC58, GPC_PROC57, 	\
GPC_PROC56, GPC_PROC55, GPC_PROC54, GPC_PROC53, GPC_PROC52, GPC_PROC51, GPC_PROC50, GPC_PROC49, 	\
GPC_PROC48, GPC_PROC47, GPC_PROC46, GPC_PROC45, GPC_PROC44, GPC_PROC43, GPC_PROC42, GPC_PROC41, 	\
GPC_PROC40, GPC_PROC39, GPC_PROC38, GPC_PROC37, GPC_PROC36, GPC_PROC35, GPC_PROC34, GPC_PROC33, 	\
GPC_PROC32, GPC_PROC31, GPC_PROC30, GPC_PROC29, GPC_PROC28, GPC_PROC27, GPC_PROC26, GPC_PROC25, 	\
GPC_PROC24, GPC_PROC23, GPC_PROC22, GPC_PROC21, GPC_PROC20, GPC_PROC19, GPC_PROC18, GPC_PROC17, 	\
GPC_PROC16, GPC_PROC15, GPC_PROC14, GPC_PROC13, GPC_PROC12, GPC_PROC11, GPC_PROC10, GPC_PROC9, 	\
GPC_PROC8, GPC_PROC7, GPC_PROC6, GPC_PROC5, GPC_PROC4, GPC_PROC3, GPC_PROC2, GPC_PROC1)	\
(FUNC, SEPARATOR, __VA_ARGS__)

#define GPC_PROCESS_ALL_BUT_1ST(FUNC, SEPARATOR, ...) GPC_OVERLOAD64(__VA_ARGS__, 	\
GPC_PROC64_1, GPC_PROC63_1, GPC_PROC62_1, GPC_PROC61_1, GPC_PROC60_1, GPC_PROC59_1, GPC_PROC58_1, 	\
GPC_PROC57_1, GPC_PROC56_1, GPC_PROC55_1, GPC_PROC54_1, GPC_PROC53_1, GPC_PROC52_1, GPC_PROC51_1, 	\
GPC_PROC50_1, GPC_PROC49_1, GPC_PROC48_1, GPC_PROC47_1, GPC_PROC46_1, GPC_PROC45_1, GPC_PROC44_1, 	\
GPC_PROC43_1, GPC_PROC42_1, GPC_PROC41_1, GPC_PROC40_1, GPC_PROC39_1, GPC_PROC38_1, GPC_PROC37_1, 	\
GPC_PROC36_1, GPC_PROC35_1, GPC_PROC34_1, GPC_PROC33_1, GPC_PROC32_1, GPC_PROC31_1, GPC_PROC30_1, 	\
GPC_PROC29_1, GPC_PROC28_1, GPC_PROC27_1, GPC_PROC26_1, GPC_PROC25_1, GPC_PROC24_1, GPC_PROC23_1, 	\
GPC_PROC22_1, GPC_PROC21_1, GPC_PROC20_1, GPC_PROC19_1, GPC_PROC18_1, GPC_PROC17_1, GPC_PROC16_1, 	\
GPC_PROC15_1, GPC_PROC14_1, GPC_PROC13_1, GPC_PROC12_1, GPC_PROC11_1, GPC_PROC10_1, GPC_PROC9_1, 	\
GPC_PROC8_1, GPC_PROC7_1, GPC_PROC6_1, GPC_PROC5_1, GPC_PROC4_1, GPC_PROC3_1, GPC_PROC2_1, 	\
GPC_PROC1_1)(FUNC, SEPARATOR, __VA_ARGS__)

#define GPC_PROC1(F, SEP, A) F(A)
#define GPC_PROC2(F, SEP, A, ...) F(A) SEP(A) GPC_PROC1(F, SEP, __VA_ARGS__)
#define GPC_PROC3(F, SEP, A, ...) F(A) SEP(A) GPC_PROC2(F, SEP, __VA_ARGS__)
#define GPC_PROC4(F, SEP, A, ...) F(A) SEP(A) GPC_PROC3(F, SEP, __VA_ARGS__)
#define GPC_PROC5(F, SEP, A, ...) F(A) SEP(A) GPC_PROC4(F, SEP, __VA_ARGS__)
#define GPC_PROC6(F, SEP, A, ...) F(A) SEP(A) GPC_PROC5(F, SEP, __VA_ARGS__)
#define GPC_PROC7(F, SEP, A, ...) F(A) SEP(A) GPC_PROC6(F, SEP, __VA_ARGS__)
#define GPC_PROC8(F, SEP, A, ...) F(A) SEP(A) GPC_PROC7(F, SEP, __VA_ARGS__)
#define GPC_PROC9(F, SEP, A, ...) F(A) SEP(A) GPC_PROC8(F, SEP, __VA_ARGS__)
#define GPC_PROC10(F, SEP, A, ...) F(A) SEP(A) GPC_PROC9(F, SEP, __VA_ARGS__)
#define GPC_PROC11(F, SEP, A, ...) F(A) SEP(A) GPC_PROC10(F, SEP, __VA_ARGS__)
#define GPC_PROC12(F, SEP, A, ...) F(A) SEP(A) GPC_PROC11(F, SEP, __VA_ARGS__)
#define GPC_PROC13(F, SEP, A, ...) F(A) SEP(A) GPC_PROC12(F, SEP, __VA_ARGS__)
#define GPC_PROC14(F, SEP, A, ...) F(A) SEP(A) GPC_PROC13(F, SEP, __VA_ARGS__)
#define GPC_PROC15(F, SEP, A, ...) F(A) SEP(A) GPC_PROC14(F, SEP, __VA_ARGS__)
#define GPC_PROC16(F, SEP, A, ...) F(A) SEP(A) GPC_PROC15(F, SEP, __VA_ARGS__)
#define GPC_PROC17(F, SEP, A, ...) F(A) SEP(A) GPC_PROC16(F, SEP, __VA_ARGS__)
#define GPC_PROC18(F, SEP, A, ...) F(A) SEP(A) GPC_PROC17(F, SEP, __VA_ARGS__)
#define GPC_PROC19(F, SEP, A, ...) F(A) SEP(A) GPC_PROC18(F, SEP, __VA_ARGS__)
#define GPC_PROC20(F, SEP, A, ...) F(A) SEP(A) GPC_PROC19(F, SEP, __VA_ARGS__)
#define GPC_PROC21(F, SEP, A, ...) F(A) SEP(A) GPC_PROC20(F, SEP, __VA_ARGS__)
#define GPC_PROC22(F, SEP, A, ...) F(A) SEP(A) GPC_PROC21(F, SEP, __VA_ARGS__)
#define GPC_PROC23(F, SEP, A, ...) F(A) SEP(A) GPC_PROC22(F, SEP, __VA_ARGS__)
#define GPC_PROC24(F, SEP, A, ...) F(A) SEP(A) GPC_PROC23(F, SEP, __VA_ARGS__)
#define GPC_PROC25(F, SEP, A, ...) F(A) SEP(A) GPC_PROC24(F, SEP, __VA_ARGS__)
#define GPC_PROC26(F, SEP, A, ...) F(A) SEP(A) GPC_PROC25(F, SEP, __VA_ARGS__)
#define GPC_PROC27(F, SEP, A, ...) F(A) SEP(A) GPC_PROC26(F, SEP, __VA_ARGS__)
#define GPC_PROC28(F, SEP, A, ...) F(A) SEP(A) GPC_PROC27(F, SEP, __VA_ARGS__)
#define GPC_PROC29(F, SEP, A, ...) F(A) SEP(A) GPC_PROC28(F, SEP, __VA_ARGS__)
#define GPC_PROC30(F, SEP, A, ...) F(A) SEP(A) GPC_PROC29(F, SEP, __VA_ARGS__)
#define GPC_PROC31(F, SEP, A, ...) F(A) SEP(A) GPC_PROC30(F, SEP, __VA_ARGS__)
#define GPC_PROC32(F, SEP, A, ...) F(A) SEP(A) GPC_PROC31(F, SEP, __VA_ARGS__)
#define GPC_PROC33(F, SEP, A, ...) F(A) SEP(A) GPC_PROC32(F, SEP, __VA_ARGS__)
#define GPC_PROC34(F, SEP, A, ...) F(A) SEP(A) GPC_PROC33(F, SEP, __VA_ARGS__)
#define GPC_PROC35(F, SEP, A, ...) F(A) SEP(A) GPC_PROC34(F, SEP, __VA_ARGS__)
#define GPC_PROC36(F, SEP, A, ...) F(A) SEP(A) GPC_PROC35(F, SEP, __VA_ARGS__)
#define GPC_PROC37(F, SEP, A, ...) F(A) SEP(A) GPC_PROC36(F, SEP, __VA_ARGS__)
#define GPC_PROC38(F, SEP, A, ...) F(A) SEP(A) GPC_PROC37(F, SEP, __VA_ARGS__)
#define GPC_PROC39(F, SEP, A, ...) F(A) SEP(A) GPC_PROC38(F, SEP, __VA_ARGS__)
#define GPC_PROC40(F, SEP, A, ...) F(A) SEP(A) GPC_PROC39(F, SEP, __VA_ARGS__)
#define GPC_PROC41(F, SEP, A, ...) F(A) SEP(A) GPC_PROC40(F, SEP, __VA_ARGS__)
#define GPC_PROC42(F, SEP, A, ...) F(A) SEP(A) GPC_PROC41(F, SEP, __VA_ARGS__)
#define GPC_PROC43(F, SEP, A, ...) F(A) SEP(A) GPC_PROC42(F, SEP, __VA_ARGS__)
#define GPC_PROC44(F, SEP, A, ...) F(A) SEP(A) GPC_PROC43(F, SEP, __VA_ARGS__)
#define GPC_PROC45(F, SEP, A, ...) F(A) SEP(A) GPC_PROC44(F, SEP, __VA_ARGS__)
#define GPC_PROC46(F, SEP, A, ...) F(A) SEP(A) GPC_PROC45(F, SEP, __VA_ARGS__)
#define GPC_PROC47(F, SEP, A, ...) F(A) SEP(A) GPC_PROC46(F, SEP, __VA_ARGS__)
#define GPC_PROC48(F, SEP, A, ...) F(A) SEP(A) GPC_PROC47(F, SEP, __VA_ARGS__)
#define GPC_PROC49(F, SEP, A, ...) F(A) SEP(A) GPC_PROC48(F, SEP, __VA_ARGS__)
#define GPC_PROC50(F, SEP, A, ...) F(A) SEP(A) GPC_PROC49(F, SEP, __VA_ARGS__)
#define GPC_PROC51(F, SEP, A, ...) F(A) SEP(A) GPC_PROC50(F, SEP, __VA_ARGS__)
#define GPC_PROC52(F, SEP, A, ...) F(A) SEP(A) GPC_PROC51(F, SEP, __VA_ARGS__)
#define GPC_PROC53(F, SEP, A, ...) F(A) SEP(A) GPC_PROC52(F, SEP, __VA_ARGS__)
#define GPC_PROC54(F, SEP, A, ...) F(A) SEP(A) GPC_PROC53(F, SEP, __VA_ARGS__)
#define GPC_PROC55(F, SEP, A, ...) F(A) SEP(A) GPC_PROC54(F, SEP, __VA_ARGS__)
#define GPC_PROC56(F, SEP, A, ...) F(A) SEP(A) GPC_PROC55(F, SEP, __VA_ARGS__)
#define GPC_PROC57(F, SEP, A, ...) F(A) SEP(A) GPC_PROC56(F, SEP, __VA_ARGS__)
#define GPC_PROC58(F, SEP, A, ...) F(A) SEP(A) GPC_PROC57(F, SEP, __VA_ARGS__)
#define GPC_PROC59(F, SEP, A, ...) F(A) SEP(A) GPC_PROC58(F, SEP, __VA_ARGS__)
#define GPC_PROC60(F, SEP, A, ...) F(A) SEP(A) GPC_PROC59(F, SEP, __VA_ARGS__)
#define GPC_PROC61(F, SEP, A, ...) F(A) SEP(A) GPC_PROC60(F, SEP, __VA_ARGS__)
#define GPC_PROC62(F, SEP, A, ...) F(A) SEP(A) GPC_PROC61(F, SEP, __VA_ARGS__)
#define GPC_PROC63(F, SEP, A, ...) F(A) SEP(A) GPC_PROC62(F, SEP, __VA_ARGS__)
#define GPC_PROC64(F, SEP, A, ...) F(A) SEP(A) GPC_PROC63(F, SEP, __VA_ARGS__)

#define GPC_PROC1_1(F, SEP, A) A
#define GPC_PROC2_1(F, SEP, A, ...) A, GPC_PROC1(F, SEP, __VA_ARGS__)
#define GPC_PROC3_1(F, SEP, A, ...) A, GPC_PROC2(F, SEP, __VA_ARGS__)
#define GPC_PROC4_1(F, SEP, A, ...) A, GPC_PROC3(F, SEP, __VA_ARGS__)
#define GPC_PROC5_1(F, SEP, A, ...) A, GPC_PROC4(F, SEP, __VA_ARGS__)
#define GPC_PROC6_1(F, SEP, A, ...) A, GPC_PROC5(F, SEP, __VA_ARGS__)
#define GPC_PROC7_1(F, SEP, A, ...) A, GPC_PROC6(F, SEP, __VA_ARGS__)
#define GPC_PROC8_1(F, SEP, A, ...) A, GPC_PROC7(F, SEP, __VA_ARGS__)
#define GPC_PROC9_1(F, SEP, A, ...) A, GPC_PROC8(F, SEP, __VA_ARGS__)
#define GPC_PROC10_1(F, SEP, A, ...) A, GPC_PROC9(F, SEP, __VA_ARGS__)
#define GPC_PROC11_1(F, SEP, A, ...) A, GPC_PROC10(F, SEP, __VA_ARGS__)
#define GPC_PROC12_1(F, SEP, A, ...) A, GPC_PROC11(F, SEP, __VA_ARGS__)
#define GPC_PROC13_1(F, SEP, A, ...) A, GPC_PROC12(F, SEP, __VA_ARGS__)
#define GPC_PROC14_1(F, SEP, A, ...) A, GPC_PROC13(F, SEP, __VA_ARGS__)
#define GPC_PROC15_1(F, SEP, A, ...) A, GPC_PROC14(F, SEP, __VA_ARGS__)
#define GPC_PROC16_1(F, SEP, A, ...) A, GPC_PROC15(F, SEP, __VA_ARGS__)
#define GPC_PROC17_1(F, SEP, A, ...) A, GPC_PROC16(F, SEP, __VA_ARGS__)
#define GPC_PROC18_1(F, SEP, A, ...) A, GPC_PROC17(F, SEP, __VA_ARGS__)
#define GPC_PROC19_1(F, SEP, A, ...) A, GPC_PROC18(F, SEP, __VA_ARGS__)
#define GPC_PROC20_1(F, SEP, A, ...) A, GPC_PROC19(F, SEP, __VA_ARGS__)
#define GPC_PROC21_1(F, SEP, A, ...) A, GPC_PROC20(F, SEP, __VA_ARGS__)
#define GPC_PROC22_1(F, SEP, A, ...) A, GPC_PROC21(F, SEP, __VA_ARGS__)
#define GPC_PROC23_1(F, SEP, A, ...) A, GPC_PROC22(F, SEP, __VA_ARGS__)
#define GPC_PROC24_1(F, SEP, A, ...) A, GPC_PROC23(F, SEP, __VA_ARGS__)
#define GPC_PROC25_1(F, SEP, A, ...) A, GPC_PROC24(F, SEP, __VA_ARGS__)
#define GPC_PROC26_1(F, SEP, A, ...) A, GPC_PROC25(F, SEP, __VA_ARGS__)
#define GPC_PROC27_1(F, SEP, A, ...) A, GPC_PROC26(F, SEP, __VA_ARGS__)
#define GPC_PROC28_1(F, SEP, A, ...) A, GPC_PROC27(F, SEP, __VA_ARGS__)
#define GPC_PROC29_1(F, SEP, A, ...) A, GPC_PROC28(F, SEP, __VA_ARGS__)
#define GPC_PROC30_1(F, SEP, A, ...) A, GPC_PROC29(F, SEP, __VA_ARGS__)
#define GPC_PROC31_1(F, SEP, A, ...) A, GPC_PROC30(F, SEP, __VA_ARGS__)
#define GPC_PROC32_1(F, SEP, A, ...) A, GPC_PROC31(F, SEP, __VA_ARGS__)
#define GPC_PROC33_1(F, SEP, A, ...) A, GPC_PROC32(F, SEP, __VA_ARGS__)
#define GPC_PROC34_1(F, SEP, A, ...) A, GPC_PROC33(F, SEP, __VA_ARGS__)
#define GPC_PROC35_1(F, SEP, A, ...) A, GPC_PROC34(F, SEP, __VA_ARGS__)
#define GPC_PROC36_1(F, SEP, A, ...) A, GPC_PROC35(F, SEP, __VA_ARGS__)
#define GPC_PROC37_1(F, SEP, A, ...) A, GPC_PROC36(F, SEP, __VA_ARGS__)
#define GPC_PROC38_1(F, SEP, A, ...) A, GPC_PROC37(F, SEP, __VA_ARGS__)
#define GPC_PROC39_1(F, SEP, A, ...) A, GPC_PROC38(F, SEP, __VA_ARGS__)
#define GPC_PROC40_1(F, SEP, A, ...) A, GPC_PROC39(F, SEP, __VA_ARGS__)
#define GPC_PROC41_1(F, SEP, A, ...) A, GPC_PROC40(F, SEP, __VA_ARGS__)
#define GPC_PROC42_1(F, SEP, A, ...) A, GPC_PROC41(F, SEP, __VA_ARGS__)
#define GPC_PROC43_1(F, SEP, A, ...) A, GPC_PROC42(F, SEP, __VA_ARGS__)
#define GPC_PROC44_1(F, SEP, A, ...) A, GPC_PROC43(F, SEP, __VA_ARGS__)
#define GPC_PROC45_1(F, SEP, A, ...) A, GPC_PROC44(F, SEP, __VA_ARGS__)
#define GPC_PROC46_1(F, SEP, A, ...) A, GPC_PROC45(F, SEP, __VA_ARGS__)
#define GPC_PROC47_1(F, SEP, A, ...) A, GPC_PROC46(F, SEP, __VA_ARGS__)
#define GPC_PROC48_1(F, SEP, A, ...) A, GPC_PROC47(F, SEP, __VA_ARGS__)
#define GPC_PROC49_1(F, SEP, A, ...) A, GPC_PROC48(F, SEP, __VA_ARGS__)
#define GPC_PROC50_1(F, SEP, A, ...) A, GPC_PROC49(F, SEP, __VA_ARGS__)
#define GPC_PROC51_1(F, SEP, A, ...) A, GPC_PROC50(F, SEP, __VA_ARGS__)
#define GPC_PROC52_1(F, SEP, A, ...) A, GPC_PROC51(F, SEP, __VA_ARGS__)
#define GPC_PROC53_1(F, SEP, A, ...) A, GPC_PROC52(F, SEP, __VA_ARGS__)
#define GPC_PROC54_1(F, SEP, A, ...) A, GPC_PROC53(F, SEP, __VA_ARGS__)
#define GPC_PROC55_1(F, SEP, A, ...) A, GPC_PROC54(F, SEP, __VA_ARGS__)
#define GPC_PROC56_1(F, SEP, A, ...) A, GPC_PROC55(F, SEP, __VA_ARGS__)
#define GPC_PROC57_1(F, SEP, A, ...) A, GPC_PROC56(F, SEP, __VA_ARGS__)
#define GPC_PROC58_1(F, SEP, A, ...) A, GPC_PROC57(F, SEP, __VA_ARGS__)
#define GPC_PROC59_1(F, SEP, A, ...) A, GPC_PROC58(F, SEP, __VA_ARGS__)
#define GPC_PROC60_1(F, SEP, A, ...) A, GPC_PROC59(F, SEP, __VA_ARGS__)
#define GPC_PROC61_1(F, SEP, A, ...) A, GPC_PROC60(F, SEP, __VA_ARGS__)
#define GPC_PROC62_1(F, SEP, A, ...) A, GPC_PROC61(F, SEP, __VA_ARGS__)
#define GPC_PROC63_1(F, SEP, A, ...) A, GPC_PROC62(F, SEP, __VA_ARGS__)
#define GPC_PROC64_1(F, SEP, A, ...) A, GPC_PROC63(F, SEP, __VA_ARGS__)

#define GPC_OVERLOAD1(_0, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD2(_0, _1, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD3(_0, _1, _2, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD4(_0, _1, _2, _3, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD5(_0, _1, _2, _3, _4, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD6(_0, _1, _2, _3, _4, _5, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD7(_0, _1, _2, _3, _4, _5, _6, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD8(_0, _1, _2, _3, _4, _5, _6, _7, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD9(_0, _1, _2, _3, _4, _5, _6, _7, _8, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, RESOLVED, ...) 	\
RESOLVED
#define GPC_OVERLOAD12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, RESOLVED, ...) 	\
RESOLVED
#define GPC_OVERLOAD13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, RESOLVED, 	\
...) RESOLVED
#define GPC_OVERLOAD14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, 	\
RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, RESOLVED, ...) 	\
RESOLVED
#define GPC_OVERLOAD31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, RESOLVED, 	\
...) RESOLVED
#define GPC_OVERLOAD32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, 	\
RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, RESOLVED, ...) 	\
RESOLVED
#define GPC_OVERLOAD49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, RESOLVED, 	\
...) RESOLVED
#define GPC_OVERLOAD50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, 	\
RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, _58, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, _58, _59, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, RESOLVED, ...) RESOLVED
#define GPC_OVERLOAD64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, 	\
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, 	\
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, RESOLVED, ...) RESOLVED

#endif // GPC_OVERLOAD_INCLUDED
