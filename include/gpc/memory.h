// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_MEMORY_H
#define GPC_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "attributes.h"

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header in case of
// namespacing issues to require gpc_ prefix for identifiers. 
#if !defined(GPC_MEMORY_NAMESPACING) && !defined(GPC_NAMESPACING)

// Every dynamic object needs to be assigned to an owner on creation.
// Owners should ALWAYS be zero initialized. NEW_OWNER() macro is recommended to
// create a zero initialized pointer on stack safely. 
// Free all allocated resources owned by owner with freeAll().
typedef struct gpc_Owner Owner;

// Pointer to owner created with newOwner(). Pointer to owner created earlier 
// with newOwner() can be obtained with defaultOwner->parent. 
// Don't write values to it yourself!
#define gDefaultOwner							gpc_gDefaultOwner

// Returns a pointer to a new block scoped owner
#define newOwner()								gpc_newOwner()

// Allocate zero initialized memory on stack and assign ownership to the last 
// owner created with newOwner().
// size and capacity will be sizeof(type).
// Returns newly created object. 
#define newS(type,/*initVal=0*/...)				gpc_newS(type, __VA_ARGS__)

// Allocate zero initialized memory on heap and assign ownership to the last 
// owner created with newOwner().
// size will be sizeof(type) and capacity will be nextPowerOf2(sizeof(type)).
// Returns newly created object. 
#define newH(type,/*initVal=0*/...)				gpc_newH(type, __VA_ARGS__)

// Allocate zero initialized memory on stack and assign ownership to be the last
// owner created with newOwner().
// size will be 0. 
#define allocS(capacity)						gpc_allocS(capacity)

// Allocate zero initialized memory on heap and assign ownership to be the last
// owner created with newOwner().
// size will be 0. 
#define allocH(capacity)						gpc_allocH(capacity)

// Reallocate object
// Returns pointer to newly allocated block and NULL on failure. 
// Does nothing if newCapacity<=getCapacity(object).
#define reallocate(object, newCapacity)			gpc_reallocate(object, newCapacity)

// Assign a new owner
#define moveOwnership(object, newOwner)			gpc_moveOwnership(object, newOwner)

// Frees every heap allocated object owned by owner. Frees last owner created 
// with newOwner() if owner is NULL. 
// Does nothing if owner only has objects on stack or no objects at all.
#define freeOwner(owner)						gpc_freeOwner(owner)

// Allocate memory on stack and assign ownership
// owner will be the last one created with newOwner() if NULL
// Memory will be zeroed and size will be 0 and NULL on failure.
#define allocaAssign(capacity, owner)			gpc_allocaAssign(capacity, owner)

// malloc and assign ownership
// owner will be the last one created with newOwner() if NULL
// Returns pointer to an object of size 0 and NULL on failure.
#define mallocAssign(capacity, owner)			gpc_mallocAssign(capacity, owner)

// calloc and assign ownership
// owner will be the last one created with newOwner() if NULL
// Memory will be zeroed and size will be 0.
// Returns NULL on failure.
#define callocAssign(nmemb, size, owner)		gpc_callocAssign(nmemb, size, owner)

// Returns pointer to object's owner
#define getOwner(object)						gpc_getOwner(object)

// Gets size of object excluding it's metadata
#define getSize(object)							gpc_getSize(object)

// Sets size of object excluding it's metadata. Rerturns object. 
// Reallocates if newSize exceeds size of block allocated for object. This 
// means that the return value has to be stored to object. 
// Does nothing if newSize == getSize(object).
#define setSize(object, newSize)				gpc_setSize(object, newSize)

// Gets size of memory block allocated for object
#define getCapacity(object)						gpc_getCapacity(object)

// Returns a copy of object on heap
#define duplicate(object)						gpc_duplicate(object)

// Returns true if object is allocated on stack, false otherwise
#define onStack(object)							gpc_onStack(object)

// Returns true if object is allocated on heap, false otherwise
#define onHeap(object)							gpc_onHeap(object)

#endif // GPC_NAMESPACING ----------------------------------------------------

