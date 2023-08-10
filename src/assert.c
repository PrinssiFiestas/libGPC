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
#include "../include/gpc/memory.h"
#include "terminalcolors.h"

static unsigned gPassedAsserts = 0;
static unsigned gFailedAsserts = 0;

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
} nullStack = {0};

struct globalData
{
	unsigned passed;
	unsigned failed;
	struct Stack* stack;
} gTests = { .stack = &nullStack }, gSuites = { .stack = &nullStack };

static size_t nextPowOf2(size_t x)
{
	size_t y = 1;
	while (x >= (y *= 2));
	return y;
}

static void stackPush(struct Stack** s, const char* str)
{
	if (*s == NULL || *s == &nullStack)
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
		*s = &nullStack;
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

static void printResult(const char* target, unsigned passed, unsigned failed)
{
	FILE* out = gpc_anyFails() ? stderr : stdout;
	fprintf(out, "From a total of %u %s ", passed + failed, target);
	if (failed > 0)
		fprintf(out, GPC_RED("%u failed!") "\n", failed);
	else
		fprintf(out, GPC_GREEN("%u failed.") "\n", failed);
}

static void exitWithMsgAndStatus(void)
{
	fprintf(gpc_anyFails() ? stderr : stdout, "\n\t\tFinished testing\n\n");
	
	printResult("suites",     gSuites.passed, gSuites.failed);
	printResult("tests",      gTests.passed,  gTests.failed);
	printResult("assertions", gPassedAsserts, gFailedAsserts);
	
	if (gpc_anyFails())
		exit(EXIT_FAILURE); // set exit status
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

static bool testOrSuite(const char* name, const char** current, struct globalData* testOrSuite)
{
	bool shouldEnd = false;
	if (name == NULL)
		shouldEnd = true;
	if (name != NULL && *current != NULL)
		shouldEnd = strcmp(name, *current) == 0;
	
	bool isSuite = testOrSuite == &gSuites;
	size_t indentLevel = gSuites.stack->length;
	
	if ( ! shouldEnd)
	{
		*current = name;
		stackPush(&testOrSuite->stack, name);
		if (isSuite)
		{
			puts("");
			indent(stdout, indentLevel);
			printf("Begin suite " GPC_ORANGE("\"%s\"") "\n", name);
		}
		return true;
	}
	else
	{
		struct StackData t = stackPop(&testOrSuite->stack);
		if (t.failed)
			testOrSuite->failed++;
		else
			testOrSuite->passed++;
		FILE* out = t.failed ? stderr : stdout;
		indent(out, indentLevel - isSuite);
		fprintf(out, "%s " GPC_ORANGE("\"%s\"") " %s\n",
				isSuite ? "Suite" : "Test", t.name,
				t.failed ? GPC_RED("[FAILED]") : GPC_GREEN("[PASSED]"));
		if (isSuite)
			puts("");
		
		*current = stackPeek(testOrSuite->stack)->name;
		
		return false;
	}
}

bool gpc_test(const char* name)
{	
	static const char* current = NULL;
	gpc_initStartAndExitMessages();
	
	return testOrSuite(name, &current, &gTests);
}

bool gpc_testSuite(const char* name)
{
	static const char* current = NULL;
	gpc_initStartAndExitMessages();
	
	return testOrSuite(name, &current, &gSuites);
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

static void arrElemStrfy(char* outBuf, enum gpc_Type T, const void* _arr)
{
	const gpc_Byte* arr = _arr;
	
	#define BUF_SIZE 25
	char buf[BUF_SIZE] = "";
	
	if (arr != NULL)
	{
		strcpy(outBuf, "{ ");
		//for (size_t i = 0; i < gpc_size(arr); i += gpc_sizeof(T))
		for (const gpc_Byte* item = arr; item != arr + gpc_size(arr); item += gpc_sizeof(T))
		{
			switch (T)
			{
				case GPC_UNSIGNED_CHAR:      sprintf(buf,"%c",  *(unsigned char*)item); break;
				case GPC_UNSIGNED_SHORT:     sprintf(buf,"%hu", *(unsigned short*)item); break;
				case GPC_UNSIGNED:           sprintf(buf,"%u",  *(unsigned*)item); break;
				case GPC_UNSIGNED_LONG:      sprintf(buf,"%lu", *(unsigned long*)item); break;
				case GPC_UNSIGNED_LONG_LONG: sprintf(buf,"%llu",*(unsigned long long*)item);break;
				case GPC_BOOL:               sprintf(buf,"%s",  *(int*)item?"true":"false");break;
				case GPC_CHAR:               sprintf(buf,"%c",  *(char*)item); break;
				case GPC_SHORT:              sprintf(buf,"%hi", *(short*)item); break;
				case GPC_INT:                sprintf(buf,"%i",  *(int*)item); break;
				case GPC_LONG:               sprintf(buf,"%li", *(long*)item); break;
				case GPC_LONG_LONG:          sprintf(buf,"%lli",*(long long*)item); break;
				case GPC_FLOAT:              sprintf(buf,"%g",  *(float*)item); break;
				case GPC_DOUBLE:             sprintf(buf,"%g",  *(double*)item); break;
				case GPC_PTR:                sprintf(buf,"%p",  *(void**)item); break;
				
				// TEMP MAY OVERFLOW
				case GPC_CHAR_PTR:           sprintf(buf,"%s",  *(char**)item); break;
			}
			strcat(outBuf, buf);
			strcat(outBuf, item != arr + gpc_size(arr) - gpc_sizeof(T) ? ", " : "");
			memset(buf, 0, BUF_SIZE);
		}
		// {
			// switch (T)
			// {
				// case GPC_UNSIGNED_CHAR:      sprintf(buf,"%c",  *(unsigned char*)arr); break;
				// case GPC_UNSIGNED_SHORT:     sprintf(buf,"%hu", *(unsigned short*)arr); break;
				// case GPC_UNSIGNED:           sprintf(buf,"%u",  *(unsigned*)arr); break;
				// case GPC_UNSIGNED_LONG:      sprintf(buf,"%lu", *(unsigned long*)arr); break;
				// case GPC_UNSIGNED_LONG_LONG: sprintf(buf,"%llu",*(unsigned long long*)arr);break;
				// case GPC_BOOL:               sprintf(buf,"%s",  *(int*)arr?"true":"false");break;
				// case GPC_CHAR:               sprintf(buf,"%c",  *(char*)arr); break;
				// case GPC_SHORT:              sprintf(buf,"%hi", *(short*)arr); break;
				// case GPC_INT:                sprintf(buf,"%i",  *(int*)arr); break;
				// case GPC_LONG:               sprintf(buf,"%li", *(long*)arr); break;
				// case GPC_LONG_LONG:          sprintf(buf,"%lli",*(long long*)arr); break;
				// case GPC_FLOAT:              sprintf(buf,"%g",  *(float*)arr); break;
				// case GPC_DOUBLE:             sprintf(buf,"%g",  *(double*)arr); break;
				// case GPC_PTR:                sprintf(buf,"%p",  *(void**)arr); break;
				
				// //TEMP MAY OVERFLOW
				// case GPC_CHAR_PTR:           sprintf(buf,"%s",  *(char**)arr); break;
			// }
			// strcat(outBuf, buf);
			// strcat(outBuf, i < gpc_size(arr) - gpc_sizeof(T) ? ", " : "");
			// memset(buf, 0, BUF_SIZE);
		// }
		strcat(outBuf, " }");
	}
	else
	{
		strcpy(outBuf, "(null)");
	}
}

int gpc_assertArrcmp(enum gpc_Type lhst, const void* lhs, enum gpc_Type rhst, const void* rhs)
{
	size_t lhslen = lhs ? sizeof("{,}")*gpc_size(lhs)/gpc_sizeof(lhst) : sizeof("(null)");
	size_t rhslen = rhs ? sizeof("{,}")*gpc_size(rhs)/gpc_sizeof(rhst) : sizeof("(null)");
	gpc_CmpArgs* argBufs = gpc_getCmpArgs(lhslen >= rhslen ? lhslen : rhslen);
	
	arrElemStrfy(argBufs->a, lhst, lhs);
	arrElemStrfy(argBufs->b, rhst, rhs);
	
	// const char* strp1 = str1 ? str1 : "";
	// const char* strp2 = str2 ? str2 : "";
	// return strcmp(strp1, strp2);
	size_t lhssize = lhs ? gpc_size(lhs) : 0;
	size_t rhssize = rhs ? gpc_size(rhs) : 0;
	if (lhssize == rhssize)
		return memcmp(lhs, rhs, lhssize);
	else if (lhssize > rhssize) // TODO test this!
		return 1;
	else
		return -1;
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
	{
		gPassedAsserts++;
		goto finish;
	}

	func = gTests.stack->length  > 0 ? stackPeek(gTests.stack)->name  :
		   gSuites.stack->length > 0 ? stackPeek(gSuites.stack)->name : func;
	
	const char* a_eval = gCmpArgs.a;
	const char* b_eval = gCmpArgs.b;
	if ( ! *b)
	{
		a_eval = "false";
		b_eval = "";
	}
	
	size_t indentLevel = gSuites.stack->length + gTests.stack->length;
	
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
	
	gFailedAsserts++;
	stackPeek(gTests.stack)->failed = true;
	stackPeek(gSuites.stack)->failed = true;
	nullStack = (struct Stack){0}; // in case of accidental modification
	
	finish:
	if (gCmpArgs.a != NULL)
	{
		free(gCmpArgs.a);
		free(gCmpArgs.b);
		gCmpArgs.a = NULL;
		gCmpArgs.b = NULL;
	}
	return expr;
}

bool gpc_exitTests(bool b)
{
	if ( ! b)
		return true;
	while (gTests.stack->length > 0)
		gpc_test(NULL);
	while (gSuites.stack->length > 0)
		gpc_testSuite(NULL);
	// TODO kill all owners
	exit(EXIT_FAILURE);
	return true; // prevent compiler complaints
}
