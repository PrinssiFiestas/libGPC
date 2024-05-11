// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifdef __GNUC__
#define _GNU_SOURCE // memmem()
#endif

#include <gpc/bytes.h>
#include <gpc/utils.h>
#include <gpc/overload.h>
#include <gpc/string.h>
#include <printf/conversions.h>
#include "pfstring.h"
#include <stdlib.h> // malloc() TODO use allocator instead
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>
#include <limits.h>

static void* gp_memmem(
    const void* haystack, const size_t hlen, const void* needle, const size_t nlen)
{
    #if defined(__GNUC__) && defined(__linux__)
    return memmem(haystack, hlen, needle, nlen);
    #endif
    if (hlen == 0 || nlen == 0)
        return NULL;

    const char n0 = *(char*)needle;
    for (void* p = memchr(haystack, n0, hlen); p != NULL;)
    {
        if (p + nlen > haystack + hlen)
            return NULL;
        if (memcmp(p, needle, nlen) == 0)
            return p;

        p++;
        p = memchr(p, n0, hlen - (p - haystack));
    }
    return NULL;
}

size_t gp_bytes_find(
    const void*  haystack,
    const size_t haystack_size,
    const void*  needle,
    const size_t needle_size,
    const size_t start)
{
    const void* result = gp_memmem(
        haystack + start, haystack_size - start, needle, needle_size);
    return result ? (size_t)(result - haystack) : GP_NOT_FOUND;
}

// Find first occurrence of ch looking from right to left
static const char* gp_memchr_r(const char* ptr_r, const char ch, size_t count)
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

size_t gp_bytes_find_last(
    const void*  _haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_length)
{
    const char* haystack = (const char*)_haystack;

    if (needle_length > haystack_length || needle_length==0 || haystack_length==0)
        return GP_NOT_FOUND;

    size_t position = GP_NOT_FOUND;
    const size_t needle_last = needle_length - 1;
    const char* data = haystack + haystack_length - needle_last;
    size_t to_be_searched = haystack_length - needle_last;

    while ((data = gp_memchr_r(data, *(char*)needle, to_be_searched)))
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

size_t gp_bytes_count(
    const void*  haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_size)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_bytes_find(haystack, haystack_length, needle, needle_size, i))
        != GP_NOT_FOUND)
    {
        count++;
        i++;
    }
    return count;
}

bool gp_bytes_equal(
    const void*  s1,
    const size_t s1_size,
    const void*  s2,
    const size_t s2_size)
{
    if (s1_size != s2_size)
        return false;
    else
        return memcmp(s1, s2, s2_size) == 0;
}

size_t gp_bytes_codepoint_length(
    const void* str)
{
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[*(uint8_t*)str >> 3];
}

bool gp_bytes_equal_case(
    const void*  s1,
    const size_t s1_size,
    const void*  s2,
    const size_t s2_size)
{
    size_t s1_length = gp_bytes_codepoint_count(s1, s1_size);
    size_t s2_length = gp_bytes_codepoint_count(s2, s2_size);
    if (s1_length != s2_length)
        return false;

    mbstate_t state1 = {0};
    mbstate_t state2 = {0};
    wchar_t wc1;
    wchar_t wc2;
    for (size_t i = 0; i < s1_length; i++)
    {
        size_t wc1_length = mbrtowc(&wc1, (char*)s1, sizeof(wchar_t), &state1);
        size_t wc2_length = mbrtowc(&wc2, (char*)s2, sizeof(wchar_t), &state2);
        if (sizeof(wchar_t) < sizeof(uint32_t)/* Windows probably */&&
            (wc1_length == (size_t)-2) != (wc2_length == (size_t)-2))
        { // one fits to wchar_t and other doesn't so most likely different
            return false;
        }
        else if (sizeof(wchar_t) < sizeof(uint32_t) &&
                 wc1_length == (size_t)-2) // char wider than sizeof(wchar_t)
        {                                  // so just compare raw bytes
            size_t s1_codepoint_size = gp_bytes_codepoint_length(s1);
            size_t s2_codepoint_size = gp_bytes_codepoint_length(s2);
            if (s1_codepoint_size != s2_codepoint_size ||
                memcmp(s1, s2, s1_codepoint_size) != 0)
            {
                return false;
            }
            s1 += s1_codepoint_size;
            s2 += s2_codepoint_size;
        }
        else
        {
            wc1 = towlower(wc1);
            wc2 = towlower(wc2);
            if (wc1 != wc2)
                return false;

            s1 += wc1_length;
            s2 += wc2_length;
        }
    }
    return true;
}

