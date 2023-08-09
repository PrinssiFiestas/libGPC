// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>
#include <stdint.h>
#include "../include/gpc/array.h"
#include "../include/gpc/error.h"
#include "errormsgs.h"

extern inline void* gpc_passTrough(void*);
extern inline void* gpc_arrLastElem(void* arr, size_t elemSize);
extern inline bool gpc_arrIsEmpty(void* arr);
extern inline void* gpc_arrPushGpcArr(void* parr, void* arr);

size_t gpc_arrIncSize(void* parr, size_t elemSize)
{
	size_t oldSize = gpc_size(*(void**)parr);
	gpc_resizeObj(parr, oldSize + elemSize);
	return oldSize/elemSize;
}

size_t gpc_arrDecSize(void* parr, size_t elemSize)
{
	size_t oldSize = gpc_size(*(void**)parr);
	size_t newSize = elemSize < oldSize ? oldSize - elemSize : 0;
	gpc_resizeObj(parr, newSize);
	return newSize/elemSize;
}

void* gpc_arrPushMem(void* parr, const void* src, size_t srcSize)
{
	size_t oldSize = gpc_size(*(void**)parr);
	gpc_resizeObj(parr, oldSize + srcSize);
	memcpy(*(gpc_Byte**)parr + oldSize, src, srcSize);
	return *(void**)parr;
}

void* gpc_arrPopElem(void* parr, size_t elemSize)
{
	return (*(gpc_Byte**)parr) + gpc_arrDecSize(parr, elemSize) * elemSize;
}

void* gpc_arrMoveElemsR(void* parr, size_t pos, size_t elemSize)
{
	gpc_arrIncSize(parr, elemSize);
	gpc_Byte* arr = *(gpc_Byte**)parr;
	memmove(arr + pos + elemSize, arr + pos, gpc_size(arr) - pos);
	return arr;
}

void* gpc_arrMoveElemsL(void* parr, size_t pos, size_t elemSize)
{
	gpc_arrDecSize(parr, elemSize);
	gpc_Byte* arr = *(gpc_Byte**)parr;
	memmove(arr + pos, arr + pos + elemSize, gpc_size(arr) - pos);
	return arr;
}

void* gpc_arrInsertMem(void* parr, size_t pos, const void* src, size_t srcSize)
{
	gpc_arrMoveElemsR(parr, pos, srcSize);
	gpc_Byte* arr = *(gpc_Byte**)parr;
	memcpy(arr + pos, src, srcSize);
	return arr;
}

void* gpc_arrSwitchElems(void* parr, size_t pos1, size_t pos2, size_t elemSize, size_t nElems)
{
	int8_t** arr = (int8_t**)parr;
	for (size_t i = 0; i < nElems * elemSize; i++)
	{
		int8_t c = (*arr)[pos1 * elemSize + i];
		(*arr)[pos1 * elemSize + i] = (*arr)[pos2 * elemSize + i];
		(*arr)[pos2 * elemSize + i] = c;
	}
	return *arr;
}
