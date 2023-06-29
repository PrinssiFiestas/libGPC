/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */
 
#include "../include/gpc/assert.h"
#include "../src/gpc.c"
 
int main()
{
	TEST(handleError)
	{
		ASSERT(gpc_handleError(NULL) EQ GPC_ERROR_NO_HANDLING);
		gpc_setErrorHandlingMode(GPC_ERROR_DEBUG);
		ASSERT(gpc_handleError("No error") EQ GPC_ERROR_SHOULD_HANDLE);
	}
}