size_t gp_bytes_codepoint_count(
    const void* _str,
    const size_t n)
{
    size_t count = 0;
    const char* str = _str;
    static const size_t valid_leading_nibble[] = {
        1,1,1,1, 1,1,1,1, 0,0,0,0, 1,1,1,1
    };
    const size_t align_offset = (uintptr_t)str     % 8;
    const size_t remaining    = (n - align_offset) % 8;
    size_t i = 0;

    for (size_t len = gp_min(align_offset, n); i < len; i++)
        count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];

    for (; i < n - remaining; i += 8)
    {
        // Read 8 bytes to be processed in parallel
        uint64_t x;
        memcpy(&x, str + i, sizeof x);

        // Extract bytes that start with 0b10
        const uint64_t a =   x & 0x8080808080808080llu;
        const uint64_t b = (~x & 0x4040404040404040llu) << 1;

        // Each byte in c is either 0 or 0b10000000
        uint64_t c = a & b;

        uint32_t bit_count;
        #ifdef __clang__ // only Clang seems to benefit from popcount()
        bit_count = __builtin_popcountll(c);
        #else
        //https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
        uint32_t v0 = c & 0xffffffffllu;
        uint32_t v1 = c >> 32;

        v0 = v0 - (v0 >> 1);
        v0 = (v0 & 0x33333333) + ((v0 >> 2) & 0x33333333);
        bit_count = (((v0 + (v0 >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;

        v1 = v1 - (v1 >> 1);
        v1 = (v1 & 0x33333333) + ((v1 >> 2) & 0x33333333);
        bit_count += (((v1 + (v1 >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
        #endif

        count += 8 - bit_count;
    }
    for (; i < n; i++)
        count += valid_leading_nibble[(uint8_t)*(str + i) >> 4];

    return count;
}

// https://dev.to/rdentato/utf-8-strings-in-c-2-3-3kp1
static bool gp_valid_codepoint(
    const uint32_t c)
{
  if (c <= 0x7Fu)
      return true;

  if (0xC280u <= c && c <= 0xDFBFu)
     return ((c & 0xE0C0u) == 0xC080u);

  if (0xEDA080u <= c && c <= 0xEDBFBFu)
     return 0; // Reject UTF-16 surrogates

  if (0xE0A080u <= c && c <= 0xEFBFBFu)
     return ((c & 0xF0C0C0u) == 0xE08080u);

  if (0xF0908080u <= c && c <= 0xF48FBFBFu)
     return ((c & 0xF8C0C0C0u) == 0xF0808080u);

  return false;
}

bool gp_bytes_is_valid(
    const void*restrict _str,
    const size_t length)
{
    const char* str = (const char*)_str;
    for (size_t i = 0; i < length;)
    {
        size_t cp_length = gp_bytes_codepoint_length(str + i);
        if (cp_length == 0 || i + cp_length > length)
            return false;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)str[i + j];
        if ( ! gp_valid_codepoint(codepoint))
            return false;

        i += cp_length;
    }
    return true;
}

size_t gp_bytes_copy(
    void*restrict dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    memcpy(dest, src + start, end - start);
    return end - start;
}

size_t gp_bytes_repeat(
    void*restrict dest,
    const size_t n,
    const void*restrict mem,
    const size_t mem_length)
{
    if (mem_length == 1) {
        memset(dest, *(uint8_t*)mem, n);
    } else for (size_t i = 0; i < n; i++) {
        memcpy(dest + i * mem_length, mem, mem_length);
    }
    return n * mem_length;
}

size_t gp_bytes_append(
    void*restrict dest,
    const size_t dest_length,
    const void* src,
    const size_t src_length)
{
    memcpy(dest + dest_length, src, src_length + sizeof(""));
    return dest_length + src_length;
}

size_t gp_bytes_insert(
    void*restrict dest,
    const size_t dest_length,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    memmove(dest + pos + n, dest + pos, dest_length - pos);
    memcpy(dest + pos, src, n);
    return dest_length + n;
}

size_t gp_bytes_replace_range(
    void*restrict me,
    const size_t me_length,
    const size_t start,
    const size_t end,
    const void*restrict replacement,
    const size_t replacement_length)
{
    memmove(
        me + start + replacement_length,
        me + end,
        me_length - end);

    memcpy(me + start, replacement, replacement_length);
    return me_length + replacement_length - (end - start);
}
#if 0 // TODO use arrays when implemented to implement this to optimize replace_all()
static size_t gp_bytes_find_indices(
// vvvvvv MODIFY ALL THIS vvvvvvvvvvvvvv
    const void*  haystack,
    const size_t haystack_length,
    const void*  needle,
    const size_t needle_size)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_bytes_find(haystack, haystack_length, needle, needle_size, i))
        != GP_NOT_FOUND)
    {
        count++;
        i++;
    }
    return count;
}
#endif
size_t gp_bytes_replace(
    void*restrict haystack,
    const size_t haystack_length,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t* in_start_out_pos)
{
    size_t start = in_start_out_pos != NULL ? *in_start_out_pos : 0;
    if ((start = gp_bytes_find(haystack, haystack_length, needle, needle_length, start))
        == GP_NOT_FOUND) {
        return GP_NOT_FOUND;
    }

    if (in_start_out_pos != NULL)
        *in_start_out_pos = start;

    const size_t end = start + needle_length;
    return gp_bytes_replace_range(
        haystack,
        haystack_length,
        start,
        end,
        replacement,
        replacement_length);
}

size_t gp_bytes_replace_all(
    void*restrict haystack,
    size_t haystack_length,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t* optional_replacement_count)
{
    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_bytes_find(haystack, haystack_length, needle, needle_length, start))
        != GP_NOT_FOUND)
    {
        haystack_length = gp_bytes_replace_range(
            haystack,
            haystack_length,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    if (optional_replacement_count != NULL)
        *optional_replacement_count = replacement_count;
    return haystack_length;
}

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
            return (gp_sizeof(T) * 18)/CHAR_BIT + 2;
    }
    return 0;
}

size_t gp_bytes_print_internal(
    bool is_println,
    bool is_n,
    void*restrict _out,
    const size_t n,
    const size_t arg_count,
    const struct GPBytesPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    PFString out = {_out };
    bool capacity_sufficed_for_trailing_space = false;

    for (size_t i = 0; i < arg_count; i++)
    {
        if (objs[i].identifier[0] == '\"')
        {
            const char* fmt = va_arg(args.list, char*);
            for (const char* c = fmt; (c = strchr(c, '%')) != NULL; c++)
            {
                if (c[1] == '%')
                    c++;
                else // consuming more args
                    i++;
            }
            out.length += pf_vsnprintf_consuming(
                out.data + out.length,
                pf_capacity_left(out),
                fmt,
                &args);

            if (is_println) {
                capacity_sufficed_for_trailing_space = pf_push_char(&out, ' ');
            }
            continue;
        }

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                pf_push_char(&out, (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                out.length += pf_utoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                out.length += pf_utoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                out.length += pf_utoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                if (va_arg(args.list, int))
                    pf_concat(&out, "true", strlen("true"));
                else
                    pf_concat(&out, "false", strlen("false"));
                break;

            case GP_SHORT:
            case GP_INT:
                out.length += pf_itoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, int));
                break;

            case GP_LONG:
                out.length += pf_itoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, long int));
                break;

            case GP_LONG_LONG:
                out.length += pf_itoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, long long int));
                break;

            case GP_FLOAT:
            case GP_DOUBLE:
                out.length += pf_gtoa(
                    pf_capacity_left(out),
                    out.data + out.length,
                    va_arg(args.list, double));
                break;

            char* p;
            size_t p_len;
            case GP_CHAR_PTR:
                p = va_arg(args.list, char*);
                p_len = strlen(p);
                pf_concat(&out, p, p_len);
                break;

            GPString s;
            case GP_STRING:
                s = va_arg(args.list, GPString);
                pf_concat(&out, (char*)s, gp_length(s));
                break;

            case GP_PTR:
                p = va_arg(args.list, void*);
                if (p != NULL) {
                    pf_concat(&out, "0x", strlen("0x"));
                    out.length += pf_xtoa(
                        pf_capacity_left(out),
                        out.data + out.length,
                        (uintptr_t)p);
                } else {
                    pf_concat(&out, "(nil)", strlen("(nil)"));
                } break;
        }
        if (is_println) {
            capacity_sufficed_for_trailing_space = pf_push_char(&out, ' ');
        }
    }
    va_end(_args);
    va_end(args.list);
    if (n > 0 && capacity_sufficed_for_trailing_space)
        (out.data)[out.length - 1] = '\n';

    if (is_n && out.length > n) {
        size_t result = out.length;
        out.length = n;
        return result;
    }
    return out.length;
}

