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
	if (gpc_handleError(obj == NULL, "GPC INTERNAL ERROR: obj is NULL in assignOwner()"))
		return;
	if (gpc_handleError(owner == NULL, "GPC INTERNAL ERROR: owner is NULL in assignOwner()"))
		return;
	
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

[[nodiscard]] void* gpc_mallocAssign(size_t capacity, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Owner is NULL in mallocAssign(). Every object requires an owner!"))
		return NULL;
	if (gpc_handleError(capacity == 0, "Trying to allocate 0 bytes in mallocAssign()"))
		return NULL;
	if (gpc_handleError(capacity >= PTRDIFF_MAX, "Trying to allocate over PTRDIFF_MAX bytes in mallocAssign(). Is the input positive?"))
		return NULL;
	
	capacity = gpc_nextPowerOf2(capacity);
	struct gpc_DynamicObjectList* p = malloc(sizeof(p[0]) + capacity);
	if (gpc_handleError(p == NULL, "malloc() failed in mallocAssign()"))
		return NULL;
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = capacity;
	return p + 1;
}

[[nodiscard]] void* gpc_callocAssign(size_t nmemb, size_t size, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Owner is NULL in callocAssign(). Every object requires an owner!"))
		return NULL;
	if (gpc_handleError(size == 0, "Trying to allocate 0 bytes in callocAssign()"))
		return NULL;
	if (gpc_handleError(nmemb*size >= PTRDIFF_MAX, "Trying to allocate over PTRDIFF_MAX bytes in callocAssign(). Is the input positive?"))
		return NULL;
	
	size_t blockSize = gpc_nextPowerOf2(nmemb * size);
	struct gpc_DynamicObjectList* p = calloc(sizeof(p[0]) + blockSize, 1);
	if (gpc_handleError(p == NULL, "calloc() failed in callocAssign()"))
		return NULL;
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = blockSize;
	return p + 1;
}

static struct gpc_DynamicObjectList* listData(void* object)
{
	if (gpc_handleError(object == NULL, "GPC INTERNAL ERROR: object is NULL in listData()"))
		return NULL;
	return ((struct gpc_DynamicObjectList*)object) - 1;
}

#undef gpc_reallocate
[[nodiscard]] void* gpc_reallocate(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, "NULL passed to reallocate()"))
		return NULL;
	if (gpc_handleError(newCapacity >= PTRDIFF_MAX, "Trying to allocate over PTRDIFF_MAX bytes in reallocate(). Is the input positive?"))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, "Object passed to reallocate has no owner. Every object requires an owner!"))
		return NULL;
	if (newCapacity <= me->capacity)
		return object;
	
	me->capacity = newCapacity = gpc_nextPowerOf2(newCapacity);
	
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
	if (gpc_handleError(me == NULL, "realloc() failed in reallocate()"))
		return NULL;
	
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
	if (gpc_handleError(me->owner == NULL, "Object passed to moveOwnership() has no owner. Every object requires an owner!"))
		return;
	
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
	if (gpc_handleError(me->owner == NULL, "Object passed to getOwner has no owner. Every object requires an owner!"))
		return NULL;
	return me->owner;
}

size_t gpc_getSize(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to getSize()."))
		return 0;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, "Object passed to getSize has no owner. Every object requires an owner!"))
		return 0;
	return me->size;
}

#undef gpc_setSize
[[nodiscard]] void* gpc_setSize(void* object, size_t newSize)
{
	if (gpc_handleError(object == NULL, "Passed NULL to setSize()."))
		return NULL;
	if (gpc_handleError(newSize >= PTRDIFF_MAX, "Trying to allocate over PTRDIFF_MAX bytes in setSize(). Is the input positive?"))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, "Object passed to setSize has no owner. Every object requires an owner!"))
		return NULL;
	me->size = newSize;
	object = gpc_reallocate(object, newSize);
	if (gpc_handleError(object == NULL, "reallocate() failed at setSize()"))
		return NULL;
	return object;
}

size_t gpc_getCapacity(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to getCapacity()."))
		return 0;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, "Object passed to getCapacity() has no owner. Every object requires an owner!"))
		return 0;
	return me->capacity;
}

#undef gpc_setCapacity
[[nodiscard]] void* gpc_setCapacity(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, "Passed NULL to setCapacity()."))
		return NULL;
	if (gpc_handleError(newCapacity >= PTRDIFF_MAX, "Trying to allocate over PTRDIFF_MAX bytes in setCapacity(). Is the input positive?"))
		return NULL;
	if (gpc_handleError(getOwner(object) == NULL, "Object passed to setCapacity() has no owner. Every object requires an owner!"))
		return NULL;
	
	object = gpc_reallocate(object, newCapacity);
	if (gpc_handleError(object == NULL, "reallocate() failed at setCapacity()"))
		return NULL;
	return object;
}

[[nodiscard]] void* gpc_duplicate(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to duplicate()."))
		return NULL;
	if (gpc_handleError(gpc_getOwner(object) == NULL, "Object passed to duplicate() has no owner. Every object requires an owner!"))
		return NULL;
	
	void* copy = gpc_mallocAssign(gpc_getCapacity(object), gpc_getOwner(object));
	if (gpc_handleError(copy == NULL, "mallocAssign() failed in duplicate()."))
		return NULL;
	memcpy(copy, object, gpc_getSize(object));
	return copy;
}

bool gpc_onStack(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to onStack()."))
			return false;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, "Object passed to onStack() has no owner. Every object requires an owner!"))
		return false;
	return !(me->previous || me->next || me->owner->firstObject == me);
}

bool gpc_onHeap(void* object)
{
	if (gpc_handleError(object == NULL, "Passed NULL to onHeap()."))
		return false;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, "Object passed to onHeap() has no owner. Every object requires an owner!"))
		return false;
	return me->previous || me->next || me->owner->firstObject == me;
}

void* gpc_buildStackObject(void* buffer,
						   size_t objectSize,
						   size_t capacity,
						   gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Owner is NULL in buildStackObject(). Every object requires an owner!"))
			return NULL;
	
	struct gpc_DynamicObjectList* me = buffer;
	me->size		= objectSize;
	me->capacity	= capacity;
	me->owner		= owner;
	return me + 1;
}

void* gpc_buildHeapObject(size_t size, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, "Owner is NULL in buildHeapObject(). Every object requires an owner!"))
		return NULL;
	if (gpc_handleError(size >= PTRDIFF_MAX, "Trying to allocate over PTRDIFF_MAX bytes in buildHeapObject(). Is the input positive?"))
		return NULL;
	
	void* obj = gpc_mallocAssign(size, owner);
	if (gpc_handleError(owner == NULL, "mallocAssign() returned NULL in buildHeapObject()."))
		return NULL;
	obj = gpc_setSize(obj, size);
	return obj;
}