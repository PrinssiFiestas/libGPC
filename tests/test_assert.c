// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <stdlib.h>

#define GPC_NAMESPACE
#include "../src/assert.c"

int main(void)
{
	// while (test("Whitespace"))
	{
		EXPECT(1,==,1);
		EXPECT(1,      ==          ,1);
	}
	
	// while (test("Types in expectations"))
	{
		char c = -1;
		unsigned char uc = (unsigned char)-1;
		long long i = -1;
		unsigned long long u = (unsigned long long)-1;
		double f = -1.;
		void* p = malloc(1);
		char s[] = "Xtring"; // typo to prevent literals sharing address
		s[0] = 's';
		
		EXPECT(c, ==, i);
		EXPECT(uc,==, f);
		EXPECT(uc,==, i);
		EXPECT(c, ==, f);
		EXPECT(s, !=, p); // ptr comparison
		EXPECT_STR(s, ==, "string");
		
		// #define GPC_TEST_WARNINGS
		#ifdef GPC_TEST_WARNINGS
		EXPECT(i,!=,u);        // GCC, Clang: different sign
		EXPECT(c,!=,p);        // GCC, Clang: comparison between pointer and integer
		EXPECT(i,!=,f);        // Clang: int to float may lose precision
		EXPECT(s,==,"string"); // Clang: comparison against string literal
		#endif
		
		#define GPC_NON_PASSING_TESTS
		#ifdef GPC_NON_PASSING_TESTS
		// while (test("Non-passing test error messages"))
		{
			EXPECT(c, !=, i);
			EXPECT(uc,!=, f);
			EXPECT(uc,!=, i);
			EXPECT(c, !=, f);
			EXPECT(s, ==, p);
			EXPECT_STR(s, ==, NULL);
			EXPECT(false);
			EXPECT_STR(!"blah");
		}
		#endif
		
		free(p);
	}
	
	/*{
		struct Stack testStack = {0};
	
		stackPush(&testStack, "blah1");
		ASSERT_STR(stackPeek(&testStack)->name, EQ, "blah1");
		ASSERT(testStack.length, EQ, 1);
		stackPush(&testStack, "blah2");
		ASSERT_STR(stackPeek(&testStack)->name, EQ, "blah2");
		ASSERT(testStack.length, EQ, 2);
		
		// FUCKING SIDE EFFECTS IN ASSERT_STR FUCKS UP MY SHIT!!!
		ASSERT_STR(stackPop(&testStack).name, EQ, "blah2");
		
		puts(GPC_GREEN("All is good"));
		exit(1);
	}
	
	test("TEST");
	test("VLAGH");
	test("VLAGH");
	test("TEST");
	exit(1);
	
	// Suites are always stand alone but tests can be either part of a suite
	// or stand alone. Suites can be nested arbitrarily but it they have no
	// awareness of each other. Tests can also be nested arbitrarily and they 
	// also have no awareness of each other but they get regitered to suites 
	// they belong if any. 
	
	// So a global suite stack is needed and each suite needs a test stack. 
	
	while (test("not in suite"))
		EXPECT(true);
	
	while (testSuite("suite1"))
	{
		while (test("test in suite1 before nested suite"))
			EXPECT(true);
		
		while (testSuite("nested suite"))
		{
			while (test("test1 in nested suite"))
				EXPECT(true);
			
			while (test("test2 in nested suite"))
				EXPECT(true);
		}
		
		while (test("test in suite1"))
			EXPECT(true);
		
		while (test("test2 in suite1"))
			EXPECT(true);
	}
	
	while (test("also not in suite"))
		EXPECT(true);
	
	while (test("this as well is not in suite"))
		EXPECT(true);
	
	while (testSuite("suite2"))
	{
		while (test("test in suite2"))
			EXPECT(true);
		
		while (test("test2 in suite2"))
		{
			EXPECT(true);
			while (test("nested test"))
				EXPECT(true);
		}
	}*/
	
	// -----------------------------------------------------------------------
	
	// // while (test("EXPECT()"))
	{
		//ASSERT(false);
		//EXPECT(!"Fail!"); // does not work!
		
		//EXPECT((bool)0, !=, (bool)false); // .--jfdsagfdk-sajg
		
		/*EXPECT((bool)0);
		EXPECT((bool)0, NE, (bool)false);
		EXPECT(0, "Failed message!");
		EXPECT(.5, GT, 8);
		EXPECT((void*)4, NE, (char*)4, "Other fail message!");
		
		//EXPECT_STR("Literal ptr!", EQ, NULL); // non-compliant!
		//EXPECT_STR(NULL, EQ, "Literal ptr!"); // non-compliant!
		EXPECT_STR("bloink", NE, "bloink");
		EXPECT_STR("abc", GT, "cde");*/

	}
}

