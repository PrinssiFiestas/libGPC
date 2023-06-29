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
	// Uncomment for heap visualization in stdout
	// fakeHeapSetAutoLog(true);
	
	NEW_OWNER(thisScope);
	
	TEST(nextPowerOf2)
	{
		ASSERT(nextPowerOf2(3) EQ 4);
		ASSERT(nextPowerOf2(4) EQ 4, "Prevent needless allocations!");
		ASSERT(nextPowerOf2(28) EQ 32);
	}
	
	const size_t obj0Cap = 4;
	char* obj0 = mallocAssign(obj0Cap, thisScope);
	obj0[0] = 'X';
	obj0[1] = '\0';
	obj0 = setSize(obj0, 2);
	int* obj1 = callocAssign(3, sizeof(obj1[0]), thisScope);
	
	TEST(callocAssign)
		ASSERT(obj1[2] EQ 0);
	
	// To help debugging
	obj1[1] = 0xEFBE; // BEEF
	
	TEST(reallocate)
	{
		uint32_t* obj0Original = (uint32_t*)obj0;
		obj0 = reallocate(obj0, obj0Cap * 2);
		ASSERT(obj0 EQ "X");
		ASSERT(*obj0Original EQ FREED4);
		ASSERT(getCapacity(obj0) EQ obj0Cap * 2);
	}
	
	TEST(setCapacity_and_setSize)
	{
		uint32_t* obj1Original = (uint32_t*)obj1;
		size_t oldCapacity = getCapacity(obj1);
		obj1 = setCapacity(obj1, oldCapacity + 1);
		ASSERT(*obj1Original EQ FREED4);
		ASSERT(getCapacity(obj1) EQ nextPowerOf2(oldCapacity + 1));
		
		typeof(obj1) obj1NonMoved = obj1;
		ASSERT(obj1 = setSize(obj1, getCapacity(obj1)) EQ obj1NonMoved);
	}
	
	TEST(duplicate)
	{
		char* copy = duplicate(obj0);
		obj0[0] = 'Y';
		ASSERT(obj0 EQ "Y");
		ASSERT(copy EQ "X");
	}
	
	freeAll(thisScope);
	TEST(automatic_freeing)
		ASSERT(fakeHeapFindFirstReserved() EQ EMPTY_HEAP, "Heap not empty after killing owner!");
	
	fakeHeapDestroy();
}
