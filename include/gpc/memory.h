/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stddef.h>

typedef struct DynamicObjOwner
{
	struct DynamicObjectList* firstObject;
	struct DynamicObjectList* lastObject;
} DynamicObjOwner;

// Use this macro to safely create a new owner on stack
#define NEW_OWNER(name) DynamicObjOwner* const name = &(DynamicObjOwner){};

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

// Reallocate object
// Returns pointer to newly allocated block
// 'object' will be mutated to new memory so return value can be ignored. 
// Can be used for stack allocated objects to reallocate them on heap. 
void* reallocate(void* object, size_t newSize);

// Assign a new owner
void moveOwnership(void* object, DynamicObjOwner* newOwner);

// Frees every heap allocated object owned by owner
void freeAll(DynamicObjOwner* owner);

// Returns pointer to object's owner
DynamicObjOwner* getOwner(void* object);

// Gets size of object excluding it's metadata
size_t getSize(void* object);

// Sets size of object excluding it's metadata
// Reallocates if 'newSize' exceeds size of block allocated for object
void* setSize(void* object, size_t newSize);

// Gets size of memory block allocated for 'object'
size_t getCapacity(void* object);

// Sets size of memory block allocated for 'object'
// Reallocates if 'newCapacity' exceeds size of block allocated for object
void* setCapacity(void* object, size_t newCapacity);

// Returns a copy of 'object'
void* duplicate(void* object);

#endif // GPC_MEMORY_H