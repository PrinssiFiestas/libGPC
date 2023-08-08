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
// Returns *parr as typeof(*parr) if C23 or GNUC, void* otherwise.
#define arrSetLength(parr, newLen)				gpc_arrSetLength(parr, newLen)

// Returns the number of elements that arr can hold without reallocating
#define arrCapacity(arr)						gpc_arrCapacity(arr)

// Set number of elements arr can hold
// Reallocates if newCap>arrCapacity(arr).
// Does nothing if newCap<=arrCapacity(arr).
// Returns *parr as typeof(*parr) if C23 or GNUC, void* otherwise.
#define arrReserve(parr, newCap)				gpc_arrReserve(parr, newCap)

// Returns the last element
// arr is evaluated twice if not using C23 or GNUC.
#define arrLast(arr)							gpc_arrLast(arr)

// Returns a pointer to the last element
// arr is evaluated twice if not using C23 or GNUC.
#define arrBack(arr)							gpc_arrBack(arr)

// Returns true if arrLength(arr)==0
#define arrIsEmpty(arr)							gpc_arrIsEmpty(arr)

// Add one element to the back of *parr or multiple elements if C23 or GNUC.
// parr is evaluated multiple times if not using C23 or GNUC. 
// Returns *parr
#define arrPush(parr, ...)						gpc_arrPush(parr, __VA_ARGS__)

// Add all elements from arr to the back of *parrDestination
// Returns *parrDestination as typeof(*parr) if C23 or GNUC, void* otherwise
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

#define gpc_arrLength(arr)						(gpc_size(arr)/sizeof((arr)[0]))

#define gpc_arrSetLength(parr, newLen)		\
	(GPC_CAST_TO_TYPEOF(*parr)gpc_resize((parr), (newLen) * sizeof((*(parr))[0])))

#define gpc_arrCapacity(arr)					gpc_capacity(arr)/sizeof((arr)[0])

#define gpc_arrReserve(parr, newCap)	\
	(GPC_CAST_TO_TYPEOF(*parr)gpc_reserve((parr), (newCap) * sizeof((*(parr))[0])))

// TODO check for C23 once it comes out
#if defined(__GNUC__)

#define gpc_arrLast(arr)	\
	(*(typeof(arr))gpc_arrLastElem((arr), sizeof((arr)[0])))

#define gpc_arrBack(arr)	\
	((typeof(arr))gpc_arrLastElem((arr), sizeof((arr)[0])))

#define gpc_arrPush(parr, ...)									\
	((typeof(*(parr)))gpc_arrPushMem(							\
						(parr),									\
						(typeof(*(parr)[0])[]){__VA_ARGS__},	\
						sizeof((typeof(*(parr)[0])[]){__VA_ARGS__})))
						

#else

#define gpc_arrLast(arr)	\
	((arr)[gpc_size(arr)/sizeof((arr)[0]) - 1])

#define gpc_arrBack(arr)	\
	(&(arr)[gpc_size(arr)/sizeof((arr)[0]) - 1])

#define gpc_arrPush(parr, elem)		\
	(gpc_arrIncSize((parr), sizeof(**(parr))), gpc_arrLast(*(parr)) = elem, *(parr))

#endif // defined(__GNUC__)

inline bool gpc_arrIsEmpty(void* arr)
{
	return gpc_size(arr) == 0;
}

#define gpc_arrPushArr(parr, arr)	\
	GPC_CAST_TO_TYPEOF(*(parr))gpc_arrPushGpcArr(GPC_PTR_REF(parr), (arr))

#define gpc_arrPop(parr, nElems)						\
	( gpc_arrSetLength( (parr),							\
		gpc_arrLength(*(parr)) >= (nElems) ?			\
			gpc_arrLength(*(parr)) - (nElems) : 0 ),	\
				*(gpc_arrBack(*(parr)) + 1) ) 

#define gpc_arrInsert(parr, pos, elem)							\
	( gpc_arrSetLength( (parr), gpc_arrLength(*(parr)) + 1),	\
		memmove(*(parr) + (pos) + 1,							\
				*(parr) + (pos),								\
				gpc_size(*(parr)) - (pos)*sizeof(**(parr))),	\
			arr[pos] = (elem) )

#define gpc_arrInsertArr(parr, pos, arr)									\
	( gpc_arrSetLength((parr), gpc_arrLength(*(parr)) + gpc_arrLength(arr)),\
		memmove(*(parr) + (pos) + gpc_arrLength(arr),						\
				*(parr) + (pos),											\
				gpc_size(*(parr)) - (pos)*sizeof(**(parr))),				\
			memcpy(*(parr) + pos, arr, gpc_size(arr)) )

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

inline void* gpc_arrLastElem(void* arr, size_t elemSize)
{
	return ((gpc_Byte*)arr) + gpc_size(arr) - elemSize;
}

inline void* gpc_arrPushMem(void* parr, const void* src, size_t count)
{
	size_t oldSize = gpc_size(*(void**)parr);
	gpc_resizeObj(parr, oldSize + count);
	memcpy(*(gpc_Byte**)parr + oldSize, src, count);
	return *(void**)parr;
}

// Increments array length by one and returns old array length
size_t gpc_arrIncSize(void* parr, size_t elemSize);

// Switch elements. parr is a pointer to an array. Returns *parr
void* gpc_arrSwitchElems(void* parr, size_t pos1, size_t pos2, size_t elemSize, size_t nElems);

// Copies source to the end of *pDestination. Returns *pDestination. 
// Use arrPushArr() macro for better type safety. 
void* gpc_arrPushGpcArr(void* pDestination, const void* source);

// Used by some macros to return self like this:
// #define macro(parr) (modifyArrAndRetSomethingElse(parr), *(parr) = gpc_passTrough(parr))
inline void* gpc_passTrough(void* pptr)
{
	return *(void**)pptr;
}

#endif // GPC_ARRAY_H
