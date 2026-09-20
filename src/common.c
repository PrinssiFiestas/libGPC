// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "common.h"
#include <gpc/gpthread.h>
#include <gpc/gperrno.h>
#ifdef GP_TARGET_OS_WINDOWS
#include <windows.h>
#endif

static GPThreadKey gp_s_internal_thread_local_key;
static GPOnce gp_s_internal_thread_local_initialized = GP_ONCE_INIT;

static void gp_s_internal_thread_local_destructor(void* p)
{
    void** internal_slots = p;
    if (internal_slots != NULL) {
        // TODO delete scratch arena
        #ifdef GP_TARGET_OS_WINDOWS
        LocalFree(internal_slots[GP_INTERNAL_THREAD_LOCAL_DOSERRNO_STR_BUFFER]);
        #endif
    }
    free(internal_slots);
}

static void gp_s_internal_main_thread_local_destructor(void)
{
    gp_s_internal_thread_local_destructor(gp_thread_local_get(gp_s_internal_thread_local_key));
}

static void gp_s_internal_thread_local_init(void)
{
    if ( ! gp_thread_local_create(
            &gp_s_internal_thread_local_key, gp_s_internal_thread_local_destructor))
        return;

    // Free main thread local data as well to silence Valgrind and leak sanitizers.
    atexit(gp_s_internal_main_thread_local_destructor);
}

void* gp_internal_thread_local_get(gp_internal_thread_local_slot_t index)
{
    void** internal_slots = gp_thread_local_get(gp_s_internal_thread_local_key);
    if (internal_slots == NULL)
        return NULL;
     return internal_slots[index];
}

void gp_internal_thread_local_set(gp_internal_thread_local_slot_t index, const void* p)
{
    gp_call_once(&gp_s_internal_thread_local_initialized, gp_s_internal_thread_local_init);
    const void** internal_slots = gp_thread_local_get(gp_s_internal_thread_local_key);
    if (internal_slots == NULL) {
        GPErrno errs = gp_errno_set(NULL);
        internal_slots = calloc(GP_INTERNAL_THREAD_LOCAL_SLOTS_LENGTH, sizeof internal_slots[0]);
        gp_errno_set(&errs);
        if (internal_slots == NULL)
            return;
        gp_thread_local_set(gp_s_internal_thread_local_key, internal_slots);
    }
    internal_slots[index] = p;
}
