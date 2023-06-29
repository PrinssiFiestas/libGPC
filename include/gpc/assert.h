/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_ASSERT_H
#define GPC_ASSERT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "overload.h"

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Use these macros to define tests and suites.
// Tests and suites have to be defined in function scope so they can be run automatically.
// Tests and suites are optional: EXPECT() and ASSERT() can be used anywhere in your code.
// Tests and suites can be nested arbitrarily.
#define TEST(NAME)			GPC_TEST_OR_SUITE(NAME,test)
#define TEST_SUITE(NAME)	GPC_TEST_OR_SUITE(NAME,suite)
// Example use:
/*
int main() // function scope required!
{
	TEST_SUITE(exampleSuite) // optional suite
	{
		TEST(exampleTest) // optional test
		{
			ASSERT(1 + 1 EQ 2);
		}
	}
}
*/

// Does nothing when expression is true.
// Exits program and prints failure message when expression is false.
// Assertions are counted as expectations.
#define ASSERT(/*bool expression, char* failMessage = NULL*/...) GPC_ASSERT_OL(__VA_ARGS__)

// Returns 0 when expression is true.
// Prints failure message and returns 1 when expression is false.
#define EXPECT(/*bool expression, char* failMessage = NULL*/...) GPC_EXPECT_OL(__VA_ARGS__)

// 'Pseudo-operators' to be used in argument for ASSERT() or EXPECT().
// Use ASSERT(A EQ B) instead of ASSERT(A == B) for more info at failure.
#define EQ ,GPC_OP_EQ, // ==
#define NE ,GPC_OP_NE, // !=
#define GT ,GPC_OP_GT, // >
#define LT ,GPC_OP_LT, // <
#define GE ,GPC_OP_GE, // >=
#define LE ,GPC_OP_LE, // <=

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

typedef struct gpc_TestAndSuiteData
{
	const char* name;
	int testFails, suiteFails, expectationFails/*includes assertion fails*/;
	int testCount, suiteCount, expectationCount;
	const union {bool isTest;  bool testDefined;};
	const union {bool isSuite; bool suiteDefined;};
	bool testOrSuiteRunning;
	struct gpc_TestAndSuiteData* parent;
} gpc_TestAndSuiteData;

extern gpc_TestAndSuiteData gpc_gTestData;

#define GPC_OP_TABLE	\
	X(_OP_EQ, ==)		\
	X(_OP_NE, !=)		\
	X(_OP_GT, >)		\
	X(_OP_LT, <)		\
	X(_OP_GE, >=)		\
	X(_OP_LE, <=)		\

enum gpc_BooleanOperator
{
	GPC_NO_OP = -1,

#define X(OP, DUMMY) GPC##OP,
	GPC_OP_TABLE
#undef X

// Expands to

/*	GPC_EQ,
	GPC_NE,
	GPC_GT,
	GPC_LT,
	GPC_GE,
	GPC_LE,*/

	GPC_OPS_LENGTH
};

enum gpc_Datatype
{
	GPC_NUMBER,
	GPC_POINTER,
	GPC_CHAR_POINTER
};

struct gpc_ExpectationData
{
	const double a, b;
	const void *pa, *pb;
	const char *str_a, *str_b, *str_operator, *additionalFailMessage;
	const enum gpc_BooleanOperator operation;
	const bool isAssertion;
	const int line;
	const char *func, *file;
	const enum gpc_Datatype type;
};

// Boolean operations as a function
// Allows macros EQ, NE, etc. to be used like operators
bool gpc_compare(double expression_a, enum gpc_BooleanOperator, double expression_b);

void gpc_printStartingMessageAndInitExitMessage();

void gpc_printTestOrSuiteResult(struct gpc_TestAndSuiteData*);

void gpc_printExpectationFail(struct gpc_ExpectationData*, struct gpc_TestAndSuiteData*);

int gpc_assert(struct gpc_ExpectationData, struct gpc_TestAndSuiteData*);

bool gpc_testOrSuiteRunning(struct gpc_TestAndSuiteData*);

void gpc_printTestOrSuiteResult(struct gpc_TestAndSuiteData*);

void gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(struct gpc_TestAndSuiteData*);

extern struct gpc_TestAndSuiteData *const gpc_currentTestOrSuite;

extern const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3];

#define GPC_COMMON_DATA .line = __LINE__, .func = __func__, .file = __FILE__

