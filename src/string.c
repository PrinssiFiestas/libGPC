// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// TODO GET RID OF THIS
#include <signal.h>
#ifdef __GNUC__
    #define GP_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
    #define GP_ALWAYS_INLINE inline
#endif
GP_ALWAYS_INLINE void gp_debug_segfault(void) // TODO just put this in a macro
{
    #if GP_DEBUG
        raise(SIGSEGV);
    #endif
}

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <printf/printf.h>
#include "pfstring.h"

size_t gp_cstr_copy(
    char*restrict dest,
    const char*restrict src)
{
    size_t len = strlen(src);
    memcpy(dest, src, len + sizeof(""));
    return len;
}

size_t gp_cstr_copy_n(
    char*restrict dest,
    const char*restrict src,
    size_t n)
{
    memcpy(dest, src, n);
    dest[n] = '\0';
    return n;
}

size_t gp_cstr_slice(
    char* str,
    size_t start,
    size_t end)
{
    memmove(str, str + start, end - start);
    str[end - start] = '\0';
    return end - start;
}

size_t gp_big_cstr_slice(
    char** str,
    size_t start,
    size_t end)
{
    *str += start;
    (*str)[end - start] = '\0';
    return end - start;
}

size_t gp_cstr_substr(
    char*restrict dest,
    const char*restrict src,
    size_t start,
    size_t end)
{
    memcpy(dest, src + start, end - start);
    dest[end - start] = '\0';
    return end - start;
}

size_t gp_cstr_append(
    char*restrict dest,
    const char*restrict src)
{
    size_t dest_length = strlen(dest);
    size_t src_length  = strlen(src);
    memcpy(dest + dest_length, src, src_length + sizeof(""));
    dest[dest_length + src_length] = '\0';
    return dest_length + src_length;
}

size_t gp_cstr_append_n(
    char*restrict dest,
    const char*restrict src,
    size_t n)
{
    size_t dest_length = strlen(dest);
    memcpy(dest + dest_length, src, n);
    dest[dest_length + n] = '\0';
    return dest_length + n;
}

size_t gp_cstr_insert(
    char*restrict dest,
    size_t pos,
    const char*restrict src)
{
    size_t dest_length = strlen(dest);
    size_t src_length  = strlen(src);
    memmove(dest + pos + src_length, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, src_length);
    dest[dest_length + src_length] = '\0';
    return dest_length + src_length;
}

size_t gp_cstr_insert_n(
    char*restrict dest,
    size_t pos,
    const char*restrict src,
    size_t n)
{
    size_t dest_length = strlen(dest);
    memmove(dest + pos + n, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, n);
    dest[dest_length + n] = '\0';
    return dest_length + n;
}

static size_t cstr_replace_range(
    const size_t me_length,
    char*restrict me,
    const size_t start,
    const size_t end,
    const char* replacement,
    const size_t replacement_length)
{
    memmove(
        me + start + replacement_length,
        me + end,
        me_length - end);

    memcpy(me + start, replacement, replacement_length);
    return me_length + replacement_length - (end - start);
}

size_t gp_cstr_replace(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_in_start_out_pos)
{
    size_t start = optional_in_start_out_pos != NULL ?
        *optional_in_start_out_pos : 0;

    if ((start = gp_cstr_find(haystack, needle, start)) == GP_NOT_FOUND)
    {
        if (optional_in_start_out_pos != NULL)
            *optional_in_start_out_pos = GP_NOT_FOUND;
        return strlen(haystack);
    }
    const size_t haystack_length    = strlen(haystack);
    const size_t needle_length      = strlen(needle);
    const size_t replacement_length = strlen(replacement);
    const size_t end = start + needle_length;

    const size_t out_length = cstr_replace_range(
        haystack_length,
        haystack,
        start,
        end,
        replacement,
        replacement_length);

    haystack[out_length] = '\0';
    if (optional_in_start_out_pos != NULL)
        *optional_in_start_out_pos = start;

    return out_length;
}

