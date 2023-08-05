// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Only test allocations with size divisible by 4!

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define GPC_NAMESPACE
#include "../include/gpc/assert.h"

#include "fakeheap.c" // replace allocator
#include "../src/memory.c"

char msgBuf[500];
void getMsg(const char* msg);
bool doubleCheck(void* obj, Owner* owner);
uint32_t* func(void);

int main(void)
{
	fakeHeapInit();
	// Uncomment for heap visualization in stdout
	//fakeHeapSetAutoLog(true);

	gpc_setErrorHandlingMode(GPC_ERROR_SEND_MESSAGE);
	
	gpc_Owner* thisScope = newOwner();

	while (test("nextPowerOf2"))
	{
		unsigned long n = 3;
		ASSERT(gpc_nextPowerOf2(n),==,(unsigned long)4);
		ASSERT(gpc_nextPowerOf2(4),==,(unsigned long)4, "Prevent needless allocations!");
		ASSERT(gpc_nextPowerOf2(28),==,(unsigned long)32);
		ASSERT(gpc_nextPowerOf2(SIZE_MAX/2 + 1),<=,SIZE_MAX);
	}
	
	const size_t obj0Cap = 4;
	char* obj0 = mallocAssign(obj0Cap, NULL);
	
	while (test("owner mallocate"))
		ASSERT(getOwner(obj0),==,thisScope);
	
	obj0 = resize(obj0, 2);
	obj0[0] = 'X';
	obj0[1] = '\0';
	
	while (test("owner"))
		ASSERT(getOwner(obj0),==,thisScope);
	
	int* obj1 = callocAssign(3, sizeof(obj1[0]), thisScope);

	while (testSuite("memory location check"))
	{
		while (test("onHeap"))
		{
			ASSERT(doubleCheck(obj1, thisScope));
			ASSERT(onHeap(obj1));
		}
		while (test("onStack"))
		{
			uint8_t objSmem[sizeof(struct gpc_ObjectList) + 1] = {0};
			struct gpc_ObjectList* objSdata = (struct gpc_ObjectList*)objSmem;
			objSdata->owner = thisScope;
			void* objS = objSdata + 1;
			
			ASSERT( ! doubleCheck(objS, thisScope));
			ASSERT(onStack(objS));
		}
		
	}

	while (test("newH and allocH"))
	{
		ASSERT(!onStack(newH(int, 0)));
		ASSERT(onHeap(allocH(sizeof(int))));
		ASSERT(size(newH(int8_t[15], 0)),==,(size_t)15);
		ASSERT(*(int*)newH(int, 3),==,3);
		ASSERT(capacity(allocH(sizeof(int8_t[15]))),==,gpc_nextPowerOf2(15));
	}

	while (test("newS and allocS"))
	{
		ASSERT(onStack(newS(int, 0)));
		ASSERT(!onHeap(allocS(sizeof(int))));
		ASSERT(size(allocS(sizeof(int8_t[15]))),==,(size_t)0);
		ASSERT(size(newS(int8_t[15], 0)),==,(size_t)15);
		ASSERT(*(int*)newS(int, 5),==,5);
		ASSERT(capacity(newS(int8_t[15], 0)),==,(size_t)15);
	}
	
	while (test("allocaAssign"))
	{
		ASSERT(size(allocaAssign(5, NULL)),==,(size_t)0);
		ASSERT(capacity(allocaAssign(5, thisScope)),==,(size_t)5);
	}
	
	while (test("callocAssign"))
		ASSERT(obj1[2],==,0);
	
	// To help debugging
	obj1[1] = 0xEFBE; // BEEF
	
	while (test("reallocate"))
	{
		uint32_t* obj0Original = (uint32_t*)obj0;
		obj0 = reallocate(obj0, obj0Cap * 2);
		ASSERT(obj0[0],==,'X', "Memory not copied!");
		ASSERT(*obj0Original,==,FREED4);
		ASSERT(capacity(obj0),==,obj0Cap * 2);
	}
	
	while (test("setSize and reallocate"))
	{
		uint32_t* obj1Original = (uint32_t*)obj1;
		size_t oldCapacity = capacity(obj1);
		obj1 = reallocate(obj1, oldCapacity + 1);
		ASSERT(*obj1Original,==,FREED4);
		ASSERT(capacity(obj1),==,gpc_nextPowerOf2(oldCapacity + 1));
		
		int* obj1NonMoved = obj1;
		ASSERT(obj1 = resize(obj1, capacity(obj1)),==,obj1NonMoved);
		
		// Test that data is copied properly
		size_t obj1OldCap = capacity(obj1);
		obj1 = resize(obj1, obj1OldCap + 1);
		ASSERT(size(obj1),==,obj1OldCap + 1);
	}
	
	while (test("duplicate"))
	{
		char* copy = duplicate(obj0);
		obj0[0] = 'Y';
		ASSERT(obj0[0],==,'Y');
		ASSERT(copy[0],==,'X');
	}
	
	// Test nested owners in func()
	uint32_t* dummy_u = func();
	(void)dummy_u;
	
	while (test("error handling"))
	{
		#ifdef __GNUC__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wsign-conversion"
		#endif
		gpc_setDebugMessageCallback(getMsg);
		
		void* dummy = mallocAssign(-1, thisScope);
		(void)dummy;
		ASSERT_STR(msgBuf,==,GPC_EMSG_OVERALLOC(mallocAssign));
		#ifdef __GNUC__
		#pragma GCC diagnostic pop
		#endif
	}
	
	// This test is only here because it might give useful information that can
	// be used with the arena allocator. It only works now with the replaced 
	// allocator, but is undefined with real malloc()!
	while (test("objects addresses should always grow in list"))
	{
		// Does heap grow up or down? Doesn't matter, let's just check that the
		// sign of p-p->next stays the same throughout the list.
		ptrdiff_t diff = thisScope->firstObject - thisScope->firstObject->next;
		#define signb(i) ((i)<0)
		ptrdiff_t sign = signb(diff);
		bool signChanged = false;
		for (struct gpc_ObjectList* p = thisScope->firstObject; p->next != NULL; p = p->next)
			if ((signChanged = signb((ptrdiff_t)(p - p->next)) != sign))
				break;
		EXPECT( ! signChanged);
	}
	
	freeOwner(NULL);
	while (test("automatic freeing"))
		ASSERT(fakeHeapFindFirstReserved(),==,EMPTY_HEAP, "Heap not empty after killing owner!");
	
	fakeHeapDestroy();
}

uint32_t* func(void)
{
	newOwner();
	uint32_t* u1 = newH(uint32_t, 1);
	uint32_t* u2 = newH(uint32_t, 2);
	uint32_t* u3 = newH(uint32_t, 3);
	moveOwnership(u2, gDefaultOwner->parent);
	freeOwner(NULL);
	
	while (test("nested_owners"))
	{
		ASSERT(*u1,==,FREED4);
		ASSERT(*u2,==,(uint32_t)2);
		ASSERT(*u3,==,FREED4);
	}
	
	return u2;
}

// ---------------------------------------------------------------------------
//		Helpers

void getMsg(const char* msg)
{
	size_t len = strlen(msg) < sizeof(msgBuf)-1 ? strlen(msg) : sizeof(msgBuf)-1;
	strncpy(msgBuf, msg, len);
	msgBuf[len] = '\0';
}

bool doubleCheck(void* obj, Owner* owner)
{
	bool onObjectList = false;
	for (struct gpc_ObjectList* p = owner->firstObject; p != NULL; p = p->next)
		onObjectList = p + 1 == obj;
	return onObjectList;
}