#define GPC_EXPECT_EXP(EXP, ADDITIONAL_MSG, IS_ASS)									\
	gpc_assert																		\
	(																				\
		(struct gpc_ExpectationData)												\
		{																			\
			.a 					 	= IF_IS_NUMBER_OR_CHAR(EXP, EXP, default: 0),	\
			.b						= 0,											\
			.pa						= IF_IS_NUMBER_OR_CHAR(EXP, NULL, default: EXP),\
			.pb						= NULL,											\
			.str_a				 	= #EXP,											\
			.str_b					= NULL,											\
			.str_operator			= NULL,											\
			.additionalFailMessage 	= ADDITIONAL_MSG,								\
			.operation	 			= GPC_NO_OP,									\
			.isAssertion 			= IS_ASS,										\
			.type					= IF_IS_NUMBER_OR_CHAR(EXP,						\
										GPC_NUMBER,									\
										const char*: GPC_CHAR_POINTER,				\
										char*: GPC_CHAR_POINTER,					\
										default: GPC_POINTER),						\
			GPC_COMMON_DATA															\
		},																			\
		gpc_currentTestOrSuite														\
	)

#define GPC_EXPECT_CMP(A, OP, B, ADDITIONAL_MSG, IS_ASS)							\
	gpc_assert																		\
	(																				\
	 	(struct gpc_ExpectationData)												\
		{																			\
			.a 	   		  			= IF_IS_NUMBER_OR_CHAR(A, A, default: 0),		\
			.b 			  			= IF_IS_NUMBER_OR_CHAR(B, B, default: 0),		\
			.pa						= IF_IS_NUMBER_OR_CHAR(A, NULL, default: A),	\
			.pb						= IF_IS_NUMBER_OR_CHAR(B, NULL, default: B),	\
			.str_a 		  			= #A,											\
			.str_b 		  			= #B,											\
			.str_operator 			= GPC_STR_OPERATORS[OP],						\
			.additionalFailMessage 	= ADDITIONAL_MSG,								\
			.operation	  			= OP,											\
			.isAssertion  			= IS_ASS,										\
			.type					= IF_IS_NUMBER_OR_CHAR(A,						\
										GPC_NUMBER,									\
										const char*: GPC_CHAR_POINTER,				\
										char*: GPC_CHAR_POINTER,					\
										default: GPC_POINTER),						\
			GPC_COMMON_DATA															\
		},																			\
		gpc_currentTestOrSuite														\
	)

#define GPC_NOT_ASS 0
#define GPC_IS_ASS  1

#define GPC_EXPECT_WITH_MSG(EXP, MSG, IS_ASS)			GPC_EXPECT_EXP(EXP, MSG, IS_ASS)
#define GPC_EXPECT_WOUT_MSG(EXP, IS_ASS)				GPC_EXPECT_EXP(EXP, NULL, IS_ASS)
#define GPC_EXPECT_CMP_WITH_MSG(A, OP, B, MSG, IS_ASS)	GPC_EXPECT_CMP(A, OP, B, MSG, IS_ASS)
#define GPC_EXPECT_CMP_WOUT_MSG(A, OP, B, IS_ASS)		GPC_EXPECT_CMP(A, OP, B, NULL,IS_ASS)

#define GPC_EXPECT_OL(...) OVERLOAD(4, __VA_ARGS__,				\
									GPC_EXPECT_CMP_WITH_MSG,	\
									GPC_EXPECT_CMP_WOUT_MSG,	\
									GPC_EXPECT_WITH_MSG,		\
									GPC_EXPECT_WOUT_MSG,)	(__VA_ARGS__, GPC_NOT_ASS)

#define GPC_ASSERT_OL(...) OVERLOAD(4, __VA_ARGS__,				\
									GPC_EXPECT_CMP_WITH_MSG,	\
									GPC_EXPECT_CMP_WOUT_MSG,	\
									GPC_EXPECT_WITH_MSG,		\
									GPC_EXPECT_WOUT_MSG,)	(__VA_ARGS__, GPC_IS_ASS)

gpc_TestAndSuiteData gpc_new_test( const char* name, gpc_TestAndSuiteData* parent);
gpc_TestAndSuiteData gpc_new_suite(const char* name, gpc_TestAndSuiteData* parent);

#define GPC_TEST_OR_SUITE(NAME, TEST_OR_SUITE)												\
	struct gpc_TestAndSuiteData GPC_##TEST_OR_SUITE##_##NAME = 								\
		gpc_new_##TEST_OR_SUITE(#NAME, gpc_currentTestOrSuite);								\
	for(struct gpc_TestAndSuiteData* gpc_currentTestOrSuite = &GPC_##TEST_OR_SUITE##_##NAME;\
		gpc_testOrSuiteRunning(gpc_currentTestOrSuite);)
/*	{
		// user defined test or suite code
	}
*/

#endif // GPC_ASSERT_H