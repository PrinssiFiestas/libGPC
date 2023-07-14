// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include <stdint.h>
#include "memory.h"
#include "attributes.h"

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header in case of
// namespacing issues to require gpc_ prefix for identifiers. 
#if !defined(GPC_ARRAY_NAMESPACING) && !defined(GPC_NAMESPACING)

// Returns the number of elements in an array
#define arrLength(arr)							gpc_arrLength(arr)

// Set number of elements
// Reallocates if newLen>arrCapacity(arr).
#define arrSetLength(arr, newLen)				gpc_arrSetLength(arr, newLen)

// Returns the number of elements that arr can hold without reallocating
#define arrCapacity(arr)						gpc_arrCapacity(arr)

// Set number of elements arr can hold
// Reallocates if newCap>arrCapacity(arr).
// Does nothing if newCap<=arrCapacity(arr).
#define arrSetCapacity(arr, newCap)				gpc_arrSetCapacity(arr, newCap)

// Returns the last element
#define arrLast(arr)							gpc_arrLast(arr)

// Returns a pointer to the last element
#define arrBack(arr)							gpc_arrBack(arr)

// Returns true if arrLength(arr)==0
#define arrIsEmpty(arr)							gpc_arrIsEmpty(arr)

// Add an element to the back of arr
#define arrPush(arr, element)					gpc_arrPush(arr, element)

// Add an array to the back of arrDestination
#define arrPushArr(arrDestination, arr)			gpc_arrPushArr(arrDestination, arr)

// Remove n elements from the back of arr
#define arrPop(arr, nElements)					gpc_arrPop(arr, nElements)

// Insert an element in index pos of arr
#define arrInsert(arr, pos, element)			gpc_arrInsert(arr, pos, element)

// Insert an array in index pos of arr
#define arrInsertArr(arrDestination,pos,arr)	gpc_arrInsertArr(arrDestination,pos,arr)

// Delete n elements from index pos of arr
#define arrDelete(arr, pos, nElements)			gpc_arrDelete(arr, pos, nElements)

// Returns number of elements in a C array declared with []
// Don't use for pointers or array types from this library!
#define carrLength(arr) 						gpc_carrLength(arr)

#endif // GPC_NAMESPACING ----------------------------------------------------

#define gpc_arrLength(arr)						gpc_getSize(arr)/sizeof((arr)[0])

#define gpc_arrSetLength(arr, newLen)			gpc_setSize(arr, newLen * sizeof((arr)[0]))

#define gpc_arrCapacity(arr)					gpc_getCapacity(arr)/sizeof((arr)[0])

#define gpc_arrSetCapacity(arr, newCap)			gpc_setCapacity(arr, newCap * sizeof((arr)[0]))

#define gpc_arrLast(arr)						((arr)[gpc_getSize(arr)/sizeof((arr)[0]) - 1])

#define gpc_arrBack(arr)						(&(arr)[gpc_getSize(arr)/sizeof((arr)[0]) - 1])

#define gpc_arrIsEmpty(arr)						(gpc_getSize(arr) == 0)

//#define gpc_arrPush(arr, elem)					gpc_arrPushElement(arr, sizeof(arr[0]), elem)
#define gpc_arrPush(arr, elem)	\
	(((arr) = gpc_arrSetLength(gpc_arrLength(arr)))[gpc_arrLength(arr)] = (elem))

#define gpc_arrPushArr(arrDest, arr)			gpc_arrPushArray(arrDest, sizeof(arr[0]), arr)

#define gpc_arrPop(arr, nElems)					goc_arrPopElements(arr, sizeof(arr[0]), nElems)

#define gpc_arrInsert(arr, pos, elem)			gpc_arrInsertElement(arr,sizeof *arr,pos,elem)

#define gpc_arrInsertArr(arrDest, pos, arr)		gpc_arrInsertArray(arrDest,sizeof *arr,pos,arr)

#define gpc_arrDelete(arr, pos, nElems)			gpc_arrDeleteElements(arr,sizeof *arr,pos,nElems)

#define gpc_carrLength(arr) 					(sizeof(arr)/sizeof(arr[0]))

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

#endif // GPC_ARRAY_H
