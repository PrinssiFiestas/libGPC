// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/gpc/assert.h"
#include "terminalcolors.h"

#define GPC_IS_TEST true

gpc_TestAndSuiteData gpc_newTestOrSuite(const char* name, gpc_TestAndSuiteData* parent,
										  bool isTest)
{
	gpc_TestAndSuiteData d =
	{
		.name				= name,
		.testFails			= 0,		.suiteFails =	0,	.expectationFails =	0,
		.testCount			= 0,		.suiteCount =	0,	.expectationCount =	0,
		.testDefined		= isTest,
		.suiteDefined		= ! isTest,
		.testOrSuiteRunning	= false,
		.parent				= parent
	};
	return d;
}

gpc_TestAndSuiteData gpc_new_test(const char* name, gpc_TestAndSuiteData* parent)
{
	return gpc_newTestOrSuite(name, parent, GPC_IS_TEST);
}

gpc_TestAndSuiteData gpc_new_suite(const char* name, gpc_TestAndSuiteData* parent)
{
	return gpc_newTestOrSuite(name, parent, ! GPC_IS_TEST);
}

struct gpc_TestAndSuiteData gpc_gTestData = {0};
struct gpc_TestAndSuiteData *const gpc_currentTestOrSuite = &gpc_gTestData;

bool gpc_anyFails(struct gpc_TestAndSuiteData* data)
{
	return data->expectationFails || data->testFails || data->suiteFails;
}

