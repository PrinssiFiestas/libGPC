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

// Use these macros to define tests and suites.
// Tests and suites have to be defined in function scope.
// Tests and suites are optional: EXPECT() and ASSERT() can be used anywhere in
// your code. They can also be nested arbitrarily.
// NAME will be namespaced so tests and suites can share their names with
// existing functions or variables. 
// #define TEST(NAME)			GPC_TEST(NAME)
// #define TEST_SUITE(NAME)	GPC_TEST_SUITE(NAME)
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
// Optional detail can be added to failure message with FAIL_MESSAGE.
// enums need to be casted to ints or bools when using MSVC.
// Assertions are counted as expectations.
#define ASSERT(/*expression, failMsessage=""*/...) GPC_ASSERT(__VA_ARGS__)

// Returns true when expression is true.
// Prints failure message and returns false when expression is false.
// Optional detail can be added to failure message with FAIL_MESSAGE.
// enums need to be casted to ints or bools when using MSVC.
#define EXPECT(/*expression, failMsessage=""*/...) GPC_EXPECT(__VA_ARGS__)

#define ASSERT_STR(...) GPC_ASSERT_STR(__VA_ARGS__)
#define EXPECT_STR(...) GPC_EXPECT_STR(__VA_ARGS__)

// Ends current test and suite and exits the program if argument is true,
// returns true otherwise. 
#define exitTests(b) gpc_exitTests(b)


// 'Pseudo-operators' to be used in argument for ASSERT() or EXPECT()
// Use ASSERT(A, EQ, B) instead of ASSERT(A == B) for more info at failure.
// #define EQ GPC_EQ // ==
// #define NE GPC_NE // !=
// #define GT GPC_GT // >
// #define LT GPC_LT // <
// #define GE GPC_GE // >=
// #define LE GPC_LE // <=

#endif // GPC_NAMESPACING ----------------------------------------------------

// #define GPC_TEST(NAME)			GPC_TEST_OR_SUITE(NAME, test)
// #define GPC_TEST_SUITE(NAME)	GPC_TEST_OR_SUITE(NAME, suite)

// #define GPC_ASSERT(...) GPC_ASSERT_OL(__VA_ARGS__)
// #define GPC_EXPECT(...) GPC_EXPECT_OL(__VA_ARGS__)

// #define GPC_EQ ,GPC_OP_EQ, // ==
// #define GPC_NE ,GPC_OP_NE, // !=
// #define GPC_GT ,GPC_OP_GT, // >
// #define GPC_LT ,GPC_OP_LT, // <
// #define GPC_GE ,GPC_OP_GE, // >=
// #define GPC_LE ,GPC_OP_LE, // <=

// TODO add this
// bool gpc_anyFails(struct gpc_TestAndSuiteData* data);

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

// typedef struct gpc_TestAndSuiteData
// {
	// const char* name;
	// int testFails, suiteFails, expectationFails/*includes assertion fails*/;
	// int testCount, suiteCount, expectationCount;
	// const union {bool isTest;  bool testDefined;};
	// const union {bool isSuite; bool suiteDefined;};
	// bool testOrSuiteRunning;
	// struct gpc_TestAndSuiteData* parent;
// } gpc_TestAndSuiteData;

// extern gpc_TestAndSuiteData gpc_gTestData;

// #define GPC_OP_TABLE	\
	// X(_OP_EQ, ==)		\
	// X(_OP_NE, !=)		\
	// X(_OP_GT, >)		\
	// X(_OP_LT, <)		\
	// X(_OP_GE, >=)		\
	// X(_OP_LE, <=)		\

// enum gpc_BooleanOperator
// {
	// GPC_NO_OP = -1,

// #define X(OP, DUMMY) GPC##OP,
	// GPC_OP_TABLE
// #undef X

	// GPC_OPS_LENGTH
// };

// enum gpc_Datatype
// {
	// GPC_NUMBER,
	// GPC_BOOL,
	// GPC_POINTER,
	// GPC_CHAR_POINTER
// };

// struct gpc_ExpectationData
// {
	// const GPC_LONG_DOUBLE a, b;
	// const void *pa, *pb;
	// const char *str_a, *str_b, *str_operator, *additionalFailMessage;
	// const enum gpc_BooleanOperator operation;
	// const bool isAssertion;
	// const int line;
	// const char *func, *file;
	// const enum gpc_Datatype type;
