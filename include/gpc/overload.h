// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_OVERLOAD_H
#define GPC_OVERLOAD_H

#include <stdbool.h>
#include <stddef.h>

#define GPC_1ST_ARG(A, ...) A
#define GPC_COMMA(...) ,
#define GPC_DUMP(...) 
#define GPC_EVAL(...) __VA_ARGS__

// Define one of these macros before including this header to enable short names
// without the gpc_ prefix. 
#if defined(GPC_OVERLOAD_NAMESPACE) || defined(GPC_NAMESPACE)

// Overload function by the number of arguments up to N. Usage example:
/*
    int f1Arg(int arg) { return arg; }
    #define f2Args(a, b) a + b
    void f3Args(int i1, int i2, int i3) { printf("%i%i%i\n", i1, i2, i3); }
    #define func(...) OVERLOAD3(__VA_ARGS__, f3Args, f2Args, f1Arg)(__VA_ARGS__)
*/
// Replace N with max arguments like in the example above.
#define OVERLOADN(...) GPC_OVERLOADN(__VA_ARGS__)

// Returns EXPR_IF_TRUE if VAR is a number
// char, unsigned char, and bool types are not considered as numbers. If int8_t 
// (aka char), uint8_t (aka unsigned char), or bool needs to be considered as 
// numbers, use IF_IS_NUMERIC() instead.
// enum types are not numbers on MSVC. 
#define IF_IS_NUMBER(VARIABLE, EXPR_IF_TRUE, EXPR_IF_FALSE) \
    GPC_IF_IS_NUMBER(VARIABLE, EXPR_IF_TRUE, EXPR_IF_FALSE)

// Returns EXPR_IF_TRUE if VAR is a number, char, or bool
// enum types are not numberic on MSVC. 
#define IF_IS_NUMERIC(VARIABLE, EXPR_IF_TRUE, EXPR_IF_FALSE) \
    GPC_IF_IS_NUMERIC(VARIABLE, EXPR_IF_TRUE, EXPR_IF_FALSE)

// Returns number of arguments
#define COUNT_ARGS(...) GPC_COUNT_ARGS(__VA_ARGS__)

// List all variadic arguments separated with OP and processed with FUNC
// OP needs to be a macro function that takes a dummy argument e.g. GPC_COMMA().
#define LIST_ALL(FUNC, OP, ...) GPC_LIST_ALL(FUNC, OP, __VA_ARGS__)

#endif // GPC_NAMESPACING ----------------------------------------------------

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

inline bool gpc_isUnsigned(const enum gpc_Type T) { return T <= GPC_UNSIGNED_LONG_LONG; }
inline bool gpc_isInteger(const enum gpc_Type T) { return T <= GPC_LONG_LONG; }
inline bool gpc_isFloating(const enum gpc_Type T) { return GPC_FLOAT <= T && T <= GPC_DOUBLE; }
inline bool gpc_isPointer(const enum gpc_Type T) { return GPC_CHAR_PTR <= T && T <= GPC_PTR; }

size_t gpc_sizeof(const enum gpc_Type T);

#define GPC_TYPE(VAR)                                   \
    _Generic(VAR,                                       \
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

#define GPC_OVERLOAD(NARGS, ...) GPC_LIST##NARGS (GPC_DUMP, GPC_DUMP, GPC_1ST_ARG, __VA_ARGS__)

#define GPC_IF_IS_NUMBER(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) _Generic(VAR,       \
short: EXPR_IF_TRUE, unsigned short: EXPR_IF_TRUE, int: EXPR_IF_TRUE,          \
unsigned: EXPR_IF_TRUE, long: EXPR_IF_TRUE, unsigned long: EXPR_IF_TRUE,       \
long long: EXPR_IF_TRUE, unsigned long long: EXPR_IF_TRUE, float: EXPR_IF_TRUE,\
double: EXPR_IF_TRUE, long double: EXPR_IF_TRUE, default: EXPR_IF_FALSE)

#define GPC_IF_IS_CHAR(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) _Generic(VAR, \
char: EXPR_IF_TRUE, unsigned char: EXPR_IF_TRUE, default: EXPR_IF_FALSE)

