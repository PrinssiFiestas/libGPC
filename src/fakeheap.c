/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdlib.h>
#include <stdint.h>
#include "../include/gpc/fakeheap.h"

// fake heap
static uint8_t* fakeHeap = NULL;
// The test allocator does not look for free block but rather it just moves the
// 'freeSpace' pointer this amount. Make sure that the test allocations fit this
// space!
const size_t ALLOC_OFFSET = 64;

static size_t fakeHeapSize = 0;

const uint8_t  FREED    = 0xFF;
const uint32_t FREED4   = 0xFFFFFFFF;
const uint8_t  RESERVED = 0x01;

void fakeHeapInit()
{
	const size_t TEST_HEAP_CAPACITY = 0x100000;
	fakeHeap = malloc(TEST_HEAP_CAPACITY);
	for (size_t i = 0; i < TEST_HEAP_CAPACITY; i++)
		fakeHeap[i] = FREED;
}

void fakeHeapDestroy()
{
	fakeHeapSize = 0;
	free(fakeHeap);
}

size_t fakeHeapObjectSize(void* p)
{	
	uint32_t* ptr = p;
	size_t size = 0;
	while (ptr[size] != FREED4)
		size++;
	return size * sizeof(ptr[0]);
}

long fakeHeapFindFirstReserved()
{
	uint32_t* heap32 = (uint32_t*)fakeHeap;
	for (size_t i = 0; i < fakeHeapSize/4; i++)
		if (heap32[i] != FREED4)
			return i;
	return EMPTY_HEAP;
}

// void printHeap()
// {
	// puts("");
	// for (size_t i = 0; i < g_testHeapSize; i++)
		// printf("%2X %s", g_testHeap[i], (i+1)%8 ? "" : "\n");
	// puts("\n");
	// fflush(stdout);
// }

// --------------------------------------------------------------------------
//		Fake allocators replacing malloc() etc.

// Populates testHeap with RESERVED and moves freeSpace pointer
void* fakeHeapMalloc(size_t size)
{
	fakeHeapSize += ALLOC_OFFSET;
	
	// initialize freeSpace
	static uint8_t* freeSpace = NULL;
	if (freeSpace == NULL)
		freeSpace = fakeHeap;
	
	for (size_t i = 0; i < size; i++)
		freeSpace[i] = RESERVED;
	freeSpace += ALLOC_OFFSET;
	return freeSpace - ALLOC_OFFSET;
}

void  fakeHeapFree(void* p)
{
	uint8_t* ptr = p;
	for (size_t i = 0; i < ALLOC_OFFSET; i++)
		ptr[i] = FREED;
}

void* fakeHeapCalloc(size_t nmemb, size_t size)
{
	return fakeHeapMalloc(nmemb * size);
}

void* fakeHeapRealloc(void* p, size_t size)
{
	uint8_t* ptr = p;
	uint8_t* destination = fakeHeapMalloc(size);
	for (size_t i = 0; i < size; i++)
		destination[i] = ptr[i];
	fakeHeapFree(p);
	return destination;
}