size_t gp_cstr_replace_all(
    char*restrict haystack,
    const char*restrict needle,
    const char*restrict replacement,
    size_t* optional_replacement_count)
{
          size_t haystack_length    = strlen(haystack);
    const size_t needle_length      = strlen(needle);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_cstr_find(haystack, needle, start)) != GP_NOT_FOUND)
    {
        haystack_length = cstr_replace_range(
            haystack_length,
            haystack,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    haystack[haystack_length] = '\0';
    if (optional_replacement_count != NULL)
        *optional_replacement_count = replacement_count;

    return replacement_count;
}

size_t gp_cstr_print_internal(
    char*restrict _out,
    const size_t arg_count,
    const struct GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    //https://stackoverflow.com/questions/8047362/is-gcc-mishandling-a-pointer-to-a-va-list-passed-to-a-function
    pf_va_list args;
    va_copy(args.list, _args);

    struct PFString out_ = { _out, 0, (size_t)-1 };
    struct PFString* out = &out_;

    for (size_t i = 0; i < arg_count; i++)
    {
        if (objs[i].identifier[0] == '\"')
        {
            // Check if this is a format string or any other literal.
            const char* fmt = objs[i].identifier;
            while (true)
            {
                fmt = strchr(fmt, '%');
                if (fmt == NULL || fmt[1] != '%')
                    break;
                fmt++;
            }

            const char* fmt_without_quotes = va_arg(args.list, char*);
            if (fmt == NULL) { // not a format string
                concat(out, fmt_without_quotes, strlen(fmt_without_quotes));
            } else {
                out->length += pf_vsnprintf_consuming(
                    out->data + out->length,
                    capacity_left(*out),
                    fmt_without_quotes,
                    &args);
                i++;
            }

            continue;
        }

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                push_char(out, (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                out->length += pf_utoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                if (va_arg(args.list, int))
                    concat(out, "true", strlen("true"));
                else
                    concat(out, "false", strlen("false"));
                break;

            case GP_SHORT:
            case GP_INT:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, int));
                break;

            case GP_LONG:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, long int));
                break;

            case GP_LONG_LONG:
                out->length += pf_itoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, long long int));
                break;

            case GP_FLOAT:
            case GP_DOUBLE:
                out->length += pf_ftoa(
                    capacity_left(*out),
                    out->data + out->length,
                    va_arg(args.list, double));
                break;

            void* p;
            case GP_CHAR_PTR:
                p = va_arg(args.list, char*);
                concat(out, p, strlen(p));
                break;

            case GP_PTR:
                p = va_arg(args.list, void*);
                if (p != NULL) {
                    out->length += pf_xtoa(
                        capacity_left(*out),
                        out->data + out->length,
                        (uintptr_t)p);
                } else {
                    concat(out, "(nil)", strlen("(nil)"));
                } break;
        }
    }
    va_end(_args);
    va_end(args.list);
    if (out->capacity > 0)
        out->data[capacity_left(*out) ? out->length : out->capacity - 1] = '\0';

    return out->length;
}

// // MACRO
// size_t gp_cstr_print_n(
//     size_t n,
//     char*restrict out_str, // optional if n == 0
//     ...);
//
// //MACRO
// size_t gp_cstr_println(
//     char*restrict out_str,
//     ...) GP_NONNULL_ARGS(1);
//
// //MACRO
// size_t gp_cstr_println_n(
//     size_t n,
//     char*restrict out_str, // optional if n == 0
//     ...);

// Modes: 'l' left, 'r' right, 'u' UTF-8. Bitwise or.
// Whitespace is these: " \t\n\v\f\r". 'u' adds unicode whitespace.
size_t gp_cstr_trim(
    char*restrict str,
    const char*restrict optional_char_set, // whitespace if NULL
    int mode) GP_NONNULL_ARGS(1);
/*{

    // UTF-8 stuff
    // some loop construct here
        char utf8_char[5] = "";
        size_t utf8_char_len = // decode length
        memcpy(utf8_char, input, utf_8_char_len);
        if (strstr(char_set, utf8_char))
            input++;
        else
            break;
}*/

size_t gp_big_cstr_trim(
    char*restrict* str,
    const char*restrict optional_char_set,
    int mode) GP_NONNULL_ARGS(1);

size_t gp_cstr_to_upper(
    char*restrict str) GP_NONNULL_ARGS();

size_t gp_cstr_to_lower(
    char*restrict str) GP_NONNULL_ARGS();

bool gp_cutf8_validate(
    const char* str,
    size_t* optional_out_utf8_length) GP_NONNULL_ARGS(1);

size_t gp_cutf8_to_wcstr(
    wchar_t*restrict wcstr_buf, // with cap sizeof(wchar_t)*(strlen(utf8_src)+1)
    const char*restrict utf8_src) GP_NONNULL_ARGS();

