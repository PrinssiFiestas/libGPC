/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "overload.h"

#define begin		{ DynamicObjOwner thisScope = {NULL, NULL}; (void)thisScope;
#define ret(obj)	moveOwnership(obj, callingScope); freeAll(&thisScope); return obj;
#define end			freeAll(&thisScope); }

typedef struct DynamicObjOwner
{
	void* firstObject;
	void* lastObject;
} DynamicObjOwner;

// malloc and assign ownership
void* mallocAssign(size_t, DynamicObjOwner*);
// malloc and assign ownership to current scope
#define scopedAlloc(size) mallocAssign(size, &thisScope)
// Prevent freeing by end or freeAll()
void moveOwnership(void* object, DynamicObjOwner* newOwner);
// Frees every object owned by owner
void freeAll(DynamicObjOwner* owner);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPC_MEMORY_H