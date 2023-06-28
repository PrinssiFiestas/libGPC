/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdlib.h>
#include "../include/gpc/memory.h"

static void assignOwner(struct DynamicObjectList* obj, DynamicObjOwner* owner)
{
	// TODO: check validity of owner and object
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

void* mallocAssign(size_t size, DynamicObjOwner* owner)
{
	struct DynamicObjectList* p = malloc(sizeof(p[0]) + size);
	assignOwner(p, owner);
	p->size = size;
	return p + 1;
}

void moveOwnership(void* object, DynamicObjOwner* newOwner)
{
	struct DynamicObjectList* me = ((struct DynamicObjectList*)object) - 1;
	
	// detach from current list
	if (me->previous != NULL)
		me->previous->next = me->next;
	if (me->next != NULL)
		me->next->previous = me->previous;
	
	// update owner
	bool onlyObject = me->owner->firstObject == me && me->owner->lastObject == me;
	if (onlyObject)
		me->owner->firstObject = me->owner->lastObject = NULL;
	else if (me->owner->lastObject == me)
		me->owner->lastObject = me->previous;
	else if (me->owner->firstObject == me)
		me->owner->firstObject = me->next;	
	
	assignOwner(me, newOwner);
}

void freeAll(DynamicObjOwner* owner)
{
	for (struct DynamicObjectList *obj = owner->firstObject, *next = NULL;
		 obj != NULL; obj = next)
	{
		next = obj->next;
		free(obj);
	}
}

DynamicObjOwner* getOwner(void* object)
{
	struct DynamicObjectList* me = ((struct DynamicObjectList*)object) - 1;
	return me->owner;
}

size_t getSize(void* object)
{
	struct DynamicObjectList* me = ((struct DynamicObjectList*)object) - 1;
	return me->size;
}