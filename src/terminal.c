// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/terminal.h>
#include <gpc/thread.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <string.h>

// 16*64=1024 bits, which is default soft limit for open file descriptors. This
// is anyway overkill, most commonly it's just stdout or stderr that output to
// terminal.
static uint64_t gp_s_ansi_is_enabled[16];

bool gp_ansi_enable(int fd, bool enable)
{
    static bool is_dumb;
    static GP_MAYBE_ATOMIC bool env_checked;

    #ifndef NDEBUG
    gp_assert(fd < 1024, "File descriptor for terminal output assumed to be small.");
    #endif // too harsh to die, worst thing that can happen is user gets no color.
    if (fd >= 1024)
        return false;

    if ( ! env_checked) { // TODO should we mutex?
        char* term = getenv("TERM");
        is_dumb = term != NULL && strcmp(term, "dumb") == 0;
        env_checked = true;
    }
    if (is_dumb)
        return false;

    #ifdef _WIN32
    #else
    if ( ! isatty(fd))
        enable = false;
    #endif

    size_t ufd = fd;
    gp_s_ansi_is_enabled[ufd >> 4] |= ((uint64_t)enable << (ufd & 0xF));
    return enable;
}

// TODO test this!
bool gp_ansi_is_enabled(int _fd)
{
    size_t fd = _fd;

    #ifndef NDEBUG
    gp_assert(fd < 1024, "File descriptor for terminal output assumed to be small.");
    #endif // too harsh to die, worst thing that can happen is user gets no color.
    if (fd >= 1024)
        return false;

    return gp_s_ansi_is_enabled[fd >> 4] & ((uint64_t)1 << (fd & 0xF));
}
