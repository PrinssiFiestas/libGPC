/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdlib.h>
#include "../include/memory.h"

#ifdef TESTS
#define malloc(size)		test_malloc(size)
#define free(p)				test_free(p)
#define calloc(size, t)		test_calloc(size, t)
#define realloc(p, size)	test_realloc(p, size)
void* test_malloc(size_t);
void  test_free(void*);
void* test_calloc(size_t, size_t);
void* test_realloc(void*, size_t);
#endif

struct DynamicObjectList
{
	struct DynamicObjectList* previous;
	struct DynamicObjectList* next;
};

void* mallocAssign(size_t size, DynamicObjOwner* owner)
{
	(void)owner;
	return malloc(size);
}

// void moveOwnership(void* object, DynamicObjOwner* newOwner)
// {
// }

void freeAll(DynamicObjOwner* owner)
{
	(void)owner;
}
