// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include <gpc/gparray.h> // source included after the tests so we can get access to shadowing macros

int main(void)
{
    gp_suite("Memory");
    {
        gp_test("Arrays on Stack");
        {
            GPArrayBuffer(int, 4) buffer;
        }
    }
}

#include "../src/gparray.c"
