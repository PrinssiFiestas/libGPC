/*
 * MIT License
 * Copyright (c) 2022 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

// This file tests the unit testing framework itself. On changes to the
// to the framework, PASSING_TESTS should be commented out to test failing tests
// and the results should be verified manually. 

#include "../include/assert.h"

int factorial(int x)
{
	int y = 1;
	for(int i = 1; i <= x; i++)
		y *= i;
	return y;
}

int main()
{

//#define PASSING_TESTS
#ifndef PASSING_TESTS
	EXPECT(0+0 EQ 1+1, "Example fail message");

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
#else // PASSING_TESTS
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
#endif // PASSING_TESTS

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
