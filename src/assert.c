// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include "../include/gpc/assert.h"
#include "terminalcolors.h"

// #define GPC_IS_TEST true

// gpc_TestAndSuiteData gpc_newTestOrSuite(const char* name, gpc_TestAndSuiteData* parent,
										  // bool isTest)
// {
	// gpc_TestAndSuiteData d =
	// {
		// .name				= name,
		// .testFails			= 0,		.suiteFails =	0,	.expectationFails =	0,
		// .testCount			= 0,		.suiteCount =	0,	.expectationCount =	0,
		// .testDefined		= isTest,
		// .suiteDefined		= ! isTest,
		// .testOrSuiteRunning	= false,
		// .parent				= parent
	// };
	// return d;
// }

// gpc_TestAndSuiteData gpc_new_test(const char* name, gpc_TestAndSuiteData* parent)
// {
	// return gpc_newTestOrSuite(name, parent, GPC_IS_TEST);
// }

// gpc_TestAndSuiteData gpc_new_suite(const char* name, gpc_TestAndSuiteData* parent)
// {
	// return gpc_newTestOrSuite(name, parent, ! GPC_IS_TEST);
// }

// struct gpc_TestAndSuiteData gpc_gTestData = {0};
// struct gpc_TestAndSuiteData *const gpc_currentTestOrSuite = &gpc_gTestData;

// bool gpc_anyFails(struct gpc_TestAndSuiteData* data)
// {
	// return data->expectationFails || data->testFails || data->suiteFails;
// }

// #define PRINT_DATA(DATA)												\
	// printf("A total of " GPC_CYAN("%i") " " #DATA "s completed, ",		\
			// gpc_gTestData. DATA##Count );								\
	// if (gpc_gTestData. DATA##Fails)										\
		// printf(GPC_RED("%i failed")"\n", gpc_gTestData. DATA##Fails);	\
	// else																\
		// printf(GPC_GREEN("%i failed")"\n", gpc_gTestData. DATA##Fails);

// void gpc_printExitMessageAndAddExitStatus(void)
// {
	// printf("\n");

	// PRINT_DATA(expectation);
	// PRINT_DATA(test);
	// PRINT_DATA(suite);

	// if (gpc_anyFails(&gpc_gTestData))
		// exit(1);
// }

// #undef PRINT_DATA

// static void gpc_printStartingMessageAndInitExitMessage(void)
// {
	// static bool initialized = false;
	// if ( ! initialized)
	// {
		// printf("\n\tStarting tests...\n");
		// atexit(gpc_printExitMessageAndAddExitStatus);
		// initialized = true;
	// }
// }

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
// struct gpc_TestAndSuiteData* findSuite(struct gpc_TestAndSuiteData* data)
// {
	// bool suiteFound 	= data->isSuite;
	// bool suiteNotFound	= data == &gpc_gTestData;

	// if (suiteFound)
		// return data;
	// else if (suiteNotFound)
		// return NULL;
	// else
		// return findSuite(data->parent);
// }

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
// void gpc_addExpectationFail(struct gpc_TestAndSuiteData* data)
// {
	// data->expectationFails++;
	// if (data != &gpc_gTestData)
		// gpc_addExpectationFail(data->parent);
// }

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

// static void gpc_printTestOrSuiteResult(struct gpc_TestAndSuiteData* data)
// {
	// const char* testOrSuite = data->isTest ? "Test" : "Suite";

	// if ( ! data->expectationFails && ! data->testFails && ! data->suiteFails)
	// {
		// printf("\n%s \"%s\" " GPC_GREEN("[PASSED]") " \n", testOrSuite, data->name);
	// }
	// else
	// {
		// fprintf(stderr, "\n%s \"%s\" " GPC_RED("[FAILED]") " \n",
				// testOrSuite, data->name);
	// }
// }

// static void gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(struct gpc_TestAndSuiteData* data)
// {
	// bool anyFails = gpc_anyFails(data);
	// if (anyFails && data->isTest)
	// {
		// data->parent->testFails++;
		// if (data->parent != &gpc_gTestData)
			// gpc_gTestData.testFails++;
	// }
	// if (anyFails && data->isSuite)
	// {
		// data->parent->suiteFails++;
		// if (data->parent != &gpc_gTestData)
			// gpc_gTestData.suiteFails++;
	// }
// }

// bool gpc_testOrSuiteRunning(struct gpc_TestAndSuiteData* data)
// {
	// bool testOrSuiteHasRan = data->testOrSuiteRunning;

	// if ( ! testOrSuiteHasRan)
	// {
		// gpc_printStartingMessageAndInitExitMessage();
	// }
	// else
	// {
		// if (data->isTest)
			// gpc_gTestData.testCount++;
		// else
			// gpc_gTestData.suiteCount++;

		// gpc_addTestOrSuiteFailToParentAndGlobalIfFailed(data);
		// gpc_printTestOrSuiteResult(data);
	// }

	// return data->testOrSuiteRunning = ! testOrSuiteHasRan;
// }