size_t gp_bytes_trim(
    void*restrict _str,
    size_t length,
    void**restrict optional_out_ptr,
    const char*restrict optional_char_set,
    int flags)
{
    char* str = _str;
    const bool left  = flags & 0x04;
    const bool right = flags & 0x02;
    const bool ascii = flags & 0x01;

    if (ascii)
    {
        const char* char_set = optional_char_set != NULL ?
            optional_char_set :
            GP_ASCII_WHITESPACE;

        if (left)
        {
            char last = str[length - 1];
            str[length - 1] = '\0';
            size_t prefix_length = strspn(str, char_set);
            str[length - 1] = last;
            // TODO test this
            if (prefix_length == length - 1 && strchr(char_set, last) != NULL)
                prefix_length++;

            length -= prefix_length;

            if (optional_out_ptr != NULL)
                *optional_out_ptr = str + prefix_length;
            else
                memmove(str, str + prefix_length, length);
        }

        if (right && length > 0)
        {
            while (strchr(char_set, ((char*)str)[length - 1]) != NULL) {
                length--;
                if (length == 0)
                    break;
            }
        }
        return length;
    }
    // else utf8

    const char* char_set = optional_char_set != NULL ?
        optional_char_set :
        GP_WHITESPACE;

    if (left)
    {
        size_t prefix_length = 0;
        while (true)
        {
            char codepoint[8] = "";
            size_t size = gp_bytes_codepoint_length(str + prefix_length);
            memcpy(codepoint, str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
        }
        length -= prefix_length;
        // *str += prefix_length; // TODO this goes to bytes module
        memmove(str, str + prefix_length, length);
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_bytes_codepoint_length(str + i)) == 0 && --i != 0);
        memcpy(codepoint, str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    return length;
}

static size_t gp_bytes_to_something(
    void*restrict str,
    size_t length,
    wint_t(*const towsomething)(wint_t))
{
    size_t buf_cap  = 1 << 10;
    wchar_t stack_buf[1 << 10];
    wchar_t* buf = stack_buf;
    if (length + 1 >= buf_cap) {
        buf_cap = length + 1;
        buf = malloc(buf_cap * sizeof(wchar_t)); // TODO use allocator
    }
    const char* src = str;
    size_t buf_length = mbsrtowcs(buf,
        &src, buf_cap, &(mbstate_t){0});
    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    length = wcsrtombs(str,
        (const wchar_t**)&buf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        free(buf);
    return length;
}

size_t gp_bytes_to_upper(
    void*restrict str,
    const size_t str_length)
{
    return gp_bytes_to_something(str, str_length, towupper);
}

size_t gp_bytes_to_lower(
    void*restrict str,
    size_t str_length)
{
    return gp_bytes_to_something(str, str_length, towlower);
}

static size_t gp_bytes_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_bytes_codepoint_length(haystack + i);
        if (cp_length == 0 || i + cp_length > length)
            return i;

        uint32_t codepoint = 0;
        for (size_t j = 0; j < cp_length; j++)
            codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
        if ( ! gp_valid_codepoint(codepoint))
            return i;

        i += cp_length;
    }
    return GP_NOT_FOUND;
}

