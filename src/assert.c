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

struct StackData
{
	const char* name;
	bool failed;
};

struct Stack
{
	size_t length;
	size_t capacity;
	struct StackData stack[];
} *gTestStack = NULL, *gSuiteStack = NULL;

static size_t nextPowOf2(size_t x)
{
	size_t y = 1;
	while (x >= (y *= 2));
	return y;
}

static void stackPush(struct Stack** s, const char* str)
{
	if (*s == NULL)
	{
		const size_t INIT_CAP = 1;
		*s = calloc(sizeof(struct Stack) + nextPowOf2(INIT_CAP) * sizeof(struct StackData), 1);
		if ( ! (*s))
			perror("Failed calloc() in stackPush()");
		(*s)->capacity = nextPowOf2(INIT_CAP);
	}
	
	(*s)->length++;
	if ((*s)->length >= (*s)->capacity)
	{
		(*s)->capacity = nextPowOf2((*s)->length);
		(*s) = realloc((*s), sizeof(struct Stack) + (*s)->capacity * sizeof(struct StackData));
		if ( ! (*s))
			perror("Failed realloc() in stackPush()");
	}
	
	(*s)->stack[(*s)->length - 1] = (struct StackData){ .name = str };
}

static struct StackData stackPop(struct Stack** s)
{
	(*s)->length--;
	struct StackData out = (*s)->stack[(*s)->length];
	if ((*s)->length == 0)
	{
		free(*s);
		*s = NULL;
	}
	return out;
}

static struct StackData* stackPeek(struct Stack* s)
{
	return &(s->stack[s->length - 1]);
}

// ---------------------------------------------------------------------------

bool gpc_anyFails(void)
{
	return !!gFailedAsserts;
}

static void indent(FILE* out, size_t level)
{
	for (size_t i = 0; i < level; i++)
		fprintf(out, "\t");
}

static void exitWithMsgAndStatus(void)
{
	FILE* out = gpc_anyFails() ? stderr : stdout;
	fprintf(out, "\t\tFinished testing\n");

	if (gpc_anyFails())
		exit(EXIT_FAILURE);
}

static void gpc_initStartAndExitMessages(void)
{
	static bool initialized = false;
	if ( ! initialized)
	{
		printf("\n\t\tStarting tests in " __FILE__ "...\n\n");
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
		current = name;
		stackPush(&gTestStack, name);
		return true;
	}
	else // finishing test
	{
		struct StackData test = stackPop(&gTestStack);
		FILE* out = test.failed ? stderr : stdout;
		size_t indentLevel = gSuiteStack ? gSuiteStack->length : 0;
		indent(out, indentLevel);
		fprintf(out, "Test " GPC_ORANGE("\"%s\"") " %s\n",
				test.name,
				test.failed ? GPC_RED("[FAILED]") : GPC_GREEN("[PASSED]"));
		
		current = gTestStack ? stackPeek(gTestStack)->name : NULL;
		
		return false;
	}
}

bool gpc_testSuite(const char* name)
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
		current = name;
		size_t indentLevel = gSuiteStack ? gSuiteStack->length : 0;
		stackPush(&gSuiteStack, name);
		puts("");
		indent(stdout, indentLevel);
		printf("Begin suite " GPC_ORANGE("\"%s\"") "\n", name);
		return true;
	}
	else // finishing suite
	{
		struct StackData suite = stackPop(&gSuiteStack);
		FILE* out = suite.failed ? stderr : stdout;
		size_t indentLevel = gSuiteStack ? gSuiteStack->length : 0;
		indent(out, indentLevel);
		fprintf(out, "Suite " GPC_ORANGE("\"%s\"") " %s\n\n",
				suite.name,
				suite.failed ? GPC_RED("[FAILED]") : GPC_GREEN("[PASSED]"));
		
		current = gSuiteStack ? stackPeek(gSuiteStack)->name : NULL;
		
		return false;
	}
}

gpc_CmpArgs gCmpArgs = {0};
gpc_CmpArgs* gpc_getCmpArgs(size_t bufSize)
{
	bool undefinedMalloc = bufSize == 0;
	if (undefinedMalloc)
		bufSize = 1;
	
	gCmpArgs.a = realloc(gCmpArgs.a, bufSize);
	gCmpArgs.b = realloc(gCmpArgs.b, bufSize);

	return &gCmpArgs;
}

// Not very DRY but macrofying any more than this kills debuggability
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

int gpc_assertStrcmp(const char* str1, const char* str2)
{
	size_t str1len = str1 ? strlen(str1) + sizeof("\"\"") : sizeof("(null)");
	size_t str2len = str2 ? strlen(str2) + sizeof("\"\"") : sizeof("(null)");
	gpc_CmpArgs* argBufs = gpc_getCmpArgs(str1len >= str2len ? str1len : str2len);
	
	if (str1 != NULL)
		strcat(strcat(strcpy(argBufs->a, "\""), str1), "\"");
	else
		strcpy(argBufs->a, "(null)");
	if (str2 != NULL)
		strcat(strcat(strcpy(argBufs->b, "\""), str2), "\"");
	else
		strcpy(argBufs->b, "(null)");
	
	const char* strp1 = str1 ? str1 : "";
	const char* strp2 = str2 ? str2 : "";
	return strcmp(strp1, strp2);
}

bool gpc_expect(const bool expr,
				const char* a,
				const char* op,
				const char* b,
				const char* failMsg,
				const char* file,
				const int line,
				const char* func)
{
	if (expr == true)
		return true;

	func = gTestStack ? stackPeek(gTestStack)->name :
		   gSuiteStack ? stackPeek(gSuiteStack)->name : func;
	
	const char* a_eval = gCmpArgs.a;
	const char* b_eval = gCmpArgs.b;
	if ( ! *b)
	{
		a_eval = "false";
		b_eval = "";
	}
	
	size_t indentLevel = (gSuiteStack ? gSuiteStack->length : 0) +
						 (gTestStack  ? gTestStack->length  : 0);
	
	indent(stderr, indentLevel);
	fprintf(stderr, "Assertion in " GPC_ORANGE("\"%s\"") " " GPC_RED("[FAILED]") "\n", func);
	indent(stderr, indentLevel);
	fprintf(stderr, "%s " GPC_WHITE_BG("line %i") "\n", file, line);
	
	const char* b_space  = *b  ? " " : "";
	const char* op_space = *op ? " " : "";
	indent(stderr, indentLevel);
	fprintf(stderr, GPC_MAGENTA("%s %s%s%s%s")"evaluated to "GPC_RED("%s%s%s%s%s"),
			a, op, b_space, b, b_space, a_eval, op_space, op, op_space, b_eval);
	fprintf(stderr, ". " GPC_CYAN("%s") "\n", failMsg);
	
	if (gCmpArgs.a != NULL)
	{
		free(gCmpArgs.a);
		free(gCmpArgs.b);
		gCmpArgs.a = NULL;
		gCmpArgs.b = NULL;
	}
	
	if (gTestStack != NULL)
		stackPeek(gTestStack)->failed = true;
	if (gSuiteStack != NULL)
		stackPeek(gSuiteStack)->failed = true;
	
	return false;
}

bool gpc_exitTests(bool b)
{
	if ( ! b)
		return true;
	while (gTestStack->length > 0)
		gpc_test(NULL);
	while (gSuiteStack->length > 0)
		gpc_testSuite(NULL);
	// TODO kill all owners
	exit(EXIT_FAILURE);
	return true; // prevent compiler complaints
}
