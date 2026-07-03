// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_TERMINAL_INCLUDED
#define GP_TERMINAL_INCLUDED 1

#include <gpc/preprocessor.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------
/// @defgroup terminal Terminal
/// @code
/// #include <gpc/terminal.h>
/// @endcode
/// ANSI escape sequences.
/// @{

#define GP_STDIN  0 ///< File descriptor of standard input.
#define GP_STDOUT 1 ///< File descriptor of standard output.
#define GP_STDERR 2 ///< File descriptor of standard error.

/** Enable or disable ANSI escape sequences for given file descriptor.
 *
 * Enable escape sequences for @a fd by setting @a enable to `true`, disable for
 * @a fd by setting @a enable to `false`.
 *
 * Enabling escape sequences does two things:
 * - Check if @a fd refers to a terminal.
 * - Enable colors on terminals that by default do not support colors.
 * This can fail for multiple reasons, most notably standard output might be
 * redirected to a file or other application.
 *
 * Escape sequences should be disabled if application code redirects @a fd to
 * output that is known to not be a terminal. If @a fd is redirected to
 * unknown output, then escape sequences should be re-enabled for that @a fd by
 * calling this function again.
 *
 * Since some terminals don't support colors by default and outputs can be
 * redirected, ANSI colors are disabled for each file descriptor by default.
 * This simply means that @ref gp_ansi_is_enabled() will return `false` for any
 * file descriptor that has not been enabled by this function even if the output
 * would support ANSI escape sequences.
 *
 * @return `true` if ANSI escape sequences have been successfully enabled for
 * @a fd, `false` if enabling failed, in which case `errno` will be set, and
 * `false` if escape sequences were disabled for @a fd by setting @a enable to
 * `false`.
 */
GP_API bool gp_ansi_enable(int fd, bool enable);

/** Get cached result of @ref gp_ansi_enable(). */
GP_API bool gp_ansi_is_enabled(int fd);

// TODO put this example to docs.
// Use these macros to print colored output to terminals that support ANSI
// escape codes. Printing any of these strings changes the output color.
// Example using string concatenation:
/*
    printf(
        GP_RED               "Printing in red! "
        GP_WHITE_BG GP_BLACK "Printing in black with white background! "
        GP_RESET_TERMINAL    "Remember to reset to default color!\n");
*/

