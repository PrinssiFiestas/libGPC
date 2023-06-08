/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include "../include/assert.h"
#include "../src/memory.c"

void* malloc(size_t size)
{
	(void)size;
	return NULL;
}

void free(void* ptr)
{
	(void)ptr;
}

void* g_arena;

int main()
{
	g_arena = malloc(1000);
	
	begin
		
	end
	
	free(g_arena);
}