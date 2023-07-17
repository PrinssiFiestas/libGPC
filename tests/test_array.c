// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/assert.h"
#include "../src/array.c"

int main(void)
{
	Owner* thisScope = newOwner();
	
	int* arr = newS(int[3], 1, 4, 7);
	TEST(array_creation)
	{
		ASSERT(arr[2] EQ 7);
		ASSERT(getOwner(arr) EQ thisScope);
	}
	
	TEST(arrSetLength_and_arrLength)
	{
		ASSERT(arrLength(arr) EQ 3);
		arr = arrSetLength(arr, 1);
		ASSERT(getSize(arr) EQ arrLength(arr) * sizeof(arr[0]));
		arr = arrSetLength(arr, 3);
		ASSERT(arr[2] EQ 0);
	}
	
	freeOwner(NULL);
}
