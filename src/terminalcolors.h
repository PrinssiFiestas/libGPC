// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GPC_TERMINAL_COLORS_H
#define GPC_TERMINAL_COLORS_H

#define GPC_RED(STR_LITERAL)       "\033[0;31m"            STR_LITERAL "\033[0m"
#define GPC_GREEN(STR_LITERAL)     "\033[0;92m"            STR_LITERAL "\033[0m"
#define GPC_ORANGE(STR_LITERAL)    "\033[0;33m"            STR_LITERAL "\033[0m"
#define GPC_MAGENTA(STR_LITERAL)   "\033[0;95m"            STR_LITERAL "\033[0m"
#define GPC_CYAN(STR_LITERAL)      "\033[0;96m"            STR_LITERAL "\033[0m"
#define GPC_WHITE_BG(STR_LITERAL)  "\033[0;47m\033[30m"    STR_LITERAL "\033[0m"

#endif // GPC_TERMINAL_COLORS_H
