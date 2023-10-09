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
//        CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header to enable short names
// without the gpc_ prefix. 
#if defined(GPC_MEMORY_NAMESPACE) || defined(GPC_NAMESPACE)

// Memory handler class
typedef struct gpc_Owner Owner;

// Creates a block scoped Owner
// Newly created owner will be registered to be the owner of all objects created
// with subsequent calls to newS(), newH(), allocS(), allocH(), and duplicate().
// Returns pointer to newly created owner. 
#define newOwner() \
/* Owner* */ gpc_newOwner()

// Allocate zero initialized memory on stack and assign ownership to the last 
// owner created with newOwner().
// size and capacity will be sizeof(type).
// type must be char[] for C strings. 
// Returns newly created object.
#define newS(type, ...) \
/* void* */ gpc_newS(type,/* initVals=0 */__VA_ARGS__)

// Allocate zero initialized memory on heap and assign ownership to the last 
// owner created with newOwner().
// size will be sizeof(type) and capacity will be nextPowerOf2(sizeof(type)).
// type must be char[] for C strings. 
// Returns newly created object.
#define newH(type,...) \
/* void* */ gpc_newH(type,/* initVals=0 */__VA_ARGS__)

// Allocate zero initialized memory on stack and assign ownership to be the last
// owner created with newOwner().
// size will be 0. 
#define allocS(capacity) \
/* void* */ gpc_allocS(/* size_t */ capacity)

// Allocate zero initialized memory on heap and assign ownership to be the last
// owner created with newOwner().
// size will be 0. 
#define allocH(capacity) \
/* void* */ gpc_allocH(/* size_t */ capacity)

// Frees owner and all of it's objects and returns returnObject
// If the owner of returnObject is owner being freed, it's ownership will be
// moved to owner->parent. If returnObject is on stack, it will be copied on
// heap. Registers owner->parent to be the memory handler. 
#define freeOwner(owner, returnObject)        \
/* void* */ gpc_freeOwner(/* Owner* */ owner, \
                          /* T*     */ returnObject)

// Gets size of object excluding it's metadata
#define size(object) \
/* size_t */ gpc_size(/* void* */ object)

// Sets size of object excluding it's metadata.
// Rerturns object and NULL on failure. 
// Reallocates if newSize exceeds size of block allocated for object. This 
// means that the return value has to be stored to object. 
// Does nothing if newSize == size(object).
#define resize(pobject, newSize)             \
/* void* */ gpc_resize(/* T**    */ pobject, \
                       /* size_t */ newSize)

// Gets size of memory block allocated for object
#define capacity(object) \
/* size_t */ gpc_capacity(/* T* */ object)

// Reallocate object
// Returns pointer to newly allocated block and NULL on failure. 
// Does nothing if newCapacity<=capacity(*pobject).
#define reserve(pobject, newCapacity)         \
/* void* */ gpc_reserve(/* T**    */ pobject, \
                        /* size_t */ newCapacity)

// Returns a copy of object on heap
#define duplicate(object) \
/* void* */ gpc_duplicate(/* T* */ object)

// Returns true if object is allocated on stack, false otherwise
#define onStack(object) \
/* bool */ gpc_onStack(/* T* */ object)

// Returns true if object is allocated on heap, false otherwise
#define onHeap(object) \
/* bool */ gpc_onHeap(/* T* */ object)

// Assign a new owner
#define moveOwnership(object, newOwner)           \
/* void */ gpc_moveOwnership(/* T*     */ object, \
                             /* Owber* */ newOwner)

// Allocate memory on stack and assign ownership
// owner will be the last one created with newOwner() if NULL
// Memory will be zeroed and size will be 0 and NULL on failure.
#define allocaAssign(capacity, owner)              \
/* void* */ gpc_allocaAssign(/* size_t */capacity, \
                             /* Owner* */owner)

// malloc and assign ownership
// owner will be the last one created with newOwner() if NULL
// Returns pointer to an object of size 0 and NULL on failure.
#define mallocAssign(capacity, owner)               \
/* void* */ gpc_mallocAssign(/* size_t */ capacity, \
                             /* Owner* */ owner)

// calloc and assign ownership
// owner will be the last one created with newOwner() if NULL
// Memory will be zeroed and size will be 0.
// Returns NULL on failure.
#define callocAssign(nmemb, size, owner)         \
/* void* */ gpc_callocAssign(/* size_t */ nmemb, \
                             /* size_t */ size,  \
                             /* Owner* */ owner) \

// Reallocate object
// Returns pointer to newly allocated block and NULL on failure. 
// Does nothing if newCapacity<=capacity(object).
#define reallocate(object, newCapacity)         \
/* void* */ gpc_reallocate(/* void*  */ object, \
                           /* size_t */ newCapacity)

