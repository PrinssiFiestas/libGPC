/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdio.h>

#include "../include/assert.h"
#include "../src/memory.c"

#undef malloc
#undef free
#undef calloc
#undef realloc

// fake heap
static void** g_testHeap;
static const size_t TEST_HEAP_SIZE = 100000;
#define EMPTY_HEAP -1
#define FREED (void*)0
#define RESERVED (void*)1

// Returns the memory location index of first object and -1 if heap is empty
long testHeapFirstObject()
{
	for (size_t i = 0; i < TEST_HEAP_SIZE; i++)
		if (g_testHeap[i] != FREED)
			return i;
	return -1;
}

size_t objSize(void** p)
{
	printf("%p\n", p);
	size_t size = 0;
	while(p[size] != FREED)
	{
		printf("%p\n", p[size]);
		size++;
	}
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
			void** obj1 = scopedAlloc(3);
			void** obj2 = scopedAlloc(5);
			void** obj3 = scopedAlloc(4);
			ASSERT(objSize(obj1) EQ 3);
			ASSERT(objSize(obj2) EQ 5);
			(void)obj3;
		end
		ASSERT(testHeapFirstObject() EQ EMPTY_HEAP, "Heap not empty after scope!");
	}
	
	// teardown
	free(g_testHeap);
}

// --------------------------------------------------------------------------

// Populates testHeap with dummy data and moves freeSpace pointer
void* test_malloc(size_t size)
{
	// initialize freeSpace
	static void** freeSpace = (void**)0;
	if (freeSpace == (void**)0)
		freeSpace = g_testHeap;
	
	printf("MALLOC %p\n", freeSpace);
	for (size_t i = 0; i < size; i++)
	{
		freeSpace[i] = RESERVED;
		printf("%p\n", freeSpace[i]);
	}
	printf("%p\n\n", freeSpace[size]);
	freeSpace += 100*sizeof(void*);
	return freeSpace - 100*sizeof(void*);
}

void test_free(void* ptr)
{
	printf("FREE!\n");
	ptr = (void**)ptr;
	(void)ptr;
}

void* test_calloc(size_t nmemb, size_t size)
{
	(void)nmemb; (void)size;
	return NULL;
}

void* test_realloc(void* ptr, size_t size)
{
	(void)ptr; (void)size;
	return NULL;
}