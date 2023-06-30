/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

// This file tests the unit testing framework itself. On changes to the
// to the framework, NON_PASSING_TESTS should be defined to test failing tests
// and the results should be verified manually.

#include <stdlib.h>
#include "../include/gpc/assert.h"

int factorial(int x)
{
	int y = 1;
	for(int i = 1; i <= x; i++)
		y *= i;
	return y;
}

int main()
{
	//#define NON_PASSING_TESTS
	#ifndef NON_PASSING_TESTS
	TEST_SUITE(passingSuite)
	{
		TEST(passingTest0)
		{
			ASSERT(true);
		}

		TEST(passingTest1)
		{
			ASSERT(-1 LT 0);
		}
	}
	TEST(types)
	{
		ASSERT((uint32_t)5 EQ 5.f);
		ASSERT((const int)6 NE 9);
		ASSERT((bool)true);
		ASSERT((bool)true EQ 1==1);
	}
	TEST(pointerComparison)
	{
		void* ptr1 = malloc(1);
		void* ptr2 = ptr1;
		void* ptr3 = ptr1 + 1;
		ASSERT(ptr1 EQ ptr2);
		ASSERT(ptr1 NE ptr3);
		free(ptr1);
	}
	TEST(stringComparison)
	{
		const char* str1 = "Same string";
		char str2[]      = "Xame string"; // typo to prevent pointing to same literal
		str2[0] = 'S';
		ASSERT(str1 EQ str2);
		ASSERT(str1 NE "Different string");
		ASSERT("1" LT "2");
		ASSERT("0" GE "0");
		ASSERT("4" GT "2");
	}

	#else // NON_PASSING_TESTS -----------------------------------------------
	
	TEST(types)
	{
		EXPECT((uint32_t)5 NE 5.f);
		EXPECT((const int)6 EQ 9);
		EXPECT((bool)false);
		
		// When using operators (EQ, NE etc.) failure message should not print 
		// "true" or "false" to prevent printing "true == true" on 2 different
		// non-zero values
		EXPECT((bool)false EQ 1==1);
	}

	EXPECT(0+0 EQ 1+1, "Example fail message");
	
	TEST(string)
	{
		EXPECT("bad""append" EQ "bad append");
	}

	TEST_SUITE(factorial)
	{
		TEST(zero)
		{
			ASSERT(factorial(0) EQ 1);
		}

		EXPECT(factorial(3) EQ -1);

		TEST(positiveNumbers)
		{
			ASSERT(factorial(1) EQ 1);
			ASSERT(factorial(2) EQ 2);
			ASSERT(factorial(3) EQ 6);
			ASSERT(factorial(12) EQ 479001600);
		}
	}

	TEST_SUITE(nested)
	{
		TEST(first)
		{
			EXPECT(1 == 1);

			TEST(first_inner)
			{
				EXPECT(0, "Another example fail message");
			}
		}
		
		TEST(second)
		{
			EXPECT(1 == 1);
		}
	}

	TEST(suiteInTest)
	{
		TEST_SUITE(suite)
		{
			TEST(right)
			{
				ASSERT(1 + 1 EQ 2);
			}
			TEST(wrong)
			{
				EXPECT(1 + 1 NE 2);
			}
		}
	}

	#endif // NON_PASSING_TESTS

	//#define ASSERT_TEST
	#ifdef  ASSERT_TEST
	TEST(assert)
	{
		TEST_SUITE(assertSuite)
		{
			TEST(notYetTheActualTest)
			{
				TEST(actualAssertTest)
				{
					ASSERT(factorial(3) LE 0 - 1);
				}
			}
		}
	}
	#endif // ASSERT_TEST

	return 0;
}
