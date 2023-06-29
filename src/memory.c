/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdlib.h>
#include <string.h>
#include "../include/gpc/memory.h"

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

[[nodiscard]] void* gpc_mallocAssign(size_t size, gpc_DynamicObjOwner* owner)
{
	size = nextPowerOf2(size);
	struct gpc_DynamicObjectList* p = malloc(sizeof(p[0]) + size);
	if (p == NULL)
		return NULL;
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = size;
	return p + 1;
}

[[nodiscard]] void* gpc_callocAssign(size_t nmemb, size_t size, gpc_DynamicObjOwner* owner)
{
	size_t blockSize = nextPowerOf2(nmemb * size);
	struct gpc_DynamicObjectList* p = calloc(sizeof(p[0]) + blockSize, 1);
	if (p == NULL)
		return NULL;
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = blockSize;
	return p + 1;
}

#undef gpc_reallocate
[[nodiscard]] void* gpc_reallocate(void* object, size_t newCapacity)
{
	struct gpc_DynamicObjectList* me = ((struct gpc_DynamicObjectList*)object) - 1;
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
		return NULL;
	
	assignOwner(me, me->owner);
	return me + 1;
}

void gpc_moveOwnership(void* object, gpc_DynamicObjOwner* newOwner)
{
	struct gpc_DynamicObjectList* me = ((struct gpc_DynamicObjectList*)object) - 1;
	
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
	for (struct gpc_DynamicObjectList *obj = owner->firstObject, *next = NULL;
		 obj != NULL; obj = next)
	{
		next = obj->next;
		free(obj);
	}
}

DynamicObjOwner* gpc_getOwner(void* object)
{
	struct gpc_DynamicObjectList* me = ((struct gpc_DynamicObjectList*)object) - 1;
	return me->owner;
}

size_t gpc_getSize(void* object)
{
	struct gpc_DynamicObjectList* me = ((struct gpc_DynamicObjectList*)object) - 1;
	return me->size;
}

#undef gpc_setSize
[[nodiscard]] void* gpc_setSize(void* object, size_t newSize)
{
	struct gpc_DynamicObjectList* me = ((struct gpc_DynamicObjectList*)object) - 1;
	me->size = newSize;
	object = gpc_reallocate(object, newSize);
	return object;
}

size_t gpc_getCapacity(void* object)
{
	struct gpc_DynamicObjectList* me = ((struct gpc_DynamicObjectList*)object) - 1;
	return me->capacity;
}

#undef gpc_setCapacity
[[nodiscard]] void* gpc_setCapacity(void* object, size_t newCapacity)
{
	object = gpc_reallocate(object, newCapacity);
	return object;
}

[[nodiscard]] void* duplicate(void* object)
{
	void* copy = gpc_mallocAssign(gpc_getCapacity(object), gpc_getOwner(object));
	if (copy == NULL)
		return NULL;
	memcpy(copy, object, gpc_getSize(object));
	return copy;
}