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

#define begin		{ DynamicObjOwner thisScope;
#define ret(obj)	moveOwnership(obj, callingScope); freeAll(thisScope); return obj;
#define end			automem_end; }

typedef struct DynamicObjOwner
{
	void* firstObject;
	void* lastObject;
};

// PRIVATE
struct DynamicObjectList
{
	struct DynamicObjectList* previous;
	struct DynamicObjectList* next;
}

// malloc and assign ownership
void* automalloc(size_t, DynamicObjOwner*);
// Prevent freeing by end, automem_end or freeAll
void moveOwnership(void* object, DynamicObjOwner* newOwner);
// Frees every object owned by owner
void freeAll(DynamicObjOwner* owner);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GPC_MEMORY_H