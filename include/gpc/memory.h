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

// Define these macros before including this header in case of namespacing problems. 
#if !defined(GPC_MEMORY_NAMESPACING) && !defined(GPC_NAMESPACING)

// Every dynamic object needs to be assigned to an owner on creation.
// Owners should ALWAYS be zero initialized
// NEW_OWNER() macro is recommended to create a zero initialized pointer on stack
// Use freeAll() for each owner
// freeAll() is always safe even when there's no objects on heap or at all
typedef struct gpc_DynamicObjOwner DynamicObjOwner;

// New zero initialized owner on stack
#define NEW_OWNER(name)						GPC_NEW_OWNER(name)

// malloc and assign ownership
// Returns NULL on failure
#define mallocAssign(size, owner)			gpc_mallocAssign(size, owner)

// calloc and assign ownership
// Returns NULL on failure
#define callocAssign(nmemb, size, owner)	gpc_callocAssign(nmemb, size, owner)

// Reallocate object
// Returns pointer to newly allocated block
// Returns NULL on failure
#define reallocate(object, newSize)			gpc_reallocate(object, newSize)

// Assign a new owner
#define moveOwnership(object, newOwner)		gpc_moveOwnership(object, newOwner)

// Frees every heap allocated object owned by owner
// Does nothing if owner only has objects on stack or no objects at all
#define freeAll(owner)						gpc_freeAll(owner)

// Returns pointer to object's owner
#define getOwner(object)					gpc_getOwner(object)

// Gets size of object excluding it's metadata
#define getSize(object)						gpc_getSize(object)

// Sets size of object excluding it's metadata
// Reallocates if 'newSize' exceeds size of block allocated for object
#define setSize(object, newSize)			gpc_setSize(object, newSize)

// Gets size of memory block allocated for object
#define getCapacity(object)					gpc_getCapacity(object)

// Sets size of memory block allocated for object
// Reallocates if newCapacity exceeds size of block allocated for object
#define setCapacity(object, newCapacity)	gpc_setCapacity(object, newCapacity)

// Returns a copy of object on heap
#define duplicate(object)					gpc_duplicate(object)

#endif // GPC_NAMESPACING ----------------------------------------------------

typedef struct gpc_DynamicObjOwner gpc_DynamicObjOwner;

#define GPC_NEW_OWNER(name) gpc_DynamicObjOwner* const name = &(DynamicObjOwner){};

[[nodiscard]] void* gpc_mallocAssign(size_t, gpc_DynamicObjOwner*);

[[nodiscard]] void* gpc_callocAssign(size_t nmemb, size_t size, gpc_DynamicObjOwner*);

[[nodiscard]] void* gpc_reallocate(void* object, size_t newSize);
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

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------


enum gpc_MemoryFailBehaviour
{
	GPC_MEM_FAIL_ABORT,				// aborts execution with error message
	GPC_MEM_FAIL_RETURN_NULL,		// allows user to handle failure
	
	GPC_MEM_FAIL_SIZE				// for internal use
};

// Sets behaviour on failed malloc(), calloc(), and realloc()
// Default behaviour is GPC_MEM_FAIL_ABORT
void gpc_setMemoryFailBehaviour(enum gpc_MemoryFailBehaviour);

typedef struct gpc_DynamicObjOwner
{
	struct gpc_DynamicObjectList* firstObject;
	struct gpc_DynamicObjectList* lastObject;
} gpc_DynamicObjOwner;

struct gpc_DynamicObjectList
{
	// previous and next being NULL indicates stack allocated object
	struct gpc_DynamicObjectList* previous;
	struct gpc_DynamicObjectList* next;
	
	// Owner is required even for stack allocated objects so they can be 
	// assigned properly if capacity needs to be exceeded
	gpc_DynamicObjOwner* owner;
	
	size_t size;
	size_t capacity;
};

#endif // GPC_MEMORY_H