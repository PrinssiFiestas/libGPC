/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_OVERLOAD_H
#define GPC_OVERLOAD_H

#define GPC_1ST_ARG(A, ...) A
#define GPC_COMMA(...) ,
#define GPC_DUMP(...) 
#define GPC_EVAL(...) __VA_ARGS__

// Define one of these macros before including this header in case of
// namespacing issues.
#if !defined(GPC_OVERLOAD_NAMESPACING) && !defined(GPC_NAMESPACING)

// Overload function by the number of arguments up to NARGS. Usage example:
/*
	int f1Arg(int arg) { return arg; }
	#define f2Args(a, b) a + b
	void f3Args(int i1, int i2, int i3) { printf("%i%i%i\n", i1, i2, i3); }
	#define func(...) OVERLOAD(3, __VA_ARGS__, f3Args, f2Args, f1Arg)(__VA_ARGS__)
*/
#define OVERLOAD(NARGS, ...) GPC_OVERLOAD(NARGS, __VA_ARGS__)

// Returns IF_EXPRESSION if VARIABLE is a number. Any other types has to be 
// handled seperately. Examples:
/*
	// if (isNumber) {...} else {...}
	IF_IS_NUMBER(x, expressionForNumbers, default: expressionForAllOtherTypes);
	
	// Character is not considered as a number by default
	IF_IS_NUMBER(y, expressionForNumbers, char: expressionForCharacters);
*/
#define IF_IS_NUMBER(VARIABLE, IF_EXPRESSION, ...)		\
	GPC_IF_IS_NUMBER(VARIABLE, IF_EXPRESSION, __VA_ARGS__)

// Use this if int8_t or uint8_t needs to be considered as a number.
#define IF_IS_NUMBER_OR_CHAR(VARIABLE, IF_EXPRESSION, ...)		\
	GPC_IF_IS_NUMBER_OR_CHAR(VARIABLE, IF_EXPRESSION, __VA_ARGS__)

// Returns number of arguments
#define COUNT_ARGS(...) GPC_COUNT_ARGS(__VA_ARGS__)

#endif // GPC_NAMESPACING ----------------------------------------------------

#define GPC_OVERLOAD(NARGS, ...) GPC_LIST##NARGS (GPC_DUMP, GPC_DUMP, GPC_1ST_ARG, __VA_ARGS__)

#define GPC_IF_IS_NUMBER(VARIABLE, IF_EXPRESSION, ...) _Generic(VARIABLE,			\
short: IF_EXPRESSION, unsigned short: IF_EXPRESSION, int: IF_EXPRESSION,			\
unsigned: IF_EXPRESSION, long: IF_EXPRESSION, unsigned long: IF_EXPRESSION, 		\
long long: IF_EXPRESSION, unsigned long long: IF_EXPRESSION, float: IF_EXPRESSION, 	\
double: IF_EXPRESSION, long double: IF_EXPRESSION __VA_OPT__(,)						\
__VA_ARGS__)

// Use this if int8_t or uint8_t needs to be considered as a number.
#define GPC_IF_IS_NUMBER_OR_CHAR(VARIABLE, IF_EXPRESSION, ...) GPC_IF_IS_NUMBER(VARIABLE,	\
IF_EXPRESSION, char: IF_EXPRESSION, unsigned char: IF_EXPRESSION __VA_OPT__(,) 				\
__VA_ARGS__)

// Returns number of arguments
#define GPC_COUNT_ARGS(...) GPC_OVERLOAD(64, __VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56,\
55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 	\
33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 	\
11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

// ---------------------------------------------------------------------------

// Ignores every argument exept 64th
#define GPC_CHOOSE_64TH(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 	\
_15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, 		\
_32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, 		\
_49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63,  THIS, ...) THIS

// List all variadic arguments separated with OP and processed with FUNC
// OP needs to be a macro function that takes a dummy argument e.g. GPC_COMMA.
#define GPC_LIST_ALL(FUNC, OP, ...) GPC_CHOOSE_64TH(__VA_ARGS__, GPC_LIST64, GPC_LIST63, 	   \
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

#endif // GPC_OVERLOAD_H