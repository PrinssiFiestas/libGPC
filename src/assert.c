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

// ---------------------------------------------------------------------------
// MOVE THESE TO overload.c

bool gpc_isUnsigned(const enum gpc_Type T) { return T < GPC_BOOL; }
bool gpc_isInteger(const enum gpc_Type T) { return T < GPC_FLOAT; }
bool gpc_isFloating(const enum gpc_Type T) { return GPC_FLOAT <= T && T < GPC_CHAR_PTR; }
bool gpc_isPointer(const enum gpc_Type T) { return GPC_CHAR_PTR <= T && T < GPC_PTR; }

size_t gpc_sizeof(const enum gpc_Type T)
{
	switch (T)
	{
		case GPC_UNSIGNED_CHAR:			return sizeof(unsigned char);
		case GPC_UNSIGNED_SHORT:		return sizeof(unsigned short);
		case GPC_UNSIGNED:				return sizeof(unsigned);
		case GPC_UNSIGNED_LONG:			return sizeof(unsigned long);
		case GPC_UNSIGNED_LONG_LONG:	return sizeof(unsigned long long);
		case GPC_BOOL:					return sizeof(bool);
		case GPC_CHAR:					return sizeof(char);
		case GPC_SHORT:					return sizeof(short);
		case GPC_INT:					return sizeof(int);
		case GPC_LONG:					return sizeof(long);
		case GPC_LONG_LONG:				return sizeof(long long);
		case GPC_FLOAT:					return sizeof(float);
		case GPC_DOUBLE:				return sizeof(double);
		case GPC_CHAR_PTR:				return sizeof(char*);
		case GPC_PTR:					return sizeof(void*);
	}
	return 0;
}
// ---------------------------------------------------------------------------



#pragma GCC diagnostic ignored "-Wunused-parameter" // REMOVE
#pragma GCC diagnostic ignored "-Wunused-variable" // REMOVE

static unsigned gPassedAsserts			= 0;
static unsigned gFailedAsserts			= 0;
static unsigned gPassedTests			= 0;
static unsigned gFailedTests			= 0;
static unsigned gPassedSuites			= 0;
static unsigned gFailedSuites			= 0;

struct Stack
{
	struct StackData* stack;
	size_t length;
	size_t capacity;
} gTestStack, gSuiteStack;

struct StackData
{
	const char* name;
	struct StackData* suite; // only used by tests
	struct Stack* testStack; // only used by suites
	bool failed;
};

static size_t nextPowOf2(size_t x)
{
	size_t y = 1;
	while (x >= (y *= 2));
	return y;
}

static void stackPush(struct Stack* s, const char* str)
{
	if (s->stack == NULL)
	{
		s->stack = calloc(sizeof(struct StackData), 1);
		s->capacity = 1;
	}
	
	s->length++;
	if (s->length >= s->capacity)
		s->stack = realloc(s->stack, s->capacity = nextPowOf2(s->length));
	
	s->stack[s->length - 1].name = str;
}

static struct StackData stackPop(struct Stack* s)
{
	struct StackData data = s->stack[s->length - 1];
	s->stack[s->length - 1] = (struct StackData){0};
	s->length--;
	return data;
}

static struct StackData* stackPeek(struct Stack* s)
{
	return s->stack + s->length - 1;
}

bool gpc_anyFails(void)
{
	return !!gFailedAsserts;
}

static void exitWithMsgAndStatus(void)
{
	FILE* out = gpc_anyFails() ? stderr : stdout;
	fprintf(out, "\tFinished testing\n");

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
	static const char* current = NULL;
	gpc_initStartAndExitMessages();
		
	bool shouldEnd = false;
	if (name == NULL)
		shouldEnd = true;
	if (name != NULL && current != NULL)
		shouldEnd = strcmp(name, current) == 0;
	
	if ( ! shouldEnd)
	{
		current = (char*)name;
		stackPush(&gTestStack, name);
		
		return true;
	}
	else // finishing test
	{
		struct StackData test = stackPop(&gTestStack);
		FILE* out = test.failed ? stderr : stdout;
		fprintf(out, "Test \"" GPC_ORANGE("%s") "\" %s\n\n",
				test.name,
				test.failed ? GPC_RED("[FAILED]") : GPC_GREEN("[PASSED]"));
		
		current = gTestStack.length > 0 ? stackPeek(&gTestStack)->name : NULL;
		
		return false;
	}
}

bool gpc_testSuite(const char* name)
{
	static char* current = NULL;
	gpc_initStartAndExitMessages();
	
	bool shouldEnd = false;
	if (name == NULL)
		shouldEnd = true;
	if (name != NULL && current != NULL)
		shouldEnd = strcmp(name, current) == 0;
	
	//if ((gSuiteRunning = !gSuiteRunning))
	if ( ! shouldEnd)
	{
		stackPush(&gSuiteStack, name);
		
		return true;
	}
	else // finishing test suite
	{
		bool failed = stackPeek(&gSuiteStack)->failed;
		FILE* out = failed ? stderr : stdout;
		fprintf(out, "Test suite \"" GPC_ORANGE("%s") "\" %s\n\n", // HJGIUORFPHGURE
				stackPop(&gSuiteStack).name,
				failed ? GPC_RED("[FAILED]") : GPC_GREEN("[PASSED]"));
		
		return false;
	}
}

