/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include "../include/gpc/assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GPC_IS_TEST true

GPC_TestAndSuiteData GPC_newTestOrSuite(const char* name, GPC_TestAndSuiteData* parent,
										  bool isTest)
{
	GPC_TestAndSuiteData D =
	{
		.name				= name,
		.testFails			= 0,		.suiteFails =	0,	.expectationFails =	0,
		.testCount			= 0,		.suiteCount =	0,	.expectationCount =	0,
		.testDefined		= isTest,
		.suiteDefined		= ! isTest,
		.testOrSuiteRunning	= false,
		.parent				= parent
	};
	return D;
}

GPC_TestAndSuiteData GPC_new_test(const char* name, GPC_TestAndSuiteData* parent)
{
	return GPC_newTestOrSuite(name, parent, GPC_IS_TEST);
}

GPC_TestAndSuiteData GPC_new_suite(const char* name, GPC_TestAndSuiteData* parent)
{
	return GPC_newTestOrSuite(name, parent, ! GPC_IS_TEST);
}

struct GPC_TestAndSuiteData GPC_gTestData = {};
struct GPC_TestAndSuiteData *const GPC_currentTestOrSuite = &GPC_gTestData;

#define GPC_RED(STR_LITERAL)		"\033[0;31m"			STR_LITERAL "\033[0m"
#define GPC_GREEN(STR_LITERAL)		"\033[0;92m"			STR_LITERAL "\033[0m"
#define GPC_MAGENTA(STR_LITERAL)	"\033[0;95m"			STR_LITERAL "\033[0m"
#define GPC_CYAN(STR_LITERAL)		"\033[0;96m"			STR_LITERAL "\033[0m"
#define GPC_WHITE_BG(STR_LITERAL)	"\033[0;47m\033[30m"	STR_LITERAL "\033[0m"

bool GPC_anyFails(struct GPC_TestAndSuiteData* data)
{
	return data->expectationFails || data->testFails || data->suiteFails;
}

