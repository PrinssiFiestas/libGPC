// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file overload.h
 * Overloading macros
 */

#ifndef GP_OVERLOAD_INCLUDED
#define GP_OVERLOAD_INCLUDED 1

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// Macros in this file assume at most 64 arguments.


// Overloading by argument count
//
// Overloading functions and macro functions by the number of arguments can be
// done with OVERLOAD[N]() macros. First arg to OVERLOAD[N]() is always
// __VA_ARGS__ which is followed by names of functions/macros to be overloaded
// in descending order. Some compiler settings may also require trailing comma
// after the names.
//     The actual arguments also has to be given after OVERLOAD[N](). Zero
// arguments is not possible using these. If zero arguments is necessary, check
// this: https://github.com/jason-deng/C99FunctionOverload.
//
// Example for max 3 args:
/*
void func1(int arg1);
#define MACRO2(arg1, arg2) somefunc(arg1, arg2)
int func3(char arg1, void* arg2, const char* arg3);

// Note 3 in the name of the macro and the trailing comma.
#define func(...) OVERLOAD3(__VA_ARGS__, func3, MACRO2, func1,)(__VA_ARGS__)

int main(void)
{
    func(1);
    func(1, 2);
    func('1', (void*)2, "3");
}
*/

// Helper macros

#define GP_STRFY(A) #A
#define GP_STRFY_1ST_ARG(A, ...) #A
#define GP_1ST_ARG(A, ...) A
#define GP_ALL_BUT_1ST_ARG(A, ...) __VA_ARGS__
#define GP_COMMA(...) ,
#define GP_DUMP(...)
#define GP_EVAL(...) __VA_ARGS__
#define GP_EVAL1(A) A

// Processing variadic arguments
//
// Arguments list can be processed with GP_PROCESS_ALL_ARGS() macro. The first
// argument is a function or a macro that takes a single argument. This function
// processes the variadic argument list. The second argument determines a
// separator for the variadic argument list. It has to be a macro function that
// takes a variadic argument but just expands to the separator without doing
// anything to __VA_ARGS__ like GP_COMMA() Example uses below:
/*
    int add_one(int x) { return x + 1; }
    int array[] = { GP_PROCESS_ALL_ARGS(add_one, GP_COMMA, 3, 4, 5) };
    // The line above expands to
    int array[] = { add_one(3), add_one(4), add_one(5) };

    #define PLUS(...) +
    int sum = GP_PROCESS_ALL_ARGS(GP_EVAL, PLUS, 2, 3, 4, 5);
    // The line above expands to
    int sum = 2 + 3 + 4 + 5

    // Combining the above we can get sum of squares
    double square(double x) { return x*x; }
    double sum_of_squares = GP_LIST_ALL(square, PLUS, 3.14, .707);
    // expands to
    double sum_of_squares = square(3.14) + square(.707);
*/

// If __VA_OPT__() is needed with GP_PROCESS_ALL_ARGS(),
// GP_PROCESS_ALL_BUT_1ST() can be used instead. GP_PROCESS_ALL_BUT_1ST()
// processes every argument that is passed to it except the first one.
// __VA_OPT__() can be simulated by using the first argument as a required
// argument making all variadic arguments optional without needing __VA_OPT__().
// Example below:
/*
    int sq(int x) { return x * x; }
    #define PLUS(...) +

    // First argument required! In this case it's the format string.
    #define PRINT_SUM_OF_SQ(...) printf(GP_PROCESS_ALL_BUT_1ST(sq, PLUS, __VA_ARGS__)

    PRINT_INCREMENTED("%i", 1, 2, 3);
    // expands to
    printf("%i", sq(1) + sq(2) + sq(3));
*/

// ----------------------------------------------------------------------------

// typeof() operator. GNUC and MSVC already covers mostly used compilers, but
// not all compilers are supported.
#if __STDC_VERSION__ >= 202311L
#define GP_TYPEOF(...) typeof(__VA_ARGS__)
#elif (defined(__GNUC__) || defined(__TINYC__)) && !defined(GP_PEDANTIC)
#define GP_TYPEOF(...) typeof(__VA_ARGS__)
#elif defined(_MSC_VER) || defined(__GNUC__)
#define GP_TYPEOF(X) __typeof__(X)
#endif