#define GP_ANSI(IS_ENABLED, ESCAPE_SEQUENCE) \
( \
    (IS_ENABLED) ? (GP_ANSI_##ESCAPE_SEQUENCE) : "" \
)

GP_INLINE char* gp_ansi(bool is_enabled, const char* escape_sequence)
{
    return (char*)(is_enabled ? escape_sequence : "");
}

#define GP_ANSI_RESET               "\033[0m"

// Swap foreground and background colors.
#define GP_ANSI_INVERT_COLORS       "\033[7m"
#define GP_ANSI_NO_INVERTED_COLORS  "\033[27m"

// ----------------------------------------------------------------------------
/// @defgroup terminal_foreground_color Foreground Color
/// @{

#define GP_ANSI_BLACK               "\033[30m"
#define GP_ANSI_RED                 "\033[31m"
#define GP_ANSI_GREEN               "\033[32m"
#define GP_ANSI_YELLOW              "\033[33m"
#define GP_ANSI_BLUE                "\033[34m"
#define GP_ANSI_MAGENTA             "\033[35m"
#define GP_ANSI_CYAN                "\033[36m"
#define GP_ANSI_WHITE               "\033[37m"

#define GP_ANSI_BRIGHT_BLACK        "\033[90m"
#define GP_ANSI_BRIGHT_RED          "\033[91m"
#define GP_ANSI_BRIGHT_GREEN        "\033[92m"
#define GP_ANSI_BRIGHT_YELLOW       "\033[93m"
#define GP_ANSI_BRIGHT_BLUE         "\033[94m"
#define GP_ANSI_BRIGHT_MAGENTA      "\033[95m"
#define GP_ANSI_BRIGHT_CYAN         "\033[96m"
#define GP_ANSI_BRIGHT_WHITE        "\033[97m"

#define GP_ANSI_RGB(R, G, B)        "\033[38;2;" #R ";" #G ";" #B "m"

/// @}
// ----------------------------------------------------------------------------
/// @defgroup terminal_background_color Background Color
/// @{

#define GP_ANSI_BLACK_BG            "\033[40m"
#define GP_ANSI_RED_BG              "\033[41m"
#define GP_ANSI_GREEN_BG            "\033[42m"
#define GP_ANSI_YELLOW_BG           "\033[43m"
#define GP_ANSI_BLUE_BG             "\033[44m"
#define GP_ANSI_MAGENTA_BG          "\033[45m"
#define GP_ANSI_CYAN_BG             "\033[46m"
#define GP_ANSI_WHITE_BG            "\033[47m"

#define GP_ANSI_BRIGHT_BLACK_BG     "\033[100m"
#define GP_ANSI_BRIGHT_RED_BG       "\033[101m"
#define GP_ANSI_BRIGHT_GREEN_BG     "\033[102m"
#define GP_ANSI_BRIGHT_YELLOW_BG    "\033[103m"
#define GP_ANSI_BRIGHT_BLUE_BG      "\033[104m"
#define GP_ANSI_BRIGHT_MAGENTA_BG   "\033[105m"
#define GP_ANSI_BRIGHT_CYAN_BG      "\033[106m"
#define GP_ANSI_BRIGHT_WHITE_BG     "\033[107m"

#define GP_ANSI_RGB_BG(R, G, B)     "\033[48;2;" #R ";" #G ";" #B "m"

/// @}
// ----------------------------------------------------------------------------
/// @defgroup terminal_font Font
/// @{

#define GP_ANSI_RESET_FONT          "\033[10m"

#define GP_ANSI_BOLD                "\033[1m"
#define GP_ANSI_FAINT               "\033[2m"
#define GP_ANSI_NORMAL_INTENSITY    "\033[22m" // Neither bold nor faint
#define GP_ANSI_ITALIC              "\033[3m"  // Rarely supported
#define GP_ANSI_GOTHIC              "\033[20m" // Rarely supported
#define GP_ANSI_NO_ITALIC           "\033[23m" // Also disables gothic
#define GP_ANSI_UNDERLINE           "\033[4m"
#define GP_ANSI_DOUBLE_UNDERLINE    "\033[21m" // May disable bold instead
#define GP_ANSI_NO_UNDERLINE        "\033[24m" // Also disables double underline
#define GP_ANSI_SLOW_BLINK          "\033[5m"
#define GP_ANSI_FAST_BLINK          "\033[6m"  // Rarely supported
#define GP_ANSI_HIDE                "\033[8m"  // Rarely supported
#define GP_ANSI_REVEAL              "\033[28m" // Unhide
#define GP_ANSI_CROSSED_OUT         "\033[9m"

// Select alternative font from 0 to 9 where 0 is default font
#define GP_ANSI_FONT(N)             "\033[1" #N "m"

/// @}
// ----------------------------------------------------------------------------
/// @defgroup terminal_cursor_movement Cursor Movement

// N = steps to move

#define GP_ANSI_CURSOR_UP(N)            "\033[" #N "A"
#define GP_ANSI_CURSOR_DOWN(N)          "\033[" #N "B"
#define GP_ANSI_CURSOR_FORWARD(N)       "\033[" #N "C"
#define GP_ANSI_CURSOR_BACK(N)          "\033[" #N "D"
#define GP_ANSI_CURSOR_NEXT_LINE(N)     "\033[" #N "E"
#define GP_ANSI_CURSOR_PREVIOUS_LINE(N) "\033[" #N "F"

// Moves cursor to row N
#define GP_ANSI_CURSOR_ROW(N)           "\033[" #N "G"

// Moves cursor to row N column M
#define GP_ANSI_CURSOR_POSITION(N, M)   "\033[" #N ";" #M "H"

/// @}
/// @}
// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------
///@cond

///@endcond
#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_TERMINAL_INCLUDED
