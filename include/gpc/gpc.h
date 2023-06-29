/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_H
#define GPC_H

#include <stdbool.h>

// Functions check validity of their inputs in various ways. User can set the 
// default error handling behaviour with gpc_setErrorHandlingMode(). 
enum gpc_ErrorHandling
{
	// Return value of gpc_handleErrors() on errors
	GPC_ERROR_SHOULD_HANDLE = -1,
	
	// Crash and burn. Forces user to fix mistakes
	// Default behaviour
	GPC_ERROR_NO_HANDLING,
	
	// No aborting or sending error messages on errors
	// Moves freedom and responsibility of error handling to user
	GPC_ERROR_MANUAL_HANDLING,
	
	// No aborting on errors
	// Error message will be sent via callback or to stderr by default
	GPC_ERROR_DEBUG,
	
	// Sends error messages via callback set by gpc_setDebugMessageCallback()
	// Aborts execution
	GPC_ERROR_STRICT,
	
	// For internal user
	GPC_ERROR_SIZE
};
void gpc_setErrorHandlingMode(enum gpc_ErrorHandling);

// perror()-like function by default
// Disable error messages with NULL
// Argument to callback may be NULL
void gpc_setDebugMessageCallback(void (*callback)(const char*));

// Does nothing when condition == false
// Calls callback set by gpc_setDebugMessageCallback()
// Aborts based on setting set by gpc_setErrorHandlingMode()
enum gpc_ErrorHandling gpc_handleError(bool condition, const char* errorMessage);

#include "memory.h"

#endif // GPC_H