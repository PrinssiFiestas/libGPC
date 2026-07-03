// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_STRING_INCLUDED
#define GP_STRING_INCLUDED 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup string String
/// @code
/// #include <gpc/string.h>
/// @endcode
/// TODO description
/// @{

/** Distinct UTF-8 character type.*/
typedef struct gp_char { uint8_t c; /**< UTF-8 code unit. */ } GPChar;

/** String type.*/
typedef GPChar* GPString;
// TODO once we have array
// typedef GPArray(GPChar) GPString;

// TODO the rest

/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

///@endcond
#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_STRING_INCLUDED
