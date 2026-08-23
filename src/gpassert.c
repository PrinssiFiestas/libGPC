// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpassert.h>

#if defined(GP_TARGET_POSIX) && defined(__GNUC__)

#include <unistd.h>
#include <sys/wait.h>

pid_t gp_fork(void)
{
    return fork();
}

void gp_assert_crash_check(pid_t* pidptr)
{
    pid_t pid = *pidptr;

    int wstatus;
    gp_assert(waitpid(pid, &wstatus, 0) != -1);

    int exit_status = 0;
    int signum = 0;
    if (WIFEXITED(wstatus))
        exit_status = WEXITSTATUS(wstatus);
    if (WIFSIGNALED(wstatus))
        signum = WTERMSIG(wstatus);

    gp_assert(exit_status != 0 || signum != 0);
}

#endif // assert crash