// };

// Boolean operations as a function
// Allows macros EQ, NE, etc. to be used like operators
//bool gpc_compare(double expression_a, enum gpc_BooleanOperator, double expression_b);

//void gpc_printStartingMessageAndInitExitMessage(void);

//void gpc_printExpectationFail(struct gpc_ExpectationData*, struct gpc_TestAndSuiteData*);

//int gpc_assert_internal(struct gpc_ExpectationData, struct gpc_TestAndSuiteData*);

//bool gpc_testOrSuiteRunning(struct gpc_TestAndSuiteData*);

//void gpc_printTestOrSuiteResult(struct gpc_TestAndSuiteData*);

// void gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(struct gpc_TestAndSuiteData*);

//extern struct gpc_TestAndSuiteData *const gpc_currentTestOrSuite;

// extern const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3];

//#define GPC_COMMON_DATA .line = __LINE__, .func = __func__, .file = __FILE__

// #define GPC_EXPECT_EXP(EXP, ADDITIONAL_MSG, IS_ASS)							\
	// gpc_assert_internal														\
	// (																		\
		// (struct gpc_ExpectationData)										\
		// {																	\
			// .a 					 	= (GPC_LONG_DOUBLE)GPC_IF_IS_NUMERIC(EXP, EXP, 0),\
			// .b						= (GPC_LONG_DOUBLE)0,					\
			// .pa						= GPC_IF_IS_NUMERIC(EXP, NULL, EXP),	\
			// .pb						= NULL,									\
			// .str_a				 	= #EXP,									\
			// .str_b					= NULL,									\
			// .str_operator			= NULL,									\
			// .additionalFailMessage 	= ADDITIONAL_MSG,						\
			// .operation	 			= GPC_NO_OP,							\
			// .isAssertion 			= IS_ASS,								\
			// .type					= GPC_IF_IS_NUMBER(EXP,					\
										// GPC_NUMBER, _Generic(EXP,			\
											// bool: GPC_BOOL,					\
											// const char*: GPC_CHAR_POINTER,	\
											// char*: GPC_CHAR_POINTER,		\
											// default: GPC_POINTER)),			\
			// GPC_COMMON_DATA													\
		// },																	\
		// gpc_currentTestOrSuite												\
	// )

// #define GPC_EXPECT_CMP(A, OP, B, ADDITIONAL_MSG, IS_ASS)					\
	// gpc_assert_internal														\
	// (																		\
	 	// (struct gpc_ExpectationData)										\
		// {																	\
			// .a 	   		  			= (GPC_LONG_DOUBLE)GPC_IF_IS_NUMERIC(A, A, 0),\
			// .b 			  			= (GPC_LONG_DOUBLE)GPC_IF_IS_NUMERIC(B, B, 0),\
			// .pa						= GPC_IF_IS_NUMERIC(A, NULL, A),		\
			// .pb						= GPC_IF_IS_NUMERIC(B, NULL, B),		\
			// .str_a 		  			= #A,									\
			// .str_b 		  			= #B,									\
			// .str_operator 			= GPC_STR_OPERATORS[OP],				\
			// .additionalFailMessage 	= ADDITIONAL_MSG,						\
			// .operation	  			= OP,									\
			// .isAssertion  			= IS_ASS,								\
			// .type					= GPC_IF_IS_NUMBER(A,					\
										// GPC_NUMBER, _Generic(A,				\
											// bool: GPC_BOOL,					\
											// const char*: GPC_CHAR_POINTER,	\
											// char*: GPC_CHAR_POINTER,		\
											// default: GPC_POINTER)),			\
			// GPC_COMMON_DATA													\
		// },																	\
		// gpc_currentTestOrSuite												\
	// )

// #define GPC_NOT_ASS 0
// #define GPC_IS_ASS  1

// #define GPC_EXPECT_WITH_MSG(EXP, MSG, IS_ASS)			GPC_EXPECT_EXP(EXP, MSG, IS_ASS)
// #define GPC_EXPECT_WOUT_MSG(EXP, IS_ASS)				GPC_EXPECT_EXP(EXP, NULL, IS_ASS)
// #define GPC_EXPECT_CMP_WITH_MSG(A, OP, B, MSG, IS_ASS)	GPC_EXPECT_CMP(A, OP, B, MSG, IS_ASS)
// #define GPC_EXPECT_CMP_WOUT_MSG(A, OP, B, IS_ASS)		GPC_EXPECT_CMP(A, OP, B, NULL,IS_ASS)

