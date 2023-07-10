/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/gpc/gpc.h"
#include "errormsgs.h"

static size_t gpc_nextPowerOf2(size_t n)
{
	size_t result = 1;
	while (result < n)
		result *= 2;
	return result;
}

static void assignOwner(struct gpc_DynamicObjectList* obj, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(obj == NULL, GPC_EMSG_INTERNAL GPC_EMSG_NULL_ARG(obj, assignOwner)))
		return;
	if (gpc_handleError(owner == NULL, GPC_EMSG_INTERNAL GPC_EMSG_NULL_ARG(owner,assignOwner)))
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

GPC_NODISCARD void* gpc_mallocAssign(size_t capacity, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, GPC_EMSG_NULL_OWNER(mallocAssign)))
		return NULL;
	if (gpc_handleError(capacity == 0, GPC_EMSG_0ALLOC(mallocAssign)))
		return NULL;
	if (gpc_handleError(capacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(mallocAssign)))
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

GPC_NODISCARD void* gpc_callocAssign(size_t nmemb, size_t size, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, GPC_EMSG_NULL_OWNER(callocAssign)))
		return NULL;
	if (gpc_handleError(size == 0, GPC_EMSG_0ALLOC(callocAssign)))
		return NULL;
	if (gpc_handleError(nmemb*size >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(mallocAssign)))
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
	if (gpc_handleError(object == NULL, GPC_EMSG_INTERNAL GPC_EMSG_NULL_ARG(object, listData)))
		return NULL;
	return ((struct gpc_DynamicObjectList*)object) - 1;
}

#undef gpc_reallocate
GPC_NODISCARD void* gpc_reallocate(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_ARG(object, reallocate)))
		return NULL;
	if (gpc_handleError(newCapacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(reallocate)))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(reallocate)))
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
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_ARG(object, moveOwnership)))
		return;
	if (gpc_handleError(newOwner == NULL, GPC_EMSG_NULL_OWNER(moveOwnership)))
		return;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(moveOwnership)))
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
	if (gpc_handleError(owner == NULL, GPC_EMSG_NULL_PASSED(freeAll)))
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
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(getOwner)))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(getOwner)))
		return NULL;
	return me->owner;
}

size_t gpc_getSize(void* object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(getSize)))
		return 0;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(getSize)))
		return 0;
	return me->size;
}

#undef gpc_setSize
GPC_NODISCARD void* gpc_setSize(void* object, size_t newSize)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(setSize)))
		return NULL;
	if (gpc_handleError(newSize >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(setSize)))
		return NULL;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(setSize)))
		return NULL;
	me->size = newSize;
	object = gpc_reallocate(object, newSize);
	if (gpc_handleError(object == NULL, "reallocate() failed at setSize()"))
		return NULL;
	return object;
}

size_t gpc_getCapacity(void* object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(getCapacity)))
		return 0;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(getCapacity)))
		return 0;
	return me->capacity;
}

#undef gpc_setCapacity
GPC_NODISCARD void* gpc_setCapacity(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(setCapacity)))
		return NULL;
	if (gpc_handleError(newCapacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(setCapacity)))
		return NULL;
	if (gpc_handleError(getOwner(object) == NULL, GPC_EMSG_OBJ_NO_OWNER(setCapacity)))
		return NULL;
	
	object = gpc_reallocate(object, newCapacity);
	if (gpc_handleError(object == NULL, "reallocate() failed at setCapacity()"))
		return NULL;
	return object;
}

GPC_NODISCARD void* gpc_duplicate(void* object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(duplicate)))
		return NULL;
	if (gpc_handleError(gpc_getOwner(object) == NULL, GPC_EMSG_OBJ_NO_OWNER(duplicate)))
		return NULL;
	
	void* copy = gpc_mallocAssign(gpc_getCapacity(object), gpc_getOwner(object));
	if (gpc_handleError(copy == NULL, "mallocAssign() failed in duplicate()."))
		return NULL;
	memcpy(copy, object, gpc_getSize(object));
	return copy;
}

bool gpc_onStack(void* object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(onStack)))
			return false;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(onStack)))
		return false;
	return !(me->previous || me->next || me->owner->firstObject == me);
}

bool gpc_onHeap(void* object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(onHeap)))
		return false;
	
	struct gpc_DynamicObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(onHeap)))
		return false;
	return me->previous || me->next || me->owner->firstObject == me;
}

void* gpc_buildObject(void* buffer, size_t size, size_t cap, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, GPC_EMSG_NULL_OWNER(buildStackObject)))
			return NULL;
	
	struct gpc_DynamicObjectList* me = buffer;
	me->size		= size;
	me->capacity	= cap;
	me->owner		= owner;
	return me + 1;
}

void* gpc_buildHeapObject(size_t size, gpc_DynamicObjOwner* owner)
{
	if (gpc_handleError(owner == NULL, GPC_EMSG_NULL_OWNER(buildHeapObject)))
		return NULL;
	if (gpc_handleError(size >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(buildHeapObject)))
		return NULL;
	
	void* obj = gpc_mallocAssign(size, owner);
	if (gpc_handleError(owner == NULL, "mallocAssign() returned NULL in buildHeapObject()."))
		return NULL;
	obj = gpc_setSize(obj, size);
	return obj;
}
