// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/breakpoint.h>
#include <gpc/thread.h>
#include <stdlib.h>

#if _WIN32
#include <windows.h>
#elif __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#elif __APPLE__ // TODO this is not tested due to not having an Apple machine,
                // we just trust StackOverflow on this one, BUT TEST THIS!
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>
#endif

static GPMutex gp_s_debugger_check_mutex;
static void gp_s_delete_debugger_check_mutex(void) {
    gp_mutex_destroy(&gp_s_debugger_check_mutex);
}
static void gp_s_init_debugger_check_mutex(void) {
    gp_mutex_init(&gp_s_debugger_check_mutex);
    atexit(gp_s_delete_debugger_check_mutex); // pedantic cleanup to silence tooling
}

int gp_debugger_was_detached(void)
{
    static GPThreadOnce checked_once = GP_THREAD_ONCE_INIT;
    static GP_MAYBE_ATOMIC bool checked;
    static int result = -1;

    gp_thread_once(&checked_once, gp_s_init_debugger_check_mutex);

    if ( ! checked)
    {
        gp_mutex_lock(&gp_s_debugger_check_mutex);
        if (checked) {
            gp_mutex_unlock(&gp_s_debugger_check_mutex);
            return result;
        }
        result = gp_debugger_is_detached();
        gp_mutex_unlock(&gp_s_debugger_check_mutex);
    }
    return result;
}

int gp_debugger_is_detached(void)
{
    #if _WIN32

    return ! IsDebuggerPresent();

    #elif __linux__

    char buf[4096 + sizeof""];

    int fd = open("/proc/self/status", O_RDONLY);
    if (fd == -1)
        return -1;

    const char* tracer_pid = "TracerPid:";
    const char* match = tracer_pid;

    ssize_t buf_length;
    while ((buf_length = read(fd, buf, sizeof buf - sizeof"")) > 0)
    {
        buf[buf_length] = '\0';
        const char* c = match == tracer_pid ? strstr(buf, tracer_pid) : buf;
        if (c == NULL) {
            c = buf + sizeof buf - sizeof"TracerPid:";
            if (c < buf)
                c = buf;
        }
        if (*match != '\0') for (; *c != '\0'; ++c)
        {
            if (*c == *match) {
                ++match;
                if (*match == '\0')
                    break;
            } else
                match = tracer_pid;
        }
        if (*match == '\0') for (; *c != '\0'; ++c) if (isdigit(*c))
        {
            close(fd);
            return *c == '0';
        }
    }
    close(fd);
    return -1;

    #elif __APPLE__ // https://stackoverflow.com/questions/2200277/detecting-debugger-on-mac-os-x

    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.

    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.

    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    if (junk)
        return -1;

    // We're being debugged if the P_TRACED flag is set.

    return ( (info.kp_proc.p_flag & P_TRACED) == 0 );

    #else

    return -1;

    #endif
}