typedef struct gpc_Owner gpc_Owner;

extern GPC_THREAD_LOCAL gpc_Owner* gpc_gDefaultOwner;

#define gpc_newOwner() gpc_registerOwner(&(gpc_Owner){0})

#define gpc_newH(type, ...)	\
	gpc_buildHeapObject(sizeof((type){__VA_ARGS__}), &(type){__VA_ARGS__}, NULL)

#define gpc_newS(type, ...)											\
	gpc_buildObject(												\
		&(uint8_t[sizeof(struct gpc_ObjectList) + sizeof((type){__VA_ARGS__})]){0},\
		(struct gpc_ObjectList)										\
		{															\
			.owner = NULL,											\
			.size = sizeof((type){__VA_ARGS__}),					\
			.capacity = sizeof((type){__VA_ARGS__})					\
		}, &(type){__VA_ARGS__})

GPC_NODISCARD void* gpc_allocH(size_t capacity);

#define gpc_allocS(capacity) gpc_allocaAssign(capacity, NULL)

#define gpc_allocaAssign(_capacity, _owner)							\
	gpc_buildObject(												\
		&(uint8_t[sizeof(struct gpc_ObjectList) + _capacity]){0},	\
		(struct gpc_ObjectList)										\
		{															\
			.owner = _owner,										\
			.size = 0,												\
			.capacity = _capacity									\
		}, NULL)

GPC_NODISCARD void* gpc_mallocAssign(size_t, gpc_Owner*);

GPC_NODISCARD void* gpc_callocAssign(size_t nmemb, size_t size, gpc_Owner*);

GPC_NODISCARD void* gpc_reallocate(void* object, size_t newCapacity);

void gpc_freeOwner(gpc_Owner*);

void gpc_moveOwnership(void* object, gpc_Owner* newOwner);

gpc_Owner* gpc_getOwner(const void *const object);

size_t gpc_getSize(const void *const object);

GPC_NODISCARD void* gpc_setSize(void* object, size_t newSize);

size_t gpc_getCapacity(const void *const object);

GPC_NODISCARD void* gpc_duplicate(const void *const object);

bool gpc_onStack(const void *const object);

bool gpc_onHeap(const void *const object);

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

// Heap allocated objects are stored in a linked list. freeAll() frees all 
// objects in the list. Modifying the list manually will most likely cause a 
// memory leak or crash so it's adviseable to only use functions provided by the
// core API to interact with dynamic objects. 
struct gpc_ObjectList
{
	// If previous and next is NULL and owner->firstObject and owner->lastObject
	// does not point to self then object is stack allocated. 
	struct gpc_ObjectList* previous;
	struct gpc_ObjectList* next;
	
	// Owner is required even for stack allocated objects so they can be 
	// assigned properly if capacity needs to be exceeded.
	gpc_Owner* owner;
	
	size_t size;
	size_t capacity;
};

// Copies object and it's data to outBuf so the object can be used with dynamic
// functionality. 
// Returns pointer to object with address outBuf+sizeof(gpc_ObjectList).
void* gpc_buildObject(void* outBuf, const struct gpc_ObjectList, const void* obj);

// Allocates memory and returns a pointer to an object with capacity of
// nextPowerOf2(size). 
GPC_NODISCARD void* gpc_buildHeapObject(const size_t size, const void* obj, gpc_Owner*);

// When freeAll(owner) is called, all heap allocated objects in list pointed by
// owner->firstObject will be freed. Modifying owner manually will most likely
// cause a memory leak or crash so it's adviseable to only use functions
// provided by the core API to interact with dynamic objects. 
typedef struct gpc_Owner
{
	struct gpc_ObjectList* firstObject;
	struct gpc_ObjectList* lastObject;
	struct gpc_Owner* parent; // for default owner
	// TODO allocator
} gpc_Owner;

// Registers owner to gpc_globalOwnerList to be the default owner. 
// Returns self. 
gpc_Owner* gpc_registerOwner(gpc_Owner*);

#endif // GPC_MEMORY_H