// Use in variadic function arguments with GP_TYPE() macro
typedef enum gp_type
{
    GP_UNSIGNED_CHAR,
    GP_UNSIGNED_SHORT,
    GP_UNSIGNED,
    GP_UNSIGNED_LONG,
    GP_UNSIGNED_LONG_LONG,
    GP_BOOL,
    GP_SIGNED_CHAR,
    GP_CHAR,
    GP_SHORT,
    GP_INT,
    GP_LONG,
    GP_LONG_LONG,
    GP_FLOAT,
    GP_DOUBLE,
    GP_CHAR_PTR,
    GP_STRING,
    GP_PTR,
} GPType;

static inline size_t gp_sizeof(const GPType T) {
    switch (T) {
        case GP_CHAR: case GP_SIGNED_CHAR: case GP_UNSIGNED_CHAR:
            return sizeof(char);
        case GP_SHORT: case GP_UNSIGNED_SHORT:
            return sizeof(short);
        case GP_BOOL:
            return sizeof(bool);
        case GP_INT: case GP_UNSIGNED:
            return sizeof(int);
        case GP_LONG: case GP_UNSIGNED_LONG:
            return sizeof(long);
        case GP_LONG_LONG: case GP_UNSIGNED_LONG_LONG:
            return sizeof(long long);
        case GP_FLOAT:
            return sizeof(float);
        case GP_DOUBLE:
            return sizeof(double);
        case GP_CHAR_PTR: case GP_STRING: case GP_PTR:
            return sizeof(char*);
    }
    return 0;
}

#if __cplusplus // defined with overloads
#elif !_MSC_VER
#define GP_TYPE(VAR)                              \
_Generic(VAR,                                     \
    bool:                  GP_BOOL,               \
    short:                 GP_SHORT,              \
    int:                   GP_INT,                \
    long:                  GP_LONG,               \
    long long:             GP_LONG_LONG,          \
    unsigned short:        GP_UNSIGNED_SHORT,     \
    unsigned int:          GP_UNSIGNED,           \
    unsigned long:         GP_UNSIGNED_LONG,      \
    unsigned long long:    GP_UNSIGNED_LONG_LONG, \
    float:                 GP_FLOAT,              \
    double:                GP_DOUBLE,             \
    char:                  GP_CHAR,               \
    unsigned char:         GP_UNSIGNED_CHAR,      \
    signed char:           GP_SIGNED_CHAR,        \
    char*:                 GP_CHAR_PTR,           \
    const char*:           GP_CHAR_PTR,           \
    struct gp_char*:       GP_STRING,             \
    default:               GP_PTR)
#else // MSVC char <=> signed char, although standard says that they are different
#define GP_TYPE(VAR)                              \
_Generic(VAR,                                     \
    bool:                  GP_BOOL,               \
    short:                 GP_SHORT,              \
    int:                   GP_INT,                \
    long:                  GP_LONG,               \
    long long:             GP_LONG_LONG,          \
    unsigned short:        GP_UNSIGNED_LONG,      \
    unsigned int:          GP_UNSIGNED,           \
    unsigned long:         GP_UNSIGNED_LONG,      \
    unsigned long long:    GP_UNSIGNED_LONG_LONG, \
    float:                 GP_FLOAT,              \
    double:                GP_DOUBLE,             \
    unsigned char:         GP_UNSIGNED_CHAR,      \
    signed char:           GP_SIGNED_CHAR,        \
    char*:                 GP_CHAR_PTR,           \
    const char*:           GP_CHAR_PTR,           \
    struct gp_char*:       GP_STRING,             \
    default:               GP_PTR)
#endif

static inline bool gp_is_unsigned(const GPType T) { return T <= GP_UNSIGNED_LONG_LONG; }
static inline bool gp_is_integer (const GPType T) { return T <= GP_LONG_LONG; }
static inline bool gp_is_floating(const GPType T) { return GP_FLOAT <= T && T <= GP_DOUBLE; }
static inline bool gp_is_pointer (const GPType T) { return GP_CHAR_PTR <= T && T <= GP_PTR; }

