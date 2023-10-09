// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

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

// These are only used by func(), but are global so they can be tested. Check
// the test below. 
int* i1;
int* i2;
int* i3;
int* i;
static int* func(void)
{
    Owner* innerScope = newOwner();
    i1 = newH(int, 1);
    i2 = newH(int, 2);
    i  = newH(int, 0);
    i3 = newH(int, 3);
    int* is = newS(int, 4); // Object on stack
    *i = *i1 + *i2 + *i3 + *is;
    return freeOwner(innerScope, i); // returns i, frees everything else
}

static int* returnStackObject(void)
{
    Owner* scope = newOwner();
    int* is = newS(int, 4);
    return freeOwner(scope, is); // is will be copied to heap so it can be returned
}

int main(void)
{
    fakeHeapInit();
    
    // Uncomment for heap visualization in stdout
    //fakeHeapSetAutoLog(true);
    
    // -----------------------------------------------------------------------
    //
    //        API TESTS
    //
    // -----------------------------------------------------------------------
    
    while (testSuite("Owner memory handling"))
    {
        Owner* outerScope = newOwner();
        int* validObject = func();
        
        while (test("Ownership transfer"))
        {
            EXPECT(*validObject,==,1 + 2 + 3 + 4, "Shouldn't crash.");
            EXPECT(getOwner(validObject),==,outerScope);
        }
        
        while (test("Freed objects"))
        {
            // FREED4 is a sentinel value used by fake heap to signify free
            // memory block. In real programs these would be free. 
            EXPECT((uint32_t)*i1,==,FREED4);
            EXPECT((uint32_t)*i2,==,FREED4);
            EXPECT((uint32_t)*i3,==,FREED4);
            EXPECT((uint32_t)*i, !=,FREED4);
        }
        
        while (test("Stack object returned to heap"))
            EXPECT(onHeap(returnStackObject()));
        
        freeOwner(outerScope, NULL);
        
        while (test("All memory freed"))
            ASSERT(fakeHeapFindFirstReserved(),==,EMPTY_HEAP,
                "Heap should be freed after freeing all owners!");
    }
    
    while (testSuite("Objects"))
    {
        Owner* objectsScope = newOwner();
        
        int* intOnHeap  = newH(int, 1);
        int* arrOnStack = newS(int[], 2, 3);
        
        int* emptyIntOnHeap  = allocH(sizeof(int));
        int* emptyIntOnStack = allocS(sizeof(int));
        
        while (test("Init values"))
        {
            EXPECT(*intOnHeap,==,1);
            EXPECT(arrOnStack[0],==,2);
            EXPECT(arrOnStack[1],==,3);
            
            EXPECT(*emptyIntOnHeap, ==, 0, "Unlike malloc() memory is always zeroed");
            EXPECT(*emptyIntOnStack,==, 0, "Unlike alloca() memory is always zeroed");
        }
        
        while (test("Size"))
        {
            EXPECT(size(intOnHeap), ==, sizeof(int));
            EXPECT(size(arrOnStack),==, sizeof((int[]){2, 3}));
            
            EXPECT(size(emptyIntOnHeap), ==, (size_t)0);
            EXPECT(size(emptyIntOnStack),==, (size_t)0);
        }
        
        while (test("Capacity"))
        {
            EXPECT(capacity(intOnHeap), ==, gpc_nextPowerOf2(size(intOnHeap)),
                "Objects on heap get their capacity rounded up.");
            EXPECT(capacity(arrOnStack),==, size(arrOnStack),
                "Objects on stack only allocate their size");
            
            EXPECT(capacity(emptyIntOnHeap), ==, gpc_nextPowerOf2(sizeof(int)),
                "allocH() rounds up requested memory block size.");
            EXPECT(capacity(emptyIntOnStack),==, sizeof(int),
                "allocS() does not round up requested memory block size.");
        }
        
        while (test("Location on memory"))
        {
            EXPECT(onHeap(intOnHeap));
            EXPECT(onStack(arrOnStack));
            
            EXPECT(onHeap(emptyIntOnHeap));
            EXPECT(onStack(emptyIntOnStack));
            
            int* arrCopy;
            EXPECT(onHeap(arrCopy = duplicate(arrOnStack)));
        }
        
        //#define GPC_TEST_WARNINGS
        #if defined(GPC_TEST_WARNINGS) && defined(__GNUC__)
        while (test("Unused result warnings"))
        {
            newS(int, 0); // GCC, Clang: unused result
            newH(int, 0); // GCC, Clang: unused result
            duplicate(i); // GCC, Clang: unused result
            reallocate(i, sizeof(*i)); // GCC, Clang: unused result
        }
        #endif
        
        freeOwner(objectsScope, NULL);
    }
    
    while (testSuite("Object modification"))
    {
        Owner* scope = newOwner();
        
        while (test("Resize"))
        {
            size_t newSize = 3;
            char* cstrOnStack = newS(char[], "String ");
            
            EXPECT(size(resize(&cstrOnStack, newSize)),==,newSize);
            EXPECT_STR(cstrOnStack,==,"String ",
                "The actual memory shuoldn't be truncated on shrinks.");
            
            EXPECT_STR(resize(&cstrOnStack, newSize + 1),==,"Str",
                "New memory should be zeroed.");
            
            EXPECT(onHeap(resize(&cstrOnStack, capacity(cstrOnStack) + 1)),
                "Memory should be reallocated to heap if capacity is exceeded.");
        }
        
        while (test("Reserve"))
        {
            char* cstr = newH(char[], "String ");
            EXPECT(cstr,==,reserve(&cstr, capacity(cstr) - 1),
                "Nothing should be done if new capacity doesn't exceed old cap.");
            size_t newCap = capacity(cstr) + 1;
            EXPECT(capacity(reserve(&cstr, newCap)),==,gpc_nextPowerOf2(newCap));
        }
        
        //#define GPC_TEST_WARNINGS
        #if defined(GPC_TEST_WARNINGS) && defined(__GNUC__)
        while (test("Pointer validation"))
        {
            int* obj = newS(int, 0);
            resize(obj, 1);  // GCC, Clang: Expected pointer passed by reference
            reserve(obj, 1); // GCC, Clang: Expected pointer passed by reference
            
            // No warnings! Beware the double pointer!
            void** doublePtr = newH(void**, NULL);
            resize(doublePtr, 1);
            reserve(doublePtr, 1);
        }
        #endif
        
        freeOwner(scope, NULL);
    }
    
    // -----------------------------------------------------------------------
    //
    //        END OF API TESTS
    //
    //        Tests below test internals. There's also some old tests for double
    //        checks. 
    //
    // -----------------------------------------------------------------------

    gpc_setErrorHandlingMode(GPC_ERROR_SEND_MESSAGE);
    
    gpc_Owner* thisScope = newOwner();
    
    const size_t obj0Cap = 4;
    char* obj0 = mallocAssign(obj0Cap, NULL);
    
    while (test("owner mallocate"))
        ASSERT(getOwner(obj0),==,thisScope);
    
    obj0 = resize(&obj0, 2);
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
            uint8_t objSmem[sizeof(struct gpc_Object) + 1] = {0};
            struct gpc_Object* objSdata = (struct gpc_Object*)objSmem;
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
        ASSERT(size(newH(int8_t[15], 0)),==,15llu);
        ASSERT(*(int*)newH(int, 3),==,3);
        ASSERT(capacity(allocH(sizeof(int8_t[15]))),==,gpc_nextPowerOf2(15));
    }

    while (test("newS and allocS"))
    {
        ASSERT(onStack(newS(int, 0)));
        ASSERT(!onHeap(allocS(sizeof(int))));
        ASSERT(size(allocS(sizeof(int8_t[15]))),==,0llu);
        ASSERT(size(newS(int8_t[15], 0)),==,15llu);
        ASSERT(*(int*)newS(int, 5),==,5);
        ASSERT(capacity(newS(int8_t[15], 0)),==,15llu);
    }
    
    while (test("allocaAssign"))
    {
        ASSERT(size(allocaAssign(5, NULL)),==,0llu);
        ASSERT(capacity(allocaAssign(5, thisScope)),==,5llu);
    }
    
    while (test("callocAssign"))
        ASSERT(obj1[2],==,0);
    
    // To help debugging
    obj1[1] = 0xEFBE; // BEEF
    
    while (test("reallocate"))
    {
        uint32_t* obj0Original = (uint32_t*)obj0;
        obj0 = reallocate(obj0, obj0Cap * 2 + 1);
        ASSERT(obj0[0],==,'X', "Memory not copied!");
        ASSERT(*obj0Original,==,FREED4);
        ASSERT(capacity(obj0),==,gpc_nextPowerOf2(obj0Cap * 2 + 1));
    }
    
    while (test("setSize and reallocate"))
    {
        uint32_t* obj1Original = (uint32_t*)obj1;
        size_t oldCapacity = capacity(obj1);
        obj1 = reallocate(obj1, oldCapacity + 1);
        ASSERT(*obj1Original,==,FREED4);
        ASSERT(capacity(obj1),==,gpc_nextPowerOf2(oldCapacity + 1));
        
        int* obj1NonMoved = obj1;
        ASSERT(obj1 = resize(&obj1, capacity(obj1)),==,obj1NonMoved);
        
        // Test that data is copied properly
        size_t obj1OldCap = capacity(obj1);
        obj1 = resize(&obj1, obj1OldCap + 1);
        ASSERT(size(obj1),==,obj1OldCap + 1);
    }
    
    while (test("duplicate"))
    {
        char* copy = duplicate(obj0);
        obj0[0] = 'Y';
        ASSERT(obj0[0],==,'Y');
        ASSERT(copy[0],==,'X');
    }
    
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
        for (struct gpc_Object* p = thisScope->firstObject; p->next != NULL; p = p->next)
            if ((signChanged = signb((ptrdiff_t)(p - p->next)) != sign))
                break;
        EXPECT( ! signChanged);
    }
    
    freeOwner(thisScope, NULL);
    while (test("automatic freeing"))
        ASSERT(fakeHeapFindFirstReserved(),==,EMPTY_HEAP, "Heap not empty after killing owner!");
    
    while (test("nextPowerOf2"))
    {
        unsigned long n = 3;
        ASSERT(gpc_nextPowerOf2(n),==,4lu);
        ASSERT(gpc_nextPowerOf2(4),==,8lu,
            "Objects only require memory management if they're expected to "
            "grow. Therefore, objects always have extra capacity just in case. "
            "Null-terminated strings require this extra space anyway. ");
        ASSERT(gpc_nextPowerOf2(28),==,32lu);
        ASSERT(gpc_nextPowerOf2(SIZE_MAX/2 + 1),<=,SIZE_MAX);
    }
    
    fakeHeapDestroy();
}

// ---------------------------------------------------------------------------
//        Helpers

void getMsg(const char* msg)
{
    size_t len = strlen(msg) < sizeof(msgBuf)-1 ? strlen(msg) : sizeof(msgBuf)-1;
    strncpy(msgBuf, msg, len);
    msgBuf[len] = '\0';
}

bool doubleCheck(void* obj, Owner* owner)
{
    bool onObjectList = false;
    for (struct gpc_Object* p = owner->firstObject; p != NULL; p = p->next)
        onObjectList = p + 1 == obj;
    return onObjectList;
}