// Returns pointer to object's owner
#define getOwner(object) \
/* Owner* */ gpc_getOwner(/* T* */object)

#endif // GPC_NAMESPACING ----------------------------------------------------

typedef struct gpc_Owner gpc_Owner;

typedef unsigned char gpc_Byte;

#define gpc_newOwner() gpc_registerOwner(&(gpc_Owner){0})

#define gpc_newH(type, ...) \
    gpc_buildHeapObject( \
        sizeof((type){__VA_ARGS__}), &(type){__VA_ARGS__}, NULL)

#define gpc_newS(type, ...) \
    (gpc_buildObject( \
        &(uint8_t[sizeof(struct gpc_Object) + sizeof((type){__VA_ARGS__})]){0},\
        (struct gpc_Object) \
        { \
            .owner = NULL, \
            .size = sizeof((type){__VA_ARGS__}), \
            .capacity = sizeof((type){__VA_ARGS__}) \
        }, &(type){__VA_ARGS__}))

GPC_NODISCARD void* gpc_allocH(size_t capacity);

#define gpc_allocS(capacity) gpc_allocaAssign(capacity, NULL)

#define gpc_allocaAssign(_capacity, _owner) \
    gpc_buildObject( \
        &(uint8_t[sizeof(struct gpc_Object) + _capacity]){0}, \
        (struct gpc_Object) \
        { \
            .owner = _owner, \
            .size = 0, \
            .capacity = _capacity \
        }, NULL)

GPC_NODISCARD void* gpc_mallocAssign(size_t, gpc_Owner*);

GPC_NODISCARD void* gpc_callocAssign(size_t nmemb, size_t size, gpc_Owner*);

GPC_NODISCARD void* gpc_reallocate(void* object, size_t newCapacity);

// When freeAll(owner) is called, all heap allocated objects in list pointed by
// owner->firstObject will be freed. Modifying owner manually will most likely
// cause a memory leak or crash so it's adviseable to only use functions
// provided by the core API to interact with dynamic objects. 
typedef struct gpc_Owner
{
    struct gpc_Object* firstObject;
    struct gpc_Object* lastObject;
    struct gpc_Owner* parent;
} gpc_Owner;

void* gpc_freeOwner(gpc_Owner owner[GPC_STATIC 1], void* returnObject);

void gpc_moveOwnership(void* object, gpc_Owner* newOwner);

gpc_Owner* gpc_getOwner(const void *const object);

size_t gpc_size(const void *const object);

inline void* gpc_ptrPass(void* p)
{
    return p;
}
// Tests if pointer is passed by reference. Compilation fails if PTR cant be
// dereferenced to a pointer. 
#define GPC_PTR_REF(PTR) (0 ? gpc_ptrPass(*(PTR)) : (PTR))

#define gpc_resize(pObject, newSize) \
    gpc_resizeObj(GPC_PTR_REF(pObject), (newSize))

void* gpc_resizeObj(void* pObject, size_t newSize);

size_t gpc_capacity(const void *const object);

#define gpc_reserve(pObject, newCapacity) \
    gpc_reserveObj(GPC_PTR_REF(pObject), (newCapacity))

void* gpc_reserveObj(void* pObject, size_t newCapacity);

GPC_NODISCARD void* gpc_duplicate(const void *const object);

bool gpc_onStack(const void *const object);

bool gpc_onHeap(const void *const object);

//----------------------------------------------------------------------------
//
//        END OF CORE API
//
//        Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

// Heap allocated objects are stored in a linked list. freeAll() frees all 
// objects in the list. Modifying the list manually will most likely cause a 
// memory leak or crash so it's adviseable to only use functions provided by the
// core API to interact with dynamic objects. 
struct gpc_Object
{
    // If previous and next is NULL and owner->firstObject and owner->lastObject
    // does not point to self then object is stack allocated. 
    struct gpc_Object* previous;
    struct gpc_Object* next;
    
    // Owner is required even for stack allocated objects so they can be 
    // assigned for correct owner capacity needs to be exceeded.
    gpc_Owner* owner;
    
    size_t size;
    size_t capacity;
};

// Copies object and it's data to outBuf so the object can be used with dynamic
// functionality. 
// Returns pointer to object with address outBuf+sizeof(gpc_Object).
GPC_NODISCARD void* gpc_buildObject(void* outBuf, const struct gpc_Object, const void* obj);

// Allocates memory and returns a pointer to an object with capacity of
// nextPowerOf2(size). 
GPC_NODISCARD void* gpc_buildHeapObject(const size_t size, const void* obj, gpc_Owner*);

// Registers owner to gpc_globalOwnerList to be the default owner. 
// Returns self. 
GPC_NODISCARD gpc_Owner* gpc_registerOwner(gpc_Owner*);

#endif // GPC_MEMORY_H
