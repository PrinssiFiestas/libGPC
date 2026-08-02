// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>
#include "../src/gpatomic.c"

// Trying to test absolutely everything would drive us insane, we'll just test
// with GPAtomicInt64, which probably has to most differences between targets.
GPAtomicInt64 shared;

int main(void)
{

}