// TODO check for C23 once it comes out
#if __STDC_VERSION__ >= 199901L
#define GPC_IF_IS_NUMERIC(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) _Generic(VAR, \
_Bool: EXPR_IF_TRUE, default: GPC_IF_IS_NUMBER(VAR, EXPR_IF_TRUE,         \
GPC_IF_IS_CHAR(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE)))
#else
#define GPC_IF_IS_NUMERIC(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE) \
GPC_IF_IS_NUMBER(VAR, EXPR_IF_TRUE, GPC_IF_IS_CHAR(VAR, EXPR_IF_TRUE, EXPR_IF_FALSE))
#endif

// Returns number of arguments
#define GPC_COUNT_ARGS(...) GPC_OVERLOAD(64, __VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56,\
55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,     \
33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,     \
11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

// ---------------------------------------------------------------------------

#define GPC_LIST_ALL(FUNC, OP, ...) GPC_CHOOSE_64TH(__VA_ARGS__, GPC_LIST64, GPC_LIST63,       \
GPC_LIST62, GPC_LIST61, GPC_LIST60, GPC_LIST59, GPC_LIST58, GPC_LIST57, GPC_LIST56, GPC_LIST55,\
GPC_LIST54, GPC_LIST53, GPC_LIST52, GPC_LIST51, GPC_LIST50, GPC_LIST49, GPC_LIST48, GPC_LIST47,\
GPC_LIST46, GPC_LIST45, GPC_LIST44, GPC_LIST43, GPC_LIST42, GPC_LIST41, GPC_LIST40, GPC_LIST39,\
GPC_LIST38, GPC_LIST37, GPC_LIST36, GPC_LIST35, GPC_LIST34, GPC_LIST33, GPC_LIST32, GPC_LIST31,\
GPC_LIST30, GPC_LIST29, GPC_LIST28, GPC_LIST27, GPC_LIST26, GPC_LIST25, GPC_LIST24, GPC_LIST23,\
GPC_LIST22, GPC_LIST21, GPC_LIST20, GPC_LIST19, GPC_LIST18, GPC_LIST17, GPC_LIST16, GPC_LIST15,\
GPC_LIST14, GPC_LIST13, GPC_LIST12, GPC_LIST11, GPC_LIST10, GPC_LIST9, GPC_LIST8, GPC_LIST7,   \
GPC_LIST6, GPC_LIST5, GPC_LIST4, GPC_LIST3, GPC_LIST2, GPC_LIST1)(FUNC, OP, GPC_DUMP,__VA_ARGS__)

#define GPC_LIST1(FUNC,OP,LOVERS,A,...) FUNC(A) LOVERS(__VA_ARGS__)
#define GPC_LIST2(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST1(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST3(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST2(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST4(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST3(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST5(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST4(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST6(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST5(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST7(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST6(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST8(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST7(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST9(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST8(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST10(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST9(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST11(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST10(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST12(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST11(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST13(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST12(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST14(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST13(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST15(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST14(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST16(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST15(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST17(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST16(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST18(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST17(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST19(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST18(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST20(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST19(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST21(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST20(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST22(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST21(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST23(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST22(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST24(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST23(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST25(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST24(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST26(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST25(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST27(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST26(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST28(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST27(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST29(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST28(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST30(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST29(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST31(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST30(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST32(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST31(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST33(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST32(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST34(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST33(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST35(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST34(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST36(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST35(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST37(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST36(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST38(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST37(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST39(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST38(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST40(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST39(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST41(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST40(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST42(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST41(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST43(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST42(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST44(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST43(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST45(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST44(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST46(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST45(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST47(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST46(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST48(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST47(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST49(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST48(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST50(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST49(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST51(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST50(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST52(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST51(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST53(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST52(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST54(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST53(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST55(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST54(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST56(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST55(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST57(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST56(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST58(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST57(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST59(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST58(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST60(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST59(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST61(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST60(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST62(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST61(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST63(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST62(FUNC,OP,LOVERS,__VA_ARGS__)
#define GPC_LIST64(FUNC,OP,LOVERS,A,...) FUNC(A) OP(A) GPC_LIST63(FUNC,OP,LOVERS,__VA_ARGS__)

