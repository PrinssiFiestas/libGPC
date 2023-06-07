/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_ASSERT_H
#define GPC_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

//****************************************************************************
//
//		CORE API
//
//****************************************************************************

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
#define ASSERT(/*bool expression, char* failMessage = NULL*/...) GPC_ASSERT(__VA_ARGS__)

// Returns 0 when expression is true.
// Prints failure message and returns 1 when expression is false.
#define EXPECT(/*bool expression, char* failMessage = NULL*/...) GPC_EXPECT(__VA_ARGS__)

// 'Pseudo-operators' to be used in argument for ASSERT() or EXPECT().
// Use ASSERT(A EQ B) instead of ASSERT(A == B) for more info at failure.
#define EQ ,GPC_EQ, // ==
#define NE ,GPC_NE, // !=
#define GT ,GPC_GT, // >
#define LT ,GPC_LT, // <
#define GE ,GPC_GE, // >=
#define LE ,GPC_LE, // <=

//****************************************************************************
//
//		END OF CORE API
//
//		Structs, functions and macros below are not meant to be used by the user.
//		However, they are required for macros to work so here you go I guess.
//
//****************************************************************************

typedef struct GPC_TestAndSuiteData
{
	const char* name;
	int testFails, suiteFails, expectationFails/*includes assertion fails*/;
	int testCount, suiteCount, expectationCount;
	const union {bool isTest;  bool testDefined;};
	const union {bool isSuite; bool suiteDefined;};
	bool testOrSuiteRunning;
	struct GPC_TestAndSuiteData* parent;
} GPC_TestAndSuiteData;
extern GPC_TestAndSuiteData GPC_globalData;

#define OP_TABLE	\
	X(_EQ, ==)		\
	X(_NE, !=)		\
	X(_GT, >)		\
	X(_LT, <)		\
	X(_GE, >=)		\
	X(_LE, <=)		\

enum GPC_BooleanOperator
{
	GPC_NO_OP = -1,

#define X(OP, DUMMY) GPC##OP,
	OP_TABLE
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

struct GPC_ExpectationData
{
	const double a, b;
	const char *str_a, *str_b, *str_operator, *additionalFailMessage;
	const enum GPC_BooleanOperator operation;
	const bool isAssertion;
	const int line;
	const char *func, *file;
};

// Boolean operations as a function
// Allows macros EQ, NE, etc. to be used like operators
bool GPC_compare(double expression_a, enum GPC_BooleanOperator, double expression_b);

void GPC_printStartingMessageAndInitExitMessage();

void GPC_printTestOrSuiteResult(struct GPC_TestAndSuiteData*);

void GPC_printExpectationFail(struct GPC_ExpectationData*, struct GPC_TestAndSuiteData*);

int GPC_assert(struct GPC_ExpectationData, struct GPC_TestAndSuiteData*);

bool GPC_testOrSuiteRunning(struct GPC_TestAndSuiteData*);

void GPC_printTestOrSuiteResult(struct GPC_TestAndSuiteData*);

void GPC_addTestOrSuiteFailToParentAndGlobalIfFailed(struct GPC_TestAndSuiteData*);

extern struct GPC_TestAndSuiteData *const GPC_currentTestOrSuite;

extern const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3];

#define GPC_COMMON_DATA .line = __LINE__, .func = __func__, .file = __FILE__

#define GPC_EXPECT_EXP(EXP, ADDITIONAL_MSG, IS_ASS)			\
	GPC_assert											\
	(													\
		(struct GPC_ExpectationData)					\
		{												\
			.a 			 	= (double)(EXP),			\
			.b				= (double)0,				\
			.str_a		 	= #EXP,						\
			.str_b			= NULL,						\
			.str_operator	= NULL,						\
			.additionalFailMessage = ADDITIONAL_MSG,	\
			.operation	 	= GPC_NO_OP,				\
			.isAssertion 	= IS_ASS,					\
			GPC_COMMON_DATA								\
		},												\
		GPC_currentTestOrSuite							\
	)

#define GPC_EXPECT_CMP(A, OP, B, ADDITIONAL_MSG, IS_ASS)\
	GPC_assert											\
	(													\
	 	(struct GPC_ExpectationData)					\
		{												\
			.a 	   		  	= (double)(A),				\
			.b 			  	= (double)(B),				\
			.str_a 		  	= #A,						\
			.str_b 		  	= #B,						\
			.str_operator 	= GPC_STR_OPERATORS[OP],	\
			.additionalFailMessage = ADDITIONAL_MSG,	\
			.operation	  	= OP,						\
			.isAssertion  	= IS_ASS,					\
			GPC_COMMON_DATA								\
		},												\
		GPC_currentTestOrSuite							\
	)

#define GPC_NOT_ASS 0
#define GPC_IS_ASS  1

#define GPC_EXPECT_WITH_MSG(EXP, MSG, IS_ASS)			GPC_EXPECT_EXP(EXP, MSG, IS_ASS)
#define GPC_EXPECT_WOUT_MSG(EXP, IS_ASS)				GPC_EXPECT_EXP(EXP, NULL, IS_ASS)
#define GPC_EXPECT_CMP_WITH_MSG(A, OP, B, MSG, IS_ASS)	GPC_EXPECT_CMP(A, OP, B, MSG, IS_ASS)
#define GPC_EXPECT_CMP_WOUT_MSG(A, OP, B, IS_ASS)		GPC_EXPECT_CMP(A, OP, B, NULL,IS_ASS)

// For overloading EXPECT() and ASSERT() on number of arguments
#define GPC_GET_MACRO_NAME(DUMMY1, DUMMY2, DUMMY3, DUMMY4, NAME, ...) NAME

#define GPC_EXPECT(...)								\
	GPC_GET_MACRO_NAME(__VA_ARGS__,					\
						GPC_EXPECT_CMP_WITH_MSG,	\
						GPC_EXPECT_CMP_WOUT_MSG,	\
						GPC_EXPECT_WITH_MSG,		\
						GPC_EXPECT_WOUT_MSG,)	(__VA_ARGS__,GPC_NOT_ASS)

#define GPC_ASSERT(...)								\
	GPC_GET_MACRO_NAME(__VA_ARGS__,					\
						GPC_EXPECT_CMP_WITH_MSG,	\
						GPC_EXPECT_CMP_WOUT_MSG,	\
						GPC_EXPECT_WITH_MSG,		\
						GPC_EXPECT_WOUT_MSG,)	(__VA_ARGS__,GPC_IS_ASS)

GPC_TestAndSuiteData GPC_new_test( const char* name, GPC_TestAndSuiteData* parent);
GPC_TestAndSuiteData GPC_new_suite(const char* name, GPC_TestAndSuiteData* parent);
#define GPC_new(test_or_suite, name) GPC_new_##test_or_suite(name, GPC_currentTestOrSuite)

#define GPC_TEST_OR_SUITE(NAME, TEST_OR_SUITE)													\
	struct GPC_TestAndSuiteData GPC_##TEST_OR_SUITE##_##NAME = GPC_new(TEST_OR_SUITE, #NAME);	\
	for(struct GPC_TestAndSuiteData* GPC_currentTestOrSuite = &GPC_##TEST_OR_SUITE##_##NAME;	\
		GPC_testOrSuiteRunning(GPC_currentTestOrSuite);)
/*	{
		// user defined test or suite code
	}
*/

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPC_ASSERT_H