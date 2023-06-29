/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stddef.h>

typedef struct DynamicObjOwner DynamicObjOwner;

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// New owner on stack
#define NEW_OWNER(name) DynamicObjOwner* const name = &(DynamicObjOwner){};

// malloc and assign ownership
// Returns NULL on failure
[[nodiscard]] void* mallocAssign(size_t, DynamicObjOwner*);

// calloc and assign ownership
// Returns NULL on failure
[[nodiscard]] void* callocAssign(size_t nmemb, size_t size, DynamicObjOwner*);

// Reallocate object
// Returns pointer to newly allocated block
// Returns NULL on failure
[[nodiscard]] void* reallocate(void* object, size_t newSize);
#define reallocate(object, newSize) ((object) = reallocate(object, newSize))

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
[[nodiscard]] void* setSize(void* object, size_t newSize);
#define setSize(object, newSize) ((object) = setSize(object, newSize))

// Gets size of memory block allocated for object
size_t getCapacity(void* object);

// Sets size of memory block allocated for object
// Reallocates if newCapacity exceeds size of block allocated for object
[[nodiscard]] void* setCapacity(void* object, size_t newCapacity);
#define setCapacity(object, newCapacity) ((object) = setCapacity(object, newCapacity))

// Returns a copy of object on heap
[[nodiscard]] void* duplicate(void* object);

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

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

#endif // GPC_MEMORY_H