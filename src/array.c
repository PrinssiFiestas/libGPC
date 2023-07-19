// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>
#include <stdint.h>
#include "../include/gpc/array.h"
#include "../include/gpc/error.h"
#include "errormsgs.h"

void* gpc_arrPushGpcArr(void* pDest, void* src)
{
	if (gpc_handleError(pDest == NULL, GPC_EMSG_NULL_ARG(pDestination, arrPushArr)))
		return NULL;
	if (gpc_handleError(src == NULL, GPC_EMSG_NULL_ARG(src, arrPushArr)))
		return NULL;
	
	int8_t** _pDest = (int8_t**)pDest;
	
	if (gpc_handleError(*_pDest == NULL, GPC_EMSG_NULL_ARG(pDestination, arrPushArr)))
		return NULL;
		
	size_t destOldSize = gpc_getSize(*_pDest);
	*_pDest = gpc_setSize(*_pDest, destOldSize + gpc_getSize(src));
	
	if (gpc_handleError(*_pDest == NULL, "setSize() failed in arrPushArr()"))
		return NULL;
	
	memcpy(*_pDest + destOldSize, src, gpc_getSize(src));
	
	return src;
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
