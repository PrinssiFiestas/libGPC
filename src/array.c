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
extern inline void* gpc_arrPushMem(void* parr, const void* src, size_t count);

size_t gpc_arrIncSize(void* parr, size_t elemSize)
{
	size_t oldSize = gpc_size(*(void**)parr);
	gpc_resizeObj(parr, oldSize + elemSize);
	return oldSize/elemSize;
}

void* gpc_arrPushGpcArr(void* pDest, const void* src)
{
	if (gpc_handleError(pDest == NULL, GPC_EMSG_NULL_ARG(pDestination, arrPushArr)))
		return NULL;
	if (gpc_handleError(src == NULL, GPC_EMSG_NULL_ARG(src, arrPushArr)))
		return NULL;
	
	int8_t** _pDest = (int8_t**)pDest;
	
	if (gpc_handleError(*_pDest == NULL, GPC_EMSG_NULL_ARG(pDestination, arrPushArr)))
		return NULL;
		
	size_t destOldSize = gpc_size(*_pDest);
	*_pDest = gpc_resize(_pDest, destOldSize + gpc_size(src));
	
	if (gpc_handleError(*_pDest == NULL, "resize() failed in arrPushArr()"))
		return NULL;
	
	memcpy(*_pDest + destOldSize, src, gpc_size(src));
	
	return *_pDest;
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
