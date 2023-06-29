/*
 * MIT License
 * Copyright (c) 2023 Lauri Lorenzo Fiestas
 * https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../include/gpc/gpc.h"

// Same as perror() but doesn't print "No error" when no errors
static void perror2(const char* msg)
{
	const char* errorString = strerror(errno);
	if ( ! errno)
		errorString = "";
	if ( ! msg)
		msg = "";
	fprintf(stderr, "%s\n%s\n", errorString, msg);
}

static enum gpc_ErrorHandling gpc_gErrorHandlingMode = GPC_ERROR_NO_HANDLING;
static void (*gpc_gDebugMessageCallback)(const char*) = perror2;

void gpc_setErrorHandlingMode(enum gpc_ErrorHandling i)
{
	gpc_gErrorHandlingMode = i < GPC_ERROR_SIZE ? i : GPC_ERROR_NO_HANDLING;
}

static void doNothing(const char* s)
{
	(void)s;
}

void gpc_setDebugMessageCallback(void (*callback)(const char*))
{
	gpc_gDebugMessageCallback = callback ? callback : doNothing;
}

enum gpc_ErrorHandling gpc_handleError(bool condition, const char* errorMessage)
{
	if (condition != true)
		return GPC_ERROR_NO_HANDLING;
	
	switch (gpc_gErrorHandlingMode)
	{
		case GPC_ERROR_NO_HANDLING:
			return GPC_ERROR_NO_HANDLING;
			break;
		case GPC_ERROR_MANUAL_HANDLING:
			return GPC_ERROR_SHOULD_HANDLE;
			break;
		case GPC_ERROR_DEBUG:
			gpc_gDebugMessageCallback(errorMessage);
			return GPC_ERROR_SHOULD_HANDLE;
			break;
		case GPC_ERROR_STRICT:
			gpc_gDebugMessageCallback(errorMessage);
			abort();
			break;
		default:
			return GPC_ERROR_NO_HANDLING;
	}
}