// Returns the number of arguments passed.
#define GP_COUNT_ARGS(...) GP_OVERLOAD64(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56,\
55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,  \
33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,  \
11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,)


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#if __clang__
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#if __STDC_VERSION__ <= 199901L
// Unique struct/union name
#define GP_MAKE_UNIQUE(A, B) A##B
#define GP_C99_UNIQUE_STRUCT(LINE) GP_MAKE_UNIQUE(_gp_uniqs__, LINE)
#else
// C11 allows structs and unions to be unnamed
#define GP_C99_UNIQUE_STRUCT(_)
#endif

// ----------------------------------------------------------------------------
// Script generated stuff

#define GP_PROCESS_ALL_ARGS(FUNC, SEPARATOR, ...) GP_OVERLOAD64(__VA_ARGS__, 	\
GP_PROC64, GP_PROC63, GP_PROC62, GP_PROC61, GP_PROC60, GP_PROC59, GP_PROC58, GP_PROC57, \
GP_PROC56, GP_PROC55, GP_PROC54, GP_PROC53, GP_PROC52, GP_PROC51, GP_PROC50, GP_PROC49, \
GP_PROC48, GP_PROC47, GP_PROC46, GP_PROC45, GP_PROC44, GP_PROC43, GP_PROC42, GP_PROC41, \
GP_PROC40, GP_PROC39, GP_PROC38, GP_PROC37, GP_PROC36, GP_PROC35, GP_PROC34, GP_PROC33, \
GP_PROC32, GP_PROC31, GP_PROC30, GP_PROC29, GP_PROC28, GP_PROC27, GP_PROC26, GP_PROC25, \
GP_PROC24, GP_PROC23, GP_PROC22, GP_PROC21, GP_PROC20, GP_PROC19, GP_PROC18, GP_PROC17, \
GP_PROC16, GP_PROC15, GP_PROC14, GP_PROC13, GP_PROC12, GP_PROC11, GP_PROC10, GP_PROC9, 	\
GP_PROC8, GP_PROC7, GP_PROC6, GP_PROC5, GP_PROC4, GP_PROC3, GP_PROC2, GP_PROC1,)	\
(FUNC, SEPARATOR, __VA_ARGS__)

#define GP_PROCESS_ALL_BUT_1ST(FUNC, SEPARATOR, ...) GP_OVERLOAD64(__VA_ARGS__, 	\
GP_PROC64_1, GP_PROC63_1, GP_PROC62_1, GP_PROC61_1, GP_PROC60_1, GP_PROC59_1, GP_PROC58_1, \
GP_PROC57_1, GP_PROC56_1, GP_PROC55_1, GP_PROC54_1, GP_PROC53_1, GP_PROC52_1, GP_PROC51_1, \
GP_PROC50_1, GP_PROC49_1, GP_PROC48_1, GP_PROC47_1, GP_PROC46_1, GP_PROC45_1, GP_PROC44_1, \
GP_PROC43_1, GP_PROC42_1, GP_PROC41_1, GP_PROC40_1, GP_PROC39_1, GP_PROC38_1, GP_PROC37_1, \
GP_PROC36_1, GP_PROC35_1, GP_PROC34_1, GP_PROC33_1, GP_PROC32_1, GP_PROC31_1, GP_PROC30_1, \
GP_PROC29_1, GP_PROC28_1, GP_PROC27_1, GP_PROC26_1, GP_PROC25_1, GP_PROC24_1, GP_PROC23_1, \
GP_PROC22_1, GP_PROC21_1, GP_PROC20_1, GP_PROC19_1, GP_PROC18_1, GP_PROC17_1, GP_PROC16_1, \
GP_PROC15_1, GP_PROC14_1, GP_PROC13_1, GP_PROC12_1, GP_PROC11_1, GP_PROC10_1, GP_PROC9_1, \
GP_PROC8_1, GP_PROC7_1, GP_PROC6_1, GP_PROC5_1, GP_PROC4_1, GP_PROC3_1, GP_PROC2_1, 	\
GP_PROC1_1,)(FUNC, SEPARATOR, __VA_ARGS__)