#define GPC_OVERLOAD1(...) GPC_CHOOSE_1TH(__VA_ARGS__)
#define GPC_OVERLOAD2(...) GPC_CHOOSE_2TH(__VA_ARGS__)
#define GPC_OVERLOAD3(...) GPC_CHOOSE_3TH(__VA_ARGS__)
#define GPC_OVERLOAD4(...) GPC_CHOOSE_4TH(__VA_ARGS__)
#define GPC_OVERLOAD5(...) GPC_CHOOSE_5TH(__VA_ARGS__)
#define GPC_OVERLOAD6(...) GPC_CHOOSE_6TH(__VA_ARGS__)
#define GPC_OVERLOAD7(...) GPC_CHOOSE_7TH(__VA_ARGS__)
#define GPC_OVERLOAD8(...) GPC_CHOOSE_8TH(__VA_ARGS__)
#define GPC_OVERLOAD9(...) GPC_CHOOSE_9TH(__VA_ARGS__)
#define GPC_OVERLOAD10(...) GPC_CHOOSE_10TH(__VA_ARGS__)
#define GPC_OVERLOAD11(...) GPC_CHOOSE_11TH(__VA_ARGS__)
#define GPC_OVERLOAD12(...) GPC_CHOOSE_12TH(__VA_ARGS__)
#define GPC_OVERLOAD13(...) GPC_CHOOSE_13TH(__VA_ARGS__)
#define GPC_OVERLOAD14(...) GPC_CHOOSE_14TH(__VA_ARGS__)
#define GPC_OVERLOAD15(...) GPC_CHOOSE_15TH(__VA_ARGS__)
#define GPC_OVERLOAD16(...) GPC_CHOOSE_16TH(__VA_ARGS__)
#define GPC_OVERLOAD17(...) GPC_CHOOSE_17TH(__VA_ARGS__)
#define GPC_OVERLOAD18(...) GPC_CHOOSE_18TH(__VA_ARGS__)
#define GPC_OVERLOAD19(...) GPC_CHOOSE_19TH(__VA_ARGS__)
#define GPC_OVERLOAD20(...) GPC_CHOOSE_20TH(__VA_ARGS__)
#define GPC_OVERLOAD21(...) GPC_CHOOSE_21TH(__VA_ARGS__)
#define GPC_OVERLOAD22(...) GPC_CHOOSE_22TH(__VA_ARGS__)
#define GPC_OVERLOAD23(...) GPC_CHOOSE_23TH(__VA_ARGS__)
#define GPC_OVERLOAD24(...) GPC_CHOOSE_24TH(__VA_ARGS__)
#define GPC_OVERLOAD25(...) GPC_CHOOSE_25TH(__VA_ARGS__)
#define GPC_OVERLOAD26(...) GPC_CHOOSE_26TH(__VA_ARGS__)
#define GPC_OVERLOAD27(...) GPC_CHOOSE_27TH(__VA_ARGS__)
#define GPC_OVERLOAD28(...) GPC_CHOOSE_28TH(__VA_ARGS__)
#define GPC_OVERLOAD29(...) GPC_CHOOSE_29TH(__VA_ARGS__)
#define GPC_OVERLOAD30(...) GPC_CHOOSE_30TH(__VA_ARGS__)
#define GPC_OVERLOAD31(...) GPC_CHOOSE_31TH(__VA_ARGS__)
#define GPC_OVERLOAD32(...) GPC_CHOOSE_32TH(__VA_ARGS__)
#define GPC_OVERLOAD33(...) GPC_CHOOSE_33TH(__VA_ARGS__)
#define GPC_OVERLOAD34(...) GPC_CHOOSE_34TH(__VA_ARGS__)
#define GPC_OVERLOAD35(...) GPC_CHOOSE_35TH(__VA_ARGS__)
#define GPC_OVERLOAD36(...) GPC_CHOOSE_36TH(__VA_ARGS__)
#define GPC_OVERLOAD37(...) GPC_CHOOSE_37TH(__VA_ARGS__)
#define GPC_OVERLOAD38(...) GPC_CHOOSE_38TH(__VA_ARGS__)
#define GPC_OVERLOAD39(...) GPC_CHOOSE_39TH(__VA_ARGS__)
#define GPC_OVERLOAD40(...) GPC_CHOOSE_40TH(__VA_ARGS__)
#define GPC_OVERLOAD41(...) GPC_CHOOSE_41TH(__VA_ARGS__)
#define GPC_OVERLOAD42(...) GPC_CHOOSE_42TH(__VA_ARGS__)
#define GPC_OVERLOAD43(...) GPC_CHOOSE_43TH(__VA_ARGS__)
#define GPC_OVERLOAD44(...) GPC_CHOOSE_44TH(__VA_ARGS__)
#define GPC_OVERLOAD45(...) GPC_CHOOSE_45TH(__VA_ARGS__)
#define GPC_OVERLOAD46(...) GPC_CHOOSE_46TH(__VA_ARGS__)
#define GPC_OVERLOAD47(...) GPC_CHOOSE_47TH(__VA_ARGS__)
#define GPC_OVERLOAD48(...) GPC_CHOOSE_48TH(__VA_ARGS__)
#define GPC_OVERLOAD49(...) GPC_CHOOSE_49TH(__VA_ARGS__)
#define GPC_OVERLOAD50(...) GPC_CHOOSE_50TH(__VA_ARGS__)
#define GPC_OVERLOAD51(...) GPC_CHOOSE_51TH(__VA_ARGS__)
#define GPC_OVERLOAD52(...) GPC_CHOOSE_52TH(__VA_ARGS__)
#define GPC_OVERLOAD53(...) GPC_CHOOSE_53TH(__VA_ARGS__)
#define GPC_OVERLOAD54(...) GPC_CHOOSE_54TH(__VA_ARGS__)
#define GPC_OVERLOAD55(...) GPC_CHOOSE_55TH(__VA_ARGS__)
#define GPC_OVERLOAD56(...) GPC_CHOOSE_56TH(__VA_ARGS__)
#define GPC_OVERLOAD57(...) GPC_CHOOSE_57TH(__VA_ARGS__)
#define GPC_OVERLOAD58(...) GPC_CHOOSE_58TH(__VA_ARGS__)
#define GPC_OVERLOAD59(...) GPC_CHOOSE_59TH(__VA_ARGS__)
#define GPC_OVERLOAD60(...) GPC_CHOOSE_60TH(__VA_ARGS__)
#define GPC_OVERLOAD61(...) GPC_CHOOSE_61TH(__VA_ARGS__)
#define GPC_OVERLOAD62(...) GPC_CHOOSE_62TH(__VA_ARGS__)
#define GPC_OVERLOAD63(...) GPC_CHOOSE_63TH(__VA_ARGS__)
#define GPC_OVERLOAD64(...) GPC_CHOOSE_64TH(__VA_ARGS__)

