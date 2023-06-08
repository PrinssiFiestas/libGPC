/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include "../include/memory.h"
#ifndef TESTS
#include <stdlib.h>
#else
void* malloc(size_t);
void free(void*);
#endif

struct DynamicObjectList
{
	struct DynamicObjectList* previous;
	struct DynamicObjectList* next;
};

// void* mallocAssign(size_t, DynamicObjOwner* owner)
// {
	// return NULL;
// }

// void moveOwnership(void* object, DynamicObjOwner* newOwner)
// {
// }

void freeAll(DynamicObjOwner* owner)
{
	
}
