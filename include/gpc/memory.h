/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stddef.h>

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define these macros before including this header in case of namespacing issues. 
#if !defined(GPC_MEMORY_NAMESPACING) && !defined(GPC_NAMESPACING)

// Every dynamic object needs to be assigned to an owner on creation.
// Owners should ALWAYS be zero initialized. NEW_OWNER() macro is recommended to
// create a zero initialized pointer on stack safely. 
// Free all allocated resources owned by owner with freeAll().
typedef struct gpc_DynamicObjOwner DynamicObjOwner;

// New zero initialized owner on stack
#define NEW_OWNER(name)						GPC_NEW_OWNER(name)

// Allocate zero initialized memory on stack and assign ownership
// Size will be sizeof(type) and capacity will be nextPowerOf2(sizeof(type)).
#define newS(type, owner)					gpc_newS(type, owner)

// Allocate zero initialized memory on heap and assign ownership
// size will be sizeof(type) and capacity will be nextPowerOf2(sizeof(type)).
#define newH(type, owner)					gpc_newH(type, owner)

// Allocate memroy on stack and assign ownership
// Memory will be zeroed but size will be 0.
// Returns NULL on failure.
#define allocaAssign(capacity, owner)		gpc_allocaAssign(capacity, owner)

// malloc and assign ownership
#define mallocAssign(capacity, owner)		gpc_mallocAssign(capacity, owner)

// calloc and assign ownership
// Memory will be zeroed but size will be 0.
// Returns NULL on failure.
#define callocAssign(nmemb, size, owner)	gpc_callocAssign(nmemb, size, owner)

// Reallocate object
// Returns pointer to newly allocated block and NULL on failure. 
#define reallocate(object, newCapacity)		gpc_reallocate(object, newCapacity)

// Assign a new owner
#define moveOwnership(object, newOwner)		gpc_moveOwnership(object, newOwner)

// Frees every heap allocated object owned by owner
// Does nothing if owner only has objects on stack or no objects at all.
#define freeAll(owner)						gpc_freeAll(owner)

// Returns pointer to object's owner
#define getOwner(object)					gpc_getOwner(object)

// Gets size of object excluding it's metadata
#define getSize(object)						gpc_getSize(object)

// Sets size of object excluding it's metadata
// Reallocates if newSize exceeds size of block allocated for object.
#define setSize(object, newSize)			gpc_setSize(object, newSize)

// Gets size of memory block allocated for object
#define getCapacity(object)					gpc_getCapacity(object)

// Sets size of memory block allocated for object
// Reallocates if newCapacity exceeds size of block allocated for object.
#define setCapacity(object, newCapacity)	gpc_setCapacity(object, newCapacity)

// Returns a copy of object on heap
#define duplicate(object)					gpc_duplicate(object)

// Returns true if object is allocated on stack, false otherwise
#define onStack(object)						gpc_onStack(object)

// Returns true if object is allocated on heap, false otherwise
#define onHeap(object)						gpc_onHeap(object)

#endif // GPC_NAMESPACING ----------------------------------------------------

typedef struct gpc_DynamicObjOwner gpc_DynamicObjOwner;

#define GPC_NEW_OWNER(name) gpc_DynamicObjOwner* const name = &(DynamicObjOwner){};

#define gpc_newS(type, owner)																\
	gpc_buildStackObject(																	\
		(uint8_t[sizeof(struct gpc_DynamicObjectList) + gpc_nextPowerOf2(sizeof(type))]){},	\
		sizeof(type),																		\
		gpc_nextPowerOf2(sizeof(type)),														\
		owner)

#define gpc_newH(type, owner) gpc_buildHeapObject(sizeof(type), owner)

#define gpc_allocaAssign(capacity, owner)													\
	gpc_buildStackObject(																	\
		(uint8_t[sizeof(struct gpc_DynamicObjectList) + gpc_nextPowerOf2(capacity)]){},		\
		0,																					\
		gpc_nextPowerOf2(capacity),															\
		owner)

[[nodiscard]] void* gpc_mallocAssign(size_t, gpc_DynamicObjOwner*);

[[nodiscard]] void* gpc_callocAssign(size_t nmemb, size_t size, gpc_DynamicObjOwner*);

[[nodiscard]] void* gpc_reallocate(void* object, size_t newCapacity);
#define gpc_reallocate(object, newSize) ((object) = gpc_reallocate(object, newSize))

void gpc_moveOwnership(void* object, gpc_DynamicObjOwner* newOwner);

void gpc_freeAll(gpc_DynamicObjOwner* owner);

gpc_DynamicObjOwner* gpc_getOwner(void* object);

size_t gpc_getSize(void* object);

[[nodiscard]] void* gpc_setSize(void* object, size_t newSize);
#define gpc_setSize(object, newSize) ((object) = gpc_setSize(object, newSize))

size_t gpc_getCapacity(void* object);

[[nodiscard]] void* gpc_setCapacity(void* object, size_t newCapacity);
#define gpc_setCapacity(object, newCapacity) ((object) = gpc_setCapacity(object, newCapacity))

[[nodiscard]] void* gpc_duplicate(void* object);

bool gpc_onStack(void* object);
bool gpc_onHeap(void* object);

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

// Rounds n up to next power of 2
#define gpc_nextPowerOf2(n) (((n) == 0) ? 1 : (1 << (64 - __builtin_clzll((n) - 1))))

// Returns pointer to object with address buffer+sizeof(gpc_DynamicObjectList)
// Make sure that buffer is at least large enough to contain
// gpc_DynamicObjectList and object. 
// buffer will NOT be freed by freeAll(). This function is meant for stack 
// allocated objects. 
void* gpc_buildStackObject(void* buffer,
						   size_t objectSize,
						   size_t capacity,
						   gpc_DynamicObjOwner*);

// Returns pointer to heap allocated object with capacity of nextPowerOf2(size)
void* gpc_buildHeapObject(size_t size, gpc_DynamicObjOwner*);

// Heap allocated objects are stored in a linked list. freeAll() frees all 
// objects in the list. Modifying the list manually will most likely cause a 
// memory leak or crash so it's adviseable to only use functions provided by the
// core API to interact with dynamic objects. 
struct gpc_DynamicObjectList
{
	// If previous and next is NULL and owner->firstObject and owner->lastObject
	// does not point to self then object is stack allocated. 
	struct gpc_DynamicObjectList* previous;
	struct gpc_DynamicObjectList* next;
	
	// Owner is required even for stack allocated objects so they can be 
	// assigned properly if capacity needs to be exceeded.
	gpc_DynamicObjOwner* owner;
	
	size_t size;
	size_t capacity;
};

// When freeAll(owner) is called, all heap allocated objects in list pointed by
// owner->firstObject will be freed. Modifying owner manually will most likely
// cause a memory leak or crash so it's adviseable to only use functions
// provided by the core API to interact with dynamic objects. 
typedef struct gpc_DynamicObjOwner
{
	struct gpc_DynamicObjectList* firstObject;
	struct gpc_DynamicObjectList* lastObject;
} gpc_DynamicObjOwner;

#endif // GPC_MEMORY_H