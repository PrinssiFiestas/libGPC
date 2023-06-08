/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdio.h>
#include <stdint.h>

#include "../include/assert.h"
#include "../src/memory.c"

#undef malloc
#undef free
#undef calloc
#undef realloc

// fake heap
static uint8_t* g_testHeap;
static const size_t TEST_HEAP_SIZE = 0xFE;

#define EMPTY_HEAP -1
#define FREED 0x00
#define RESERVED 0xFF

// Returns the memory location index of first object and -1 if heap is empty
long testHeapFirstObject()
{
	for (size_t i = 0; i < TEST_HEAP_SIZE; i++)
		if (g_testHeap[i] != FREED)
			return i;
	return -1;
}

size_t objSize(void* p)
{
	size_t size = 0;
	uint8_t* ptr = (uint8_t*)p;
	printf("OBJSIZE %p\n", p);
	while (ptr[size] != FREED)
	{
		printf("%2X %s", ptr[size], (size+1)%8 ? "" : "\n");
		size++;
	}
	puts("\n");
	return size;
}

int main()
{
	// setup
	g_testHeap = calloc(TEST_HEAP_SIZE, sizeof(void*));
	// pointless calloc() test because paranoia
	ASSERT(testHeapFirstObject() EQ EMPTY_HEAP, "calloc() failed!");
	
	TEST(allocations)
	{
		begin
			void* obj1 = scopedAlloc(3);
			void* obj2 = scopedAlloc(5);
			void* obj3 = scopedAlloc(4);
			ASSERT(objSize(obj1) EQ 3);
			ASSERT(objSize(obj2) EQ 5);
			ASSERT(objSize(obj3) EQ 4);
		end
		ASSERT(testHeapFirstObject() EQ EMPTY_HEAP, "Heap not empty after scope!");
	}
	
	// teardown
	free(g_testHeap);
}

// --------------------------------------------------------------------------
//		Fake allocators replacing malloc() etc. in memory.c

// Populates testHeap with RESERVED and moves freeSpace pointer
void* test_malloc(size_t size)
{
	// initialize freeSpace
	static uint8_t* freeSpace = NULL;
	if (freeSpace == NULL)
		freeSpace = g_testHeap + 8;
	
	printf("MALLOC %p\n", freeSpace);
	for (size_t i = 0; i < size; i++)
	{
		freeSpace[i] = RESERVED;
		printf("%2X %s", freeSpace[i], (i+1)%8 ? "" : "\n" );
	}
	printf("%2X\n\n", freeSpace[size]);
	freeSpace += 32;
	return freeSpace - 32;
}

void test_free(void* p)
{
	uint8_t* ptr = (uint8_t*)p;
	for (size_t i = 0; ptr[i] != FREED; i++)
		ptr[i] = FREED;
}

void* test_calloc(size_t nmemb, size_t size)
{
	return test_malloc(nmemb * size);
}

void* test_realloc(void* p, size_t size)
{
	uint8_t* ptr = (uint8_t*)p;
	uint8_t* destination = test_malloc(size);
	for (size_t i = 0; i < size; i++)
		destination[i] = ptr[i];
	test_free(p);
	return NULL;
}