// int factorial(int x)
// {
	// int y = 1;
	// for(int i = 1; i <= x; i++)
		// y *= i;
	// return y;
// }

// int main(void)
// {
	// //#define NON_PASSING_TESTS
	// #ifndef NON_PASSING_TESTS
	// TEST_SUITE(passingSuite)
	// {
		// TEST(passingTest0)
		// {
			// ASSERT(true);
		// }

		// TEST(passingTest1)
		// {
			// ASSERT(-1 LT 0);
		// }
	// }
	// TEST(types)
	// {
		// ASSERT((uint32_t)5 EQ 5.f);
		// ASSERT((const int)6 NE 9);
		// ASSERT((bool)true);
		// ASSERT((bool)true EQ 1==1);
	// }
	// TEST(pointerComparison)
	// {
		// void* ptr1 = malloc(1);
		// void* ptr2 = ptr1;
		// void* ptr3 = (char*)ptr1 + 1;
		// ASSERT(ptr1 EQ ptr2);
		// ASSERT(ptr1 NE ptr3);
		// free(ptr1);
	// }
	// TEST(stringComparison)
	// {
		// const char* str1 = "Same string";
		// char str2[]      = "Xame string"; // typo to prevent pointing to same literal
		// str2[0] = 'S';
		// ASSERT(str1 EQ str2);
		// ASSERT(str1 NE "Different string");
		// ASSERT("1" LT "2");
		// ASSERT("0" GE "0");
		// ASSERT("4" GT "2");
	// }

	// #else // NON_PASSING_TESTS -----------------------------------------------
	
	// TEST(types)
	// {
		// EXPECT((uint32_t)5 NE 5.f);
		// EXPECT((const int)6 EQ 9);
		// EXPECT((bool)false);
		
		// // When using operators (EQ, NE etc.) failure message should not print 
		// // "true" or "false" to prevent printing "true == true" on 2 different
		// // non-zero values
		// EXPECT((bool)false EQ 1==1);
	// }

	// EXPECT(0+0 EQ 1+1, "Example fail message");
	
	// TEST(string)
	// {
		// EXPECT("bad""append" EQ "bad append");
	// }

	// TEST_SUITE(factorial)
	// {
		// TEST(zero)
		// {
			// ASSERT(factorial(0) EQ 1);
		// }

		// EXPECT(factorial(3) EQ -1);

		// TEST(positiveNumbers)
		// {
			// ASSERT(factorial(1) EQ 1);
			// ASSERT(factorial(2) EQ 2);
			// ASSERT(factorial(3) EQ 6);
			// ASSERT(factorial(12) EQ 479001600);
		// }
	// }

	// TEST_SUITE(nested)
	// {
		// TEST(first)
		// {
			// EXPECT(1 == 1);

			// TEST(first_inner)
			// {
				// EXPECT(0, "Another example fail message");
			// }
		// }
		
		// TEST(second)
		// {
			// EXPECT(1 == 1);
		// }
	// }

	// TEST(suiteInTest)
	// {
		// TEST_SUITE(suite)
		// {
			// TEST(right)
			// {
				// ASSERT(1 + 1 EQ 2);
			// }
			// TEST(wrong)
			// {
				// EXPECT(1 + 1 NE 2);
			// }
		// }
	// }

	// #endif // NON_PASSING_TESTS

	// //#define ASSERT_TEST
	// #ifdef  ASSERT_TEST
	// TEST(assert)
	// {
		// TEST_SUITE(assertSuite)
		// {
			// TEST(notYetTheActualTest)
			// {
				// TEST(actualAssertTest)
				// {
					// ASSERT(factorial(3) LE 0 - 1);
				// }
			// }
		// }
	// }
	// #endif // ASSERT_TEST

	// return 0;
// }
