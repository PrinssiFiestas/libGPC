// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>
#include "../include/gpc/string.h"
#include "../include/gpc/memory.h"

gpc_String gpc_strAppendCharArrs(gpc_String* dest, const char* arr[GPC_STATIC 1])
{
	size_t newSize = gpc_size(*dest);
	for (size_t i = 0; arr[i] != NULL; i++)
		newSize += strlen(arr[i]);
	gpc_reserve(dest, newSize + sizeof(""));
	gpc_resize(dest, newSize);
	for (size_t i = 0; arr[i] != NULL; i++)
		strcat((char*)*dest, arr[i]);
	return *dest;
}

const char* gpc_strCstr(gpc_String str)
{
	((char*)str)[gpc_size(str)] = '\0';
	return (const char*)str;
}