// ***************************************************************************


		// N E W  S T U F F


// ***************************************************************************

#pragma GCC diagnostic ignored "-Wunused-parameter" // REMOVE
#pragma GCC diagnostic ignored "-Wunused-variable" // REMOVE

static unsigned gPassedAsserts			= 0;
static unsigned gFailesAsserts			= 0;
static unsigned gPassedTests			= 0;
static unsigned gFailedTests			= 0;
static unsigned gPassedSuites			= 0;
static unsigned gFailedSuites			= 0;
static unsigned gPassedAssertsInTest	= 0;
static unsigned gFailedAssertsInTest	= 0;
static unsigned gPassedAssertsInSuite	= 0;
static unsigned gFailedAssertsInSuite	= 0;
static unsigned gPassedTestsInSuite		= 0;
static unsigned gFailedTestsInSuite		= 0;

static bool gTestRunning  = false;
static bool gSuiteRunning = false;

struct strStack
{
	const char** stack;
	size_t length;
	size_t capacity;
} gTestStack, gSuiteStack;

static size_t nextPowOf2(size_t x)
{
	size_t y = 1;
	while (x >= (y *= 2));
	return y;
}

static void strStackPush(struct strStack* s, const char* str)
{
	if (s->capacity == 0)
	{
		const size_t WHATEVER = 2;
		s->stack = malloc(WHATEVER * sizeof(*s->stack));
		s->capacity = WHATEVER;
	}
	
	s->length++;
	if (s->length >= s->capacity)
		s->stack = realloc(s->stack, s->capacity = nextPowOf2(s->length));
	s->stack[s->length - 1] = str;
}

static const char* strStackPop(struct strStack* s)
{
	s->length--;
	return s->stack[s->length];
}

static const char* strStackPeek(struct strStack s)
{
	return s.stack[s.length - 1];
}

bool gpc_anyFails(void)
{
	return !!gFailesAsserts;
}

static void exitWithMsgAndStatus(void)
{
	printf("\n");

	if (gpc_anyFails())
		exit(EXIT_FAILURE);
}

static void gpc_initStartAndExitMessages(void)
{
	static bool initialized = false;
	if ( ! initialized)
	{
		printf("\n\tStarting tests in " __FILE__ "...\n\n");
		atexit(exitWithMsgAndStatus);
		initialized = true;
	}
}

bool gpc_test(const char* name)
{	
	gpc_initStartAndExitMessages();
	
	if ((gTestRunning = !gTestRunning))
	{
		//gAssertsInTest = 0;
		strStackPush(&gTestStack, name);
	}
	else // finishing test
	{
		printf("finished test %s\n", strStackPop(&gTestStack));
	}
	
	return gTestRunning;
}

bool gpc_testSuite(const char* name)
{
	gpc_initStartAndExitMessages();
	
	if ((gSuiteRunning = !gSuiteRunning))
	{
		// gAssertsInSuite	= 0;
		// gTestsInSuite	= 0;
		strStackPush(&gSuiteStack, name);
	}
	else // finishing test suite
	{
		printf("finished test suite %s\n", strStackPop(&gSuiteStack));
	}
	
	return gSuiteRunning;
}

#define MAX_STRFIED_LENGTH 28 // = more than max digits in uint64_t

// *buf is in case of required formatting. In case of T == GPC_ASSERT_CHAR_PTR,
// buf is modified to point to the string in arg in full length. 
// *buf must be zero initialized!
static void strfy(char** buf, const enum gpc_Type T, va_list* arg)
{
	char c;
	switch (T)
	{
		case GPC_BOOL:
			sprintf(*buf, "%s", va_arg(*arg, int) ? "true" : "false");
			break;
		case GPC_SHORT:
			sprintf(*buf, "%i", va_arg(*arg, int));
			break;
		case GPC_INT:
			sprintf(*buf, "%i", va_arg(*arg, int));
			break;
		case GPC_LONG:
			sprintf(*buf, "%li", va_arg(*arg, long));
			break;
		case GPC_LONG_LONG:
			sprintf(*buf, "%lli", va_arg(*arg, long long));
			break;
		case GPC_UNSIGNED_SHORT:
			sprintf(*buf, "%u", va_arg(*arg, unsigned));
			break;
		case GPC_UNSIGNED:
			sprintf(*buf, "%u", va_arg(*arg, unsigned));
			break;
		case GPC_UNSIGNED_LONG:
			sprintf(*buf, "%lu", va_arg(*arg, unsigned long));
			break;
		case GPC_UNSIGNED_LONG_LONG:
			sprintf(*buf, "%llu", va_arg(*arg, unsigned long long));
			break;
		case GPC_FLOAT:
			sprintf(*buf, "%g", va_arg(*arg, double));
			break;
		case GPC_DOUBLE:
			sprintf(*buf, "%g", va_arg(*arg, double));
			break;
		case GPC_CHAR:
			c = (char)va_arg(*arg, int);
			sprintf(*buf, "\'%c\'=%i", c, (int)c);
			break;
		case GPC_UNSIGNED_CHAR:
			c = (char)va_arg(*arg, int);
			sprintf(*buf, "\'%c\'=%i", c, (int)c);
			break;
		case GPC_CHAR_PTR:
			*buf = va_arg(*arg, char*);
			break;
		case GPC_PTR:
			sprintf(*buf, "%p", va_arg(*arg, void*));
			break;
	}

	bool nullTerminated = T == GPC_CHAR_PTR;
	if ( ! nullTerminated)
		(*buf)[MAX_STRFIED_LENGTH - 1] = '\0';
}