gpc_CmpArgs gCmpArgs = {0};

gpc_CmpArgs* gpc_getCmpArgs(size_t bufSize)
{
	bool undefinedMalloc = bufSize == 0;
	if (undefinedMalloc)
		bufSize = 1;
	
	// realloc() is used instead of malloc() in case of multiple calls. This 
	// allows user to do something like
	/*
	strcpy(getCmpArgs(10)->a, "short a");
	strcpy(getCmpArgs(50)->a, "possibly longer b");
	*/
	gCmpArgs.a = realloc(gCmpArgs.a, bufSize);
	gCmpArgs.b = realloc(gCmpArgs.b, bufSize);
	return &gCmpArgs;
}

// Macrofying any more than this kills debuggability
#define GET_VAL(T)			\
	va_list arg;			\
	va_start(arg, buf);		\
	T val = va_arg(arg, T);	\
	va_end(arg);
bool gpc_strfyb(char* buf, ...)
{
	GET_VAL(int);
	sprintf(buf, "%s", val ? "true" : "false");
	return (bool)val;
}
long long gpc_strfyi(char* buf, ...)
{
	GET_VAL(long long);
	sprintf(buf, "%lli", val);
	return val;
}
unsigned long long gpc_strfyu(char* buf, ...)
{
	GET_VAL(unsigned long long);
	sprintf(buf, "%llu", val);
	return val;
}
double gpc_strfyf(char* buf, ...)
{
	GET_VAL(double);
	sprintf(buf, "%g", val);
	return val;
}
char gpc_strfyc(char* buf, ...)
{
	GET_VAL(int);
	sprintf(buf, "\'%c\'", val);
	return (char)val;
}
void* gpc_strfyp(char* buf, ...)
{
	GET_VAL(void*);
	sprintf(buf, "%p", val);
	return val;
}
#undef GET_VAL

static const char* getOp(const char* op)
{
	switch(op[0] + op[1])
	{
		case '=' + '=' : return " == ";
		case '!' + '=' : return " != ";
		case '<' + '\0': return " < " ;
		case '>' + '\0': return " > " ;
		case '<' + '=' : return " <= ";
		case '>' + '=' : return " >= ";
	}
	return "";
}

int gpc_strcmp(const char str1[static 1], const char str2[static 1])
{
	if (str1 == str2)
		return 0;
	const char* strp1 = str1 ? str1 : "";
	const char* strp2 = str2 ? str2 : "";
	return strcmp(strp1, strp2);
}

char* gpc_quotify(char** buf, const char* str)
{
	(void)buf;
	if (str == NULL)
		return strcpy(malloc(sizeof("NULL")), "NULL");
	const size_t len = strlen(str);
	char* out = malloc(len + 1/*null*/+ 2/*quotes*/);
	out[0] = '\"';
	strcpy(out + 1, str);
	out[len + 1] = '\"';
	out[len + 2] = '\0';
	return out;
}

// TODO more logical order
bool gpc_expect(const bool expr,
				const char* op,
				const char* file,
				const int line,
				const char* func,
				const char* failMsg,
				const char* a,
				const char* b)
{
	if (expr == true)
		return true;

	/*func = gTestStack.length > 0 ? stackPeek(&gTestStack)->name :
		   gSuiteStack.length > 0 ? stackPeek(&gSuiteStack)->name : func;*/
	
	// TODO test allocating, validity of pointers, never calling getCmpArgs() etc.
	const char* a_eval = gCmpArgs.a;
	const char* b_eval = gCmpArgs.b;
	
	if (gCmpArgs.a != NULL)
	{
		free(gCmpArgs.a);
		free(gCmpArgs.b);
		gCmpArgs.a = NULL;
		gCmpArgs.b = NULL;
	}
	
	fprintf(stderr, "%s"GPC_ORANGE("%s%s%s")GPC_RED("%s")"%s%s%s"GPC_WHITE_BG("%s%i")"%s",
			"Assertion in ","\"",func,"\" ","[FAILED]"," in ",file," ","line ",line,"\n");
	
	const char* b_space = *b ? " " : "";
	fprintf(stderr, GPC_MAGENTA("%s%s%s%s%s")"%s%s"GPC_RED("%s%s%s")"%s%s%s",
			a, " ", op, b_space, b, b_space, "evaluated to ",
			a_eval, getOp(op), b_eval, ". ", failMsg, "\n\n");
	
	/*
	if (gTestStack.length > 0)
		stackPeek(&gTestStack)->failed = true;
	if (gSuiteStack.length > 0)
		stackPeek(&gSuiteStack)->failed = true;
	*/
	return false;
}

bool gpc_exitTests(bool b)
{
	if ( ! b)
		return true;
	while (gTestStack.length > 0)
		gpc_test(NULL);
	while (gSuiteStack.length > 0)
		gpc_testSuite(NULL);
	// TODO kill all owners
	exit(EXIT_FAILURE);
	return true; // prevent compiler complaints
}
