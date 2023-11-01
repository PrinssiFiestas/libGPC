// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// Metaprogramming script to create overloading macros. This does not create a
// complete header which would add unnecessary complications to the build. This
// creates some copy-pasteable code instead. 

// There would be more 'elegant' ways of doing overloading with some 'smart'
// macros, but nested macros are hard to debug and they tend to butcher error
// messages. A generalized OVERLOAD(NARGS,...) was also error prone: forget the
// NARGS and you would've gotten the most confusing error message (at least with
// default settings in GCC). To keep the error messages sane, it's better to use
// dirty brute force metaprogramming hacks. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ARGS 64
#define TABW 4
#define COLW 94

int main(void)
{
    // Test if running this script from script directory. If not, project root 
    // assumed. 
    FILE* ftest = fopen("overloadgen.c", "r");
    bool in_script_dir = ftest != NULL;
    if (ftest)
        fclose(ftest);
    
    const char* out_path = in_script_dir ? "../build/overloadgen_out.h" : "build/overloadgen_out.h";
    
    FILE* f = fopen(out_path, "w+");
    if (f == NULL)
    {
        perror("couldn't create overload.h");
        return EXIT_FAILURE;
    }
    
    // Hold string to be printed in buf before printing so aligment can be made
    char buf[100] = {0};
    int col = 0;
    
    // Changing this value for different parts might make aligment prettier
    int aligment_offset = 0;

    // Print to f aligned. Aligment happens between fpf() calls. Use fprintf if
    // no aligment required.
    #define fpf(...) (col += sprintf(buf, __VA_ARGS__), \
        col >= COLW - TABW - aligment_offset ?          \
            col = 0 * fprintf(f, "\t\\\n%s", buf) :     \
            fprintf(f, "%s", buf)                       \
        )
    
    // Print newline and reset column
    #define endl() (col = 0 * fprintf(f, "\n"))

    // -----------------------------------------------------------------------

    aligment_offset = 2;

    fpf(
    "#define GPC_PROCESS_ALL_ARGS(FUNC, SEPARATOR, ...) GPC_OVERLOAD%i(__VA_ARGS__, ",
    MAX_ARGS);
    for (int i = MAX_ARGS; i >= 1; i--)
    {
        fpf("_GPC_PROC%i%s", i, i > 1 ? ", " : ")");
    }
    fpf("(FUNC, SEPARATOR, __VA_ARGS__)\n");
    endl();

    fprintf(f, "#define _GPC_PROC1(F, SEP, A) F(A)\n");
    for (int i = 2; i <= MAX_ARGS; i++)
    {
        fprintf(f,
        "#define _GPC_PROC%i(F, SEP, A, ...) F(A) SEP(A) _GPC_PROC%i(F, SEP, __VA_ARGS__)\n",
        i, i - 1);
    }
    endl();

    fpf(
    "#define GPC_PROCESS_ALL_BUT_1ST(FUNC, SEPARATOR, ...) GPC_OVERLOAD%i(__VA_ARGS__, ",
    MAX_ARGS);
    for (int i = MAX_ARGS; i >= 1; i--)
    {
        fpf("_GPC_PROC%i_1%s", i, i > 1 ? ", " : ")");
    }
    fpf("(FUNC, SEPARATOR, __VA_ARGS__)\n");
    endl();

    fprintf(f, "#define _GPC_PROC1_1(F, SEP, A) A\n");
    for (int i = 2; i <= MAX_ARGS; i++)
    {
        fprintf(f,
                "#define _GPC_PROC%i_1(F, SEP, A, ...) A, _GPC_PROC%i(F, SEP, __VA_ARGS__)\n",
                i, i - 1);
    }
    endl();

    aligment_offset = 1;

    for (int i = 1; i <= MAX_ARGS; i++)
    {
        fpf("#define GPC_OVERLOAD%i(", i);
        for (int j = 0; j < i; j++)
            fpf("_%i, ", j);
        fpf("RESOLVED, ");
        fpf("...) ");
        fpf("RESOLVED");
        endl();
    }
    endl();
    
    fclose(f);
}
