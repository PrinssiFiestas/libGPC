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
	
	//s->stack[s->length - 1] = (struct StackData){.name = str};
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
	//fprintf(out, "");

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
		
	//if (gTestRunning)
	bool shouldEnd = false;
	if (name == NULL)
		shouldEnd = true;
	if (name != NULL && current != NULL)
		shouldEnd = strcmp(name, current) == 0;
	
	if ( ! shouldEnd)
	{
		current = (char*)name;
		stackPush(&gTestStack, name);
		//gTestRunning = true;
		
		return true;
	}
	else // finishing test
	{
		struct StackData test = stackPop(&gTestStack);
		FILE* out = test.failed ? stderr : stdout;
		fprintf(out, "Test \"" GPC_ORANGE("%s") "\" %s\n\n",
				test.name,
				test.failed ? GPC_RED("[FAILED]") : GPC_GREEN("[PASSED]"));
		//gTestRunning = false; // CHANGEBGHRBGHF
		
		current = gTestStack.length > 0 ? stackPeek(&gTestStack)->name : NULL;
		
		return false;
	}
	
	//return gTestRunning;
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
	
	//return gSuiteRunning;
}

#define MAX_STRFIED_LENGTH 25 // = more than max digits in uint64_t

// CHANGE THE MAGIC VALUE
// maybe not used though?
char gpc_ga_buf[40] = "";
char gpc_gb_buf[40] = "";

// These will be used! May need to be removed from header. 
// char* gpc_ga_bufp = gpc_ga_buf;
// char* gpc_gb_bufp = gpc_gb_buf;

char* gpc_ga_bufp = NULL;
char* gpc_gb_bufp = NULL;


// this to overload.h? or make a new header for generics?
// union Any
// {
	// unsigned long long u;
	// long long i;
	// double f;
	// void* p;
// };

// union Any gpc_getVal(va_list* arg, enum gpc_Type T)
// {
	// union Any x;
	// if (gpc_isFloating(T))
		// return x.f = va_arg(*arg, double);
	// else if (gpc_isUnsigned(T) && gpc_sizeof(T) >= sizeof(long long))
		// return x.u = va_arg(*arg, unsigned long long);
	// else if (gpc_isInteger(T))
		// return x.i = va_arg(*arg, long long);
	// else
		// return x.p = va_arg(*arg, void*);
// }

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
	gpc_ga_bufp = realloc(gpc_ga_bufp, bufSize);
	gpc_gb_bufp = realloc(gpc_gb_bufp, bufSize);
	gCmpArgs = (gpc_CmpArgs){ gpc_ga_bufp, gpc_gb_bufp };
	return &gCmpArgs;
}

bool gpc_compare(const enum gpc_Type T_a,
				 const enum gpc_Type T_b,
				 const char* op,
				 // a,
				 // b
				 ...)
{
	va_list args;
	va_start(args, op);
	
	// union Any a = gpc_getVal(&args, T_a);
	// union Any b = gpc_getVal(&args, T_b);
	
	// #define cmp(at, bt)									\
		// switch (op[0] + op[1])							\
		// {												\
			// case '=' + '=' : B = a.at == b.bt; break;	\
			// case '!' + '=' : B = a.at != b.bt; break;	\
			// case '<' + '\0': B = a.at <  b.bt; break;	\
			// case '>' + '\0': B = a.at >  b.bt; break;	\
			// case '<' + '=' : B = a.at <= b.bt; break;	\
			// case '>' + '=' : B = a.at >= b.bt; break;	\
		// }
	
	// bool B = false;
	// if (isFloating(T_a) && isFloating(T_b))
		// cmp(f, f);
	// if (isUnsigned(T_a) && isFloating(T_b))
		// cmp(u, f);
	// if (isInteger(T_a) && isFloating(T_b))
		// cmp(i, f);
	// if (isFloating(T_a) && isUnsigned(T_b))
		// cmp(f, u);
	// if (isUnsigned(T_a) && isUnsigned(T_b))
		// cmp(u, u);
	// if (isInteger(T_a) && isUnsigned(T_b))
		// cmp(i, u);
	// if (isFloating(T_a) && isInteger(T_b))
		// cmp(f, i);
	// if (isUnsigned(T_a) && isInteger(T_b))
		// cmp(u, i);
	// if (isInteger(T_a) && isInteger(T_b))
		// cmp(i, i);
	
	// if (isPointer(T_a) && isPointer(T_b))
		// cmp(p, p);
	
	/*#define cmp(at, bt)															\
		switch (op[0] + op[1])													\
		{																		\
			case '=' + '=' : B = va_arg(args, at) == va_arg(args, bt); break;	\
			case '!' + '=' : B = va_arg(args, at) != va_arg(args, bt); break;	\
			case '<' + '\0': B = va_arg(args, at) <  va_arg(args, bt); break;	\
			case '>' + '\0': B = va_arg(args, at) >  va_arg(args, bt); break;	\
			case '<' + '=' : B = va_arg(args, at) <= va_arg(args, bt); break;	\
			case '>' + '=' : B = va_arg(args, at) >= va_arg(args, bt); break;	\
		}
	
	bool B = false;
	if (gpc_isFloating(T_a) && gpc_isFloating(T_b))
		cmp(float,					float);
	if (gpc_isUnsigned(T_a) && gpc_isFloating(T_b))
		cmp(unsigned long long,		float);
	if (gpc_isInteger(T_a) && gpc_isFloating(T_b))
		cmp(long long,				float);
	if (gpc_isFloating(T_a) && gpc_isUnsigned(T_b))
		cmp(float,					unsigned long long);
	if (gpc_isUnsigned(T_a) && gpc_isUnsigned(T_b))
		cmp(unsigned long long,		unsigned long long);
	if (gpc_isInteger(T_a) && gpc_isUnsigned(T_b))
		cmp(long long,				unsigned long long);
	if (gpc_isFloating(T_a) && gpc_isInteger(T_b))
		cmp(float,					long long);
	if (gpc_isUnsigned(T_a) && gpc_isInteger(T_b))
		cmp(unsigned long long,		long long);
	if (gpc_isInteger(T_a) && gpc_isInteger(T_b))
		cmp(long long,				long long);
	
	if (gpc_isPointer(T_a) && gpc_isPointer(T_b))
		cmp(void*, void*);
	
	#undef cmp*/

	va_end(args);
	//return B;
	return false;
}

