// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/gpc/gpc.h"
#include "errormsgs.h"

// globalOwner makes sure that gpc_gpc_gDefaultOwner always has a parent
static GPC_THREAD_LOCAL gpc_Owner globalOwner = {0};
GPC_THREAD_LOCAL gpc_Owner* gpc_gDefaultOwner = NULL;

static size_t gpc_nextPowerOf2(size_t n)
{
	// prevent integer overflow
	if (n >= SIZE_MAX/2)
		return SIZE_MAX;
	
	size_t result = 1;
	while (result < n)
		result *= 2;
	return result;
}

static void assignOwner(struct gpc_ObjectList* obj, gpc_Owner* owner)
{
	// initialize gpc_gDefaultOwner
	if ( ! gpc_gDefaultOwner)
		gpc_gDefaultOwner = &globalOwner;
	
	if (gpc_handleError(obj == NULL, GPC_EMSG_INTERNAL GPC_EMSG_NULL_ARG(obj, assignOwner)))
		return;
	
	obj->owner = owner = owner ? owner : gpc_gDefaultOwner;
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

GPC_NODISCARD void* gpc_allocH(size_t capacity)
{
	if (gpc_handleError(capacity == 0, GPC_EMSG_0ALLOC(allocH)))
		return NULL;
	if (gpc_handleError(capacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(allocH)))
		return NULL;
	
	return gpc_callocAssign(capacity, 1, gpc_gDefaultOwner);
}

GPC_NODISCARD void* gpc_mallocAssign(size_t capacity, gpc_Owner* owner)
{
	owner = owner ? owner : gpc_gDefaultOwner;
	
	if (gpc_handleError(capacity == 0, GPC_EMSG_0ALLOC(mallocAssign)))
		return NULL;
	if (gpc_handleError(capacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(mallocAssign)))
		return NULL;
	
	capacity = gpc_nextPowerOf2(capacity);
	struct gpc_ObjectList* p = malloc(sizeof(p[0]) + capacity);
	if (gpc_handleError(p == NULL, "malloc() failed in mallocAssign()"))
		return NULL;
	assignOwner(p, owner);
	
	p->size = 0;
	p->capacity = capacity;
	return p + 1;
}

GPC_NODISCARD void* gpc_callocAssign(size_t nmemb, size_t size, gpc_Owner* owner)
{
	owner = owner ? owner : gpc_gDefaultOwner;
	
	if (gpc_handleError(nmemb * size == 0, GPC_EMSG_0ALLOC(callocAssign)))
		return NULL;
	if (gpc_handleError(nmemb * size >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(mallocAssign)))
		return NULL;
	
	size_t blockSize = gpc_nextPowerOf2(nmemb * size);
	struct gpc_ObjectList* p = calloc(sizeof(p[0]) + blockSize, 1);
	if (gpc_handleError(p == NULL, "calloc() failed in callocAssign()"))
		return NULL;
	assignOwner(p, owner);
	p->size = 0;
	p->capacity = blockSize;
	return p + 1;
}

static struct gpc_ObjectList* listData(const void *const object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_INTERNAL GPC_EMSG_NULL_ARG(object, listData)))
		return NULL;
	return ((struct gpc_ObjectList*)object) - 1;
}

GPC_NODISCARD void* gpc_reallocate(void* object, size_t newCapacity)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_ARG(object, reallocate)))
		return NULL;
	if (gpc_handleError(newCapacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(reallocate)))
		return NULL;
	
	struct gpc_ObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(reallocate)))
		return NULL;
	
	if (newCapacity <= me->capacity)
		return object;
	
	newCapacity = gpc_nextPowerOf2(newCapacity);
	
	// detach from current list in case of allocation failure
	if (me->previous != NULL)
		me->previous->next = me->next;
	if (me->next != NULL)
		me->next->previous = me->previous;
	
	bool onlyHeapObject = me->owner->firstObject == me && me->owner->lastObject == me;
	if (onlyHeapObject)
		me->owner->firstObject = me->owner->lastObject = NULL;
	else if (me->owner->lastObject == me)
		me->owner->lastObject = me->previous;
	else if (me->owner->firstObject == me)
		me->owner->firstObject = me->next;	
	
	struct gpc_ObjectList* newMe;
	if (gpc_onStack(object))
		newMe = malloc(sizeof(*me) + newCapacity);
	else
		newMe = realloc(me, sizeof(*me) + newCapacity);
		
	if (gpc_handleError(newMe == NULL, "allocation failed in reallocate()"))
		return NULL;
	
	if (gpc_onStack(object))
		memcpy(newMe, me, me->capacity + sizeof(*me));
	
	newMe->capacity = newCapacity = gpc_nextPowerOf2(newCapacity);
	
	assignOwner(newMe, newMe->owner);
	return newMe + 1;
}

void gpc_moveOwnership(void* object, gpc_Owner* newOwner)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_ARG(object, moveOwnership)))
		return;

	newOwner = newOwner ? newOwner : gpc_gDefaultOwner;
	
	struct gpc_ObjectList* me = listData(object);
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

