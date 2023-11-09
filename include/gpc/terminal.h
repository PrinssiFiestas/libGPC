// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_TERMINAL_INCLUDED
#define GPC_TERMINAL_INCLUDED

// Use these macros to print coloured output to terminals that support ANSI
// escape codes. Printing any of these strings changes the output color.
// Example using string concatenation:
/*
    printf(
        GPC_RED                "Printing in red! "
        GPC_WHITE_BG GPC_BLACK "Printing in black with white background! "
        GPC_RESET_TERMINAL     "Remember to reset to default color!\n");
*/

#define GPC_RESET_TERMINAL      "\033[0m"

// ----------------------------------------------------------------------------
// Foreground color

#define GPC_BLACK               "\033[30m"
#define GPC_RED                 "\033[31m"
#define GPC_GREEN               "\033[32m"
#define GPC_YELLOW              "\033[33m"
#define GPC_BLUE                "\033[34m"
#define GPC_MAGENTA             "\033[35m"
#define GPC_CYAN                "\033[36m"
#define GPC_WHITE               "\033[37m"

#define GPC_BRIGHT_BLACK        "\033[90m"
#define GPC_BRIGHT_RED          "\033[91m"
#define GPC_BRIGHT_GREEN        "\033[92m"
#define GPC_BRIGHT_YELLOW       "\033[93m"
#define GPC_BRIGHT_BLUE         "\033[94m"
#define GPC_BRIGHT_MAGENTA      "\033[95m"
#define GPC_BRIGHT_CYAN         "\033[96m"
#define GPC_BRIGHT_WHITE        "\033[97m"

#define GPC_RGB(R, G, B)        "\033[38;2;" #R ";" #G ";" #B "m"

// ----------------------------------------------------------------------------
// Background color

#define GPC_BLACK_BG            "\033[40m"
#define GPC_RED_BG              "\033[41m"
#define GPC_GREEN_BG            "\033[42m"
#define GPC_YELLOW_BG           "\033[43m"
#define GPC_BLUE_BG             "\033[44m"
#define GPC_MAGENTA_BG          "\033[45m"
#define GPC_CYAN_BG             "\033[46m"
#define GPC_WHITE_BG            "\033[47m"

#define GPC_BRIGHT_BLACK_BG     "\033[100m"
#define GPC_BRIGHT_RED_BG       "\033[101m"
#define GPC_BRIGHT_GREEN_BG     "\033[102m"
#define GPC_BRIGHT_YELLOW_BG    "\033[103m"
#define GPC_BRIGHT_BLUE_BG      "\033[104m"
#define GPC_BRIGHT_MAGENTA_BG   "\033[105m"
#define GPC_BRIGHT_CYAN_BG      "\033[106m"
#define GPC_BRIGHT_WHITE_BG     "\033[107m"

#define GPC_RGB_BG(R, G, B)     "\033[38;2;" #R ";" #G ";" #B "m"

// Swap foreground and background colors.
#define GPC_INVERT_COLORS       "\033[7m"
#define GPC_NO_INVERTED_COLORS  "\033[27m"

// ----------------------------------------------------------------------------
// Font

#define GPC_RESET_FONT          "\033[10m"

#define GPC_BOLD                "\033[1m"
#define GPC_FAINT               "\033[2m"
#define GPC_NORMAL_INTENSITY    "\033[22m" // Neither bold nor faint
#define GPC_ITALIC              "\033[3m"  // Rarely supported
#define GPC_GOTHIC              "\033[20m" // Rarely supported
#define GPC_NO_ITALIC           "\033[23m" // Also disables gothic
#define GPC_UNDERLINE           "\033[4m"
#define GPC_DOUBLE_UNDERLINE    "\033[21m" // May disable bold instead
#define GPC_NO_UNDERLINE        "\033[24m" // Also disables double underline
#define GPC_SLOW_BLINK          "\033[5m"
#define GPC_FAST_BLINK          "\033[6m"  // Rarely supported
#define GPC_HIDE                "\033[8m"  // Rarely supported
#define GPC_REVEAL              "\033[28m" // Unhide
#define GPC_CROSSED_OUT         "\033[9m"

// Select alternative font from 0 to 9 where 0 is default font
#define GPC_FONT(N)             "\033[1" #N "m"

// ----------------------------------------------------------------------------
// Cursor movement

// N = steps to move

#define GPC_CURSOR_UP(N)            "\033[" #N "A"
#define GPC_CURSOR_DOWN(N)          "\033[" #N "B"
#define GPC_CURSOR_FORWARD(N)       "\033[" #N "C"
#define GPC_CURSOR_BACK(N)          "\033[" #N "D"
#define GPC_CURSOR_NEXT_LINE(N)     "\033[" #N "E"
#define GPC_CURSOR_PREVIOUS_LINE(N) "\033[" #N "F"

// Moves cursor to row N
#define GPC_CURSOR_ROW(N)           "\033[" #N "G"

// Moves cursor to row N column M
#define GPC_CURSOR_POSITION(N, M)   "\033[" #N ";" #M "H"

#endif // GPC_TERMINAL_INCLUDED
