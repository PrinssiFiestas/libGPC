// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

typedef struct FILE FILE;

// Print formatted
// Usage example:
// gpc_print_fmt("int: %i\n", 1, "char: %c\n", 'x', "No format ends print\n");
int gpc_print_fmt(const char* fmt, ...);

int gpc_file_print_fmt(FILE*, const char* fmt, ...);

// Print with automatic formatting. Requires C11 _Generic()
#define gpc_print(...) \
gpc_print_fmt(GPC_LIST_ALL(gpc_type, GPC_COMMA, __VA_ARGS__))
