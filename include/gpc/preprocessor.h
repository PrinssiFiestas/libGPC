// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_PREPROCESSOR_INCLUDED
#define GP_PREPROCESSOR_INCLUDED 1

#include <gpc/overload.h>

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup preprocessor Preprocessor
/// @code
/// #include <gpc/preprocessor.h>
/// @endcode
/// Macros for function/macro overloading by argument number or types.
/// @{

#ifdef GP_DOXYGEN
/** Processing variadic arguments.
 * Process all variadic arguments with @a FUNC separated with @a SEP. @a FUNC
 * must be a function or macro that takes a single argument. Each variadic
 * argument will be passed to that function. @a SEP separates each variadic
 * argument. It has to be a macro function that takes a variadic argument and
 * expands to the separator like @ref GP_COMMA.
 *
 * Examples:
 * @code
 * int add_one(int x) { return x + 1; }
 * int array[] = { GP_PROCESS_ALL_ARGS(add_one, GP_COMMA, 3, 4, 5) };
 * // The line above expands to
 * int array[] = { add_one(3), add_one(4), add_one(5) };
 *
 * #define PLUS(...) +
 * int sum = GP_PROCESS_ALL_ARGS(GP_EVAL, PLUS, 2, 3, 4, 5);
 * // The line above expands to
 * int sum = 2 + 3 + 4 + 5;
 *
 * // Combine code above for sum of squares.
 * double square(double x) { return x*x; }
 * double sum_of_squares = GP_PROCESS_ALL_ARGS(square, PLUS, 3.14, 0.707);
 * // expands to
 * double sum_of_squares = square(3.14) + square(0.707);
 * @endcode
 */
#define GP_PROCESS_ALL_ARGS(FUNC, SEP, ...) \
    FUNC(__VA_ARG__1) SEP FUNC(__VA_ARG__2) SEP ... SEP FUNC(__VA_ARG__N)
#endif // GP_DOXYGEN

#define GP_TOKEN_PASTE(A, B) A##B ///< Concatenate @a A and @a B to a single token.
#define GP_TOKEN_TO_STRING(A) #A ///< Convert @a A to a string literal.
#define GP_TOKEN_EXPAND_TO_STRING(A) GP_TOKEN_TO_STRING(A) ///< Expands @a A (if macro) to a string literal.
#define GP_1ST_ARG(A, ...) A ///< Ignore all passed arguments except the first.

/** Comma token.
 * Expands to a comma. Most useful as a separator for @ref GP_PROCESS_ALL_ARGS.
 */
#define GP_COMMA(...) ,

/** Semicolon token.
 * Expands to a semicolon. Most useful as a separator for
 * @ref GP_PROCESS_ALL_ARGS.
 */
#define GP_SEMICOLON(...) ;

/** Expand variadic arguments with no processing.
 * Most useful as a no-op for @ref GP_PROCESS_ALL_ARGS when it has a separator
 * that is not a comma.
 */
#define GP_EVAL(...) __VA_ARGS__

/** Count variadic arguments.
 * @return number of arguments passed.
 */
#define GP_COUNT_ARGS(...) GP_OVERLOAD64(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56,\
55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34,  \
33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12,  \
11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,)

/** Check in preprocessor if GNU built-in is supported.
 *
 * Some compilers allow to check in preprocessor if any given `__builtin`
 * function is supported using `__has_builtin` operator. However, portable
 * applications should check the availability of `__has_builtin` itself before
 * using it, which is cumbersome and can blow up the complexity of more complex
 * `#if` expressions. This macro wraps `__has_builtin` to avoid nested `#if`
 * expressions.
 *
 * @return zero if the built-in passed as argument is not supported or if
 * `__has_builtin` is not supported. Otherwise, a non-zero integer is returned.
 */
#if defined(__has_builtin) || defined(GP_DOXYGEN)
#  define GP_HAS_BUILTIN(...) __has_builtin(__VA_ARGS__)
#else
#  define GP_HAS_BUILTIN(...) 0
#endif

/** Check in preprocessor if a header file exist.
 *
 * Some compilers allow to check in preprocessor if any given header file can be
 * included using `__has_include` operator. However, portable applications
 * should check the availability of `__has_include` itself before using it,
 * which is cumbersome and can blow up the complexity of more complex `#if`
 * expressions. This macro wraps `__has_include` to avoid nested `#if`
 * expressions.
 *
 * The operand should be given in same form as operand for `#include` directive.
 *
 * @return zero if the header file passed as argument is not supported or if
 * `__has_builtin` is not supported. Otherwise, a non-zero integer is returned.
 *
 * Example:
 * @code
 * #if GP_HAS_INCLUDE(<stdatomic.h>)
 * #  include <stdatomic.h>
 * #endif
 * @endcode
 */
#if defined(__has_include) || defined(GP_DOXYGEN)
#  define GP_HAS_INCLUDE(...) __has_include(__VA_ARGS__)
#else
#  define GP_HAS_INCLUDE(...) 0
#endif


/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

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

#define GP_PROC1( F, SEP, A)      F(A)
#define GP_PROC2( F, SEP, A, ...) F(A) SEP(A) GP_PROC1(F, SEP, __VA_ARGS__)
#define GP_PROC3( F, SEP, A, ...) F(A) SEP(A) GP_PROC2(F, SEP, __VA_ARGS__)
#define GP_PROC4( F, SEP, A, ...) F(A) SEP(A) GP_PROC3(F, SEP, __VA_ARGS__)
#define GP_PROC5( F, SEP, A, ...) F(A) SEP(A) GP_PROC4(F, SEP, __VA_ARGS__)
#define GP_PROC6( F, SEP, A, ...) F(A) SEP(A) GP_PROC5(F, SEP, __VA_ARGS__)
#define GP_PROC7( F, SEP, A, ...) F(A) SEP(A) GP_PROC6(F, SEP, __VA_ARGS__)
#define GP_PROC8( F, SEP, A, ...) F(A) SEP(A) GP_PROC7(F, SEP, __VA_ARGS__)
#define GP_PROC9( F, SEP, A, ...) F(A) SEP(A) GP_PROC8(F, SEP, __VA_ARGS__)
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

///@endcond
#endif // GP_PREPROCESSOR_INCLUDED
