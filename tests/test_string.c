// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GPC_NAMESPACE
#include "../include/gpc/assert.h"
#include "../src/string.c"

int main(void)
{
	Owner* thisScope = newOwner();
	
	while (testSuite("String constructor"))
	{
		String str = newStringS("string");
		
		EXPECT_STR(str,==,"string");
		
		EXPECT(size(str),==,strlen("string"));
		EXPECT(capacity(str),==,strlen("string") + 1,
			"String should always have capacity for null-terminator.");
		
		//#define GPC_TEST_WARNINGS
		#ifdef GPC_TEST_WARNINGS
		char* charPointer = "Initializer must be a string literal!";
		String str2 = newStringS(charPointer);
		(void)str2;
		#endif
	}
	
	
	
	freeOwner(thisScope, NULL);
}
