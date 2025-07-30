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

#ifdef __SANITIZE_ADDRESS__ // GCC and MSVC defines this with -fsanitize=address
#include <sanitizer/asan_interface.h>
#include <sanitizer/common_interface_defs.h>
#define GP_HAS_SANITIZER 1
#elif defined(__has_feature) // Clang defines this
    #if __has_feature(address_sanitizer)
    #include <sanitizer/asan_interface.h>
    #include <sanitizer/common_interface_defs.h>
    #define GP_HAS_SANITIZER 1
    #else
    #define ASAN_POISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
    #define ASAN_UNPOISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
    #define GP_HAS_SANITIZER 0
    #endif
#else
#define ASAN_POISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
#define ASAN_UNPOISON_MEMORY_REGION(A, S) ((void)(A), (void)(S))
#define GP_HAS_SANITIZER 0
#endif

void gp_arena_dealloc(GPAllocator*, void*);
void gp_carena_dealloc(GPAllocator*, void*);

static inline size_t gp_max_digits_in(const GPType T)
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
