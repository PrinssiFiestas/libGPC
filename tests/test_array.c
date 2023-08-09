// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#define GPC_NAMESPACE
#include <stdio.h>
#include <wchar.h>
#include "../include/gpc/assert.h"
#include "../src/array.c"

int main(void)
{
	gpc_setErrorHandlingMode(GPC_ERROR_SEND_MESSAGE);
	
	Owner* thisScope = newOwner();
	
	int* arr = newS(int[3], 1, 4, 7);
	while (test("array creation"))
	{
		ASSERT(arr[2],==,7);
		ASSERT(getOwner(arr),==,thisScope);
	}
	
	while (test("arrSetLength and arrLength"))
	{
		ASSERT(arrLength(arr),==,(size_t)3);
		arrSetLength(&arr, 1);
		ASSERT(size(arr),==,/*arrLength(arr) * */sizeof(arr[0]));
		arrSetLength(&arr, 3);
		ASSERT(arr[2],==,0);
		ASSERT(size(arr),==,3*sizeof(arr[0]));
	}
	
	while (test("arrSetCapacity and arrGetCapacity"))
	{
		Owner* scope = newOwner();
		double* arrd = newS(double[], .4);
		arrReserve(&arrd, 5);
		ASSERT(onHeap(arrd));
		ASSERT(arrCapacity(arrd),==,/*nextPowerOf(5)=*/(size_t)8);
		freeOwner(scope, NULL);
	}
	
	while (test("arrBack and arrLast"))
	{
		int* arr2 = newS(int[], 3, 6, 7, 9);
		ASSERT(*arrBack(arr2),==,arrLast(arr2));
		ASSERT(arrLast(arr2),==,9);
	}
	
	while (test("arrPush"))
	{
		size_t oldLength = arrLength(arr);
		EXPECT(arrPush(&arr, 100),==,arr);
		EXPECT(arrLast(arr),==,100);
		ASSERT(arrLength(arr),==,oldLength + 1);
		
		// TODO check for C23 once it comes out
		#if defined(__GNUC__)
		// many elements
		arrPush(&arr, 54, 8, 10);
		EXPECT(arrLength(arr),==,oldLength + 4);
		#endif
	}
	
	while (test("arrPushArr"))
	{
		int* arrS = allocS(35);
		int* arrH = newH(int[], 6, 8, 12, 5, 7);
		
		// Return value
		ASSERT(arrPushArr(&arrS, arrH),==,arrS);
		
		ASSERT(arrLast(arrS),==,7);
		ASSERT(size(arrS),==,size(arrH));
	}
	
	while (test("arrPop"))
	{
		int lastValue = arrLast(arr);
		size_t lastLength = arrLength(arr);
		EXPECT(arrPop(&arr),==,lastValue);
		ASSERT(arrLength(arr),==,lastLength - 1);
	}
	
	while (test("arrInsert"))
	{
		long* arr = newS(long[], 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
		size_t i = 4;
		long newVal = 100;
		long old_arr_i = arr[i];
		size_t oldLength = arrLength(arr);
		
		ASSERT(arrInsert(&arr, i, newVal),==,arr);
		
		for (size_t i = 0; i < arrLength(arr); i++)
			printf("%li, ", arr[i]);
		puts("");
		
		EXPECT((arr[i] = newVal));
		ASSERT(arr[i + 1],==,old_arr_i);
		ASSERT(arrLength(arr),==,oldLength + 1);
	}
	
	while (test("arrInsertArr"))
	{
		long* arr = newS(long[], 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
		size_t i = 4;
		long* arr2 = newS(long[], 10, 11, 12);
		long old_arr_i = arr[i];
		size_t oldLength = arrLength(arr);
		
		arrInsertArr(&arr, i, arr2);
		
		for (size_t i = 0; i < arrLength(arr); i++)
			printf("%li, ", arr[i]);
		puts("");
		
		EXPECT(arr[i],==,arr2[0]);
		ASSERT(arr[i + arrLength(arr2)],==,old_arr_i);
		ASSERT(arrLength(arr),==,oldLength + arrLength(arr2));
	}
	
	while (test("arrDelete"))
	{
		long* arr = newH(long[], 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
		size_t i = 4;
		size_t oldLength = arrLength(arr);
		
		arrDelete(&arr, i);
		
		EXPECT(arr[i],==,(long)i + 1);
		EXPECT(arrLength(arr),==,oldLength - 1);
	}
	
	while (test("arrSwitchElems"))
	{
		wchar_t* str = newH(wchar_t[], L"0123456789");
		gpc_arrSwitchElems(&str, 2, 6, sizeof(str[0]), 3);
		if ( ! EXPECT( ! wcscmp(str, L"0167852349")))
			fprintf(stderr, "%ls\n", str);
	}
	
	freeOwner(thisScope, NULL);
}
