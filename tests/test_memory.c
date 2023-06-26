// /*
 // * MIT License
 // * Copyright (c) 2023 Lauri Lorenzo Fiestas
 // * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 // */

int main() {}

// // Only test allocations with size divisible by 4!

// #include <stdio.h>
// #include <stdint.h>
// #include <stdbool.h>

// #include "../include/gpc/assert.h"
// #include "../src/memory.c"

// unsigned char* retFirst(DynamicObjOwner* callingScope)
// begin
	// unsigned char* returnValue = scopedMalloc(4 * sizeof(returnValue[0]));
	// char* dummy = scopedMalloc(12 * sizeof(dummy[0]));
	// ret(returnValue);
// end

// const char str12[] = "twelve Bs\n";
// char* retLast(DynamicObjOwner* callingScope)
// begin
	// void** dummy1 = scopedMalloc(3 * sizeof(dummy1[0]));
	// size_t* dummy2 = scopedMalloc(2 * sizeof(dummy2[0]));
	// char* returnValue = scopedMalloc(12 * sizeof(returnValue[0]));
	// // copy string
	// for (size_t i = 0; i < sizeof(str12); i++)
		// returnValue[i] = str12[i];
	// ret(returnValue);
// end

// void** retMid(DynamicObjOwner* callingScope)
// begin
	// void** dummy1 = scopedMalloc(3 * sizeof(dummy1[0]));
	// size_t* dummy2 = scopedMalloc(2 * sizeof(dummy2[0]));
	// void** returnValue = scopedMalloc(1 * sizeof(returnValue[0]));
	// void** dummy3 = scopedMalloc(3 * sizeof(dummy3[0]));
	// size_t* dummy4 = scopedMalloc(2 * sizeof(dummy4[0]));
	// ret(returnValue);
// end

// void nestedScope(DynamicObjOwner* callingScope)
// begin
	// void** dummy = scopedMalloc(1 * sizeof(dummy[0]));
	// begin
		// ret();
	// end
// end

// int fakeHeapBlockIsFreed(void* first, void* last);

// int main()
// {
	// fakeHeapInit();
	
	// fakeHeapSetAutoLog(true);
	
	// TEST_SUITE(scoped_memory_management)
	// {
		// begin
			// double* obj1 = scopedMalloc(3 * sizeof(obj1[0]));
			// int*    obj2 = scopedMalloc(5 * sizeof(obj2[0]));
			// float*  obj3 = scopedMalloc(4 * sizeof(obj3[0]));
			
			// TEST(metaFuncs)
			// {
				// ASSERT(getOwner(obj2) EQ &thisScope);
				// ASSERT(getSize(obj3) EQ 4 * sizeof(obj3[0]));
			// }
			
			// TEST(allocations)
			// {
				// ASSERT(fakeHeapObjectSize(obj1) EQ 3 * sizeof(obj1[0]));
				// ASSERT(fakeHeapObjectSize(obj3) EQ 4 * sizeof(obj3[0]));
			// }
			
			// TEST(moved_ownership)
			// {
				// unsigned char* returnValue1 = retFirst(&thisScope);
				// char* returnValue2 = retLast(&thisScope);
				// void** returnValue3 = retMid(&thisScope);
				
				// nestedScope(&thisScope);
				
				// TEST(function_cleaned_its_allocations)
				// {
				// }
			// }
		// end
		
		// TEST(automatic_freeing)
			// ASSERT(fakeHeapFindFirstReserved() EQ EMPTY_HEAP, "Heap not empty after scope!");
	// }
	// fakeHeapDestroy();
// }

// int fakeHeapBlockIsFreed(void* first, void* last)
// {
	// bool result = true;
	// for (uint32_t* mem = first; mem != last; mem++)
	// {
		// result = *mem == FREED4;
		// if (result == false)
			// break;
	// }
	// return result;
// }


// #undef malloc
// #undef free
// #undef calloc
// #undef realloc

// #include "../src/fakeheap.c"