#define GPC_CHOOSE_1TH(_0, THIS, ...) THIS
#define GPC_CHOOSE_2TH(_0, _1, THIS, ...) THIS
#define GPC_CHOOSE_3TH(_0, _1, _2, THIS, ...) THIS
#define GPC_CHOOSE_4TH(_0, _1, _2, _3, THIS, ...) THIS
#define GPC_CHOOSE_5TH(_0, _1, _2, _3, _4, THIS, ...) THIS
#define GPC_CHOOSE_6TH(_0, _1, _2, _3, _4, _5, THIS, ...) THIS
#define GPC_CHOOSE_7TH(_0, _1, _2, _3, _4, _5, _6, THIS, ...) THIS
#define GPC_CHOOSE_8TH(_0, _1, _2, _3, _4, _5, _6, _7, THIS, ...) THIS
#define GPC_CHOOSE_9TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, THIS, ...) THIS
#define GPC_CHOOSE_10TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, THIS, ...) THIS
#define GPC_CHOOSE_11TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, THIS, ...) THIS
#define GPC_CHOOSE_12TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, THIS, ...) THIS
#define GPC_CHOOSE_13TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, THIS,     \
...) THIS
#define GPC_CHOOSE_14TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13,     \
THIS, ...) THIS
#define GPC_CHOOSE_15TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
THIS, ...) THIS
#define GPC_CHOOSE_16TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, THIS, ...) THIS
#define GPC_CHOOSE_17TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, THIS, ...) THIS
#define GPC_CHOOSE_18TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, THIS, ...) THIS
#define GPC_CHOOSE_19TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, THIS, ...) THIS
#define GPC_CHOOSE_20TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, THIS, ...) THIS
#define GPC_CHOOSE_21TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, THIS, ...) THIS
#define GPC_CHOOSE_22TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, THIS, ...) THIS
#define GPC_CHOOSE_23TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, THIS, ...) THIS
#define GPC_CHOOSE_24TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, THIS, ...) THIS
#define GPC_CHOOSE_25TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, THIS, ...) THIS
#define GPC_CHOOSE_26TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, THIS, ...) THIS
#define GPC_CHOOSE_27TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, THIS, ...) THIS
#define GPC_CHOOSE_28TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, THIS, ...) THIS
#define GPC_CHOOSE_29TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, THIS, ...) THIS
#define GPC_CHOOSE_30TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, THIS, ...) THIS
#define GPC_CHOOSE_31TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, THIS, ...)     \
THIS
#define GPC_CHOOSE_32TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, THIS,     \
...) THIS
#define GPC_CHOOSE_33TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
THIS, ...) THIS
#define GPC_CHOOSE_34TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, THIS, ...) THIS
#define GPC_CHOOSE_35TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, THIS, ...) THIS
#define GPC_CHOOSE_36TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, THIS, ...) THIS
#define GPC_CHOOSE_37TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, THIS, ...) THIS
#define GPC_CHOOSE_38TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, THIS, ...) THIS
#define GPC_CHOOSE_39TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, THIS, ...) THIS
#define GPC_CHOOSE_40TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, THIS, ...) THIS
#define GPC_CHOOSE_41TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, THIS, ...) THIS
#define GPC_CHOOSE_42TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, THIS, ...) THIS
#define GPC_CHOOSE_43TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, THIS, ...) THIS
#define GPC_CHOOSE_44TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, THIS, ...) THIS
#define GPC_CHOOSE_45TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, THIS, ...) THIS
#define GPC_CHOOSE_46TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, THIS, ...) THIS
#define GPC_CHOOSE_47TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, THIS, ...) THIS
#define GPC_CHOOSE_48TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, THIS, ...) THIS
#define GPC_CHOOSE_49TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, THIS, ...)     \
THIS
#define GPC_CHOOSE_50TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, THIS,     \
...) THIS
#define GPC_CHOOSE_51TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
THIS, ...) THIS
#define GPC_CHOOSE_52TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, THIS, ...) THIS
#define GPC_CHOOSE_53TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, THIS, ...) THIS
#define GPC_CHOOSE_54TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, THIS, ...) THIS
#define GPC_CHOOSE_55TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, THIS, ...) THIS
#define GPC_CHOOSE_56TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, THIS, ...) THIS
#define GPC_CHOOSE_57TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, THIS, ...) THIS
#define GPC_CHOOSE_58TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, THIS, ...) THIS
#define GPC_CHOOSE_59TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, _58, THIS, ...) THIS
#define GPC_CHOOSE_60TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, _58, _59, THIS, ...) THIS
#define GPC_CHOOSE_61TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, THIS, ...) THIS
#define GPC_CHOOSE_62TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, THIS, ...) THIS
#define GPC_CHOOSE_63TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, THIS, ...) THIS
#define GPC_CHOOSE_64TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,     \
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32,     \
_33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,     \
_51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, THIS, ...) THIS

