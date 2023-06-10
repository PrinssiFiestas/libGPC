/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

// Do not test single byte allocations! 0x00 is reserved for NULL, 0x01 for 
// RESERVED, and 0xFF for FREED. This means that testing for single bytes will
// fail at least TEST_HEAP_SIZE/3 times randomly! Always allocate at least 4 bytes!

#include <stdio.h>
#include <stdint.h>

#include "../include/gpc/assert.h"
#include "../src/memory.c"

#undef malloc
#undef free
#undef calloc
#undef realloc

// fake heap
static uint8_t* g_testHeap = NULL;
//static const size_t TEST_HEAP_SIZE = 0x200;

// The test allocator does not look for free block but rather it just moves the
// 'freeSpace' pointer this amount. Make sure that the test allocations fit this
// space!
static const size_t ALLOC_OFFSET = 64;

static size_t g_testHeapSize = 0;

static const uint8_t  FREED    = 0xFF;
static const uint32_t FREED4   = 0xFFFFFFFF;
static const uint8_t  RESERVED = 0x01;

// Returns the memory location index of first object and -1 if heap is empty
#define EMPTY_HEAP -1
long testHeapFirstObject(void);
size_t objSize(void* p);
void printHeap(void);

unsigned char* func(DynamicObjOwner* callingScope)
begin
	unsigned char* returnValue = scopedAlloc(4 * sizeof(returnValue[0]));
	char* dummy = scopedAlloc(12 * sizeof(dummy[0]));
	printHeap();
	ret(returnValue);
end

int main()
{
	const size_t TEST_HEAP_CAPACITY = 0x100000;
	g_testHeap = malloc(TEST_HEAP_CAPACITY);
	for (size_t i = 0; i < TEST_HEAP_CAPACITY; i++)
		g_testHeap[i] = FREED;
	
	TEST_SUITE(scoped_memory_management)
	{
		begin
			double* obj1 = scopedAlloc(3 * sizeof(obj1[0]));
			int*    obj2 = scopedAlloc(5 * sizeof(obj2[0]));
			float*  obj3 = scopedAlloc(4 * sizeof(obj3[0]));
			printHeap();
			
			printf("\nOWNER\n%p, %p\n\n", &thisScope, getOwner(obj2));
			TEST(get_owner)
				ASSERT((size_t)getOwner(obj2) EQ (size_t)&thisScope);
			
			TEST(allocations)
			{
				ASSERT(objSize(obj1) EQ 3 * sizeof(obj1[0]));
				ASSERT(objSize(obj2) EQ 5 * sizeof(obj2[0]));
				ASSERT(objSize(obj3) EQ 4 * sizeof(obj3[0]));
			}
			
			TEST(moved_ownership)
			{
				unsigned char* returnValue = func(&thisScope);
				printHeap();
				TEST(function_cleaned_its_allocations)
					EXPECT(*(returnValue + ALLOC_OFFSET) EQ FREED);
				ASSERT((uintptr_t)getOwner(returnValue) EQ (uintptr_t)&thisScope);
			}
		end
		
		TEST(automatic_freeing)
			ASSERT(testHeapFirstObject() EQ EMPTY_HEAP, "Heap not empty after scope!");
		
		printHeap();
	}
	
	// teardown
	free(g_testHeap);
}

// --------------------------------------------------------------------------

long testHeapFirstObject()
{
	uint32_t* heap32 = (uint32_t*)g_testHeap;
	for (size_t i = 0; i < g_testHeapSize/4; i++)
		if (heap32[i] != FREED4)
			return i;
	return EMPTY_HEAP;
}

size_t objSize(void* p)
{	
	uint32_t* ptr = p;
	size_t size = 0;
	while (ptr[size] != FREED4)
		size++;
	return size * sizeof(ptr[0]);
}

void printHeap()
{
	puts("");
	for (size_t i = 0; i < g_testHeapSize; i++)
		printf("%2X %s", g_testHeap[i], (i+1)%8 ? "" : "\n");
	puts("\n");
	fflush(stdout);
}

// --------------------------------------------------------------------------
//		Fake allocators replacing malloc() etc. in memory.c

// Populates testHeap with RESERVED and moves freeSpace pointer
void* test_malloc(size_t size)
{
	g_testHeapSize += ALLOC_OFFSET;
	
	// initialize freeSpace
	static uint8_t* freeSpace = NULL;
	if (freeSpace == NULL)
		freeSpace = g_testHeap;
	
	for (size_t i = 0; i < size; i++)
		freeSpace[i] = RESERVED;
	freeSpace += ALLOC_OFFSET;
	return freeSpace - ALLOC_OFFSET;
}

void test_free(void* p)
{
	uint8_t* ptr = p;
	for (size_t i = 0; i < ALLOC_OFFSET; i++)
		ptr[i] = FREED;
}

void* test_calloc(size_t nmemb, size_t size)
{
	return test_malloc(nmemb * size);
}

void* test_realloc(void* p, size_t size)
{
	uint8_t* ptr = p;
	uint8_t* destination = test_malloc(size);
	for (size_t i = 0; i < size; i++)
		destination[i] = ptr[i];
	test_free(p);
	return destination;
}
