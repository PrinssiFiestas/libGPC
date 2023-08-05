// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define GPC_NAMESPACE
#include "../include/gpc/assert.h"
#include "../src/error.c"

char gstr[200];
void debugMessageCallback(const char* str)
{
	strcpy(gstr, str);
}
 
int main(void)
{
	while (test("handleError"))
	{
		#define malloc(...) NULL // failing malloc
		void* p = malloc(1);
		#undef malloc
		ASSERT((int)gpc_handleError(p == NULL, NULL),==, GPC_ERROR_NO_HANDLING);
		gpc_setErrorHandlingMode(GPC_ERROR_DEBUG);
		ASSERT((int)gpc_handleError(p == NULL, "Error message!"),==, GPC_ERROR_SHOULD_HANDLE);
		gpc_setDebugMessageCallback(debugMessageCallback);
		const char* msg = "To callback";
		gpc_handleError(p == NULL, msg);
		ASSERT_STR(gstr,==,msg);
	}
	
	// #define TEST_ERROR
	#ifdef TEST_ERROR
	FILE* f = fopen("nonexistent.file", "r");
	gpc_setErrorHandlingMode(GPC_ERROR_STRICT);
	gpc_handleError(EXIT_FAILURE, "Error! Aborting");
	puts("this never gets printed");
	fclose(f);
	#endif
}
