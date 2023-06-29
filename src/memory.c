/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/gpc/gpc.h"
#include "terminalcolors.h"

static void assignOwner(struct gpc_DynamicObjectList* obj, gpc_DynamicObjOwner* owner)
{
	obj->owner = owner;
	if (owner->firstObject == NULL)
	{
		owner->firstObject = owner->lastObject = obj;
		obj->next = obj->previous = NULL;
	}
	else
	{
		obj->previous = owner->lastObject;
		owner->lastObject->next = obj;
		obj->next = NULL;
		owner->lastObject = obj;
	}
}

// For optimizing allocations
static size_t nextPowerOf2(size_t i)
{
	size_t result = 1;
	while (result < i) // Non-inclusive to prevent needless allocations
		result *= 2;
	return result;
}

static enum gpc_MemoryFailBehaviour gMemoryFailBehaviour = GPC_MEM_FAIL_ABORT;

void gpc_setMemoryFailBehaviour(enum gpc_MemoryFailBehaviour i)
{
	gMemoryFailBehaviour = i < GPC_MEM_FAIL_SIZE ? i : GPC_MEM_FAIL_ABORT;
}

static void* handleAllocNull()
{
	if (gMemoryFailBehaviour == GPC_MEM_FAIL_ABORT)
	{
		perror(NULL);
		abort();
	}
	#ifndef NDEBUG
	perror(NULL);
	#endif
	return NULL;
}

[[nodiscard]] void* gpc_mallocAssign(size_t size, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Owner is NULL in mallocAssign(). Every object requires an owner!"))
		return NULL;
	if (gpc_handleError(size == 0, "Trying to allocate 0 bytes in mallocAssign()"))
		return NULL;
	
	size = nextPowerOf2(size);
	struct gpc_DynamicObjectList* p = malloc(sizeof(p[0]) + size);
	if (p == NULL)
		return handleAllocNull();
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = size;
	return p + 1;
}

[[nodiscard]] void* gpc_callocAssign(size_t nmemb, size_t size, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Owner is NULL in callocAssign(). Every object requires an owner!"))
		return NULL;
	if (gpc_handleError(size == 0, "Trying to allocate 0 bytes in callocAssign()"))
		return NULL;
	
	size_t blockSize = nextPowerOf2(nmemb * size);
	struct gpc_DynamicObjectList* p = calloc(sizeof(p[0]) + blockSize, 1);
	if (p == NULL)
		return handleAllocNull();
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = blockSize;
	return p + 1;
}

static struct gpc_DynamicObjectList* listData(void* object)
{
	return ((struct gpc_DynamicObjectList*)object) - 1;
}

#undef gpc_reallocate
[[nodiscard]] void* gpc_reallocate(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, "NULL passed to reallocate()"))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (newCapacity <= me->capacity)
		return object;
	
	me->capacity = newCapacity = nextPowerOf2(newCapacity);
	
	// detach from current list in case of allocation failure
	if (me->previous != NULL)
		me->previous->next = me->next;
	if (me->next != NULL)
		me->next->previous = me->previous;
	
	// update old owner
	bool onlyObject = me->owner->firstObject == me && me->owner->lastObject == me;
	if (onlyObject)
		me->owner->firstObject = me->owner->lastObject = NULL;
	else if (me->owner->lastObject == me)
		me->owner->lastObject = me->previous;
	else if (me->owner->firstObject == me)
		me->owner->firstObject = me->next;	
	
	me = realloc(me, sizeof(me[0]) + newCapacity);
	if (me == NULL)
		return handleAllocNull();
	
	assignOwner(me, me->owner);
	return me + 1;
}

void gpc_moveOwnership(void* object, gpc_DynamicObjOwner* newOwner)
{
	if (gpc_handleError(object == NULL, "Object is NULL in moveOwnership()."))
		return;
	if (gpc_handleError(newOwner == NULL, "New owner is NULL in moveOwnership(). Every object requires an owner!"))
		return;
	
	struct gpc_DynamicObjectList* me = listData(object);
	
	// detach from current list
	if (me->previous != NULL)
		me->previous->next = me->next;
	if (me->next != NULL)
		me->next->previous = me->previous;
	
	// update old owner
	bool onlyObject = me->owner->firstObject == me && me->owner->lastObject == me;
	if (onlyObject)
		me->owner->firstObject = me->owner->lastObject = NULL;
	else if (me->owner->lastObject == me)
		me->owner->lastObject = me->previous;
	else if (me->owner->firstObject == me)
		me->owner->firstObject = me->next;	
	
	assignOwner(me, newOwner);
}

void gpc_freeAll(gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Passed NULL to freeAll()."))
		return;
	
	for (struct gpc_DynamicObjectList *obj = owner->firstObject, *next = NULL;
		 obj != NULL; obj = next)
	{
		next = obj->next;
		free(obj);
	}
}

DynamicObjOwner* gpc_getOwner(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to getOwner()."))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	return me->owner;
}

size_t gpc_getSize(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to getSize()."))
		return 0;
	
	struct gpc_DynamicObjectList* me = listData(object);
	return me->size;
}

#undef gpc_setSize
[[nodiscard]] void* gpc_setSize(void* object, size_t newSize)
{
	if (gpc_handleError(object == NULL, "Passed NULL to setSize()."))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	me->size = newSize;
	object = gpc_reallocate(object, newSize);
	return object;
}

size_t gpc_getCapacity(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to getCapacity()."))
		return 0;
	
	struct gpc_DynamicObjectList* me = listData(object);
	return me->capacity;
}

#undef gpc_setCapacity
[[nodiscard]] void* gpc_setCapacity(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, "Passed NULL to setCapacity()."))
		return NULL;
	
	object = gpc_reallocate(object, newCapacity);
	return object;
}

[[nodiscard]] void* gpc_duplicate(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to duplicate()."))
		return NULL;
	
	void* copy = gpc_mallocAssign(gpc_getCapacity(object), gpc_getOwner(object));
	if (copy == NULL)
		return NULL;
	memcpy(copy, object, gpc_getSize(object));
	return copy;
}