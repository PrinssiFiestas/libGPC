// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/gpterminal.h>
#include <gpc/gpthread.h>
#ifdef GP_TARGET_OS_WINDOWS
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif
#include <string.h>

// 16*64=1024 bits, which is default soft limit for open file descriptors. This
// is anyway overkill, most commonly it's just stdout or stderr that output to
// terminal.
static GP_MAYBE_ATOMIC(uint64_t) gp_s_ansi_is_enabled[16];
static bool gp_s_terminal_is_dumb;
static GPOnce gp_s_terminal_is_dumb_checked;

static void gp_s_terminal_check_if_dumb(void)
{
    char* term = getenv("TERM");
    gp_s_terminal_is_dumb = term != NULL && strcmp(term, "dumb") == 0;
}

#ifdef _MSC_VER // unused otherwise
static void gp_s_terminal_invalid_parameter_handler(
    const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int line,
    uintptr_t reserved)
{
    (void)expression;
    (void)function,
    (void)file,
    (void)line,
    (void)reserved;
}
#endif

bool gp_ansi_enable(int fd, bool enable)
{
    #ifdef GP_TARGET_DEBUG
    gp_assert(0 <= fd && fd < 1024,
              "File descriptor for terminal output assumed to be a small positive number.");
    #endif // else too harsh to die, worst thing that can happen is user gets no color.

    if (fd < 0 || 1024 <= fd)
        return false;
    gp_call_once(&gp_s_terminal_is_dumb_checked, gp_s_terminal_check_if_dumb);
    if (gp_s_terminal_is_dumb)
        return false;
    if (enable == false)
        goto set_enabled_bit;

    #ifdef GP_TARGET_OS_WINDOWS
    bool is_terminal = _isatty(fd);
    #else
    bool is_terminal = isatty(fd);
    #endif
    if ( ! is_terminal) {
        enable = false;
        goto set_enabled_bit;
    }

    #ifdef GP_TARGET_OS_WINDOWS
    #  ifdef _MSC_VER // _get_osfhandle() crashes with invalid parameter on
                      // Visual Studio. MinGW does not crash, but MinGW headers
                      // don't declare _set_thread_local_invalid_parameter_handler(),
                      // so we can't set it just in case.
    _invalid_parameter_handler old_handler = _set_thread_local_invalid_parameter_handler(
        gp_s_terminal_invalid_parameter_handler);
    #  endif
    HANDLE console = (HANDLE)_get_osfhandle(fd);
    #  ifdef _MSC_VER
    _set_thread_local_invalid_parameter_handler(old_handler);
    #  endif

    if (console == INVALID_HANDLE_VALUE) {
        enable = false;
        goto set_enabled_bit;
    }

    DWORD console_mode;
    if ( ! GetConsoleMode(console, &console_mode)) {
        enable = false;
        goto set_enabled_bit;
    }
    console_mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if ( ! SetConsoleMode(console, console_mode)) {
        enable = false;
        goto set_enabled_bit;
    }
    #endif // GP_TARGET_OS_WINDOWS

    set_enabled_bit:;
    size_t ufd = fd;
    if (enable)
        gp_s_ansi_is_enabled[ufd >> 6] |=  (1llu << (ufd & 0x3F));
    else
        gp_s_ansi_is_enabled[ufd >> 6] &= ~(1llu << (ufd & 0x3F));

    return enable;
}

bool gp_ansi_is_enabled(int _fd)
{
    size_t fd = _fd;

    #ifdef GP_TARGET_DEBUG
    gp_assert(fd < 1024, "File descriptor for terminal output assumed to be small.");
    #endif // too harsh to die, worst thing that can happen is user gets no color.
    if (fd >= 1024)
        return false;

    return gp_s_ansi_is_enabled[fd >> 6] & ((uint64_t)1 << (fd & 0x3F));
}