long long gpc_strfyi(char* buf, ...)
{
	va_list arg;
	va_start(arg, buf);
	long long val = va_arg(arg, long long);
	va_end(arg);
	sprintf(buf, "%lli", val);
	return val;
}
unsigned long long gpc_strfyu(char* buf, ...)
{
	va_list arg;
	va_start(arg, buf);
	unsigned long long val = va_arg(arg, unsigned long long);
	va_end(arg);
	sprintf(buf, "%llu", val);
	return val;
}
double gpc_strfyf(char* buf, ...)
{
	va_list arg;
	va_start(arg, buf);
	double val = va_arg(arg, double);
	va_end(arg);
	sprintf(buf, "%g", val);
	return val;
}
char gpc_strfyc(char* buf, ...)
{
	va_list arg;
	va_start(arg, buf);
	char val = (char)va_arg(arg, int);
	va_end(arg);
	sprintf(buf, "\'%c\'", val);
	return val;
}
void* gpc_strfyp(char* buf, ...)
{
	va_list arg;
	va_start(arg, buf);
	void* val = va_arg(arg, void*);
	va_end(arg);
	sprintf(buf, "%p", val);
	return val;
}

// THIS GETS REPLCACED BY gpc_compare()
char* gpc_strfy(char** buf, const/*enum gpc_Type*/int T, ...)
{
	va_list arg;
	va_start(arg, T);
	
	char c;
	switch (T)
	{
		case GPC_BOOL:
			sprintf(*buf, "%s", va_arg(arg, int) ? "true" : "false");
			break;
		case GPC_SHORT:
			sprintf(*buf, "%i", va_arg(arg, int));
			break;
		case GPC_INT:
			sprintf(*buf, "%i", va_arg(arg, int));
			break;
		case GPC_LONG:
			sprintf(*buf, "%li", va_arg(arg, long));
			break;
		case GPC_LONG_LONG:
			sprintf(*buf, "%lli", va_arg(arg, long long));
			break;
		case GPC_UNSIGNED_SHORT:
			sprintf(*buf, "%u", va_arg(arg, unsigned));
			break;
		case GPC_UNSIGNED:
			sprintf(*buf, "%u", va_arg(arg, unsigned));
			break;
		case GPC_UNSIGNED_LONG:
			sprintf(*buf, "%lu", va_arg(arg, unsigned long));
			break;
		case GPC_UNSIGNED_LONG_LONG:
			sprintf(*buf, "%llu", va_arg(arg, unsigned long long));
			break;
		case GPC_FLOAT:
			sprintf(*buf, "%g", va_arg(arg, double));
			break;
		case GPC_DOUBLE:
			sprintf(*buf, "%g", va_arg(arg, double));
			break;
		case GPC_CHAR:
			c = (char)va_arg(arg, int);
			sprintf(*buf, "\'%c\'=%i", c, (int)c);
			break;
		case GPC_UNSIGNED_CHAR:
			c = (char)va_arg(arg, int);
			sprintf(*buf, "\'%c\'=%i", c, (int)c);
			break;
		case GPC_CHAR_PTR:
			*buf = va_arg(arg, char*);
			break;
		case GPC_PTR:
			sprintf(*buf, "%p", va_arg(arg, void*));
			break;
	}

	bool nullTerminated = T == GPC_CHAR_PTR;
	if ( ! nullTerminated)
		(*buf)[MAX_STRFIED_LENGTH - 1] = '\0';
	
	va_end(arg);
	return NULL;
}