#define GP_PROC1(F, SEP, A) F(A)
#define GP_PROC2(F, SEP, A, ...) F(A) SEP(A) GP_PROC1(F, SEP, __VA_ARGS__)
#define GP_PROC3(F, SEP, A, ...) F(A) SEP(A) GP_PROC2(F, SEP, __VA_ARGS__)
#define GP_PROC4(F, SEP, A, ...) F(A) SEP(A) GP_PROC3(F, SEP, __VA_ARGS__)
#define GP_PROC5(F, SEP, A, ...) F(A) SEP(A) GP_PROC4(F, SEP, __VA_ARGS__)
#define GP_PROC6(F, SEP, A, ...) F(A) SEP(A) GP_PROC5(F, SEP, __VA_ARGS__)
#define GP_PROC7(F, SEP, A, ...) F(A) SEP(A) GP_PROC6(F, SEP, __VA_ARGS__)
#define GP_PROC8(F, SEP, A, ...) F(A) SEP(A) GP_PROC7(F, SEP, __VA_ARGS__)
#define GP_PROC9(F, SEP, A, ...) F(A) SEP(A) GP_PROC8(F, SEP, __VA_ARGS__)
#define GP_PROC10(F, SEP, A, ...) F(A) SEP(A) GP_PROC9(F, SEP, __VA_ARGS__)
#define GP_PROC11(F, SEP, A, ...) F(A) SEP(A) GP_PROC10(F, SEP, __VA_ARGS__)
#define GP_PROC12(F, SEP, A, ...) F(A) SEP(A) GP_PROC11(F, SEP, __VA_ARGS__)
#define GP_PROC13(F, SEP, A, ...) F(A) SEP(A) GP_PROC12(F, SEP, __VA_ARGS__)
#define GP_PROC14(F, SEP, A, ...) F(A) SEP(A) GP_PROC13(F, SEP, __VA_ARGS__)
#define GP_PROC15(F, SEP, A, ...) F(A) SEP(A) GP_PROC14(F, SEP, __VA_ARGS__)
#define GP_PROC16(F, SEP, A, ...) F(A) SEP(A) GP_PROC15(F, SEP, __VA_ARGS__)
#define GP_PROC17(F, SEP, A, ...) F(A) SEP(A) GP_PROC16(F, SEP, __VA_ARGS__)
#define GP_PROC18(F, SEP, A, ...) F(A) SEP(A) GP_PROC17(F, SEP, __VA_ARGS__)
#define GP_PROC19(F, SEP, A, ...) F(A) SEP(A) GP_PROC18(F, SEP, __VA_ARGS__)
#define GP_PROC20(F, SEP, A, ...) F(A) SEP(A) GP_PROC19(F, SEP, __VA_ARGS__)
#define GP_PROC21(F, SEP, A, ...) F(A) SEP(A) GP_PROC20(F, SEP, __VA_ARGS__)
#define GP_PROC22(F, SEP, A, ...) F(A) SEP(A) GP_PROC21(F, SEP, __VA_ARGS__)
#define GP_PROC23(F, SEP, A, ...) F(A) SEP(A) GP_PROC22(F, SEP, __VA_ARGS__)
#define GP_PROC24(F, SEP, A, ...) F(A) SEP(A) GP_PROC23(F, SEP, __VA_ARGS__)
#define GP_PROC25(F, SEP, A, ...) F(A) SEP(A) GP_PROC24(F, SEP, __VA_ARGS__)
#define GP_PROC26(F, SEP, A, ...) F(A) SEP(A) GP_PROC25(F, SEP, __VA_ARGS__)
#define GP_PROC27(F, SEP, A, ...) F(A) SEP(A) GP_PROC26(F, SEP, __VA_ARGS__)
#define GP_PROC28(F, SEP, A, ...) F(A) SEP(A) GP_PROC27(F, SEP, __VA_ARGS__)
#define GP_PROC29(F, SEP, A, ...) F(A) SEP(A) GP_PROC28(F, SEP, __VA_ARGS__)
#define GP_PROC30(F, SEP, A, ...) F(A) SEP(A) GP_PROC29(F, SEP, __VA_ARGS__)
#define GP_PROC31(F, SEP, A, ...) F(A) SEP(A) GP_PROC30(F, SEP, __VA_ARGS__)
#define GP_PROC32(F, SEP, A, ...) F(A) SEP(A) GP_PROC31(F, SEP, __VA_ARGS__)
#define GP_PROC33(F, SEP, A, ...) F(A) SEP(A) GP_PROC32(F, SEP, __VA_ARGS__)
#define GP_PROC34(F, SEP, A, ...) F(A) SEP(A) GP_PROC33(F, SEP, __VA_ARGS__)
#define GP_PROC35(F, SEP, A, ...) F(A) SEP(A) GP_PROC34(F, SEP, __VA_ARGS__)
#define GP_PROC36(F, SEP, A, ...) F(A) SEP(A) GP_PROC35(F, SEP, __VA_ARGS__)
#define GP_PROC37(F, SEP, A, ...) F(A) SEP(A) GP_PROC36(F, SEP, __VA_ARGS__)
#define GP_PROC38(F, SEP, A, ...) F(A) SEP(A) GP_PROC37(F, SEP, __VA_ARGS__)
#define GP_PROC39(F, SEP, A, ...) F(A) SEP(A) GP_PROC38(F, SEP, __VA_ARGS__)
#define GP_PROC40(F, SEP, A, ...) F(A) SEP(A) GP_PROC39(F, SEP, __VA_ARGS__)
#define GP_PROC41(F, SEP, A, ...) F(A) SEP(A) GP_PROC40(F, SEP, __VA_ARGS__)
#define GP_PROC42(F, SEP, A, ...) F(A) SEP(A) GP_PROC41(F, SEP, __VA_ARGS__)
#define GP_PROC43(F, SEP, A, ...) F(A) SEP(A) GP_PROC42(F, SEP, __VA_ARGS__)
#define GP_PROC44(F, SEP, A, ...) F(A) SEP(A) GP_PROC43(F, SEP, __VA_ARGS__)
#define GP_PROC45(F, SEP, A, ...) F(A) SEP(A) GP_PROC44(F, SEP, __VA_ARGS__)
#define GP_PROC46(F, SEP, A, ...) F(A) SEP(A) GP_PROC45(F, SEP, __VA_ARGS__)
#define GP_PROC47(F, SEP, A, ...) F(A) SEP(A) GP_PROC46(F, SEP, __VA_ARGS__)
#define GP_PROC48(F, SEP, A, ...) F(A) SEP(A) GP_PROC47(F, SEP, __VA_ARGS__)
#define GP_PROC49(F, SEP, A, ...) F(A) SEP(A) GP_PROC48(F, SEP, __VA_ARGS__)
#define GP_PROC50(F, SEP, A, ...) F(A) SEP(A) GP_PROC49(F, SEP, __VA_ARGS__)
#define GP_PROC51(F, SEP, A, ...) F(A) SEP(A) GP_PROC50(F, SEP, __VA_ARGS__)
#define GP_PROC52(F, SEP, A, ...) F(A) SEP(A) GP_PROC51(F, SEP, __VA_ARGS__)
#define GP_PROC53(F, SEP, A, ...) F(A) SEP(A) GP_PROC52(F, SEP, __VA_ARGS__)
#define GP_PROC54(F, SEP, A, ...) F(A) SEP(A) GP_PROC53(F, SEP, __VA_ARGS__)
#define GP_PROC55(F, SEP, A, ...) F(A) SEP(A) GP_PROC54(F, SEP, __VA_ARGS__)
#define GP_PROC56(F, SEP, A, ...) F(A) SEP(A) GP_PROC55(F, SEP, __VA_ARGS__)
#define GP_PROC57(F, SEP, A, ...) F(A) SEP(A) GP_PROC56(F, SEP, __VA_ARGS__)
#define GP_PROC58(F, SEP, A, ...) F(A) SEP(A) GP_PROC57(F, SEP, __VA_ARGS__)
#define GP_PROC59(F, SEP, A, ...) F(A) SEP(A) GP_PROC58(F, SEP, __VA_ARGS__)
#define GP_PROC60(F, SEP, A, ...) F(A) SEP(A) GP_PROC59(F, SEP, __VA_ARGS__)
#define GP_PROC61(F, SEP, A, ...) F(A) SEP(A) GP_PROC60(F, SEP, __VA_ARGS__)
#define GP_PROC62(F, SEP, A, ...) F(A) SEP(A) GP_PROC61(F, SEP, __VA_ARGS__)
#define GP_PROC63(F, SEP, A, ...) F(A) SEP(A) GP_PROC62(F, SEP, __VA_ARGS__)
#define GP_PROC64(F, SEP, A, ...) F(A) SEP(A) GP_PROC63(F, SEP, __VA_ARGS__)

