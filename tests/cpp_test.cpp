// Build the library first using `make` then build and run this file with
// `
// g++ -Wall -Wextra -Iinclude -ggdb3 tests/cpp_test.cpp -fsanitize=address -fsanitize=undefined -lm -lpthread -lasan build/libgpcd.so && ./a.out
// `
#include "../build/gpc.h"

// There is not much C++ specific functionality. Only some macros need to be
// tested.

int main()
{
    char cpp[36] = "";
    gp_bytes_print(cpp, "C++");
    gp_print("Hello ", cpp, "!\n");

    GPString str = gp_str_new(gp_heap, 16, "");
    gp_str_println(&str, "I am the prince of", cpp);
    gp_println(str, "Obay me!");

    gp_test("Failing C++ test");
    gp_assert(1 == 0, 1+1, 2*3, -7, str, "yueahg");

    gp_str_delete(str);
}