// #define GPC_EXPECT_OL(...) OVERLOAD(4, __VA_ARGS__,				\
									// GPC_EXPECT_CMP_WITH_MSG,	\
									// GPC_EXPECT_CMP_WOUT_MSG,	\
									// GPC_EXPECT_WITH_MSG,		\
									// GPC_EXPECT_WOUT_MSG,)	(__VA_ARGS__, GPC_NOT_ASS)

// #define GPC_ASSERT_OL(...) OVERLOAD(4, __VA_ARGS__,				\
									// GPC_EXPECT_CMP_WITH_MSG,	\
									// GPC_EXPECT_CMP_WOUT_MSG,	\
									// GPC_EXPECT_WITH_MSG,		\
									// GPC_EXPECT_WOUT_MSG,)	(__VA_ARGS__, GPC_IS_ASS)

// gpc_TestAndSuiteData gpc_new_test( const char* name, gpc_TestAndSuiteData* parent);
// gpc_TestAndSuiteData gpc_new_suite(const char* name, gpc_TestAndSuiteData* parent);

// #define GPC_TEST_OR_SUITE(NAME, TEST_OR_SUITE)												\
	// struct gpc_TestAndSuiteData GPC_##TEST_OR_SUITE##_##NAME = 								\
		// gpc_new_##TEST_OR_SUITE(#NAME, gpc_currentTestOrSuite);								\
	// for(struct gpc_TestAndSuiteData* gpc_currentTestOrSuite = &GPC_##TEST_OR_SUITE##_##NAME;\
		// gpc_testOrSuiteRunning(gpc_currentTestOrSuite);)
/*	{
		// user defined test or suite code
	}
*/













// ***************************************************************************


		// N E W  S T U F F


// ***************************************************************************


// UNNECESSARY
// #define GPC_EQ ==
// #define GPC_NE !=
// #define GPC_LT < 
// #define GPC_GT > 
// #define GPC_LE <=
// #define GPC_GE >=



bool gpc_test(const char* name);
bool gpc_testSuite(const char* name);

bool gpc_exitTests(bool);

//#define GPC_COMPARE(A, OP, B) ((A) GPC_##OP (B))
// #define GPC_COMPARE(A, OP, B)							\
	// (false ? (A) OP (B)/*get compiler diagnostics*/ :	\
		// gpc_compare(GPC_TYPE(A), GPC_TYPE(B), #OP, (A), (B))

// bool gpc_compare(const enum gpc_Type a_type,
				 // const enum gpc_Type b_type,
				 // const char* op,
				 // a,
				 // b
//				 ...);

// #define GPC_ASSERT_STRFY(BUF, X)						\
	// gpc_strfy((BUF), _Generic(X, const char*: GPC_PTR,	\
							  // char*: GPC_PTR,			\
							  // default: GPC_TYPE(X)), (X))

// Is that pointer fuckery correct?
#define GPC_COMPARE(A, OP, B)						\
	(0 ? (A) OP (B):/*better compiler diagnostics*/	\
	GPC_STRFYT(A, gpc_getCmpArgs(25)->a) OP GPC_STRFYT(B, gpc_getCmpArgs(25)->b))

// #define GPC_ASSERT(...)	\
	// GPC_ASSERT_CUSTOM(GPC_COMPARE, GPC_ASSERT_STRFY, __VA_ARGS__)
// #define GPC_EXPECT(...)	\
	// GPC_EXPECT_CUSTOM(GPC_COMPARE, GPC_ASSERT_STRFY, __VA_ARGS__)
	
#define GPC_ASSERT(...)	\
	GPC_ASSERT_CUSTOM(GPC_COMPARE, __VA_ARGS__)
#define GPC_EXPECT(...)	\
	GPC_EXPECT_CUSTOM(GPC_COMPARE, __VA_ARGS__)

// #define GPC_COMPARE_STR(A, OP, B) (gpc_strcmp((A),(B)) GPC_##OP 0)
// int gpc_strcmp(const char str1[static 1], const char str2[static 1]);
// char* gpc_quotify(char** buf, const char* str);

