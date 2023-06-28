/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

// Only test allocations with size divisible by 4!

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../include/gpc/assert.h"

#include "fakeheap.c"
#include "../src/memory.c"

int main()
{
	fakeHeapInit();
	
	// fakeHeapSetAutoLog(true);
	
	NEW_OWNER(thisScope);
	
	void* obj0 = mallocAssign(4, thisScope);
	
	//freeAll(thisScope);
	TEST(automatic_freeing)
		ASSERT(fakeHeapFindFirstReserved() EQ EMPTY_HEAP, "Heap not empty after scope!");
	
	fakeHeapDestroy();
}