size_t gp_cutf8_to_c16str(
    uint_least16_t*restrict c16str_buf, // with cap sizeof(char16_t)*(strlen(utf8_src)+1)
    const char*restrict utf8_src) GP_NONNULL_ARGS();

size_t gp_cutf8_to_c32str(
    uint_least32_t*restrict c32str_buf, // with cap sizeof(char32_t)*(strlen(utf8_src)+1)
    const char*restrict utf8_src) GP_NONNULL_ARGS();

size_t gp_wcstr_to_cutf8(
    char*restrict utf8_buf, // with cap sizeof(wchar_t)*wcslen(wcstr_src)+1
    const wchar_t*restrict wcstr_src) GP_NONNULL_ARGS();

size_t gp_c16str_to_cutf8(
    char*restrict utf8_buf, // with cap sizeof(char16_t)*2*strlen(c16str_src)+1
    const uint_least16_t*restrict c16str_src) GP_NONNULL_ARGS();

size_t gp_c32str_to_cutf8(
    char*restrict utf8_buf, // with cap sizeof(char32_t)*4*strlen(c32str_src)+1
    const uint_least32_t*restrict c32str_src) GP_NONNULL_ARGS();

// String examination
size_t gp_cstr_find(const char* haystack, const char* needle, size_t start)
{
    const char* result = strstr(haystack + start, needle);
    return result ? (size_t)(result - haystack) : GP_NOT_FOUND;
}

// Find first occurrence of ch looking from right to left
static const char* memchr_r(const char* ptr_r, const char ch, size_t count)
{
    const char* position = NULL;
    while (--ptr_r, --count != (size_t)-1) // <=> count >= 0
    {
        if (*ptr_r == ch) {
            position = ptr_r;
            break;
        }
    }
    return position;
}

size_t gp_cstr_find_last(const char* haystack, const char* needle)
{
    size_t haystack_length = strlen(haystack);
    size_t needle_length = strlen(needle);

    if (needle_length > haystack_length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle_length - 1;
    const char* data = haystack + haystack_length - needle_last;
    size_t to_be_searched = haystack_length - needle_last;

    while ((data = memchr_r(data, needle[0], to_be_searched)))
    {
        if (memcmp(data, needle, needle_length) == 0)
        {
            position = (size_t)(data - haystack);
            break;
        }
        data--;
        const char* haystack_end = haystack + haystack_length;
        to_be_searched = haystack_length - (size_t)(haystack_end - data);
    }
    return position;
}

size_t gp_cstr_count(const char* haystack, const char* needle)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_cstr_find(haystack, needle, i)) != GP_NOT_FOUND) {
        count++;
        i++;
    }
    return count;
}

bool gp_cstr_equal(const char* s1, const char* s2)
{
    return strcmp(s1, s2) == 0;
}





// TODO GET RID OF OLD STUFF --------------------------------------------------

extern inline const char* gpcstr(struct GPString str);
extern inline struct GPString gpstr(const char cstr[GP_NONNULL]);

// ----------------------------------------------------------------------------

struct GPString* gpstr_copy(struct GPString dest[GP_NONNULL], const struct GPString src)
{
    memcpy(dest->data, src.data, src.length);
    dest->length = src.length;
    return dest;
}

bool gpstr_eq(const struct GPString s1, const struct GPString s2)
{
    if (s1.length != s2.length)
        return false;
    return memcmp(s1.data, s2.data, s1.length) == 0;
}

struct GPString* gpstr_slice(
    struct GPString str[GP_NONNULL],
    const size_t start,
    const size_t end) // NOT inclusive!
{
    if (start >= str->length || end > str->length || end < start)
        gp_debug_segfault();

    const size_t length = end - start;
    memmove(str->data, str->data + start, length);
    str->length = length;

    return str;
}

struct GPString* gpstr_substr(
    struct GPString dest[GP_NONNULL],
    const struct GPString src,
    const size_t start,
    const size_t end)
{
    if (start >= src.length || end > src.length || end < start)
        gp_debug_segfault();

    const size_t length = end - start;
    memcpy(dest->data, src.data + start, length);
    dest->length = length;

    return dest;
}

