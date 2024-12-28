// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifndef GP_PRINT_COMMON_INCLUDED
#define GP_PRINT_COMMON_INCLUDED

#include <gpc/overload.h>
#include <gpc/attributes.h>
#include <gpc/bytes.h>
#include <gpc/array.h>
#include <gpc/utils.h>
#include <printf/conversions.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#else
#define ASAN_POISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
#define ASAN_UNPOISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
#endif

// Disable false UB positive for calling functions trough "incompatible" pointer
// types.
#if __clang_major__ > 14
#define GP_NO_FUNCTION_POINTER_SANITIZE __attribute__((no_sanitize("undefined")))
#else
#define GP_NO_FUNCTION_POINTER_SANITIZE
#endif

#ifndef __COMPCERT__
inline void gp_arena_dealloc(const GPAllocator* arena, void* mem)
{
    (void)arena;
    ASAN_POISON_MEMORY_REGION(mem, sizeof(void*));
}
#else // define in common.c so the linker can find it
void gp_arena_dealloc(const GPAllocator*, void*);
#endif

inline size_t gp_max_digits_in(const GPType T)
{
    switch (T)
    {
        case GP_FLOAT: // promoted
        case GP_DOUBLE: // %g
            return strlen("-0.111111e-9999");

        case GP_PTR:
            return strlen("0x") + sizeof(void*) * strlen("ff");

        default: // integers https://www.desmos.com/calculator/c1ftloo5ya
            return (gp_sizeof(T) * 18) / CHAR_BIT + 2;
    }
    return 0;
}

bool gp_valid_codepoint(uint32_t c);

GP_NONNULL_ARGS()
size_t gp_bytes_codepoint_count(
    const void* _str,
    const size_t n);

GP_NONNULL_ARGS(1)
bool gp_bytes_is_valid_utf8(
    const void* str,
    size_t str_length,
    size_t* optional_invalid_index);

GP_NONNULL_ARGS()
inline size_t gp_count_fmt_specs(const char* fmt)
{
    size_t i = 0;
    for (; (fmt = strchr(fmt, '%')) != NULL; fmt++)
    {
        if (fmt[1] == '%') {
            fmt++;
        } else  { // consuming more args
            const char* fmt_spec = strpbrk(fmt, "csSdioxXufFeEgGp");
            for (const char* c = fmt; c < fmt_spec; c++) if (*c == '*')
                i++; // consume asterisks as well
            i++;
        }
    }
    return i;
}

GP_NONNULL_ARGS(3)
size_t gp_convert_va_arg(
    const size_t limit,
    void*restrict const out,
    pf_va_list*restrict const args,
    const GPType type);

GP_NONNULL_ARGS(3, 4)
size_t gp_bytes_print_objects(
    const size_t limit,
    void*restrict out,
    pf_va_list* args,
    size_t*const i,
    GPPrintable obj);

#endif // GP_PRINT_COMMON_INCLUDED