#if defined(GPC_OVERLOAD_NAMESPACE) || defined(GPC_NAMESPACE)
#define OVERLOAD1(...) GPC_OVERLOAD1(__VA_ARGS__)
#define OVERLOAD2(...) GPC_OVERLOAD2(__VA_ARGS__)
#define OVERLOAD3(...) GPC_OVERLOAD3(__VA_ARGS__)
#define OVERLOAD4(...) GPC_OVERLOAD4(__VA_ARGS__)
#define OVERLOAD5(...) GPC_OVERLOAD5(__VA_ARGS__)
#define OVERLOAD6(...) GPC_OVERLOAD6(__VA_ARGS__)
#define OVERLOAD7(...) GPC_OVERLOAD7(__VA_ARGS__)
#define OVERLOAD8(...) GPC_OVERLOAD8(__VA_ARGS__)
#define OVERLOAD9(...) GPC_OVERLOAD9(__VA_ARGS__)
#define OVERLOAD10(...) GPC_OVERLOAD10(__VA_ARGS__)
#define OVERLOAD11(...) GPC_OVERLOAD11(__VA_ARGS__)
#define OVERLOAD12(...) GPC_OVERLOAD12(__VA_ARGS__)
#define OVERLOAD13(...) GPC_OVERLOAD13(__VA_ARGS__)
#define OVERLOAD14(...) GPC_OVERLOAD14(__VA_ARGS__)
#define OVERLOAD15(...) GPC_OVERLOAD15(__VA_ARGS__)
#define OVERLOAD16(...) GPC_OVERLOAD16(__VA_ARGS__)
#define OVERLOAD17(...) GPC_OVERLOAD17(__VA_ARGS__)
#define OVERLOAD18(...) GPC_OVERLOAD18(__VA_ARGS__)
#define OVERLOAD19(...) GPC_OVERLOAD19(__VA_ARGS__)
#define OVERLOAD20(...) GPC_OVERLOAD20(__VA_ARGS__)
#define OVERLOAD21(...) GPC_OVERLOAD21(__VA_ARGS__)
#define OVERLOAD22(...) GPC_OVERLOAD22(__VA_ARGS__)
#define OVERLOAD23(...) GPC_OVERLOAD23(__VA_ARGS__)
#define OVERLOAD24(...) GPC_OVERLOAD24(__VA_ARGS__)
#define OVERLOAD25(...) GPC_OVERLOAD25(__VA_ARGS__)
#define OVERLOAD26(...) GPC_OVERLOAD26(__VA_ARGS__)
#define OVERLOAD27(...) GPC_OVERLOAD27(__VA_ARGS__)
#define OVERLOAD28(...) GPC_OVERLOAD28(__VA_ARGS__)
#define OVERLOAD29(...) GPC_OVERLOAD29(__VA_ARGS__)
#define OVERLOAD30(...) GPC_OVERLOAD30(__VA_ARGS__)
#define OVERLOAD31(...) GPC_OVERLOAD31(__VA_ARGS__)
#define OVERLOAD32(...) GPC_OVERLOAD32(__VA_ARGS__)
#define OVERLOAD33(...) GPC_OVERLOAD33(__VA_ARGS__)
#define OVERLOAD34(...) GPC_OVERLOAD34(__VA_ARGS__)
#define OVERLOAD35(...) GPC_OVERLOAD35(__VA_ARGS__)
#define OVERLOAD36(...) GPC_OVERLOAD36(__VA_ARGS__)
#define OVERLOAD37(...) GPC_OVERLOAD37(__VA_ARGS__)
#define OVERLOAD38(...) GPC_OVERLOAD38(__VA_ARGS__)
#define OVERLOAD39(...) GPC_OVERLOAD39(__VA_ARGS__)
#define OVERLOAD40(...) GPC_OVERLOAD40(__VA_ARGS__)
#define OVERLOAD41(...) GPC_OVERLOAD41(__VA_ARGS__)
#define OVERLOAD42(...) GPC_OVERLOAD42(__VA_ARGS__)
#define OVERLOAD43(...) GPC_OVERLOAD43(__VA_ARGS__)
#define OVERLOAD44(...) GPC_OVERLOAD44(__VA_ARGS__)
#define OVERLOAD45(...) GPC_OVERLOAD45(__VA_ARGS__)
#define OVERLOAD46(...) GPC_OVERLOAD46(__VA_ARGS__)
#define OVERLOAD47(...) GPC_OVERLOAD47(__VA_ARGS__)
#define OVERLOAD48(...) GPC_OVERLOAD48(__VA_ARGS__)
#define OVERLOAD49(...) GPC_OVERLOAD49(__VA_ARGS__)
#define OVERLOAD50(...) GPC_OVERLOAD50(__VA_ARGS__)
#define OVERLOAD51(...) GPC_OVERLOAD51(__VA_ARGS__)
#define OVERLOAD52(...) GPC_OVERLOAD52(__VA_ARGS__)
#define OVERLOAD53(...) GPC_OVERLOAD53(__VA_ARGS__)
#define OVERLOAD54(...) GPC_OVERLOAD54(__VA_ARGS__)
#define OVERLOAD55(...) GPC_OVERLOAD55(__VA_ARGS__)
#define OVERLOAD56(...) GPC_OVERLOAD56(__VA_ARGS__)
#define OVERLOAD57(...) GPC_OVERLOAD57(__VA_ARGS__)
#define OVERLOAD58(...) GPC_OVERLOAD58(__VA_ARGS__)
#define OVERLOAD59(...) GPC_OVERLOAD59(__VA_ARGS__)
#define OVERLOAD60(...) GPC_OVERLOAD60(__VA_ARGS__)
#define OVERLOAD61(...) GPC_OVERLOAD61(__VA_ARGS__)
#define OVERLOAD62(...) GPC_OVERLOAD62(__VA_ARGS__)
#define OVERLOAD63(...) GPC_OVERLOAD63(__VA_ARGS__)
#define OVERLOAD64(...) GPC_OVERLOAD64(__VA_ARGS__)
#endif // GPC_NAMESPACING ---------------------------------------------------

#undef OVERLOADN // dummy macro for documentation purposes only

#endif // GPC_OVERLOAD_H
