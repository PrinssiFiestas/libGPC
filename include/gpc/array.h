/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_ARRAY_H
#define GPC_ARRAY_H

#include <stdint.h>
#include "memory.h"

// Creates an empty array on stack that can be used with dynamic functionality
#define STACK_ARRAY(type, name, capacity)	 											\
	uint8_t _##type##_##name##_##capacity												\
	[sizeof(struct DynamicObjectList) + (capacity) * sizeof(type)];						\
	*(struct DynamicObjectList*)_##type##_##name##_##capacity =							\
	(struct DynamicObjectList) { .owner = &thisScope .size = 0, .capacity = capacity };	\
	type* name = & _##type##_##name##_##capacity [sizeof(struct DynamicObjectList)];

// Return a zero initialized array
#define arrNewOfLength(type, length)
#define arrNewInitialized(type, values...)

/* // Do something like this
#define arrNewOnStack(type, length, ...)	
arrBuild(
	(uint8_t[sizeof(struct DynamicObjectList) + (capacity) * sizeof(type)]),
	size, capacity, owner,
	__VA_ARGS__
	)

*/

#define arrNew(type, ...)

#define arrLength(arr)					getSize(arr)/sizeof(arr[0])
#define arrSetLength(arr, newLen)		setSize(arr, newLen * sizeof(arr[0]))
#define arrCapacity(arr)				getCapacity(arr)/sizeof(arr[0])
#define arrSetCapacity(arr, newCap)		setCapacity(arr, newCap * sizeof(arr[0]))
#define arrFirst(arr)					((arr)[0])
#define arrLast(arr)					((arr)[arrSize(arr) - 1])
#define arrFront(arr)					(arr)
#define arrBack(arr)					(&(arr)[arrSize(arr) - 1])
#define arrIsEmpty(arr)					(arrSize(arr) == 0)
#define arrClear(arr)
#define arrPush(arr, elements...)
#define arrPop(arr, nElements...)
#define arrInsert(arr, pos, elements...)
#define arrDelete(arr, pos, nElements...)

// Returns length of a C array declared with []
// Don't use for array types from this library!
#define carrLength(arr) 				(sizeof(arr)/sizeof(arr[0]))

#endif // GPC_ARRAY_H