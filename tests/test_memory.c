/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

// Only test allocations with size divisible by 4!

#include <stdio.h>
#include <stdint.h>

#include "../include/gpc/assert.h"
#include "../src/memory.c"

unsigned char* func(DynamicObjOwner* callingScope)
begin
	unsigned char* returnValue = scopedAlloc(4 * sizeof(returnValue[0]));
	char* dummy = scopedAlloc(12 * sizeof(dummy[0]));
	ret(returnValue);
end

int main()
{
	fakeHeapInit();
	//#define OUTFILE
	#ifdef OUTFILE
	FILE* f = fopen("test_memory_log.txt", "w");
	fakeHeapSetLogOut(f);
	#endif
	//atexitWhenFailed(printHeapHistory);
	
	TEST_SUITE(scoped_memory_management)
	{
		begin
			double* obj1 = scopedAlloc(3 * sizeof(obj1[0]));
			int*    obj2 = scopedAlloc(5 * sizeof(obj2[0]));
			float*  obj3 = scopedAlloc(4 * sizeof(obj3[0]));
			
			TEST(get_owner)
				ASSERT((size_t)getOwner(obj2) EQ (size_t)&thisScope);
			
			TEST(allocations)
			{
				ASSERT(fakeHeapObjectSize(obj1) EQ 3 * sizeof(obj1[0]));
				ASSERT(fakeHeapObjectSize(obj2) EQ 5 * sizeof(obj2[0]));
				ASSERT(fakeHeapObjectSize(obj3) EQ 4 * sizeof(obj3[0]));
			}
			
			TEST(moved_ownership)
			{
				unsigned char* returnValue = func(&thisScope);
				TEST(function_cleaned_its_allocations)
					EXPECT(*(returnValue + ALLOC_OFFSET) EQ FREED);
				ASSERT((uintptr_t)getOwner(returnValue) EQ (uintptr_t)&thisScope);
			}
		end
		
		TEST(automatic_freeing)
			ASSERT(fakeHeapFindFirstReserved() EQ EMPTY_HEAP, "Heap not empty after scope!");
	}
	#ifdef OUTFILE
	fclose(f);
	#endif
	fakeHeapDestroy();
}

#undef malloc
#undef free
#undef calloc
#undef realloc

#include "../src/fakeheap.c"