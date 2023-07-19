// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include <string.h>
#include <stdint.h>
#include "memory.h"
#include "attributes.h"

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header to enable short names
// without the gpc_ prefix. 
#if defined(GPC_ARRAY_NAMESPACE) || defined(GPC_NAMESPACE)

// Returns the number of elements in an array
#define arrLength(arr)							gpc_arrLength(arr)

// Set number of elements
// Reallocates if newLen>arrCapacity(arr).
// Returns self. Store the return value to arr in case of reallocation. 
#define arrSetLength(arr, newLen)				gpc_arrSetLength(arr, newLen)

// Returns the number of elements that arr can hold without reallocating
#define arrCapacity(arr)						gpc_arrCapacity(arr)

// Set number of elements arr can hold
// Reallocates if newCap>arrCapacity(arr).
// Does nothing if newCap<=arrCapacity(arr).
// Returns self. Store the return value to arr in case of reallocation. 
#define arrSetCapacity(arr, newCap)				gpc_arrSetCapacity(arr, newCap)

// Returns the last element
#define arrLast(arr)							gpc_arrLast(arr)

// Returns a pointer to the last element
#define arrBack(arr)							gpc_arrBack(arr)

// Returns true if arrLength(arr)==0
#define arrIsEmpty(arr)							gpc_arrIsEmpty(arr)

// Add an element to the back of arr
// Returns element
#define arrPush(parr, element)					gpc_arrPush(parr, element)

// Add all elements from arr to the back of *parrDestination
// Returns arr
#define arrPushArr(parrDestination, arr)		gpc_arrPushArr(parrDestination, arr)

// Remove n elements from the back of arr
// Returns last element removed
#define arrPop(parr, nElements)					gpc_arrPop(parr, nElements)

// Insert an element in index pos of arr
// Returns element
#define arrInsert(parr, pos, element)			gpc_arrInsert(parr, pos, element)

// Insert an array in index pos of arr
// Returns pointer to blah // TODO SOME MEANINGFUL RETURN VALUE
#define arrInsertArr(parrDestination,pos,arr)	gpc_arrInsertArr(parrDestination,pos,arr)

// Delete n elements from index pos of arr
// Returns pointer to deleted item from arr // TODO CHANGE THIS AS WELL
#define arrDelete(parr, pos, nElements)			gpc_arrDelete(parr, pos, nElements)

// Returns number of elements in a C array declared with []
// Don't use for pointers or array types from this library!
#define carrLength(arr) 						gpc_carrLength(arr)

#endif // GPC_NAMESPACING ----------------------------------------------------

#define gpc_arrLength(arr)						gpc_getSize(arr)/sizeof((arr)[0])

#define gpc_arrSetLength(parr, newLen)		\
	(*(parr) = gpc_setSize(*(parr), (newLen) * sizeof((*(parr))[0])))

#define gpc_arrCapacity(arr)					gpc_getCapacity(arr)/sizeof((arr)[0])

#define gpc_arrSetCapacity(parr, newCap)	\
	(*(parr) = gpc_reallocate(*(parr), newCap * sizeof((*(parr))[0])))

#define gpc_arrLast(arr)						((arr)[gpc_getSize(arr)/sizeof((arr)[0]) - 1])

#define gpc_arrBack(arr)						(&(arr)[gpc_getSize(arr)/sizeof((arr)[0]) - 1])

#define gpc_arrIsEmpty(arr)						(gpc_getSize(arr) == 0)

#define gpc_arrPush(parr, elem)								\
	( gpc_arrSetLength(parr, gpc_arrLength(*(parr)) + 1),	\
		arr[gpc_arrLength(*(parr)) - 1] = (elem) )

#define gpc_arrPushArr(parr, arr)	\
	((arr) = gpc_arrPushGpcArr(((void)*(parr), parr), arr))

#define gpc_arrPop(parr, nElems)						\
	( gpc_arrSetLength( parr,							\
		gpc_arrLength(*(parr)) >= (nElems) ?			\
			gpc_arrLength(*(parr)) - (nElems) : 0 ),	\
				*(gpc_arrBack(*(parr)) + 1) ) 

// #define gpc_arrInsert(parr, pos, elem)		\
	// ( gpc_arrPush(parr, elem),				\
		// (*(parr) =							\
			// gpc_arrSwitchElems(parr, pos, gpc_arrLast(*(arr)), sizeof(**(parr)), 1))\
				// [pos] )

#define gpc_arrInsert(parr, pos, elem)							\
	( gpc_arrSetLength(parr, gpc_arrLength(*(parr)) + 1),		\
		memmove(*(parr) + (pos) + 1,							\
				*(parr) + (pos),								\
				gpc_getSize(*(parr)) - (pos)*sizeof(**(parr))),	\
			arr[pos] = (elem) )

#define gpc_arrInsertArr(parr, pos, arr)									\
	( gpc_arrSetLength(parr, gpc_arrLength(*(parr)) + gpc_arrLength(arr)),	\
		memmove(*(parr) + (pos) + gpc_arrLength(arr),						\
				*(parr) + (pos),											\
				gpc_getSize(*(parr)) - (pos)*sizeof(**(parr))),				\
			memcpy(*(parr) + pos, arr, gpc_getSize(arr)) )

#define gpc_arrDelete(parr, pos, nElems)				\
	( memmove(arr + i, arr + i + n, n * sizeof(*arr)),	\
		gpc_arrSetLength(&arr, gpc_arrLength(arr) - n)) )

#define gpc_carrLength(arr) 					(sizeof(arr)/sizeof(arr[0]))

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

// Switch elements. parr is a pointer to array. Returns *parr
void* gpc_arrSwitchElems(void* parr, size_t pos1, size_t pos2, size_t elemSize, size_t nElems);

// Copies source to the end of *pDestination. Returns source. 
// Use arrPushArr() macro for better type safety. 
void* gpc_arrPushGpcArr(void* pDestination, void* source);

#endif // GPC_ARRAY_H
