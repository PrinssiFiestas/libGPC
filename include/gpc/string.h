// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef STRING_H
#define STRING_H

#include "attributes.h"
#include "overload.h"

typedef struct gpc_Char
{
    char c;
} gpc_Char;

typedef gpc_Char* gpc_String;

//----------------------------------------------------------------------------
//
//        CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header to enable short names
// without the gpc_ prefix. 
#if defined(GPC_STRING_NAMESPACE) || defined(GPC_NAMESPACE)

typedef gpc_Char Char;
typedef gpc_String String;

// Use explicit constructors newStringS() and newStringH() for creating strings
// instead of newS() and newH(). 

// Return a String on stack initialized with literal
// GCC and Clang will warn about initialization of 'char' from 'char*' if
// literal is not an actual literal. 
#define newStringS(literal) \
/* String */ gpc_newStringS(/* const char* */ literal)

// Return a String on heap initialized with literal
// GCC and Clang will warn about initialization of 'char' from 'char*' if
// literal is not an actual literal. 
#define newStringH(literal) \
/* String */ gpc_newStringH(/* const char* */ literal)

// Appends all variadic arguments to *pdest. Returns *pdest. 
#define strAppend(pdest, ...)                        \
/* String */ gpc_strAppend(/* String*      */ pdest, \
                           /* String or char* str1, str2, ..., strn */__VA_ARGS__)

// Counts all occurrences of needle in haystack. Returns count. 
#define strCount(haystack, needle)                        \
/* size_t */ gpc_strCount(/* String          */ haystack, \
                          /* String or char* */ needle)

// Turns *pstr into it's substring. Returns created substring. 
#define strSlice(pstr, startingPos, length)          \
/* String */ gpc_strSlice(/* String* */ pstr,        \
                          /* size_t  */ startingPos, \
                          /* size_t  */ length)

// Returns index of first occurrence of needle in haystack or -1 if not found
#define strFindFirst(haystack, needle)                      \
/* long */ gpc_strFindFirst(/* String          */ haystack, \
                            /* String or char* */ needle)

// Returns index of last occurrence of needle in haystack or -1 if not found
#define strFindLast(haystack, needle)                      \
/* long */ gpc_strFindLast(/* String          */ haystack, \
                           /* String or char* */ needle)

// Returns a GPC array of indices of occurrences of needle in haystack or an
// empty GPC array if none found. 
#define strFindAll(haystack, needle)                         \
/* size_t* */ gpc_strFindAll(/* String          */ haystack, \
                             /* String or char* */ needle)

// Replaces first occurrence of needle in *phaystack with replacement.
// Returns *phaystack.
#define strReplaceFirst(phaystack, needle, replacement)           \
/* String */ gpc_strReplaceFirst(/* String          */ phaystack, \
                                 /* String or char* */ needle,    \
                                 /* String or char* */ replacement)

// Replaces last occurrence of needle in *phaystack with replacement.
// Returns *phaystack.
#define strReplaceLast(phaystack, needle, replacement)           \
/* String */ gpc_strReplaceLast(/* String          */ phaystack, \
                                /* String or char* */ needle,    \
                                /* String or char* */ replacement)

// Replaces all occurrences of needle in *phaystack with replacement.
// Returns *phaystack.
#define strReplaceAll(phaystack, needle, replacement)           \
/* String */ gpc_strReplaceAll(/* String          */ phaystack, \
                               /* String or char* */ needle,    \
                               /* String or char* */ replacement)

// Splits the string in smaller strings where the delimiter is found.
// Returns a GPC array of substrings. 
#define strSplit(str, delimiter)                      \
/* String* */ gpc_strSplit(/* String          */ str, \
                           /* String or char* */ delimiter)

// strtok() but sane
// Splits *pstr into tokens, which are sequences of contiguous characters
// separated by any of the characters that are part of delimiters. The end of
// tokens are replaced by null-characters. Returns a GPC array of C strings. 
#define strTokens(pstr, delimiters)                     \
/* char** */ gpc_strSplit(/* String* or char** */ pstr, \
                          /* String or char*   */ delimiters)

// Concatenates the Strings in the array into a large string, separated by the
// delimiter.
#define strJoin(arr, delimiter)                     \
/* String */ gpc_strJoin(/* String*         */ arr, \
                         /* String or char* */ delimiter)

// Returns str as non-modifyable null-terminated C string. 
#define strCstr(str) \
/* const char* */ gpc_strCstr(/* String */ str)

#endif // GPC_NAMESPACING ----------------------------------------------------

//----------------------------------------------------------------------------
//
//        END OF CORE API
//
//        Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

#define gpc_newStringS(literal) \
(gpc_buildObject( \
        &(uint8_t[sizeof(struct gpc_ObjectList) + sizeof((char[]){literal})]){0},\
        (struct gpc_ObjectList) \
        { \
            .owner = NULL, \
            .size = sizeof((char[]){literal}) - 1, \
            .capacity = sizeof((char[]){literal}) \
        }, &(char[]){literal}))

#define gpc_newStringH(literal) \
gpc_buildHeapObject( \
        sizeof((char[]){literal}) - 1, &(char[]){literal}, NULL)

#define gpc_strAppend(pdest, ...) \
    gpc_strAppendCharArrs(pdest, \
        (const char*[]){GPC_LIST_ALL(GPC_ANY_STRING, GPC_COMMA, __VA_ARGS__), NULL})

gpc_String gpc_strAppendCharArrs(gpc_String* dest, const char* arr[GPC_STATIC 1]);

const char* gpc_strCstr(gpc_String);

#define GPC_ANY_STRING(STR) \
    _Generic((STR), \
        char*: (STR), \
        const char*: (STR), \
        gpc_String: gpc_strCstr((gpc_String)(STR)), \
        void*: (STR)) // for NULL

#endif // STRING_H
