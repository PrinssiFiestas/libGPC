// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// compile with reference_hashmap.c

#define GPC_IMPLEMENTATION
#include "../build/gpc.h"
#include <x86intrin.h>
#include "reference_hashmap.h"

#define MEASURE(RESULT, FUNC) do { \
    T t_rdtsc = __rdtsc(); \
      t_rdtsc = __rdtsc() - t_rdtsc; \
    T t0 = __rdtsc(); \
    FUNC; \
    T t1 = __rdtsc(); \
    RESULT += t1 - t0 - t_rdtsc; \
} while (0)

typedef volatile uint64_t T;

int main(void)
{
}