#define GP_PROC1_1(F, SEP, A) A
#define GP_PROC2_1(F, SEP, A, ...) A, GP_PROC1(F, SEP, __VA_ARGS__)
#define GP_PROC3_1(F, SEP, A, ...) A, GP_PROC2(F, SEP, __VA_ARGS__)
#define GP_PROC4_1(F, SEP, A, ...) A, GP_PROC3(F, SEP, __VA_ARGS__)
#define GP_PROC5_1(F, SEP, A, ...) A, GP_PROC4(F, SEP, __VA_ARGS__)
#define GP_PROC6_1(F, SEP, A, ...) A, GP_PROC5(F, SEP, __VA_ARGS__)
#define GP_PROC7_1(F, SEP, A, ...) A, GP_PROC6(F, SEP, __VA_ARGS__)
#define GP_PROC8_1(F, SEP, A, ...) A, GP_PROC7(F, SEP, __VA_ARGS__)
#define GP_PROC9_1(F, SEP, A, ...) A, GP_PROC8(F, SEP, __VA_ARGS__)
#define GP_PROC10_1(F, SEP, A, ...) A, GP_PROC9(F, SEP, __VA_ARGS__)
#define GP_PROC11_1(F, SEP, A, ...) A, GP_PROC10(F, SEP, __VA_ARGS__)
#define GP_PROC12_1(F, SEP, A, ...) A, GP_PROC11(F, SEP, __VA_ARGS__)
#define GP_PROC13_1(F, SEP, A, ...) A, GP_PROC12(F, SEP, __VA_ARGS__)
#define GP_PROC14_1(F, SEP, A, ...) A, GP_PROC13(F, SEP, __VA_ARGS__)
#define GP_PROC15_1(F, SEP, A, ...) A, GP_PROC14(F, SEP, __VA_ARGS__)
#define GP_PROC16_1(F, SEP, A, ...) A, GP_PROC15(F, SEP, __VA_ARGS__)
#define GP_PROC17_1(F, SEP, A, ...) A, GP_PROC16(F, SEP, __VA_ARGS__)
#define GP_PROC18_1(F, SEP, A, ...) A, GP_PROC17(F, SEP, __VA_ARGS__)
#define GP_PROC19_1(F, SEP, A, ...) A, GP_PROC18(F, SEP, __VA_ARGS__)
#define GP_PROC20_1(F, SEP, A, ...) A, GP_PROC19(F, SEP, __VA_ARGS__)
#define GP_PROC21_1(F, SEP, A, ...) A, GP_PROC20(F, SEP, __VA_ARGS__)
#define GP_PROC22_1(F, SEP, A, ...) A, GP_PROC21(F, SEP, __VA_ARGS__)
#define GP_PROC23_1(F, SEP, A, ...) A, GP_PROC22(F, SEP, __VA_ARGS__)
#define GP_PROC24_1(F, SEP, A, ...) A, GP_PROC23(F, SEP, __VA_ARGS__)
#define GP_PROC25_1(F, SEP, A, ...) A, GP_PROC24(F, SEP, __VA_ARGS__)
#define GP_PROC26_1(F, SEP, A, ...) A, GP_PROC25(F, SEP, __VA_ARGS__)
#define GP_PROC27_1(F, SEP, A, ...) A, GP_PROC26(F, SEP, __VA_ARGS__)
#define GP_PROC28_1(F, SEP, A, ...) A, GP_PROC27(F, SEP, __VA_ARGS__)
#define GP_PROC29_1(F, SEP, A, ...) A, GP_PROC28(F, SEP, __VA_ARGS__)
#define GP_PROC30_1(F, SEP, A, ...) A, GP_PROC29(F, SEP, __VA_ARGS__)
#define GP_PROC31_1(F, SEP, A, ...) A, GP_PROC30(F, SEP, __VA_ARGS__)
#define GP_PROC32_1(F, SEP, A, ...) A, GP_PROC31(F, SEP, __VA_ARGS__)
#define GP_PROC33_1(F, SEP, A, ...) A, GP_PROC32(F, SEP, __VA_ARGS__)
#define GP_PROC34_1(F, SEP, A, ...) A, GP_PROC33(F, SEP, __VA_ARGS__)
#define GP_PROC35_1(F, SEP, A, ...) A, GP_PROC34(F, SEP, __VA_ARGS__)
#define GP_PROC36_1(F, SEP, A, ...) A, GP_PROC35(F, SEP, __VA_ARGS__)
#define GP_PROC37_1(F, SEP, A, ...) A, GP_PROC36(F, SEP, __VA_ARGS__)
#define GP_PROC38_1(F, SEP, A, ...) A, GP_PROC37(F, SEP, __VA_ARGS__)
#define GP_PROC39_1(F, SEP, A, ...) A, GP_PROC38(F, SEP, __VA_ARGS__)
#define GP_PROC40_1(F, SEP, A, ...) A, GP_PROC39(F, SEP, __VA_ARGS__)
#define GP_PROC41_1(F, SEP, A, ...) A, GP_PROC40(F, SEP, __VA_ARGS__)
#define GP_PROC42_1(F, SEP, A, ...) A, GP_PROC41(F, SEP, __VA_ARGS__)
#define GP_PROC43_1(F, SEP, A, ...) A, GP_PROC42(F, SEP, __VA_ARGS__)
#define GP_PROC44_1(F, SEP, A, ...) A, GP_PROC43(F, SEP, __VA_ARGS__)
#define GP_PROC45_1(F, SEP, A, ...) A, GP_PROC44(F, SEP, __VA_ARGS__)
#define GP_PROC46_1(F, SEP, A, ...) A, GP_PROC45(F, SEP, __VA_ARGS__)
#define GP_PROC47_1(F, SEP, A, ...) A, GP_PROC46(F, SEP, __VA_ARGS__)
#define GP_PROC48_1(F, SEP, A, ...) A, GP_PROC47(F, SEP, __VA_ARGS__)
#define GP_PROC49_1(F, SEP, A, ...) A, GP_PROC48(F, SEP, __VA_ARGS__)
#define GP_PROC50_1(F, SEP, A, ...) A, GP_PROC49(F, SEP, __VA_ARGS__)
#define GP_PROC51_1(F, SEP, A, ...) A, GP_PROC50(F, SEP, __VA_ARGS__)
#define GP_PROC52_1(F, SEP, A, ...) A, GP_PROC51(F, SEP, __VA_ARGS__)
#define GP_PROC53_1(F, SEP, A, ...) A, GP_PROC52(F, SEP, __VA_ARGS__)
#define GP_PROC54_1(F, SEP, A, ...) A, GP_PROC53(F, SEP, __VA_ARGS__)
#define GP_PROC55_1(F, SEP, A, ...) A, GP_PROC54(F, SEP, __VA_ARGS__)
#define GP_PROC56_1(F, SEP, A, ...) A, GP_PROC55(F, SEP, __VA_ARGS__)
#define GP_PROC57_1(F, SEP, A, ...) A, GP_PROC56(F, SEP, __VA_ARGS__)
#define GP_PROC58_1(F, SEP, A, ...) A, GP_PROC57(F, SEP, __VA_ARGS__)
#define GP_PROC59_1(F, SEP, A, ...) A, GP_PROC58(F, SEP, __VA_ARGS__)
#define GP_PROC60_1(F, SEP, A, ...) A, GP_PROC59(F, SEP, __VA_ARGS__)
#define GP_PROC61_1(F, SEP, A, ...) A, GP_PROC60(F, SEP, __VA_ARGS__)
#define GP_PROC62_1(F, SEP, A, ...) A, GP_PROC61(F, SEP, __VA_ARGS__)
#define GP_PROC63_1(F, SEP, A, ...) A, GP_PROC62(F, SEP, __VA_ARGS__)
#define GP_PROC64_1(F, SEP, A, ...) A, GP_PROC63(F, SEP, __VA_ARGS__)