struct GPString* gpstr_insert(
    struct GPString dest[GP_NONNULL],
    const size_t pos,
    const struct GPString src)
{
    if (pos >= dest->length + 1) // +1 because +0 is allowed for appending
        gp_debug_segfault();

    memmove(dest->data + pos + src.length, dest->data + pos, dest->length - pos);
    memcpy(dest->data + pos, src.data, src.length);
    dest->length += src.length;

    return dest;
}

size_t gpstr_find(const struct GPString haystack, const struct GPString needle, const size_t start)
{
    if (needle.length > haystack.length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const char* data = haystack.data + start;
    size_t to_be_searched = haystack.length - start;

    while ((data = memchr(data, needle.data[0], to_be_searched)))
    {
        if (memcmp(data, needle.data, needle.length) == 0)
        {
            position = (size_t)(data - haystack.data);
            break;
        }
        data++;
        to_be_searched = haystack.length - (size_t)(data - haystack.data);
    }
    return position;
}

size_t gpstr_find_last(const struct GPString haystack, const struct GPString needle)
{
    if (needle.length > haystack.length)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle.length - 1;
    const char* data = haystack.data + haystack.length - needle_last;
    size_t to_be_searched = haystack.length - needle_last;

    while ((data = memchr_r(data, needle.data[0], to_be_searched)))
    {
        if (memcmp(data, needle.data, needle.length) == 0)
        {
            position = (size_t)(data - haystack.data);
            break;
        }
        data--;
        const char* haystack_end = haystack.data + haystack.length;
        to_be_searched = haystack.length - (size_t)(haystack_end - data);
    }
    return position;
}

size_t gpstr_count(const struct GPString haystack, const struct GPString needle)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gpstr_find(haystack, needle, i)) != GP_NOT_FOUND) {
        count++;
        i++;
    }
    return count;
}

static struct GPString* replace_range(
    struct GPString me[GP_NONNULL],
    const size_t start,
    const size_t end,
    const struct GPString replacement)
{
    memmove(
        me->data + start + replacement.length,
        me->data + end,
        me->length - end);

    memcpy(me->data + start, replacement.data, replacement.length);
    me->length += replacement.length - (end - start);
    return me;
}

size_t gpstr_replace(
    struct GPString me[GP_NONNULL],
    const struct GPString needle,
    const struct GPString replacement,
    size_t start)
{
    if ((start = gpstr_find(*me, needle, start)) == GP_NOT_FOUND)
        return GP_NOT_FOUND;
    const size_t end = start + needle.length;
    replace_range(me, start, end, replacement);
    return start;
}

unsigned gpstr_replace_all(
    struct GPString me[GP_NONNULL],
    const struct GPString needle,
    const struct GPString replacement)
{
    size_t start = 0;
    unsigned replacement_count = 0;
    while ((start = gpstr_find(*me, needle, start)) != GP_NOT_FOUND)
    {
        replace_range(me, start, start + needle.length, replacement);
        start += replacement.length;
        replacement_count++;
    }
    return replacement_count;
}

struct GPString*
gpstr_trim(struct GPString me[GP_NONNULL], const char char_set[GP_NONNULL], int mode)
{
    size_t i_l = 0;
    size_t i_r = me->length;

    if (mode == 'l' || mode == 'l' + 'r')
        while (strchr(char_set, me->data[i_l]))
            i_l++;

    if (mode == 'r' || mode == 'l' + 'r') {
        i_r--;
        while (strchr(char_set, me->data[i_r]))
            i_r--;
    }

    memmove(me->data, me->data + i_l, me->length - i_l);
    me->length -= i_l + (me->length - i_r);
    if (mode == 'r' || mode == 'l' + 'r')
        me->length++;
    return me;
}

struct GPString*
gpstr_print_internal(
    struct GPString me[GP_NONNULL], const size_t arg_count, char** args)
{
    for (size_t i = 0; i < arg_count; i++)
    {
        if (args[i][0] != 0 && args[i][0] != 1)
        {
            gpstr_insert(me, me->length, gpstr(args[i]));
        }
        else if (args[i][0] == 0) // cstr
        {
            const char* real_arg;
            memcpy(&real_arg, args[i] + 1, sizeof(real_arg));
            gpstr_insert(me, me->length, gpstr(real_arg));
        }
        else // gpstr
        {
            struct GPString real_arg;
            memcpy(&real_arg, args[i] + 1, sizeof(real_arg));
            gpstr_insert(me, me->length, real_arg);
        }
    }
    return me;
}
