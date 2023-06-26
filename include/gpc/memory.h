/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stddef.h>
#include "overload.h"

typedef struct DynamicObjOwner
{
	struct DynamicObjectList* firstObject;
	struct DynamicObjectList* lastObject;
} DynamicObjOwner;

struct DynamicObjectList
{
	// previous and next being NULL indicates stack allocated object
	struct DynamicObjectList* previous;
	struct DynamicObjectList* next;
	
	// Owner is required even for stack allocated objects so they can be 
	// assigned properly if capacity needs to be exceeded
	DynamicObjOwner* owner;
	
	size_t size;
	size_t capacity;
};

// malloc and assign ownership
void* mallocAssign(size_t, DynamicObjOwner*);
// calloc and assign ownership
void* callocAssign(size_t, DynamicObjOwner*);
// reallocate object
void* reallocate(void* object, size_t);
// Prevent freeing by end or freeAll()
void moveOwnership(void* object, DynamicObjOwner* newOwner);
// Frees every object owned by owner
void freeAll(DynamicObjOwner* owner);
// Self explanatory
DynamicObjOwner* getOwner(void* object);
// Gets size of object excluding it's metadata
size_t getSize(void* object);

void* setSize(void* object);

size_t getCapacity(void* object);

void* setCapacity(void* object);

#endif // GPC_MEMORY_H