// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ASSERT_H
#define GPC_ASSERT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "overload.h"
#include "attributes.h"


#pragma GCC diagnostic ignored "-Wcomment" // Temp!



//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header to enable short names
// without the gpc_ prefix. 
#if defined(GPC_ASSERT_NAMESPACE) || defined(GPC_NAMESPACE)

// Tests and test suites can be created by calling gpc_test() or gpc_testSuite()
// and they will be ended by subsequent call with the same argument. First call
// will return true and next false so it can be used to run a loop once like so:
/*
	while (testSuite("My test suite"))
	{
		while (test("My first test"))
		{
			// test code
		}
		while (test("My second test"))
		{
			// test code
		}
	}
*/
#define/*bool*/test(/*const char* */name)			gpc_test(name)
#define/*bool*/testSuite(/*const char* */name)		gpc_testSuite(name)

// Does nothing when expression is true.
// Exits program and prints failure message when expression is false.
// Optional detail can be added to failure message with failMessage.
// enums need to be casted to ints or bools when using MSVC.
// Assertions are counted as expectations.
// When using logical operators, use commas to separate 2 expressions for more
// detailed fail messages e.g. ASSERT(1 + 1,!=,2) instead of ASSERT(1 + 1 != 2).
#define ASSERT(/*expression, failMsessage=""*/...) GPC_ASSERT(__VA_ARGS__)

// Returns true when expression is true.
// Prints failure message and returns false when expression is false.
// Optional detail can be added to failure message with failMessage.
// enums need to be casted to ints or bools when using MSVC.
// When using logical operators, use commas to separate 2 expressions for more
// detailed fail messages e.g. EXPECT(1 + 1,!=,2) instead of EXPECT(1 + 1 != 2).
#define EXPECT(/*expression, failMsessage=""*/...) GPC_EXPECT(__VA_ARGS__)

// TODO CHEKGGFD NULLLLLLLLLL and overloading
#define ASSERT_STR(...) GPC_ASSERT_STR(__VA_ARGS__)
#define EXPECT_STR(...) GPC_EXPECT_STR(__VA_ARGS__)

// Ends current test and suite and exits the program if argument is true,
// returns true otherwise. 
#define exitTests(b) gpc_exitTests(b)

#endif // GPC_NAMESPACING ----------------------------------------------------

bool gpc_test(const char* name);
bool gpc_testSuite(const char* name);

bool gpc_exitTests(bool);

#define GPC_COMPARE(A, OP, B)						\
	(0 ? (A) OP (B):/*better compiler diagnostics*/	\
	GPC_STRFYT(A, gpc_getCmpArgs(25)->a) OP GPC_STRFYT(B, gpc_getCmpArgs(25)->b))

#define GPC_ASSERT(...) GPC_ASSERT_CUSTOM(GPC_COMPARE, __VA_ARGS__)
#define GPC_EXPECT(...) GPC_EXPECT_CUSTOM(GPC_COMPARE, __VA_ARGS__)

int gpc_assertStrcmp(const char* str1, const char* str2);
#define GPC_STR_COMPARE(A, OP, B) (gpc_assertStrcmp((A), (B)) OP 0)

#define GPC_ASSERT_STR(...) GPC_ASSERT_CUSTOM(GPC_STR_COMPARE, __VA_ARGS__)
#define GPC_EXPECT_STR(...) GPC_EXPECT_CUSTOM(GPC_STR_COMPARE, __VA_ARGS__)

// ---------------------------------------------------------------------------

#define GPC_ASSERT_CUSTOM(COMPARATOR, ...)	\
	gpc_exitTests( ! GPC_EXPECT_CUSTOM(COMPARATOR, __VA_ARGS__))

#define GPC_EXPECT_CUSTOM(COMPARATOR, ...)			\
	GPC_OVERLOAD4(__VA_ARGS__,						\
				  GPC_EXPECT_CMP_WITH_MSG,			\
				  GPC_EXPECT_CMP_WOUT_MSG,			\
				  GPC_EXPECT_WITH_MSG,				\
				  GPC_EXPECT_WOUT_MSG,)(COMPARATOR, __VA_ARGS__)

#define GPC_FILELINEFUNC __FILE__, __LINE__, __func__

#define GPC_EXPECT_CMP_WITH_MSG(COMPARATOR, A, OP, B, MSG)	\
	gpc_expect(COMPARATOR(A, OP, B),						\
			   #OP,											\
			   GPC_FILELINEFUNC,							\
			   MSG,											\
			   #A,											\
			   #B)

#define GPC_EXPECT_CMP_WOUT_MSG(COMPARATOR, A, OP, B)		\
	gpc_expect(COMPARATOR(A, OP, B),						\
			   #OP,											\
			   GPC_FILELINEFUNC,							\
			   "",											\
			   #A,											\
			   #B)

#define GPC_EXPECT_WITH_MSG(COMPARATOR, EXPR, MSG)			\
	gpc_expect(EXPR,										\
			   "",											\
			   GPC_FILELINEFUNC,							\
			   MSG,											\
			   #EXPR,										\
			   "")

#define GPC_EXPECT_WOUT_MSG(COMPARATOR, EXPR)				\
	gpc_expect(EXPR,										\
			   "",											\
			   GPC_FILELINEFUNC,							\
			   "",											\
			   #EXPR,										\
			   "")

bool gpc_expect(const bool expr,
				const char* op_str,
				const char* file,
				const int line,
				const char* func,
				const char* failMsg,
				const char* a,
				const char* b);

typedef struct gpc_CmpArgs
{
	char* a;
	char* b;
} gpc_CmpArgs;

// Returns a pointer to struct of character buffers of size bufSize that can be
// used to store formatted values of arguments given to custom comparison function.
gpc_CmpArgs* gpc_getCmpArgs(size_t bufSize);

// Store formatted VAR in BUF and return VAR. VAR will be promoted if not bool.
#define GPC_STRFYT(VAR, BUF)								\
	_Generic(VAR,											\
			bool:				gpc_strfyb((BUF), (VAR)),	\
			short:				gpc_strfyi((BUF), (VAR)),	\
			int:				gpc_strfyi((BUF), (VAR)),	\
			long:				gpc_strfyi((BUF), (VAR)),	\
			long long:			gpc_strfyi((BUF), (VAR)),	\
			unsigned short:		gpc_strfyu((BUF), (VAR)),	\
			unsigned int:		gpc_strfyu((BUF), (VAR)),	\
			unsigned long:		gpc_strfyu((BUF), (VAR)),	\
			unsigned long long:	gpc_strfyu((BUF), (VAR)),	\
			float:				gpc_strfyf((BUF), (VAR)),	\
			double:				gpc_strfyf((BUF), (VAR)),	\
			char:				gpc_strfyc((BUF), (VAR)),	\
			unsigned char:		gpc_strfyc((BUF), (VAR)),	\
			char*:				gpc_strfyp((BUF), (VAR)),	\
			const char*:		gpc_strfyp((BUF), (VAR)),	\
			default:			gpc_strfyp((BUF), (VAR))) // pointer

// var is variadic to get around type system when using STRFYT() macro.
bool				gpc_strfyb(char* buf,/*bool var*/...);
long long			gpc_strfyi(char* buf,/*long long var*/...);
unsigned long long	gpc_strfyu(char* buf,/*unsigned long long var*/...);
double				gpc_strfyf(char* buf,/*double var*/...);
char				gpc_strfyc(char* buf,/*char var*/...);
void*				gpc_strfyp(char* buf,/*void* var*/...);

#endif // GPC_ASSERT_H
