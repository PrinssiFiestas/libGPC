// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>
#include "../include/gpc/gpc.h"
#include "errormsgs.h"

void* gpc_arrPushArr(void* pDest, void* src)
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