#define GP_OVERLOAD1(_0, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD2(_0, _1, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD3(_0, _1, _2, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD4(_0, _1, _2, _3, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD5(_0, _1, _2, _3, _4, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD6(_0, _1, _2, _3, _4, _5, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD7(_0, _1, _2, _3, _4, _5, _6, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD8(_0, _1, _2, _3, _4, _5, _6, _7, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD9(_0, _1, _2, _3, _4, _5, _6, _7, _8, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD10(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD11(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, RESOLVED, ...) 	\
RESOLVED
#define GP_OVERLOAD12(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, RESOLVED, ...) 	\
RESOLVED
#define GP_OVERLOAD13(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, RESOLVED, 	\
...) RESOLVED
#define GP_OVERLOAD14(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD15(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD16(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD17(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD18(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD19(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD20(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD21(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD22(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD23(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD25(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD26(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD27(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD28(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD29(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD30(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, RESOLVED, ...) \
RESOLVED
#define GP_OVERLOAD31(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, RESOLVED, \
...) RESOLVED
#define GP_OVERLOAD32(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD33(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD34(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD35(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD36(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD37(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD38(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD39(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD40(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD41(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD42(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD43(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD44(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD45(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD46(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD47(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD48(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, RESOLVED, ...) \
RESOLVED
#define GP_OVERLOAD49(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, RESOLVED, \
...) RESOLVED
#define GP_OVERLOAD50(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, 	\
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD51(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
RESOLVED, ...) RESOLVED
#define GP_OVERLOAD52(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD53(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD54(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD55(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD56(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD57(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD58(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD59(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD60(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD61(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD62(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD63(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, RESOLVED, ...) RESOLVED
#define GP_OVERLOAD64(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, RESOLVED, ...) RESOLVED

