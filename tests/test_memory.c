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
	fakeHeapSetAutoLog(true);
	
	NEW_OWNER(thisScope);
	
	const size_t obj0Cap = 4;
	char* obj0 = mallocAssign(obj0Cap, thisScope);
	obj0[0] = 'X';
	obj0[1] = '\0';
	int* obj1 = callocAssign(3, sizeof(obj1[0]), thisScope);
	
	TEST(reallocate)
	{
		uint32_t* obj0Original = (uint32_t*)obj0;
		obj0 = reallocate(obj0, obj0Cap);
		ASSERT(obj0 EQ "X");
		ASSERT(*obj0Original EQ FREED4);
		ASSERT(getCapacity(obj0) EQ obj0Cap);
	}
	
	freeAll(thisScope);
	TEST(automatic_freeing)
		ASSERT(fakeHeapFindFirstReserved() EQ EMPTY_HEAP, "Heap not empty after scope!");
	
	fakeHeapDestroy();
}
