// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef STRING_H
#define STRING_H

typedef struct gpc_Char
{
	char c;
} gpc_Char;

typedef gpc_Char* gpc_String;

//----------------------------------------------------------------------------
//
//		CORE API
//
//----------------------------------------------------------------------------

// Define one of these macros before including this header to enable short names
// without the gpc_ prefix. 
#if defined(GPC_STRING_NAMESPACE) || defined(GPC_NAMESPACE)

typedef gpc_Char Char;
typedef gpc_String String;

#define strAppend(pdest, ...)                        \
/* String */ gpc_strAppend(/* String*      */ pdest, \
                           /* String or char* str1, str2, ..., strn */__VA_ARGS__)

#define strCount(haystack, needle)                        \
/* size_t */ gpc_strCount(/* String          */ haystack, \
                          /* String or char* */ needle)

#define strSlice(pstr, startingPos, length)          \
/* String */ gpc_strSlice(/* String* */ pstr,        \
                          /* size_t  */ startingPos, \
                          /* size_t  */ length)

#define strFindFirst(haystack, needle)                        \
/* size_t */ gpc_strFindFirst(/* String          */ haystack, \
                              /* String or char* */ needle)

#define strFindLast(haystack, needle)                        \
/* size_t */ gpc_strFindLast(/* String          */ haystack, \
                             /* String or char* */ needle)

#define strFindAll(haystack, needle) \
/* size_t* */ gpc_strFindAll(/* String          */ haystack, \
                             /* String or char* */ needle)

#define strReplaceFirst(haystack, old, new)                        \
/* size_t */ gpc_strReplaceFirst(/* String          */ haystack, \
                              /* String or char* */ old,)

#define strReplaceLast(haystack, needle)                vdsav        \
/* size_t */ gpc_strReplaceLast(/* String          */ haystack, \
                             /* String or char* */ needle)

#define strReplaceAll(haystack, needle) \
/* size_t* */ gpc_strReplaceAll(/* String          */ haystack, \
                             /* String or char* */ needle)

#define strSplit(str, delimiter)                      \
/* String* */ gpc_strSplit(/* String          */ str, \
                           /* String or char* */ delimiter)

#define strJoin(arr, delimiter)                     \
/* String */ gpc_strJoin(/* String*         */ arr, \
                         /* String or char* */ delimiter)

#endif // GPC_NAMESPACING ----------------------------------------------------

//----------------------------------------------------------------------------
//
//		END OF CORE API
//
//		Structs, functions and macros below are for internal or advanced use
//
//----------------------------------------------------------------------------

#endif // STRING_H
