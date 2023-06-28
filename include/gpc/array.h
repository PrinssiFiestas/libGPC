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
#define STACK_ARRAY(type, name, capacity, owner)									\
	uint8_t _##type##_##name##_##capacity											\
	[sizeof(struct DynamicObjectList) + (capacity) * sizeof(type)];					\
	*(struct DynamicObjectList*)_##type##_##name##_##capacity =						\
	(struct DynamicObjectList) { .owner = owner, .size = 0, .capacity = capacity };	\
	type* name = & _##type##_##name##_##capacity [sizeof(struct DynamicObjectList)];


/* // Do something like this
#define arrNewOnStack(type, capacity, owner, ...)	
arrBuild(
	(uint8_t[sizeof(struct DynamicObjectList) + (capacity) * sizeof(type)]){},
	capacity, owner,
	__VA_ARGS__
	)

*/

// Returns new array on stack with dynamic functionality
// For arrays on heap use mallocAssign or callocAssign
#define arrNew(type, capacity, owner, ...)

#define arrLength(arr)					getSize(arr)/sizeof((arr)[0])
#define arrSetLength(arr, newLen)		setSize(arr, newLen * sizeof((arr)[0]))
#define arrCapacity(arr)				getCapacity(arr)/sizeof((arr)[0])
#define arrSetCapacity(arr, newCap)		setCapacity(arr, newCap * sizeof((arr)[0]))
#define arrLast(arr)					((arr)[getSize(arr)/sizeof((arr)[0]) - 1])
#define arrBack(arr)					(&(arr)[getSize(arr)/sizeof((arr)[0]) - 1])
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