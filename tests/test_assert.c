// MIT License
// Copyright (c) 2022 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define GPC_NAMESPACE
#include "../src/assert.c"

// ---------------------------------------------------------------------------
//
//		API TESTS
//
// ---------------------------------------------------------------------------

void foo(void)
{
	while (test("Tests and assertions can be anywhere"))
		EXPECT(true);
	
	ASSERT(true);
}

int main(void)
{
	foo();
	
	while (test("Ignored whitespace"))
	{
		ASSERT(1,==,1);
		EXPECT(1,      ==          ,1);
	}
	
	while (testSuite("Operators"))
	{
		while (test("Numeric comparisons"))
		{
			EXPECT(1, !=, 0);
			EXPECT(1, > , 0);
			ASSERT(1, &&, true);
			EXPECT(1, ||, 0);
		}
		while (test("String comparisons"))
		{
			EXPECT_STR("A", ==, "A");
			ASSERT_STR("A", !=, "B");
			EXPECT_STR("A", < , "B");
			ASSERT_STR("1", < , "2");
			EXPECT_STR("1", < , "A");
			EXPECT_STR("a", > , "A");
			EXPECT_STR("1", > , NULL);
			ASSERT_STR("2", >=, "1999", "Ordering is alphabetic, not numeric!");
		}
	}
	
	while (testSuite("Types"))
	{
		char c = -1;
		unsigned char uc = (unsigned char)-1;
		long long i = -1;
		unsigned long long u = (unsigned long long)-1; (void)u;
		double f = -1.;
		void* p = malloc(1);
		char s[] = "Xtring"; // typo to prevent literals sharing addresses
		s[0] = 's';
		
		while (test("Numbers without precision loss or sign mismatch"))
		{
			ASSERT(c, ==, i);
			EXPECT(uc,==, f);
			EXPECT(uc,==, i);
			ASSERT(c, ==, f);
			ASSERT(f, ==, -1.f);
		}
		
		while (test("Pointers"))
		{
			ASSERT(s, !=, p);
			char* s2 = "string";
			EXPECT(s, !=, s2, "char* is compared as pointer!");
			EXPECT_STR(s2, "EXPECT_STR() with single arg checks for NULL");
		}
		
		while (test("Strings"))
		{
			ASSERT_STR(s, ==, "string");
			ASSERT_STR(s, !=, NULL);
			EXPECT_STR(NULL, ==, NULL);
		}
		
		//#define GPC_TEST_WARNINGS
		#if defined(GPC_TEST_WARNINGS) && defined(__GNUC__)
		while (test("Compiler warnings"))
		{
			EXPECT(i,==,u); // GCC, Clang: different sign
			EXPECT(c,!=,p); // GCC, Clang: comparison between pointer and integer
			EXPECT(i,==,f); // Clang: int to float may lose precision
			EXPECT(1,*, 1); // GCC: '*' in boolean context
			ASSERT("string",==,"string"); // Clang: comparison against string literal
		}
		#endif
		
		free(p);
	}
	
	// -----------------------------------------------------------------------
	
	while (testSuite("No while loop"))
	{
		test("Test");
		test("Test"); // same name ends "Test"
		test("Other test");
		test(NULL); // ends "Other test"
	}
	
	while (test("No expectations always pass"));
	
	while (testSuite("Same name"))
	{
		while (test("Same name"));
		while (test("Same name"));
		
		// Forbidden! Infinite loop caused by nested tests with same name!
		// while (test("same name"))
			// while (test("same name"));
	}
	
	// -----------------------------------------------------------------------
	//		Nesting tests and suites
	
	while (test("Not in suite"))
		EXPECT(true);
	
	while (testSuite("Suite1"))
	{
		while (test("Test in suite1 before nested suite"))
			EXPECT(true);
		
		while (testSuite("Nested suite"))
		{
			while (test("Test1 in nested suite"))
				EXPECT(true);
			
			while (test("Test2 in nested suite"))
				EXPECT(true, "fail msg!");
		}
		
		while (test("Test in suite1"))
			EXPECT(true);
		
		while (test("Test2 in suite1"))
			EXPECT(true);
	}
	
	while (test("Also not in suite"))
		EXPECT(true);
	
	while (test("This as well is not in suite"))
		EXPECT(true);
	
	while (testSuite("Suite2"))
	{
		while (test("Test in suite2"))
			EXPECT(true);
		
		while (test("Test2 in suite2"))
		{
			EXPECT(true);
			while (test("Nested test"))
				EXPECT(true);
		}
	}
	
	// -----------------------------------------------------------------------
	//
	//		FAILING TESTS
	//
	// -----------------------------------------------------------------------
	//		Define the macro below to see how tests might fail
	
	//#define GPC_NON_PASSING_TESTS
	#ifdef GPC_NON_PASSING_TESTS
	
	while (test("Failing expectations"))
	{
		EXPECT('c',==,5);
		EXPECT((unsigned char)3, ==,.5);
		EXPECT(1,!=,1, "Additional message");
		EXPECT(false);
		EXPECT(0, "Other additional message");
		EXPECT(0 + 1,&&,false);
		
		EXPECT_STR(!"blah");
		EXPECT_STR(NULL);
		EXPECT_STR("string",==,NULL);
	}
	
	puts("\n\t\tStart testing nested suites and tests\n");
	
	srand((unsigned)time(NULL));
	#define COIN (rand()%2)
	#define MAY_ASSERT(...)		\
		(COIN ? EXPECT(__VA_ARGS__) : ASSERT(__VA_ARGS__, "Aborting"))
	
	while (test("not in suite"))
		MAY_ASSERT(COIN);
	
	while (testSuite("suite1"))
	{
		while (test("test in suite1 before nested suite"))
			MAY_ASSERT(1, ==, COIN);
		
		while (testSuite("nested suite"))
		{
			while (test("test1 in nested suite"))
				MAY_ASSERT(COIN ? true : false);
			
			while (test("test2 in nested suite"))
				MAY_ASSERT(true * COIN);
		}
		
		while (test("test in suite1"))
			MAY_ASSERT(COIN - 1, ==, false);
		
		while (test("test2 in suite1"))
			MAY_ASSERT(COIN + 0*15, >=, true);
	}
	
	while (test("also not in suite"))
		MAY_ASSERT(COIN, ||, false);
	
	while (test("this as well is not in suite"))
		MAY_ASSERT(1, &&, COIN);
	
	while (testSuite("suite2"))
	{
		while (test("test in suite2"))
			MAY_ASSERT(true * COIN, <, 1);
		
		while (test("test2 in suite2"))
		{
			MAY_ASSERT(COIN, +, 0);
			while (test("nested test"))
				MAY_ASSERT(COIN, -, 1);
		}
	}
	
	#endif // GPC_NON_PASSING_TESTS
	
	// -----------------------------------------------------------------------
	//
	//		END OF API TESTS
	//
	//		Tests below test internals
	//
	// -----------------------------------------------------------------------
	
	while (testSuite("Stack"))
	{
		struct Stack* testStack = NULL;
	
		while (test("Push"))
		{
			stackPush(&testStack, "blah1");
			EXPECT(testStack->length, ==, (size_t)1);
			EXPECT_STR(stackPeek(testStack)->name, ==, "blah1");
			stackPush(&testStack, "blah2");
			EXPECT_STR(stackPeek(testStack)->name, ==, "blah2");
			EXPECT(testStack->length, ==, (size_t)2);
		}
		while (test("Pop"))
		{
			EXPECT_STR(stackPop(&testStack).name, ==, "blah2");
			EXPECT_STR(stackPop(&testStack).name, ==, "blah1");
			EXPECT(testStack, ==, &nullStack);
		}
	}
}
