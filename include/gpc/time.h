// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TIME_INCLUDED
#define GP_TIME_INCLUDED 1

#if !defined(GP_HAS_TIMESPEC) && (__STDC_VERSION__ >= 201112L || defined(__unix__))
#include <time.h>
#define GP_HAS_TIMESPEC 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup time Timing
/// @code
/// #include <gpc/time.h>
/// @endcode
/// TODO description.

#ifdef GP_HAS_TIMESPEC
/// @defgroup timespec Timespec Conversions
/// TODO description
/// @{

struct timespec gp_timespec_from_time(double seconds);
struct timespec gp_timespec_from_time_ns(GPUint128 nanoseconds);
double gp_timespec_to_time(struct timespec);
GPUint128 gp_timespec_to_time_ns(struct timespec);

/// @}
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_TIME_INCLUDED