#define PRINT_DATA(DATA)												\
	printf("A total of " GPC_CYAN("%i") " " #DATA "s completed, ",		\
			gpc_gTestData. DATA##Count );								\
	if (gpc_gTestData. DATA##Fails)										\
		printf(GPC_RED("%i failed")"\n", gpc_gTestData. DATA##Fails);	\
	else																\
		printf(GPC_GREEN("%i failed")"\n", gpc_gTestData. DATA##Fails);

void gpc_printExitMessageAndAddExitStatus(void)
{
	printf("\n");

	PRINT_DATA(expectation);
	PRINT_DATA(test);
	PRINT_DATA(suite);

	if (gpc_anyFails(&gpc_gTestData))
		exit(1);
}

#undef PRINT_DATA

static void gpc_printStartingMessageAndInitExitMessage(void)
{
	static bool initialized = false;
	if ( ! initialized)
	{
		printf("\n\tStarting tests...\n");
		atexit(gpc_printExitMessageAndAddExitStatus);
		initialized = true;
	}
}

//const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3] = {"==", "!=", ">", "<", ">=", "<="};
// const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3] = {
// #define X(DUMMY, OP) #OP,
	// GPC_OP_TABLE
// #undef X
// };

// static bool gpc_compareNumber(GPC_LONG_DOUBLE a, enum gpc_BooleanOperator op, GPC_LONG_DOUBLE b)
// {
	// switch(op)
	// {
		// case GPC_NO_OP:
			// return (bool)a;

	// #define X(OP_ENUM, OP) 	\
		// case GPC##OP_ENUM:	\
			// return a OP b;
		// GPC_OP_TABLE
	// #undef X

		// case GPC_OPS_LENGTH: {} // Suppress -Wswitch
	// }
	// return 0&&((bool)a+(bool)b); // Gets rid of pointless compiler warnings
// }

// static bool gpc_comparePointer(const void* a, enum gpc_BooleanOperator op, const void* b)
// {
	// switch(op)
	// {
		// case GPC_NO_OP:
			// return a;
	// #define X(OP_ENUM, OP) 	\
		// case GPC##OP_ENUM:	\
			// return a OP b;
		// GPC_OP_TABLE
	// #undef X
		// case GPC_OPS_LENGTH: {}
	// }
	// return (bool)(0&&((char*)a-(char*)b));
// }

// static bool gpc_compareCharPointer(const char* a, enum gpc_BooleanOperator op, const char* b)
// {
	// switch(op)
	// {
		// case GPC_NO_OP:
			// return a;
	// #define X(OP_ENUM, OP) 	\
		// case GPC##OP_ENUM:	\
			// return strcmp(a, b) OP 0;
		// GPC_OP_TABLE
	// #undef X
		// case GPC_OPS_LENGTH: {}
	// }
	// return (bool)(0&&((char*)a-(char*)b));
// }

// Finds suite by going trough all parent data
struct gpc_TestAndSuiteData* findSuite(struct gpc_TestAndSuiteData* data)
{
	bool suiteFound 	= data->isSuite;
	bool suiteNotFound	= data == &gpc_gTestData;

	if (suiteFound)
		return data;
	else if (suiteNotFound)
		return NULL;
	else
		return findSuite(data->parent);
}

// static void gpc_printExpectationFail(struct gpc_ExpectationData* expectation,
								 // struct gpc_TestAndSuiteData* data)
// {
	// const char* finalTestName = data->isTest || data->isSuite ? data->name : expectation->func;

	// if (expectation->isAssertion)
		// fprintf(stderr, "\nAssertion ");
	// else
		// fprintf(stderr, "\nExpectation ");

	// fprintf(stderr,
			// "in \"%s\" " GPC_RED("[FAILED]") " in \"%s\" " GPC_WHITE_BG("line %i") "\n", 
			// finalTestName, expectation->file, expectation->line);

	// fprintf(stderr, GPC_MAGENTA("%s"), expectation->str_a);
	// if (expectation->operation != GPC_NO_OP)
		// fprintf(stderr, GPC_MAGENTA(" %s %s"), expectation->str_operator, expectation->str_b);
	
	// if (expectation->type == GPC_NUMBER)
	// {
		// fprintf(stderr, " evaluated to " GPC_RED(GPC_LG_FORMAT), expectation->a);
		// if (expectation->operation != GPC_NO_OP)
			// fprintf(stderr, GPC_RED(" %s " GPC_LG_FORMAT), expectation->str_operator, expectation->b);
		// fprintf(stderr, ".\n");
	// }
	// else if (expectation->type == GPC_BOOL)
	// {
		// fprintf(stderr, " evaluated to ");

		// if (expectation->operation == GPC_NO_OP)
			// fprintf(stderr, GPC_RED("%s"), (bool)expectation->a ? "true" : "false");
		// else
			// fprintf(stderr, GPC_RED(GPC_LG_FORMAT " %s " GPC_LG_FORMAT), expectation->a, expectation->str_operator, expectation->b);
		// fprintf(stderr, ".\n");
	// }
	// else if (expectation->type == GPC_POINTER)
	// {
		// fprintf(stderr, " evaluated to " GPC_RED("%p"), expectation->pa);
		// if (expectation->operation != GPC_NO_OP)
			// fprintf(stderr, GPC_RED(" %s %p"), expectation->str_operator, expectation->pb);
		// fprintf(stderr, ".\n");
	// }
	// else if (expectation->type == GPC_CHAR_POINTER)
	// {
		// fprintf(stderr, " evaluated to " GPC_RED("%s"), (char*)expectation->pa);
		// if (expectation->operation != GPC_NO_OP)
			// fprintf(stderr, GPC_RED(" %s %s"), expectation->str_operator, (char*)expectation->pb);
		// fprintf(stderr, ".\n");
	// }
	
	// if (expectation->additionalFailMessage != NULL)
		// printf("%s\n", expectation->additionalFailMessage);

	// if (expectation->isAssertion) // print test and suite results early before exiting
	// {
		// if (data->isTest)
			// gpc_printTestOrSuiteResult(data);
		// struct gpc_TestAndSuiteData* suite = findSuite(data);
		// if (suite != NULL)
			// gpc_printTestOrSuiteResult(suite);
	// }
// }

// Adds one fail to all parents all the way to gpc_gTestData
void gpc_addExpectationFail(struct gpc_TestAndSuiteData* data)
{
	data->expectationFails++;
	if (data != &gpc_gTestData)
		gpc_addExpectationFail(data->parent);
}

// int gpc_assert_internal(struct gpc_ExpectationData expectation,
						// struct gpc_TestAndSuiteData* data)
// {
	// gpc_gTestData.expectationCount++;
	// bool passed = false;
	// if (expectation.type == GPC_NUMBER || expectation.type == GPC_BOOL)
		// passed = gpc_compareNumber(expectation.a,
								   // expectation.operation,
								   // expectation.b);
	// else if (expectation.type == GPC_POINTER)
		// passed = gpc_comparePointer(expectation.pa,
									// expectation.operation,
									// expectation.pb);
	// else if (expectation.type == GPC_CHAR_POINTER)
		// passed = gpc_compareCharPointer(expectation.pa,
										// expectation.operation,
										// expectation.pb);
	
	// if ( ! passed)
	// {
		// gpc_addExpectationFail(data);
		// gpc_printExpectationFail(&expectation, data);
		
		// if(expectation.isAssertion)
		// {
			// gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(data);
			// exit(1);
		// }
		// else
		// {
			// return 1;
		// }
	// }
	// return 0;
// }

static void gpc_printTestOrSuiteResult(struct gpc_TestAndSuiteData* data)
{
	const char* testOrSuite = data->isTest ? "Test" : "Suite";

	if ( ! data->expectationFails && ! data->testFails && ! data->suiteFails)
	{
		printf("\n%s \"%s\" " GPC_GREEN("[PASSED]") " \n", testOrSuite, data->name);
	}
	else
	{
		fprintf(stderr, "\n%s \"%s\" " GPC_RED("[FAILED]") " \n",
				testOrSuite, data->name);
	}
}

static void gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(struct gpc_TestAndSuiteData* data)
{
	bool anyFails = gpc_anyFails(data);
	if (anyFails && data->isTest)
	{
		data->parent->testFails++;
		if (data->parent != &gpc_gTestData)
			gpc_gTestData.testFails++;
	}
	if (anyFails && data->isSuite)
	{
		data->parent->suiteFails++;
		if (data->parent != &gpc_gTestData)
			gpc_gTestData.suiteFails++;
	}
}

bool gpc_testOrSuiteRunning(struct gpc_TestAndSuiteData* data)
{
	bool testOrSuiteHasRan = data->testOrSuiteRunning;

	if ( ! testOrSuiteHasRan)
	{
		gpc_printStartingMessageAndInitExitMessage();
	}
	else
	{
		if (data->isTest)
			gpc_gTestData.testCount++;
		else
			gpc_gTestData.suiteCount++;

		gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(data);
		gpc_printTestOrSuiteResult(data);
	}

	return data->testOrSuiteRunning = ! testOrSuiteHasRan;
}


// ***************************************************************************


		// N E W  S T U F F


// ***************************************************************************

#pragma GCC diagnostic ignored "-Wunused-parameter"

int gpc_assert(struct gpc_TestAndSuiteData* tdata,
			   bool expr,
			   const enum gpc_AssertOp op,
			   const char* file,
			   const int line,
			   const char* func,
			   const char* failMsg, // = ""
			   const enum gpc_AssertType a_type,
			   const char* a_str,
			   // const T a,
			   // const enum gpc_AssertType b_type,
			   // const char* b_str,
			   // const T b
			   ...)
{
	printf("%s\n", a_str);
	
	//if (a_type != GPC_ASSERT_BOOL)
	
	
	
	return 0;
}

