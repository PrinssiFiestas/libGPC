// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GPC_NAMESPACE
#include "../include/gpc/assert.h"
#include "../include/gpc/error.h"
#include "../src/string.c"

#include <stdio.h>
#include <time.h>

int main(void)
{
    Owner* thisScope = newOwner();
    
    gpc_setErrorHandlingMode(GPC_ERROR_SEND_MESSAGE);
    
    while (testSuite("String constructor"))
    {
        String str = newStringS("string");
        
        EXPECT_STR(str,==,"string");
        
        EXPECT(size(str),==,strlen("string"));
        EXPECT(capacity(str),==,strlen("string") + 1,
            "String should always have capacity for null-terminator.");
        
        //#define GPC_TEST_WARNINGS
        #ifdef GPC_TEST_WARNINGS
        char* charPointer = "Initializer must be a string literal!";
        String str2 = newStringS(charPointer);
        (void)str2;
        #endif
    }
    
    while (test("strAppend"))
    {
        String heres    = newStringH("Here's ");
        String sentence = newStringH("sentence.");
        EXPECT_STR(strAppend(&heres, "a ", sentence),==,"Here's a sentence.",
            "strAppend() should take any number of Strings and char*s");
    }
    
    freeOwner(thisScope, NULL);
}
