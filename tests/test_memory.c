/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

// Do not test single byte allocations! 0x00 is reserved for NULL, 0x01 for 
// RESERVED, and 0xFF for FREED. This means that testing for single bytes will
// fail at least 0x100/3 times randomly! Always allocate at least 4 bytes!

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
static const size_t TEST_HEAP_SIZE = 0x100;

#define EMPTY_HEAP -1
#define FREED 0xFF
#define RESERVED 0x01

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
	uint64_t* ptr = (uint64_t*)p;
	size_t size = 0;
	while ((void*)ptr[size] != NULL)
		size++;
	return size;
}

void printHeap(void);

int main()
{
	// setup
	g_testHeap = malloc(TEST_HEAP_SIZE * sizeof(void*));
	for (size_t i = 0; i < TEST_HEAP_SIZE; i++)
		g_testHeap[i] = FREED;
	// pointless calloc() test because paranoia
	ASSERT(testHeapFirstObject() EQ EMPTY_HEAP, "calloc() failed!");
	
	TEST(allocations)
	{
		begin
			void* obj1 = scopedAlloc(3 * sizeof(void*));
			void* obj2 = scopedAlloc(5 * sizeof(void*));
			void* obj3 = scopedAlloc(4 * sizeof(void*));
			printHeap();
			ASSERT(objSize(obj1) EQ 3 * sizeof(void*));
			ASSERT(objSize(obj2) EQ 5 * sizeof(void*));
			ASSERT(objSize(obj3) EQ 4 * sizeof(void*));
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
		freeSpace = g_testHeap;
	
	printf("MALLOC %p\n", freeSpace);
	for (size_t i = 0; i < size; i++)
	{
		freeSpace[i] = RESERVED;
		printf("%2X %s", freeSpace[i], (i+1)%8 ? "" : "\n" );
	}
	printf("%2X\n\n", freeSpace[size]);
	freeSpace += 64;
	return freeSpace - 64;
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

void printHeap()
{
	puts("FULL HEAP");
	for (size_t i = 0; i < TEST_HEAP_SIZE; i++)
		printf("%2X %s", g_testHeap[i], (i+1)%8 ? "" : "\n");
	puts("\n");
}