// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../include/gpc/gpc.h"
#include "../include/gpc/assert.h"
#include "../src/array.c"

int main(void)
{
	gpc_setErrorHandlingMode(GPC_ERROR_SEND_MESSAGE);
	
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
		arrSetLength(&arr, 1);
		ASSERT(getSize(arr) EQ arrLength(arr) * sizeof(arr[0]));
		arrSetLength(&arr, 3);
		ASSERT(arr[2] EQ 0);
	}
	
	TEST(arrSetCapacity_and_arrGetCapacity)
	{
		newOwner();
		double* arrd = newS(double[], .4);
		arrSetCapacity(&arrd, 5);
		ASSERT(onHeap(arrd));
		ASSERT(arrCapacity(arrd) EQ /*nextPowerOf(5)=*/8);
		freeOwner(NULL);
	}
	
	TEST(arrBack_and_arrLast)
	{
		int* arr2 = newS(int[], 3, 6, 7, 9);
		ASSERT(*arrBack(arr2) EQ arrLast(arr2));
		ASSERT(arrLast(arr2) EQ 9);
	}
	
	TEST(arrPush)
	{
		size_t oldLength = arrLength(arr);
		arrPush(&arr, 100);
		EXPECT(arrLast(arr) EQ 100);
		ASSERT(arrLength(arr) EQ oldLength + 1);
	}
	
	// int* arr2 = newH(int[], 6, 8, 12, 5, 7);
	// TEST(arrPushArr)
	// {
		// ASSERT(arr)
	// }
	
	freeOwner(NULL);
}