gpc_Owner* gpc_registerOwner(gpc_Owner* owner)
{
	// initialize gpc_gDefaultOwner
	if ( ! gpc_gDefaultOwner)
		gpc_gDefaultOwner = &globalOwner;
	
	if (gpc_handleError(owner == NULL, GPC_EMSG_NULL_PASSED(registerOwner)))
		return NULL;
	
	owner->parent = gpc_gDefaultOwner;
	return gpc_gDefaultOwner = owner;
}

void gpc_freeOwner(gpc_Owner* owner)
{
	owner = owner ? owner : gpc_gDefaultOwner;
	
	for (struct gpc_ObjectList *obj = owner->firstObject, *next = NULL;
		 obj != NULL; obj = next)
	{
		next = obj->next;
		free(obj);
	}
	
	if (owner->parent != NULL && owner == gpc_gDefaultOwner)
		gpc_gDefaultOwner = gpc_gDefaultOwner->parent;
}

gpc_Owner* gpc_getOwner(const void *const object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(getOwner)))
		return NULL;
	
	struct gpc_ObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(getOwner)))
		return NULL;
	
	return me->owner;
}

size_t gpc_getSize(const void *const object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(getSize)))
		return 0;
	
	struct gpc_ObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(getSize)))
		return 0;
	
	return me->size;
}

GPC_NODISCARD void* gpc_setSize(void* object, size_t newSize)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(setSize)))
		return NULL;
	if (gpc_handleError(newSize >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(setSize)))
		return NULL;
	if (gpc_handleError(getOwner(object) == NULL, GPC_EMSG_OBJ_NO_OWNER(setSize)))
		return NULL;
	
	struct gpc_ObjectList* me = listData(object);
	if (newSize == me->size)
		return object;
	
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(setSize)))
		return NULL;
	
	object = gpc_reallocate(object, newSize);
	me = listData(object);
	
	if (gpc_handleError(object == NULL, "reallocate() failed at setSize()"))
		return NULL;
	
	for (size_t i = me->size; i < newSize; i++)
		((int8_t*)object)[i] = 0;
	me->size = newSize;
	return object;
}

size_t gpc_getCapacity(const void *const object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(getCapacity)))
		return 0;
	
	struct gpc_ObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(getCapacity)))
		return 0;
	
	return me->capacity;
}

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

GPC_NODISCARD void* gpc_duplicate(const void *const object)
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

bool gpc_onStack(const void *const object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(onStack)))
		return false;
	
	struct gpc_ObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(onStack)))
		return false;
	return !(me->previous || me->next || me->owner->firstObject == me);
}

bool gpc_onHeap(const void *const object)
{
	if (gpc_handleError(object == NULL, GPC_EMSG_NULL_PASSED(onHeap)))
		return false;
	
	struct gpc_ObjectList* me = listData(object);
	if (gpc_handleError(me->owner == NULL, GPC_EMSG_OBJ_NO_OWNER(onHeap)))
		return false;
	return me->previous || me->next || me->owner->firstObject == me;
}

void* gpc_buildObject(void* outBuf, const struct gpc_ObjectList data, const void* initVal)
{
	if (gpc_handleError(data.capacity == 0, "Failed to build object with 0 capacity in buildObject()"))
		return NULL;
	if (gpc_handleError(data.capacity >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(buildObject)))
		return NULL;
	if (gpc_handleError(data.size >= PTRDIFF_MAX, "Object too big at buildObject()"))
		return NULL;
	if (gpc_handleError(outBuf == NULL, GPC_EMSG_NULL_ARG(outBuf, buildObject)))
		return NULL;
	if (gpc_handleError(!initVal && data.size, GPC_EMSG_NULL_ARG(initVal, buildObject)))
		return NULL;
	
	struct gpc_ObjectList* me = (struct gpc_ObjectList*)outBuf;
	*me = data;
	me->owner = me->owner ? me->owner : gpc_gDefaultOwner;
	return memcpy(me + 1, initVal, data.size);
}

void* gpc_buildHeapObject(const size_t size, const void* initVal, gpc_Owner* owner)
{
	owner = owner ? owner : gpc_gDefaultOwner;
	if (gpc_handleError(size >= PTRDIFF_MAX, GPC_EMSG_OVERALLOC(buildHeapObject)))
		return NULL;
	if (gpc_handleError(size == 0, GPC_EMSG_0ALLOC(buildHeapObject)))
		return NULL;
	if (gpc_handleError(initVal == NULL, GPC_EMSG_NULL_ARG(initVal,buildHeapObject)))
		return NULL;
	
	void* obj = gpc_mallocAssign(size, owner);
	if (gpc_handleError(obj == NULL, "mallocAssign() returned NULL in buildHeapObject()."))
		return NULL;
	obj = gpc_setSize(obj, size);
	return memcpy(obj, initVal, size);
}
