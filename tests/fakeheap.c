// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 
// Include this file to replace malloc(), calloc(), realloc(), and free() with
// fake ones. See the macros at the bottom of this file.

#include <stdint.h>
#include <string.h>

#include "../src/terminalcolors.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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

struct FakeHeapCallData
{
    const char* file;
    const int line;
    const char* func;
    const char* callArgs;
};

// ---------------------------------------------------------------------------

// fake heap
static uint8_t* fakeHeap = NULL;
// The test allocator does not look for free block but rather it just moves the
// 'freeSpace' pointer in multiples of ALLOC_OFFSET
const size_t ALLOC_OFFSET = 64;

static size_t fakeHeapSize = 0;

const uint8_t   FREED     = 0xFF;
const uint32_t  FREED4    = 0xFFFFFFFF;
const uint8_t   RESERVED  = 0x01;
const uint32_t  RESERVED4 = 0x01010101;

static size_t fakeHeapCapacity = 0;

static char lastHeapOperation[500];

static char* currentHeap;
static char* currentHeapColored;
static size_t heapHistoryCapacity = 0;
static char* heapHistory;
static char* heapHistoryColored;

static FILE* logOut = NULL;
static bool autoLogEnabled = false;

void fakeHeapInit(void)
{
    logOut = stdout;
    const size_t FAKE_HEAP_INITIAL_CAPACITY = 0x100000;
    fakeHeapCapacity = FAKE_HEAP_INITIAL_CAPACITY;
    fakeHeap = malloc(FAKE_HEAP_INITIAL_CAPACITY);
    for (size_t i = 0; i < FAKE_HEAP_INITIAL_CAPACITY; i++)
        fakeHeap[i] = FREED;
    
    currentHeap         = malloc(0x1000);
    currentHeapColored     = malloc(0x1000);
    const size_t HEAP_HISTORY_INITIAL_CAPACITY = 0x10000;
    heapHistoryCapacity = HEAP_HISTORY_INITIAL_CAPACITY;
    heapHistoryColored     = malloc(HEAP_HISTORY_INITIAL_CAPACITY);
    heapHistory         = malloc(HEAP_HISTORY_INITIAL_CAPACITY);
}

void fakeHeapDestroy(void)
{
    fakeHeapSize = 0;
    free(fakeHeap);
    free(currentHeap);
    free(currentHeapColored);
    free(heapHistory);
    free(heapHistoryColored);
}

size_t fakeHeapObjectSize(void* p)
{    
    uint32_t* ptr = p;
    size_t size = 0;
    while (ptr[size] != FREED4)
        size++;
    return size * sizeof(ptr[0]);
}

long fakeHeapFindFirstReserved(void)
{
    uint32_t* heap32 = (uint32_t*)fakeHeap;
    for (size_t i = 0; i < fakeHeapSize/4; i++)
        if (heap32[i] != FREED4)
            return (long)i;
    return EMPTY_HEAP;
}

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

