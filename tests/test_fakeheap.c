/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include "../include/gpc/assert.h"
#include "../src/fakeheap.c"

int main()
{
	fakeHeapInit();
	
	TEST(fakeHeapCalloc)
	{
		uint8_t* p = fakeHeapCalloc(12, 1);
		ASSERT(p[1] EQ 0);
		ASSERT(p[11] EQ 0);
	}
	
	fakeHeapDestroy();
}