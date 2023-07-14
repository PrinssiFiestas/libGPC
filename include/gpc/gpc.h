/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#ifndef GPC_H
#define GPC_H

#include <stdbool.h>

// Functions check validity of their inputs in various ways. User can set the 
// default error handling behaviour with gpc_setErrorHandlingMode(). Error
// handling mode should be set to each thread separately. 
enum gpc_ErrorHandling
{
	// Return value of gpc_handleErrors() on errors
	GPC_ERROR_SHOULD_HANDLE = -1,
	
	// Crash and burn. Forces user to fix mistakes
	// No error messages are sent. 
	// Default behaviour
	GPC_ERROR_NO_HANDLING,
	
	// Send error message and probably crash. Useful for getting stack trace. 
	GPC_ERROR_SEND_MESSAGE,
	
	// No aborting or sending error messages on errors
	// Makes the library resilient to invalid arguments. Functions return with
	// error codes or error values that should be checked by user.
	GPC_ERROR_RESILIENT,
	
	// No aborting on errors
	// Same as GPC_ERROR_RESILIENT except error message will be sent via
	// callback or to stderr by default.
	GPC_ERROR_DEBUG,
	
	// Sends error messages via callback set by gpc_setDebugMessageCallback()
	// Aborts execution
	GPC_ERROR_STRICT,
	
	// For internal use
	GPC_ERROR_SIZE
};
void gpc_setErrorHandlingMode(enum gpc_ErrorHandling);

// perror()-like function by default. Reset by passing NULL. 
void gpc_setDebugMessageCallback(void (*callback)(const char* errorMessage));

// Does nothing when condition == false
// Calls callback set by gpc_setDebugMessageCallback()
// Aborts based on setting set by gpc_setErrorHandlingMode()
enum gpc_ErrorHandling gpc_handleError(bool condition, const char* errorMessage);

#include "memory.h"

#endif // GPC_H
