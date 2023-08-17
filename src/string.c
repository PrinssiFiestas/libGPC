// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <string.h>
#include "../include/gpc/string.h"
#include "../include/gpc/memory.h"

const char* gpc_strCstr(gpc_String str)
{
	((char*)str)[gpc_size(str)] = '\0';
	return (const char*)str;
}
