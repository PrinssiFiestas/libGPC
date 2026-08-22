// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include <gpc/gparray.h>

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
