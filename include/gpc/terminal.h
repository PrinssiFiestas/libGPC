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

#endif // GPC_TERMINAL_INCLUDED