// #define GPC_ASSERT_STR(...)	\
	// GPC_ASSERT_CUSTOM(GPC_COMPARE_STR, gpc_quotify, __VA_ARGS__)
// #define GPC_EXPECT_STR(...)	\
	// GPC_EXPECT_CUSTOM(GPC_COMPARE_STR, gpc_quotify, __VA_ARGS__)

// ---------------------------------------------------------------------------

// #define GPC_ASSERT_CUSTOM(COMPARATOR, STRINGFIER, ...)	\
	// gpc_exitTests( ! GPC_EXPECT_CUSTOM(COMPARATOR, STRINGFIER, __VA_ARGS__))

// #define GPC_EXPECT_CUSTOM(COMPARATOR, STRFIER, ...)	\
	// GPC_OVERLOAD4(__VA_ARGS__,						\
				  // GPC_EXPECT_CMP_WITH_MSG,			\
				  // GPC_EXPECT_CMP_WOUT_MSG,			\
				  // GPC_EXPECT_WITH_MSG,				\
				  // GPC_EXPECT_WOUT_MSG,)(COMPARATOR, STRFIER, __VA_ARGS__)

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

/*#define GPC_EXPECT_CMP_WITH_MSG(COMPARATOR, STRFIER, A, OP, B, MSG)	\
	gpc_expect(COMPARATOR(A, OP, B),								\
			   #OP,												\
			   GPC_FILELINEFUNC,								\
			   MSG,												\
			   #A,												\
			   STRFIER(&gpc_ga_bufp, A),						\
			   #B,												\
			   STRFIER(&gpc_gb_bufp, B))

#define GPC_EXPECT_CMP_WOUT_MSG(COMPARATOR, STRFIER, A, OP, B)	\
	gpc_expect(COMPARATOR(A, OP, B),							\
			   #OP,												\
			   GPC_FILELINEFUNC,								\
			   "",												\
			   #A,												\
			   STRFIER(&gpc_ga_bufp, A),						\
			   #B,												\
			   STRFIER(&gpc_gb_bufp, B))

#define GPC_EXPECT_WITH_MSG(COMPARATOR, STRFIER, EXPR, MSG)	\
	gpc_expect(EXPR,										\
			   "",											\
			   GPC_FILELINEFUNC,							\
			   MSG,											\
			   #EXPR,										\
			   STRFIER(&gpc_ga_bufp, EXPR),					\
			   "",											\
			   NULL)

#define GPC_EXPECT_WOUT_MSG(COMPARATOR, STRFIER, EXPR)		\
	gpc_expect(EXPR,										\
			   "",											\
			   GPC_FILELINEFUNC,							\
			   "",											\
			   #EXPR,										\
			   STRFIER(&gpc_ga_bufp, EXPR),					\
			   "",											\
			   NULL)*/

// TODO move to overloagt OR REMOVE
char* gpc_strfy(char** buf, const/*enum gpc_Type*/int T, ...);

// bool gpc_expect(const bool expr,
				// const char* op_str,
				// const char* file,
				// const int line,
				// const char* func,
				// const char* failMsg,
				// const char* a,
				// char* a_eval,
				// const char* b,
				// char* b_eval);
bool gpc_expect(const bool expr,
				const char* op_str,
				const char* file,
				const int line,
				const char* func,
				const char* failMsg,
				const char* a,
				const char* b);

// CHANGE THE MAGIC VALUE
// Also maybe move these to implementation file?
extern char gpc_ga_buf[40];
extern char gpc_gb_buf[40];
extern char* gpc_ga_bufp;
extern char* gpc_gb_bufp;

typedef struct gpc_CmpArgs
{
	char* a;
	char* b;
} gpc_CmpArgs;

// Returns a pointer to struct of character buffers that can be used to store
// formatted values of arguments given to custom comparison function. 
gpc_CmpArgs* gpc_getCmpArgs(size_t bufSize);

long long gpc_strfyi(char* buf, ...);
unsigned long long gpc_strfyu(char* buf, ...);
double gpc_strfyf(char* buf, ...);
char gpc_strfyc(char* buf, ...);
void* gpc_strfyp(char* buf, ...);

#define GPC_STRFYT(VAR, BUF)								\
	_Generic(VAR,											\
			bool:				gpc_strfyi((BUF), (VAR)),	\
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
			default:			gpc_strfyp((BUF), (VAR)))





#endif // GPC_ASSERT_H