#define PRINT_DATA(DATA)												\
	printf("A total of " GPC_CYAN("%i") " " #DATA "s completed, ",		\
			GPC_gTestData. DATA##Count );								\
	if (GPC_gTestData. DATA##Fails)										\
		printf(GPC_RED("%i failed")"\n", GPC_gTestData. DATA##Fails);	\
	else																\
		printf(GPC_GREEN("%i failed")"\n", GPC_gTestData. DATA##Fails);

void GPC_printExitMessageAndAddExitStatus()
{
	printf("\n");

	PRINT_DATA(expectation);
	PRINT_DATA(test);
	PRINT_DATA(suite);

	if (GPC_anyFails(&GPC_gTestData))
		exit(1);
}

#undef PRINT_DATA

void GPC_printStartingMessageAndInitExitMessage()
{
	static bool initialized = false;
	if ( ! initialized)
	{
		printf("\n\tStarting tests...\n");
		atexit(GPC_printExitMessageAndAddExitStatus);
		initialized = true;
	}
}

//const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3] = {"==", "!=", ">", "<", ">=", "<="};
const char GPC_STR_OPERATORS[GPC_OPS_LENGTH][3] = {
#define X(DUMMY, OP) #OP,
	OP_TABLE
#undef X
};

bool GPC_compareNumber(double a, enum GPC_BooleanOperator operation, double b)
{
	switch(operation)
	{
		case GPC_NO_OP:
			return a;

	#define X(OP_ENUM, OP) 	\
		case GPC##OP_ENUM:	\
			return a OP b;
		OP_TABLE
	#undef X

	// Expands to
	
	/*	case GPC_EQ:
			return a == b;
		case GPC_NE:
			return a != b;
		// etc...
	*/
		case GPC_OPS_LENGTH: {} // Suppress -Wswitch
	}
	return 0&&(a+b); // Gets rid of pointless compiler warnings
}

bool GPC_comparePointer(const void* a, enum GPC_BooleanOperator operation, const void* b)
{
	switch(operation)
	{
		case GPC_NO_OP:
			return a;
	#define X(OP_ENUM, OP) 	\
		case GPC##OP_ENUM:	\
			return a OP b;
		OP_TABLE
	#undef X
		case GPC_OPS_LENGTH: {}
	}
	return 0&&(a-b);
}

bool GPC_compareCharPointer(const char* a, enum GPC_BooleanOperator operation, const char* b)
{
	switch(operation)
	{
		case GPC_NO_OP:
			return a;
	#define X(OP_ENUM, OP) 	\
		case GPC##OP_ENUM:	\
			return strcmp(a, b) OP 0;
		OP_TABLE
	#undef X
		case GPC_OPS_LENGTH: {}
	}
	return 0&&(a-b);
}

// Finds suite by going trough all parent data
struct GPC_TestAndSuiteData* findSuite(struct GPC_TestAndSuiteData* data)
{
	bool suiteFound 	= data->isSuite;
	bool suiteNotFound	= data == &GPC_gTestData;

	if (suiteFound)
		return data;
	else if (suiteNotFound)
		return NULL;
	else // keep looking
		return findSuite(data->parent);
}

void GPC_printExpectationFail(struct GPC_ExpectationData* expectation,
								 struct GPC_TestAndSuiteData* data)
{
	const char* finalTestName = data->isTest || data->isSuite ? data->name : expectation->func;

	if (expectation->isAssertion)
		fprintf(stderr, "\nAssertion ");
	else
		fprintf(stderr, "\nExpectation ");

	fprintf(stderr,
			"in \"%s\" " GPC_RED("[FAILED]") " in \"%s\" " GPC_WHITE_BG("line %i") "\n", 
			finalTestName, expectation->file, expectation->line);

	fprintf(stderr, GPC_MAGENTA("%s"), expectation->str_a);
	if (expectation->operation != GPC_NO_OP)
		fprintf(stderr, GPC_MAGENTA(" %s %s"), expectation->str_operator, expectation->str_b);
	
	if (expectation->type == GPC_NUMBER)
	{
		fprintf(stderr, " evaluated to " GPC_RED("%g"), expectation->a);
		if (expectation->operation != GPC_NO_OP)
			fprintf(stderr, GPC_RED(" %s %g"), expectation->str_operator, expectation->b);
		fprintf(stderr, ".\n");
	}
	else if (expectation->type == GPC_POINTER)
	{
		fprintf(stderr, " evaluated to " GPC_RED("%p"), expectation->pa);
		if (expectation->operation != GPC_NO_OP)
			fprintf(stderr, GPC_RED(" %s %p"), expectation->str_operator, expectation->pb);
		fprintf(stderr, ".\n");
	}
	
	if (expectation->additionalFailMessage != NULL)
		printf("%s\n", expectation->additionalFailMessage);

	if (expectation->isAssertion) // print test and suite results early before exiting
	{
		if (data->isTest)
			GPC_printTestOrSuiteResult(data);
		struct GPC_TestAndSuiteData* suite = findSuite(data);
		if (suite != NULL)
			GPC_printTestOrSuiteResult(suite);
	}
}

// Adds one fail to all parents all the way to GPC_gTestData
void GPC_addExpectationFail(struct GPC_TestAndSuiteData* data)
{
	data->expectationFails++;
	if (data != &GPC_gTestData)
		GPC_addExpectationFail(data->parent);
}

int GPC_assert(struct GPC_ExpectationData expectation,
				  struct GPC_TestAndSuiteData* data)
{
	GPC_gTestData.expectationCount++;
	bool passed = false;
	if (expectation.type == GPC_NUMBER)
		passed = GPC_compareNumber(expectation.a,
								   expectation.operation,
								   expectation.b);
	else if (expectation.type == GPC_POINTER)
		passed = GPC_comparePointer(expectation.pa,
									expectation.operation,
									expectation.pb);
	else if (expectation.type == GPC_CHAR_POINTER)
		passed = GPC_compareCharPointer(expectation.pa,
										expectation.operation,
										expectation.pb);
	
	if ( ! passed)
	{
		GPC_addExpectationFail(data);
		GPC_printExpectationFail(&expectation, data);
		// if(expectation.isAssertion)
			// exit(1);
		// else 
			// return 1;
		if(expectation.isAssertion)
		{
			GPC_addTestOrSuiteFailToParentAndGlobalIfFailed(data);
			exit(1);
		}
		else
		{
			return 1;
		}
	}
	return 0;
}

bool GPC_testOrSuiteRunning(struct GPC_TestAndSuiteData* data)
{
	bool testOrSuiteHasRan = data->testOrSuiteRunning;

	if ( ! testOrSuiteHasRan)
	{
		GPC_printStartingMessageAndInitExitMessage();
	}
	else
	{
		if (data->isTest)
			GPC_gTestData.testCount++;
		else
			GPC_gTestData.suiteCount++;

		GPC_addTestOrSuiteFailToParentAndGlobalIfFailed(data);
		GPC_printTestOrSuiteResult(data);
	}

	return data->testOrSuiteRunning = ! testOrSuiteHasRan;
}

void GPC_addTestOrSuiteFailToParentAndGlobalIfFailed(struct GPC_TestAndSuiteData* data)
{
	bool anyFails = GPC_anyFails(data);
	if (anyFails && data->isTest)
	{
		data->parent->testFails++;
		if (data->parent != &GPC_gTestData)
			GPC_gTestData.testFails++;
	}
	if (anyFails && data->isSuite)
	{
		data->parent->suiteFails++;
		if (data->parent != &GPC_gTestData)
			GPC_gTestData.suiteFails++;
	}
}

void GPC_printTestOrSuiteResult(struct GPC_TestAndSuiteData* data)
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
