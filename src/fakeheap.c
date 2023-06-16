/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../include/gpc/fakeheap.h"
#include "terminalcolors.h"

// fake heap
static uint8_t* fakeHeap = NULL;
// The test allocator does not look for free block but rather it just moves the
// 'freeSpace' pointer this amount. Make sure that the test allocations fit this
// space!
const size_t ALLOC_OFFSET = 64;

static size_t fakeHeapSize = 0;

const uint8_t   FREED     = 0xFF;
const uint32_t  FREED4    = 0xFFFFFFFF;
const uint8_t   RESERVED  = 0x01;
const uint32_t  RESERVED4 = 0x01010101;

static FILE* logOut = NULL;
static bool autoLogEnabled = false;

void fakeHeapInit()
{
	logOut = stdout;
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

static char lastHeapOperation[200];
static char currentHeap[5000];
static char currentHeapColored[50000];
static char heapHistory[10000000];
static char heapHistoryColored[50000000];

enum FormatMode
{
	FORMAT_TO_HEX,
	FORMAT_TO_CHAR
};
void appendFormattedBytes(char* out, char* outColored, uint32_t bytes, enum FormatMode mode)
{
	if (bytes == FREED4)
	{
		strcat(out, "\\F \\F \\F \\F ");
		strcat(outColored, GPC_CYAN("\\F \\F \\F \\F "));
	}
	else if (bytes == RESERVED4)
	{
		strcat(out, "\\R \\R \\R \\R ");
		strcat(outColored, GPC_RED("\\R \\R \\R \\R "));
	}
	else
	{
		uint8_t* bytes8 = (uint8_t*)&bytes;
		char buf[64] = "";
		for (int i = 0; i < 4; i++)
		{
			if (mode == FORMAT_TO_HEX)
			{
				sprintf(buf, "%2X ", bytes8[i]);
			}
			else
			{
				const unsigned char c = bytes8[i];
				const char* ASCII[] = 
				{
					"\\0", "??" , "??" , "??" ,
					"??" , "??" , "??" , "\\a",
					"\\b", "\\t", "\\n", "\\v",
					"\\f", "\\r", "??"
				};
				if (c < sizeof(ASCII)/sizeof(ASCII[0]))
					sprintf(buf, "%s ", ASCII[c]);
				else if (c < 32 || c == 127)
					sprintf(buf, "?? ");
				else
					sprintf(buf, " %c ", c);
			}
			strcat(out, buf);
			strcat(outColored, buf);
		}
	}
}

void updateCurrentHeap()
{
	for (size_t i = 0; i < 5000; i++)
		currentHeap[i] = '\0';
	for (size_t i = 0; i < 50000; i++)
		currentHeapColored[i] = '\0';
	
	uint32_t* fakeHeap32 = (uint32_t*)fakeHeap;
	for (size_t i = 0; i < fakeHeapSize/4; i++)
	{
		appendFormattedBytes(currentHeap, currentHeapColored, fakeHeap32[i], FORMAT_TO_HEX);
		
		if (i%2)
		{
			strcat(currentHeap, "\t\t");
			strcat(currentHeapColored, "\t\t");
			
			appendFormattedBytes(currentHeap, currentHeapColored, fakeHeap32[i-1], FORMAT_TO_CHAR);
			appendFormattedBytes(currentHeap, currentHeapColored, fakeHeap32[i], FORMAT_TO_CHAR);
			
			strcat(currentHeap, "\n");
			strcat(currentHeapColored, "\n");
		}
	}
	
	strcat(currentHeap, "\n\n");
	strcat(currentHeapColored, "\n\n");
	
	strcat(heapHistory, currentHeap);
	strcat(heapHistoryColored, currentHeapColored);
}

void fakeHeapSetLogOut(FILE* destination)
{
	logOut = destination;
}

void fakeHeapSetAutoLog(bool b)
{
	autoLogEnabled = b;
}

void autoLog()
{
	if (logOut == NULL)
		logOut = stdout;
	
	if (autoLogEnabled)
	{
		fprintf(logOut, "%s\n", lastHeapOperation);
		if (logOut == stdout || logOut == stderr)
			fprintf(logOut, "%s", currentHeapColored);
		else
			fprintf(logOut, "%s", currentHeap);
	}
}

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
	uint8_t* out = freeSpace - ALLOC_OFFSET;
	
	snprintf(lastHeapOperation, 200, "malloc(%lli) -> %p", size, out);
	updateCurrentHeap();
	autoLog();
	return out;
}

void  fakeHeapFree(void* p)
{
	uint8_t* ptr = p;
	for (size_t i = 0; i < ALLOC_OFFSET; i++)
		ptr[i] = FREED;
	snprintf(lastHeapOperation, 200, "free(%p)", p);
	updateCurrentHeap();
	autoLog();
}

void* fakeHeapCalloc(size_t nmemb, size_t size)
{
	uint8_t* out = fakeHeapMalloc(nmemb * size);
	for (size_t i = 0; i < nmemb * size; i++)
		out[i] = 0;
	snprintf(lastHeapOperation, 200, "calloc(%lli, %lli) -> %p", nmemb, size, out);
	updateCurrentHeap();
	autoLog();
	return out;
}

void* fakeHeapRealloc(void* p, size_t size)
{
	uint8_t* ptr = p;
	uint8_t* destination = fakeHeapMalloc(size);
	for (size_t i = 0; i < size; i++)
		destination[i] = ptr[i];
	fakeHeapFree(p);
	snprintf(lastHeapOperation, 200, "fakeHeapRealloc(%p, %lli)", p, size);
	updateCurrentHeap();
	autoLog();
	return destination;
}
