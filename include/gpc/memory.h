/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stddef.h>
#include "overload.h"

#define begin		{ DynamicObjOwner thisScope = {NULL, NULL}; (void)thisScope;
#define ret(obj)	moveOwnership(obj, callingScope); freeAll(&thisScope); return obj;
#define end			freeAll(&thisScope); }

struct DynamicObjectList;

typedef struct DynamicObjOwner
{
	struct DynamicObjectList* firstObject;
	struct DynamicObjectList* lastObject;
} DynamicObjOwner;

// malloc and assign ownership
void* mallocAssign(size_t, DynamicObjOwner*);
// malloc and assign ownership to current scope
#define scopedAlloc(size) mallocAssign(size, &thisScope)
// Prevent freeing by end or freeAll()
void moveOwnership(void* object, DynamicObjOwner* newOwner);
// Frees every object owned by owner
void freeAll(DynamicObjOwner* owner);
// Self explanatory
DynamicObjOwner* getOwner(void* object);
// Gets size of object excluding it's metadata
size_t getSize(void* object);

#endif // GPC_MEMORY_H