static const char* getOp(const char* op)
{
	static const char* const table[][2] = { {"EQ"," == "},
											{"NE"," != "},
											{"LT"," < " },
											{"GT"," > " },
											{"LE"," <= "},
											{"GE"," >= "} };
	const char* out = "";
	for (size_t i = 0; i < sizeof(table)/sizeof(table[0]); i++)
		if (strcmp(op, table[i][0]) == 0)
		{
			out = table[i][1];
			break;
		}
	return out;
	
	
		// case 'E' + 'Q':
		// case 'N' + 'E':
		// case 'L' + 'T':
		// case 'G' + 'T':
		// case 'L' + 'E':
		// case 'G' + 'E':
}

static bool strCompare(const char* a, const char* b, const char* op)
{
	switch (op[0] + op[1])
	{
		case 'E' + 'Q': return strcmp(a, b) == 0;
		case 'N' + 'E': return strcmp(a, b) != 0;
		case 'L' + 'T': return strcmp(a, b) <  0;
		case 'G' + 'T': return strcmp(a, b) >  0;
		case 'L' + 'E': return strcmp(a, b) <= 0;
		case 'G' + 'E': return strcmp(a, b) >= 0;
	}
	return false;
}

bool gpc_assert(const bool expr,
				const char* op_str,
				const char* file,
				const int line,
				const char* func,
				const char* failMsg,
				const enum gpc_Type a_type,
				const char* a_str,
				// const T a,
				// const enum gpc_Type b_type,
				// const char* b_str,
				// const T b
				...)
{
	if (expr == true)
		return true;

	func = gTestRunning ? strStackPeek(gTestStack) :
		   gSuiteRunning ? strStackPeek(gSuiteStack) : func;
		
	va_list args;
	va_start(args, a_str);
	
	char a_evalbuf[MAX_STRFIED_LENGTH] = "";
	char* a_eval = a_evalbuf;
	char* a; // only used if string comparison required
	if (a_type == GPC_CHAR_PTR)
		a_eval = a = va_arg(args, char*);
	else
		strfy(&a_eval, a_type, &args);
	
	enum gpc_Type b_type = 0;
	#define MAX_B_STR_LENGTH 80
	char b_str[MAX_B_STR_LENGTH] = "";
	char b_evalbuf[MAX_STRFIED_LENGTH] = "";
	char* b_eval = b_evalbuf;
	char* b;
	
	if (*op_str)
	{
		b_type = va_arg(args, enum gpc_Type);
		b_str[0] = ' ';
		strncpy(b_str + 1, va_arg(args, char*), MAX_B_STR_LENGTH - 2);
		strcpy(b_str + strlen(b_str), " ");
		if (b_type == GPC_CHAR_PTR)
			b_eval = b = va_arg(args, char*);
		else
			strfy(&b_eval, b_type, &args);
	}
	
	bool strComparison = a_type == GPC_CHAR_PTR && b_type == GPC_CHAR_PTR;
	bool strexpr = false;
	if (strComparison)
		strexpr = strCompare(a, b, op_str);
	if (strexpr == true)
		return true;
	
	if (a_type == GPC_CHAR_PTR && !strComparison)
		sprintf(a_eval = a_evalbuf, "%p", a);
	if (b_type == GPC_CHAR_PTR && !strComparison)
		sprintf(b_eval = b_evalbuf, "%p", b);
	
	// TODO check the lengths of a_eval and b_eval and add "..." based on their
	// differences appropriately to a different buffer. Only with strig cmp.  
	// something = malloc(strlen(a_eval));
	
	fprintf(stderr, "%s"GPC_ORANGE("%s%s%s")GPC_RED("%s")"%s%s%s"GPC_WHITE_BG("%s%i")"%s",
			"Assertion in ","\"",func,"\" ","[FAILED]"," in ",file," ","line ",line,"\n");
	
	const char* quote = strComparison ? "\"" : "";
	fprintf(stderr, GPC_MAGENTA("%s%s%s%s")"%s"GPC_RED("%s%s%s%s%s%s%s")"%s%s%s",
			a_str, " ", op_str, b_str, "evaluated to ",
			quote, a_eval, quote, getOp(op_str), quote, b_eval, quote, ". ", failMsg, "\n\n");
	
	va_end(args);
	return false;
}

char* gpc_charptrfy(int dummy, ...)
{
	va_list p;
	va_start(p, dummy);
	char* out = va_arg(p, char*);
	va_end(p);
	return out;
}