static const char* getOp(const char* op)
{
	switch(op[0] + op[1])
	{
		// case 'E' + 'Q': return " == ";
		// case 'N' + 'E': return " != ";
		// case 'L' + 'T': return " < ";
		// case 'G' + 'T': return " > ";
		// case 'L' + 'E': return " <= ";
		// case 'G' + 'E': return " >= ";
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

/*bool gpc_expect(const bool expr,
				const char* op,
				const char* file,
				const int line,
				const char* func,
				const char* failMsg,
				const char* a,
				char* a_eval,
				const char* b,
				char* b_eval)
{
	if (expr == true)
		return true;

	//func = gTestRunning ? stackPeek(&gTestStack)->name :
	//	   gSuiteRunning ? stackPeek(&gSuiteStack)->name : func;
	func = gTestStack.length > 0 ? stackPeek(&gTestStack)->name :
		   gSuiteStack.length > 0 ? stackPeek(&gSuiteStack)->name : func;
	
	bool a_eval_allocated = (bool)a_eval;
	bool b_eval_allocated = (bool)b_eval;
	a_eval = a_eval ? a_eval : gpc_ga_bufp;
	b_eval = b_eval ? b_eval : gpc_gb_bufp;
	
	fprintf(stderr, "%s"GPC_ORANGE("%s%s%s")GPC_RED("%s")"%s%s%s"GPC_WHITE_BG("%s%i")"%s",
			"Assertion in ","\"",func,"\" ","[FAILED]"," in ",file," ","line ",line,"\n");
	
	const char* b_space = *b ? " " : "";
	fprintf(stderr, GPC_MAGENTA("%s%s%s%s%s")"%s%s"GPC_RED("%s%s%s")"%s%s%s",
			a, " ", op, b_space, b, b_space, "evaluated to ",
			a_eval, getOp(op), b_eval, ". ", failMsg, "\n\n");
	
	// -------------------------------------------------
	// TODO remove this block once String is implemented
	memset(gpc_ga_buf, 0, 40);
	memset(gpc_gb_buf, 0, 40);
	gpc_ga_bufp = gpc_ga_buf;
	gpc_gb_bufp = gpc_gb_buf;
	
	if (a_eval_allocated)
		free(a_eval);
	if (b_eval_allocated)
		free(b_eval);
	// -------------------------------------------------
	
	//if (gTestRunning)
	if (gTestStack.length > 0)
		stackPeek(&gTestStack)->failed = true;
		//gTestStack.stack[gTestStack.length].failed = true;
	//if (gSuiteRunning)
	if (gSuiteStack.length > 0)
		stackPeek(&gSuiteStack)->failed = true;
		//gSuiteStack.stack[gSuiteStack.length].failed = true;
	return false;
}*/

// bool gpc_expect(const bool expr,
				// const char* op,
				// const char* file,
				// const int line,
				// const char* func,
				// const char* failMsg,
				// const char* a,
				// char* a_eval,
				// const char* b,
				// char* b_eval)

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

	//func = gTestRunning ? stackPeek(&gTestStack)->name :
	//	   gSuiteRunning ? stackPeek(&gSuiteStack)->name : func;
	/*func = gTestStack.length > 0 ? stackPeek(&gTestStack)->name :
		   gSuiteStack.length > 0 ? stackPeek(&gSuiteStack)->name : func;*/
	
	/*bool a_eval_allocated = (bool)a_eval;
	bool b_eval_allocated = (bool)b_eval;
	a_eval = a_eval ? a_eval : gpc_ga_bufp;
	b_eval = b_eval ? b_eval : gpc_gb_bufp;*/
	
	const char* a_eval = gCmpArgs.a;
	const char* b_eval = gCmpArgs.b;
	if (gpc_ga_bufp != NULL)
	{
		free(gpc_ga_bufp);
		gpc_ga_bufp = NULL;
	}
	if (gpc_gb_bufp != NULL)
	{
		free(gpc_gb_bufp);
		gpc_gb_bufp = NULL;
	}
	
	fprintf(stderr, "%s"GPC_ORANGE("%s%s%s")GPC_RED("%s")"%s%s%s"GPC_WHITE_BG("%s%i")"%s",
			"Assertion in ","\"",func,"\" ","[FAILED]"," in ",file," ","line ",line,"\n");
	
	const char* b_space = *b ? " " : "";
	fprintf(stderr, GPC_MAGENTA("%s%s%s%s%s")"%s%s"GPC_RED("%s%s%s")"%s%s%s",
			a, " ", op, b_space, b, b_space, "evaluated to ",
			a_eval, getOp(op), b_eval, ". ", failMsg, "\n\n");
	
	/*
	// -------------------------------------------------
	// TODO remove this block once String is implemented
	memset(gpc_ga_buf, 0, 40);
	memset(gpc_gb_buf, 0, 40);
	gpc_ga_bufp = gpc_ga_buf;
	gpc_gb_bufp = gpc_gb_buf;
	
	if (a_eval_allocated)
		free(a_eval);
	if (b_eval_allocated)
		free(b_eval);
	// -------------------------------------------------
	
	//if (gTestRunning)
	if (gTestStack.length > 0)
		stackPeek(&gTestStack)->failed = true;
		//gTestStack.stack[gTestStack.length].failed = true;
	//if (gSuiteRunning)
	if (gSuiteStack.length > 0)
		stackPeek(&gSuiteStack)->failed = true;
		//gSuiteStack.stack[gSuiteStack.length].failed = true;*/
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
	return true;
}
