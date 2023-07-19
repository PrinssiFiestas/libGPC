// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GPC_NAMESPACE
#include <stdio.h>
#include <string.h>
#include <wchar.h>
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
		ASSERT(getSize(arr) EQ /*arrLength(arr) * */sizeof(arr[0]));
		arrSetLength(&arr, 3);
		ASSERT(arr[2] EQ 0);
		ASSERT(getSize(arr) EQ 3*sizeof(arr[0]));
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
		ASSERT(arrPush(&arr, 100) EQ 100);
		ASSERT(arrLast(arr) EQ 100);
		ASSERT(arrLength(arr) EQ oldLength + 1);
	}
	
	TEST(arrPushArr)
	{
		int* arrS = allocS(35);
		int* arrH = newH(int[], 6, 8, 12, 5, 7);
		
		// Return value
		ASSERT(arrPushArr(&arrS, arrH) EQ arrH);
		
		ASSERT(arrLast(arrS) EQ 7);
		ASSERT(getSize(arrS) EQ getSize(arrH));
	}
	
	TEST(arrPop)
	{
		int lastValue = arrLast(arr);
		size_t lastLength = arrLength(arr);
		ASSERT(arrPop(&arr, 1) EQ lastValue);
		ASSERT(arrLength(arr) EQ lastLength - 1);
		
		lastLength = arrLength(arr);
		arrPop(&arr, 2);
		ASSERT(arrLength(arr) EQ lastLength - 2);
		
		arrPop(&arr, 5329);
		ASSERT(arrLength(arr) EQ 0);
	}

	TEST(arrSwitchElems)
	{
		wchar_t* str = newH(wchar_t[], L"0123456789");
		gpc_arrSwitchElems(&str, 2, 6, sizeof(str[0]), 3);
		if (EXPECT( ! wcscmp(str, L"0167852349")))
			fprintf(stderr, "%ls", str);
	}
	
	TEST(arrInsert)
	{
		long* arr = newS(long[], 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
		size_t i = 4;
		long newVal = 100;
		long old_arr_i = arr[i];
		size_t oldLength = arrLength(arr);
		
		ASSERT(arrInsert(&arr, i, newVal) EQ newVal);
		
		for (size_t i = 0; i < arrLength(arr); i++)
			printf("%li, ", arr[i]);
		puts("");
		
		EXPECT(arr[i] = newVal);
		ASSERT(arr[i + 1] EQ old_arr_i);
		ASSERT(arrLength(arr) EQ oldLength + 1);
	}
	
	freeOwner(NULL);
}
