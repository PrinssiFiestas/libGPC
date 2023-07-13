/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include <stdint.h>
#include "memory.h"

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header in case of
// namespacing issues to require gpc_ prefix for identifiers. 
#if !defined(GPC_ARRAY_NAMESPACING) && !defined(GPC_NAMESPACING)

// Creates an empty array on stack that can be used with dynamic functionality.
// 
#define STACK_ARRAY(type, name, capacity, owner) GPC_STACK_ARRAY(type,name,capacity,owner)

// Returns new array on stack with dynamic functionality
#define arrNewS(type, capacity, owner)			gpc_arrNewS(type, capacity, owner)
#define arrNewH(type, capacity, owner)			gpc_arrNewH(type, capacity, owner)
#define arrLength(arr)							gpc_arrLength(arr)
#define arrSetLength(arr, newLen)				gpc_arrSetLength(arr, newLen)
#define arrCapacity(arr)						gpc_arrCapacity(arr)
#define arrSetCapacity(arr, newCap)				gpc_arrSetCapacity(arr, newCap)
#define arrLast(arr)							gpc_arrLast(arr)
#define arrBack(arr)							gpc_arrBack(arr)
#define arrIsEmpty(arr)							gpc_arrIsEmpty(arr)
#define arrPush(arr, element)					gpc_arrPush(arr, element)
#define arrPushArr(arrDestination, arr)			gpc_arrPushArr(arrDestination, arr)
#define arrPop(arr, nElements)					gpc_arrPop(arr, nElements)
#define arrInsert(arr, pos, element)			gpc_arrInsert(arr, pos, element)
#define arrInsertArr(arrDestination,pos,arr)	gpc_arrInsertArr(arrDestination,pos,arr)
#define arrDelete(arr, pos, nElements)			gpc_arrDelete(arr, pos, nElements)

// Returns length of a C array declared with []
// Don't use for pointers or array types from this library!
#define carrLength(arr) 						gpc_carrLength(arr)

#endif // GPC_NAMESPACING ----------------------------------------------------

#define GPC_STACK_ARRAY(type, name, capacity, owner)						\
	uint8_t _##type##_##name##_##capacity									\
	[sizeof(struct ObjectList) + (capacity) * sizeof(type)];				\
	*(struct ObjectList*)_##type##_##name##_##capacity =					\
	(struct ObjectList) { .owner = owner, .size = 0, .capacity = capacity };\
	type* name = & _##type##_##name##_##capacity [sizeof(struct ObjectList)];

#define gpc_arrNewS(type, capacity, owner)		gpc_newS(type[capacity], owner)
#define gpc_arrNewH(type, capacity, owner)		gpc_newH(type[capacity], owner)
#define gpc_arrLength(arr)						gpc_getSize(arr)/sizeof((arr)[0])
#define gpc_arrSetLength(arr, newLen)			gpc_setSize(arr, newLen * sizeof((arr)[0]))
#define gpc_arrCapacity(arr)					gpc_getCapacity(arr)/sizeof((arr)[0])
#define gpc_arrSetCapacity(arr, newCap)			gpc_setCapacity(arr, newCap * sizeof((arr)[0]))
#define gpc_arrLast(arr)						((arr)[gpc_getSize(arr)/sizeof((arr)[0]) - 1])
#define gpc_arrBack(arr)						(&(arr)[gpc_getSize(arr)/sizeof((arr)[0]) - 1])
#define gpc_arrIsEmpty(arr)						(gpc_getSize(arr) == 0)
#define gpc_arrPush(arr, element)
#define gpc_arrPushArr(arrDest, arr)
#define gpc_arrPop(arr, nElements)
#define gpc_arrInsert(arr, pos, element)
#define gpc_arrInsertArr(arrDest, pos, arr)
#define gpc_arrDelete(arr, pos, nElements)

#define gpc_carrLength(arr) 				(sizeof(arr)/sizeof(arr[0]))

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

#endif // GPC_ARRAY_H
