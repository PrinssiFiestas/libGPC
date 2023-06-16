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
	
	//#define OUTFILE
	#ifdef OUTFILE
	FILE* f = fopen("test_memory_log.txt", "w");
	fakeHeapSetLogOut(f);
	#endif
	//atexitWhenFailed(printHeapHistory);
	
	TEST(fakeHeapCalloc)
	{
		// uint8_t* p = fakeHeapCalloc(12, 1);
		// ASSERT(p[1] EQ 0);
		// ASSERT(p[11] EQ 0);
	}
	
	#ifdef OUTFILE
	fclose(f);
	#endif
	
	fakeHeapDestroy();
}