static size_t gp_bytes_find_valid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length; i++)
    {
        size_t cp_length = gp_bytes_codepoint_length(haystack + i);
        if (cp_length == 1)
            return i;
        if (cp_length == 0)
            continue;

        if (cp_length + i < length) {
            uint32_t codepoint = 0;
            for (size_t j = 0; j < cp_length; j++)
                codepoint = codepoint << 8 | (uint8_t)haystack[i + j];
            if (gp_valid_codepoint(codepoint))
                return i;
        } // else maybe there's ascii in last bytes so continue
    }
    return length;
}

size_t gp_bytes_to_valid(
    void*restrict str,
    size_t length,
    const char* replacement)
{
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_bytes_find_invalid(str, start, length)) != GP_NOT_FOUND)
    {
        length = gp_bytes_replace_range(
            str,
            length,
            start,
            gp_bytes_find_valid(str, start, length),
            replacement,
            replacement_length);

        start += replacement_length;
    }
    return length;
}

int gp_bytes_case_compare( // TODO use allocator
    const void*restrict _s1,
    const size_t s1_length,
    const void*restrict _s2,
    const size_t s2_length)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;

    size_t buf1_cap  = 1 << 10;
    size_t buf2_cap  = 1 << 10;
    wchar_t stack_buf1[1 << 10];
    wchar_t stack_buf2[1 << 10];
    wchar_t* buf1 = stack_buf1;
    wchar_t* buf2 = stack_buf2;
    if (s1_length + 1 >= buf1_cap) {
        buf1_cap = s1_length + 1;
        buf1 = malloc(buf1_cap * sizeof(wchar_t));
    } if (s2_length + 1 >= buf2_cap) {
        buf2_cap = s2_length + 1;
        buf2 = malloc(buf2_cap * sizeof(wchar_t));
    }
    mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0});
    mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0});

    int result = wcscoll(buf1, buf2);
    if (buf1 != stack_buf1)
        free(buf1);
    if (buf2 != stack_buf2)
        free(buf2);
    return result;
}

