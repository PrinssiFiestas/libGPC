/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../include/gpc/assert.h"
#include "../src/gpc.c"

char gstr[200];
void debugMessageCallback(const char* str)
{
	strcpy(gstr, str);
}
 
int main()
{
	TEST(handleError)
	{
		#define malloc(...) NULL // failing malloc
		void* p = malloc(1);
		#undef malloc
		ASSERT(gpc_handleError(p == NULL, NULL) EQ GPC_ERROR_NO_HANDLING);
		gpc_setErrorHandlingMode(GPC_ERROR_DEBUG);
		ASSERT(gpc_handleError(p == NULL, "Error message!") EQ GPC_ERROR_SHOULD_HANDLE);
		gpc_setDebugMessageCallback(debugMessageCallback);
		const char* msg = "To callback";
		gpc_handleError(p == NULL, msg);
		ASSERT(gstr EQ msg);
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