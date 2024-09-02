// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file terminal.h
 * ANSI escape sequences.
 */

#ifndef GP_TERMINAL_INCLUDED
#define GP_TERMINAL_INCLUDED


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


/** Relevant for Windows only.*/
void gp_enable_terminal_colors(void);

// Use these macros to print coloured output to terminals that support ANSI
// escape codes. Printing any of these strings changes the output color.
// Example using string concatenation:
/*
    printf(
        GP_RED               "Printing in red! "
        GP_WHITE_BG GP_BLACK "Printing in black with white background! "
        GP_RESET_TERMINAL    "Remember to reset to default color!\n");
*/

#define GP_RESET_TERMINAL      "\033[0m"

// ----------------------------------------------------------------------------
// Foreground color

#define GP_BLACK               "\033[30m"
#define GP_RED                 "\033[31m"
#define GP_GREEN               "\033[32m"
#define GP_YELLOW              "\033[33m"
#define GP_BLUE                "\033[34m"
#define GP_MAGENTA             "\033[35m"
#define GP_CYAN                "\033[36m"
#define GP_WHITE               "\033[37m"

#define GP_BRIGHT_BLACK        "\033[90m"
#define GP_BRIGHT_RED          "\033[91m"
#define GP_BRIGHT_GREEN        "\033[92m"
#define GP_BRIGHT_YELLOW       "\033[93m"
#define GP_BRIGHT_BLUE         "\033[94m"
#define GP_BRIGHT_MAGENTA      "\033[95m"
#define GP_BRIGHT_CYAN         "\033[96m"
#define GP_BRIGHT_WHITE        "\033[97m"

#define GP_RGB(R, G, B)        "\033[38;2;" #R ";" #G ";" #B "m"

// ----------------------------------------------------------------------------
// Background color

#define GP_BLACK_BG            "\033[40m"
#define GP_RED_BG              "\033[41m"
#define GP_GREEN_BG            "\033[42m"
#define GP_YELLOW_BG           "\033[43m"
#define GP_BLUE_BG             "\033[44m"
#define GP_MAGENTA_BG          "\033[45m"
#define GP_CYAN_BG             "\033[46m"
#define GP_WHITE_BG            "\033[47m"

#define GP_BRIGHT_BLACK_BG     "\033[100m"
#define GP_BRIGHT_RED_BG       "\033[101m"
#define GP_BRIGHT_GREEN_BG     "\033[102m"
#define GP_BRIGHT_YELLOW_BG    "\033[103m"
#define GP_BRIGHT_BLUE_BG      "\033[104m"
#define GP_BRIGHT_MAGENTA_BG   "\033[105m"
#define GP_BRIGHT_CYAN_BG      "\033[106m"
#define GP_BRIGHT_WHITE_BG     "\033[107m"

#define GP_RGB_BG(R, G, B)     "\033[38;2;" #R ";" #G ";" #B "m"

// Swap foreground and background colors.
#define GP_INVERT_COLORS       "\033[7m"
#define GP_NO_INVERTED_COLORS  "\033[27m"

// ----------------------------------------------------------------------------
// Font

#define GP_RESET_FONT          "\033[10m"

#define GP_BOLD                "\033[1m"
#define GP_FAINT               "\033[2m"
#define GP_NORMAL_INTENSITY    "\033[22m" // Neither bold nor faint
#define GP_ITALIC              "\033[3m"  // Rarely supported
#define GP_GOTHIC              "\033[20m" // Rarely supported
#define GP_NO_ITALIC           "\033[23m" // Also disables gothic
#define GP_UNDERLINE           "\033[4m"
#define GP_DOUBLE_UNDERLINE    "\033[21m" // May disable bold instead
#define GP_NO_UNDERLINE        "\033[24m" // Also disables double underline
#define GP_SLOW_BLINK          "\033[5m"
#define GP_FAST_BLINK          "\033[6m"  // Rarely supported
#define GP_HIDE                "\033[8m"  // Rarely supported
#define GP_REVEAL              "\033[28m" // Unhide
#define GP_CROSSED_OUT         "\033[9m"

// Select alternative font from 0 to 9 where 0 is default font
#define GP_FONT(N)             "\033[1" #N "m"

// ----------------------------------------------------------------------------
// Cursor movement

// N = steps to move

#define GP_CURSOR_UP(N)            "\033[" #N "A"
#define GP_CURSOR_DOWN(N)          "\033[" #N "B"
#define GP_CURSOR_FORWARD(N)       "\033[" #N "C"
#define GP_CURSOR_BACK(N)          "\033[" #N "D"
#define GP_CURSOR_NEXT_LINE(N)     "\033[" #N "E"
#define GP_CURSOR_PREVIOUS_LINE(N) "\033[" #N "F"

// Moves cursor to row N
#define GP_CURSOR_ROW(N)           "\033[" #N "G"

// Moves cursor to row N column M
#define GP_CURSOR_POSITION(N, M)   "\033[" #N ";" #M "H"

#endif // GP_TERMINAL_INCLUDED
