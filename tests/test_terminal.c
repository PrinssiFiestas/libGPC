// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/terminal.c"
#include <gpc/assert.h>
#include <gpc/utils.h>

#ifdef GP_TARGET_OS_WINDOWS
#  include <windows.h>
#  define dup _dup
#  define isatty _isatty
#else
#  include <unistd.h>
#endif

int main(void)
{
    if (!isatty(1) || !isatty(2)) // output redirected, skip test
        return 0;

    gp_suite("Test terminal");
    gp_test("ANSI enable");

    char zeros[sizeof gp_s_ansi_is_enabled] = {0};
    gp_assert(memcmp(gp_s_ansi_is_enabled, zeros, sizeof zeros) == 0);

    gp_assert( ! gp_ansi_is_enabled(1));
    gp_assert(gp_ansi_enable(1, true));
    gp_assert(memcmp(gp_s_ansi_is_enabled, zeros, sizeof zeros) != 0);
    gp_assert(gp_ansi_is_enabled(1));

    gp_assert( ! gp_ansi_is_enabled(2));
    gp_assert(gp_ansi_enable(2, true));
    gp_assert(gp_ansi_is_enabled(2));

    int terminals[] = { 1, 2, 54, 76, 99, 122 }; // should be in order
    for (int i = 2; (size_t)i < gp_countof(terminals); i++) {
        dup2(1 + (i&1), terminals[i]);
        gp_assert( ! gp_ansi_is_enabled(terminals[i]));
        gp_assert(gp_ansi_enable(terminals[i], true));
        gp_assert(gp_ansi_is_enabled(terminals[i]));
    }

    int i_terminal = 0;
    for (int i = 0; i < 1024; i++) {
        if (i == terminals[i_terminal]) {
            i_terminal++;
            gp_assert(gp_ansi_is_enabled(i)); // redundant, but whatever
        } else
            gp_assert( ! gp_ansi_is_enabled(i));
    }
}