void updateCurrentHeap(void)
{
    free(currentHeap);
    free(currentHeapColored);
    const size_t POSSIBLE_BYTE_STR_SIZE = 0x10;
    const size_t SAFETY_MARGIN = 0x100;
    const size_t SIZE = fakeHeapSize * POSSIBLE_BYTE_STR_SIZE * sizeof(GPC_WHITE_BG("")) + SAFETY_MARGIN;
    currentHeap = calloc(SIZE, 1);
    currentHeapColored = calloc(SIZE, 1);
    if ( !currentHeap || !currentHeapColored)
    {
        perror("calloc failed for currentHeap!");
        exit(EXIT_FAILURE);
    }

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
    
    if (strlen(heapHistoryColored) + 1 + SIZE > heapHistoryCapacity)
    {
        heapHistoryCapacity *= 2;
        heapHistory        = realloc(heapHistory, heapHistoryCapacity);
        heapHistoryColored = realloc(heapHistoryColored, heapHistoryCapacity);
        for (size_t i = heapHistoryCapacity/2; i < heapHistoryCapacity; i++)
            heapHistory[i] = heapHistoryColored[i] = '\0';
    }
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

void autoLog(void)
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
//        Fake allocators replacing malloc() etc.

void updateLastHeapOperation(const char* operation, struct FakeHeapCallData data, void* arg1, void* arg2, void* returnValue)
{
    char args[255] = {0};
    char ret[64] = {0};
    if (operation[0] == 'm')
    {
        snprintf(args, sizeof(args), "%zu", *(size_t*)arg1);
        snprintf(ret, sizeof(ret), " -> %p", returnValue);
    }
    else if (operation[0] == 'f')
    {
        snprintf(args, sizeof(args), "%p", arg1);
        snprintf(ret, sizeof(ret), " ");
    }    
    else if (operation[0] == 'c')
    {
        snprintf(args, sizeof(args), "%zu, %zu", *(size_t*)arg1, *(size_t*)arg2);
        snprintf(ret, sizeof(ret), " -> %p", returnValue);
    }
    else
    {
        snprintf(args, sizeof(args), "%p, %zu", arg1, *(size_t*)arg2);
        snprintf(ret, sizeof(ret), " -> %p", returnValue);
    }
    
    snprintf(lastHeapOperation, sizeof(lastHeapOperation), "%s%s%i%s%s%s%s%s%s%s%s%s%s%s%s\n",
             data.file, " line ", data.line, "\n",
             operation, "(", data.callArgs, ") at function \'", data.func, "\'\n",
             operation, "(", args, ")", ret);
}

void* fakeHeapAllocate(size_t size)
{
    const size_t OFFSET = (size/ALLOC_OFFSET + 1) * ALLOC_OFFSET;
    fakeHeapSize += OFFSET;
    
    if (fakeHeapSize > fakeHeapCapacity)
    {
        fakeHeapCapacity *= 2;
        fakeHeap = realloc(fakeHeap, fakeHeapCapacity);
        if ( ! fakeHeap)
            perror("Failed to reallocate fakeHeap!");
        for (size_t i = fakeHeapCapacity/2; i < fakeHeapCapacity; i++)
            fakeHeap[i] = FREED;
    }
    
    // initialize freeSpace
    static uint8_t* freeSpace = NULL;
    if (freeSpace == NULL)
        freeSpace = fakeHeap;
    
    for (size_t i = 0; i < size; i++)
        freeSpace[i] = RESERVED;
    freeSpace += OFFSET;
    return freeSpace - OFFSET;
}

void* fakeHeapMalloc(size_t size, struct FakeHeapCallData data)
{
    void* out = fakeHeapAllocate(size);
    
    updateLastHeapOperation("malloc", data, &size, NULL, out);
    updateCurrentHeap();
    autoLog();
    return out;
}

void fakeHeapFreeMemory(void* p)
{
    uint32_t* ptr = p;
    for (size_t i = 0; ptr[i] != FREED4; i++)
        ptr[i] = FREED4;
}

void fakeHeapFree(void* p, struct FakeHeapCallData data)
{
    fakeHeapFreeMemory(p);

    updateLastHeapOperation("free", data, p, NULL, NULL);
    updateCurrentHeap();
    autoLog();
}

void* fakeHeapCalloc(size_t nmemb, size_t size, struct FakeHeapCallData data)
{
    uint8_t* out = fakeHeapAllocate(nmemb * size);
    for (size_t i = 0; i < nmemb * size; i++)
        out[i] = 0;
    updateLastHeapOperation("calloc", data, &nmemb, &size, out);
    updateCurrentHeap();
    autoLog();
    return out;
}

void* fakeHeapRealloc(void* p, size_t size, struct FakeHeapCallData data)
{
    uint8_t* ptr = p;
    uint8_t* destination = fakeHeapAllocate(size);
    for (size_t i = 0; i < size && ((uint32_t*)p)[i/4] != FREED4; i++)
        destination[i] = ptr[i];
    fakeHeapFreeMemory(p);
    updateLastHeapOperation("realloc", data, p, &size, destination);
    updateCurrentHeap();
    autoLog();
    return destination;
}

// ---------------------------------------------------------------------------

#define malloc(size)        fakeHeapMalloc(size, (struct FakeHeapCallData)        \
    {__FILE__, __LINE__, __func__, #size })

#define free(p)              fakeHeapFree(p, (struct FakeHeapCallData)            \
    {__FILE__, __LINE__, __func__, #p })

#define calloc(nmemb, size)  fakeHeapCalloc(nmemb, size, (struct FakeHeapCallData)\
    {__FILE__, __LINE__, __func__, #nmemb ", " #size })

#define realloc(p, size)    fakeHeapRealloc(p, size, (struct FakeHeapCallData)    \
    {__FILE__, __LINE__, __func__, #p ", " #size })

