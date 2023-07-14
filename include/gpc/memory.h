/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

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

// Returns a pointer to a new block scopen owner
#define newOwner()							gpc_newOwner()

// Allocate zero initialized memory on stack and assign ownership
// If owner is not passed then it will be the last one created with newOwner()
// size and capacity will be sizeof(type).
#define newS(/*type, owner=last*/...)		gpc_newS(__VA_ARGS__)

// Allocate zero initialized memory on heap and assign ownership
// If owner is not passed then it will be the last one created with newOwner()
// size will be sizeof(type) and capacity will be nextPowerOf2(sizeof(type)).
#define newH(/*type, owner=last*/...)		gpc_newH(__VA_ARGS__)

// Allocate zero initialized memory on stack and assign ownership
// If owner is not passed then it will be the last one created with newOwner()
// size will be 0. 
#define allocS(/*capacity, owner=last*/...)	gpc_allocS(__VA_ARGS__)

// Allocate zero initialized memory on heap and assign ownership
// If owner is not passed then it will be the last one created with newOwner()
// size will be 0. 
#define allocH(/*capacity, owner=last*/...)	gpc_allocH(__VA_ARGS__)

// Reallocate object
// Returns pointer to newly allocated block and NULL on failure. 
// Does nothing if newCapacity<=getCapacity(object).
#define reallocate(object, newCapacity)		gpc_reallocate(object, newCapacity)

// Assign a new owner
#define moveOwnership(object, newOwner)		gpc_moveOwnership(object, newOwner)

// Frees last owner created with newOwner()
#define freeLastOwner()						gpc_freeLastOwner()

// Frees every heap allocated object owned by owner
// Does nothing if owner only has objects on stack or no objects at all.
#define freeOwner(owner)					gpc_freeOwner(owner)

// Allocate memory on stack
// Owner will be the last one created by newOwner()
// Memory will be zeroed and size will be 0. 
#define allocateS(capacity)					gpc_allocateS(capacity)

// Allocate memory on stack and assign ownership
// Memory will be zeroed and size will be 0. 
#define allocaAssign(capacity, owner)		gpc_allocaAssign(capacity, owner)

// malloc
// Owner will be the last one created by newOwner()
#define mallocate(capacity)					gpc_mallocate(capacity)

// malloc and assign ownership
// Returns pointer to an object of size 0.
#define mallocAssign(capacity, owner)		gpc_mallocAssign(capacity, owner)

// calloc
// Owner will be the last one created by newOwner()
#define callocate(nmemb, size)				gpc_callocate(nmemb, size)

// calloc and assign ownership
// Memory will be zeroed and size will be 0.
// Returns NULL on failure.
#define callocAssign(nmemb, size, owner)	gpc_callocAssign(nmemb, size, owner)

// Returns pointer to object's owner
#define getOwner(object)					gpc_getOwner(object)

// Returns pointer to default owner
#define getDefaultOwner()					gpc_getDefaultOwner()

// Gets size of object excluding it's metadata
#define getSize(object)						gpc_getSize(object)

// Sets size of object excluding it's metadata
// Reallocates if newSize exceeds size of block allocated for object.
#define setSize(object, newSize)			gpc_setSize(object, newSize)

// Gets size of memory block allocated for object
#define getCapacity(object)					gpc_getCapacity(object)

// Sets size of memory block allocated for object
// Reallocates if newCapacity exceeds size of block allocated for object.
// Does nothing if newCapacity<=getCapacity(object)
#define setCapacity(object, newCapacity)	gpc_setCapacity(object, newCapacity)

// Returns a copy of object on heap
#define duplicate(object)					gpc_duplicate(object)

// Returns true if object is allocated on stack, false otherwise
#define onStack(object)						gpc_onStack(object)

// Returns true if object is allocated on heap, false otherwise
#define onHeap(object)						gpc_onHeap(object)

#endif // GPC_NAMESPACING ----------------------------------------------------

typedef struct gpc_Owner gpc_Owner;

#define gpc_newOwner() gpc_registerOwner(&(gpc_Owner){0})

#define gpc_newH1(type) gpc_newH3(type, 0, NULL)
#define gpc_newH2(type, initValue) gpc_newH3(type, initValue, NULL)
#define gpc_newH3(type, initValue, owner) gpc_buildHeapObject(sizeof(type),&(type){initValue},owner)
#define gpc_newH(...) GPC_OVERLOAD(3, __VA_ARGS__, gpc_newH3, gpc_newH2, gpc_newH1)(__VA_ARGS__)

#define gpc_newS1(type) gpc_newS3(type, 0, NULL)
#define gpc_newS2(type, initValue) gpc_newS3(type, initValue, NULL)
#define gpc_newS3(type, initValue, _owner)							\
	gpc_buildObject(												\
		&(uint8_t[sizeof(struct gpc_ObjectList) + sizeof(type)]){0},\
		(struct gpc_ObjectList)										\
		{															\
			.owner = _owner,										\
			.size = sizeof(type),									\
			.capacity = sizeof(type)								\
		}, &(type){initValue})
#define gpc_newS(...) GPC_OVERLOAD(3,__VA_ARGS__,gpc_newS3,gpc_newS2,gpc_newS1)(__VA_ARGS__)

#define gpc_allocS(...) GPC_OVERLOAD(2,__VA_ARGS__,gpc_allocaAssign,gpc_allocateS)(__VA_ARGS__)

#define gpc_allocH(...) GPC_OVERLOAD(2,__VA_ARGS__,gpc_callocAssign,gpc_callocate)(1,__VA_ARGS__)

#define gpc_allocateS(capacity) gpc_allocaAssign(capacity, NULL)

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
GPC_NODISCARD void* gpc_mallocate(size_t);
GPC_NODISCARD void* gpc_callocAssign(size_t nmemb, size_t size, gpc_Owner*);
GPC_NODISCARD void* gpc_callocate(size_t nmemb, size_t size);
GPC_NODISCARD void* gpc_reallocate(void* object, size_t newCapacity);

void gpc_moveOwnership(void* object, gpc_Owner* newOwner);

void gpc_freeOwner(gpc_Owner*);

void gpc_freeLastOwner(void);

gpc_Owner* gpc_getOwner(void* object);

size_t gpc_getSize(void* object);

GPC_NODISCARD void* gpc_setSize(void* object, size_t newSize);

size_t gpc_getCapacity(void* object);

GPC_NODISCARD void* gpc_setCapacity(void* object, size_t newCapacity);

GPC_NODISCARD void* gpc_duplicate(void* object);

bool gpc_onStack(void* object);
bool gpc_onHeap(void* object);

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
gpc_Owner* gpc_registerOwner(gpc_Owner*);

#endif // GPC_MEMORY_H
