/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_FAKEHEAP_H
#define GPC_FAKEHEAP_H

#include <stdio.h>

extern const uint8_t  FREED;
extern const uint32_t FREED4;
extern const uint8_t  RESERVED;
extern const size_t ALLOC_OFFSET;

void fakeHeapInit(void);
void fakeHeapDestroy(void);

#define EMPTY_HEAP -1
long   fakeHeapFindFirstReserved(void);
size_t fakeHeapObjectSize(void* object);

// If called, every malloc, calloc, realloc, and free calls print out to destination
void fakeHeapSetAutoLog(bool);
void fakeHeapSetLogOut(FILE* destination);

// Prints fake heap contents
// Array printed left is heap as hex bytes
// Array printed right is same data but as characters
void  fakeHeapPrint(void);
void  fakeHeapPrintStderr(void);
void  fakeHeapPrintToFile(FILE*);
char* fakeHeapContents(void);

// Prints full heap history with all allocations
void  fakeHeapPrintHistory(void);
void  fakeHeapPrintHistoryStderr(void);
void  fakeHeapPrintHistoryToFile(FILE*);
char* fakeHeapHistoryContents(void);

void* fakeHeapMalloc(size_t size);
void  fakeHeapFree(void* p);
void* fakeHeapCalloc(size_t nmemb, size_t size);
void* fakeHeapRealloc(void* p, size_t size);

struct FakeHeapCallData
{
	const char* file;
	const int line;
	const char* func;
	const char* callArgs;
};
void* fakeHeapLogMalloc(size_t size, struct FakeHeapCallData);
void  fakeHeapLogFree(void* p, struct FakeHeapCallData);
void* fakeHeapLogCalloc(size_t nmemb, size_t size, struct FakeHeapCallData);
void* fakeHeapLogRealloc(void* p, size_t size, struct FakeHeapCallData);

// TODO remove these after testing. Much better to just use the ones below. 
#ifdef GPC_FAKEHEAP_REPLACE_ALLOCATOR
#define malloc(size)		fakeHeapMalloc(size)
#define free(p)				fakeHeapFree(p)
#define calloc(size, t)		fakeHeapCalloc(size, t)
#define realloc(p, size)	fakeHeapRealloc(p, size)
#endif

#ifdef GPC_FAKEHEAP_LOG_REPLACE_ALLOCATOR
#define malloc(size)		fakeHeapLogMalloc(size, (struct FakeHeapCallData)		\
	{__FILE__, __LINE__, __func__, #size })
#define free(p)				fakeHeapLogFree(p, (struct FakeHeapCallData)			\
	{__FILE__, __LINE__, __func__, #p })
#define calloc(nmemb, size)	fakeHeapLogCalloc(nmemb, size, (struct FakeHeapCallData)\
	{__FILE__, __LINE__, __func__, #nmemb ", " #size })
#define realloc(p, size)	fakeHeapLogRealloc(p, size, (struct FakeHeapCallData)	\
	{__FILE__, __LINE__, __func__, #p ", " #size }))
#endif

#endif // GPC_FAKEHEAP_H