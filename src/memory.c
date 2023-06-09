/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdlib.h>
#include "../include/gpc/memory.h"

#ifdef TESTS
#define malloc(size)		test_malloc(size)
#define free(p)				test_free(p)
#define calloc(size, t)		test_calloc(size, t)
#define realloc(p, size)	test_realloc(p, size)
void* test_malloc(size_t);
void  test_free(void*);
void* test_calloc(size_t, size_t);
void* test_realloc(void*, size_t);
#endif

struct DynamicObjectList
{
	struct DynamicObjectList* previous;
	struct DynamicObjectList* next;
	DynamicObjOwner* owner;
};

void* mallocAssign(size_t size, DynamicObjOwner* owner)
{
	struct DynamicObjectList* p = malloc(sizeof(struct DynamicObjectList) + size);
	
	if (owner->firstObject == NULL)
	{
		owner->firstObject = owner->lastObject = p;
	}
	else
	{
		p->owner = owner;
		p->previous = owner->lastObject;
		owner->lastObject->next = p;
		p->next = NULL;
		owner->lastObject = p;
	}
	return (void*)((char*)p + sizeof(struct DynamicObjectList));
}

// void moveOwnership(void* object, DynamicObjOwner* newOwner)
// {
// }

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