#ifdef __cplusplus
} // extern "C"

#if __cpp_decltype
#ifdef GP_TYPEOF
#undef GP_TYPEOF
#endif
#define GP_TYPEOF(...) decltype(__VA_ARGS__)
#endif

static inline GPType GP_TYPE(bool               x) { (void)x; return GP_BOOL;               }
static inline GPType GP_TYPE(short              x) { (void)x; return GP_SHORT;              }
static inline GPType GP_TYPE(int                x) { (void)x; return GP_INT;                }
static inline GPType GP_TYPE(long               x) { (void)x; return GP_LONG;               }
static inline GPType GP_TYPE(long long          x) { (void)x; return GP_LONG_LONG;          }
static inline GPType GP_TYPE(unsigned short     x) { (void)x; return GP_UNSIGNED_SHORT;     }
static inline GPType GP_TYPE(unsigned           x) { (void)x; return GP_UNSIGNED;           }
static inline GPType GP_TYPE(unsigned long      x) { (void)x; return GP_UNSIGNED_LONG;      }
static inline GPType GP_TYPE(unsigned long long x) { (void)x; return GP_UNSIGNED_LONG_LONG; }
static inline GPType GP_TYPE(float              x) { (void)x; return GP_FLOAT;              }
static inline GPType GP_TYPE(double             x) { (void)x; return GP_DOUBLE;             }
static inline GPType GP_TYPE(char               x) { (void)x; return GP_CHAR;               }
static inline GPType GP_TYPE(unsigned char      x) { (void)x; return GP_UNSIGNED_CHAR;      }
static inline GPType GP_TYPE(signed char        x) { (void)x; return GP_SIGNED_CHAR;        }
static inline GPType GP_TYPE(char*              x) { (void)x; return GP_CHAR_PTR;           }
static inline GPType GP_TYPE(const char*        x) { (void)x; return GP_CHAR_PTR;           }
static inline GPType GP_TYPE(struct gp_char*    x) { (void)x; return GP_STRING;             }
static inline GPType GP_TYPE(const void*        x) { (void)x; return GP_PTR;                }

#endif

#endif // GP_OVERLOAD_INCLUDED
