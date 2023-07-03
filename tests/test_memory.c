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

char msgBuf[500];
void getMsg(const char* msg);
bool doubleCheck(void* obj, DynamicObjOwner* owner);

int main()
{
	fakeHeapInit();
	// Uncomment for heap visualization in stdout
	// fakeHeapSetAutoLog(true);

	gpc_setErrorHandlingMode(GPC_ERROR_DEBUG);
	
	NEW_OWNER(thisScope);

	TEST(nextPowerOf2)
	{
		unsigned long n = 3;
		ASSERT(gpc_nextPowerOf2(n) EQ 4);
		ASSERT(gpc_nextPowerOf2(4) EQ 4, "Prevent needless allocations!");
		ASSERT(gpc_nextPowerOf2(28) EQ 32);
	}
	
	const size_t obj0Cap = 4;
	char* obj0 = mallocAssign(obj0Cap, thisScope);
	obj0[0] = 'X';
	obj0[1] = '\0';
	obj0 = setSize(obj0, 2);
	int* obj1 = callocAssign(3, sizeof(obj1[0]), thisScope);

	TEST_SUITE(memoryLocationCheck)
	{
		TEST(onHeap)
		{
			//ASSERT(doubleCheck(obj1) AND onHeap(obj1)); // TODO this syntax
			ASSERT(doubleCheck(obj1, thisScope));
			ASSERT(onHeap(obj1));
		}
		TEST(onStack)
		{
			uint8_t objSmem[sizeof(struct gpc_DynamicObjectList) + 1] = {0};
			struct gpc_DynamicObjectList* objSdata = (struct gpc_DynamicObjectList*)objSmem;
			objSdata->owner = thisScope;
			void* objS = objSdata + 1;
			
			ASSERT( ! doubleCheck(objS, thisScope));
			ASSERT(onStack(objS));
		}
		
	}

	TEST(newH)
	{
		ASSERT(!onStack(newH(int, thisScope)));
		ASSERT(onHeap(newH(int, thisScope)));
		ASSERT(getSize(newH(int8_t[15], thisScope)) EQ sizeof(int8_t[15]));
		ASSERT(getCapacity(newH(int8_t[15], thisScope)) EQ gpc_nextPowerOf2(sizeof(int8_t[15])));
	}

	TEST(newS)
	{
		ASSERT(onStack(newS(int, thisScope)));
		ASSERT(!onHeap(newS(int, thisScope)));
		ASSERT(getSize(newS(int8_t[15], thisScope)) EQ sizeof(int8_t[15]));
		ASSERT(getCapacity(newS(int8_t[15], thisScope)) EQ sizeof(int8_t[15]));
	}
	
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
		ASSERT(getCapacity(obj1) EQ gpc_nextPowerOf2(oldCapacity + 1));
		
		int* obj1NonMoved = obj1;
		ASSERT(obj1 = setSize(obj1, getCapacity(obj1)) EQ obj1NonMoved);
	}
	
	TEST(duplicate)
	{
		char* copy = duplicate(obj0);
		obj0[0] = 'Y';
		ASSERT(obj0 EQ "Y");
		ASSERT(copy EQ "X");
	}
	
	TEST(errorHandling)
	{
		gpc_setDebugMessageCallback(getMsg);
		
		(void)mallocAssign(-1, thisScope);
		ASSERT(msgBuf EQ GPC_EMSG_OVERALLOC(mallocAssign));
	}
	
	freeAll(thisScope);
	TEST(automatic_freeing)
		ASSERT(fakeHeapFindFirstReserved() EQ EMPTY_HEAP, "Heap not empty after killing owner!");
	
	fakeHeapDestroy();
}

void getMsg(const char* msg)
{
	strcpy(msgBuf, msg);
}

bool doubleCheck(void* obj, DynamicObjOwner* owner)
{
	bool onObjectList = false;
	for (struct gpc_DynamicObjectList* p = owner->firstObject; p != NULL; p = p->next)
		onObjectList = p + 1 == obj;
	return onObjectList;
}

#include "../src/gpc.c"