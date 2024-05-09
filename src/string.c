// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#ifdef __GNUC__
#define _GNU_SOURCE // memmem()
#endif

#include <gpc/string.h>
#include <gpc/memory.h>
#include <gpc/utils.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>
#include <wctype.h>
#include <printf/printf.h>
#include "pfstring.h"

extern inline GPArrayHeader* gp_str_set(GPStringOut*);
extern inline GPArrayHeader* gp_str_set_new(GPStringOut*);

GPString gp_str_new_init_n(
    const void* allocator,
    size_t capacity,
    const void* init,
    size_t n)
{
    const size_t header_size = sizeof(GPArrayHeader);
    capacity = gp_next_power_of_2(gp_max(capacity, n));
    void* block = gp_mem_alloc(allocator, header_size + capacity + sizeof"");
    *(GPArrayHeader*)block = (GPArrayHeader){
        .length     = n,
        .capacity   = capacity,
        .allocator  = allocator,
        .allocation = block },
    memcpy((uint8_t*)block + header_size, init, n);
    return (GPString)block + header_size;
}

GPString gp_str_build(
    void* buf,
    const void* allocator,
    size_t capacity,
    const char* init)
{
    GPArrayHeader header =
        {.length = strlen(init), .capacity = capacity, .allocator = allocator };
    extern const struct gp_allocator gp_crash_on_alloc;
    if (header.allocator == NULL)
        header.allocator = &gp_crash_on_alloc;
    memcpy(buf, &header, sizeof header);
    return memcpy(buf + sizeof header, init, header.length + 1);
}

GPString gp_str_delete(GPString me)
{
    if (me == NULL)
        return NULL;
    gp_dealloc(gp_allocator(me), gp_allocation(me));
    return NULL;
}

GPArrayHeader* gp_str_header(const void* arr)
{
    size_t ptr_size = sizeof(GPArrayHeader*);
    size_t aligment_offset = (uintptr_t)arr % ptr_size;
    void* header;
    memcpy(&header, (uint8_t*)arr - aligment_offset - ptr_size, ptr_size);
    if (header == NULL)
        header = (uint8_t*)arr - aligment_offset - sizeof(GPArrayHeader);
    return header;
}

size_t gp_length(const void* arr)
{
    return gp_str_header(arr)->length;
}

size_t gp_capacity(const void* arr)
{
    return gp_str_header(arr)->capacity;
}

void* gp_allocation(const void* arr)
{
    return gp_str_header(arr)->allocation;
}

const struct gp_allocator* gp_allocator(const void* arr)
{
    return gp_str_header(arr)->allocator;
}

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

size_t gp_str_find(
    GPStringIn  haystack,
    const void* needle,
    size_t      needle_size,
    size_t      start)
{
    const GPChar* result = gp_memmem(
        haystack + start, gp_length(haystack) - start, needle, needle_size);
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

size_t gp_str_find_last(
    GPStringIn str_haystack,
    const void* needle,
    size_t needle_length)
{
    const char* haystack = (const char*)str_haystack;
    size_t haystack_length = gp_length(str_haystack);

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

size_t gp_str_count(
    GPStringIn haystack,
    const void* needle,
    size_t     needle_size)
{
    size_t count = 0;
    size_t i = 0;
    while ((i = gp_str_find(haystack, needle, needle_size, i)) != GP_NOT_FOUND){
        count++;
        i++;
    }
    return count;
}

bool gp_str_equal(
    GPStringIn  s1,
    const void* s2,
    size_t      s2_size)
{
    if (gp_length(s1) != s2_size)
        return false;
    else
        return memcmp(s1, s2, s2_size) == 0;
}

static size_t gp_mem_codepoint_length(
    const void* str)
{
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[(uint8_t)*(char*)str >> 3];
}

static size_t gp_mem_codepoint_count(
    const void* _str,
    size_t n)
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

bool gp_str_equal_case(
    GPStringIn  s1,
    const void* s2,
    size_t      s2_size)
{
    size_t s1_length = gp_mem_codepoint_count(s1, gp_length(s1));
    size_t s2_length = gp_mem_codepoint_count(s2, s2_size);
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
            size_t s1_codepoint_size = gp_mem_codepoint_length(s1);
            size_t s2_codepoint_size = gp_mem_codepoint_length(s2);
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

size_t gp_str_codepoint_count(
    GPStringIn str)
{
    return gp_mem_codepoint_count(str, gp_length(str));
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

bool gp_str_is_valid(
    GPStringIn _str)
{
    const char* str = (const char*)_str;
    const size_t length = gp_length(str);
    for (size_t i = 0; i < length;)
    {
        size_t cp_length = gp_mem_codepoint_length(str + i);
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

size_t gp_str_codepoint_length(
    GPStringIn str)
{
    return gp_mem_codepoint_length(str);
}

const char* gp_cstr(GPString str)
{
    str[gp_length(str)].c = '\0';
    return (const char*)str;
}

void gp_str_reserve(
    GPStringOut* str,
    size_t capacity)
{
    if (gp_capacity(*str) < capacity)
    {
        GPArrayHeader* header = gp_str_header(*str);
        GPChar* block_start = (GPChar*)(header + 1);
        size_t offset = *str - block_start;
        if (header->capacity + offset >= capacity) {
            *str = memmove(block_start, *str, header->length);
            memcpy(&((GPArrayHeader*)*str - 1)->capacity, &capacity, sizeof capacity);
            return;
        }

        capacity = gp_next_power_of_2(capacity);
        void* block = gp_mem_alloc(
            header->allocator,
            sizeof(GPArrayHeader) + capacity);

        ((GPArrayHeader*)block)->length     = header->length;
        ((GPArrayHeader*)block)->capacity   = capacity;
        ((GPArrayHeader*)block)->allocator  = header->allocator;
        ((GPArrayHeader*)block)->allocation = block;
        memcpy((GPArrayHeader*)block + 1, *str, header->length);

        gp_mem_dealloc(header->allocator, header->allocation);

        *str = (GPChar*)block + sizeof(GPArrayHeader);
    }
}

static void gp_str_clear_contents(
    GPStringOut* str)
{
    GPArrayHeader* header = gp_str_header(*str);
    size_t offset = *str - (GPChar*)(header + 1);
    header->length   = 0;
    header->capacity += offset;
    *str = (GPChar*)(header + 1);
}

void gp_str_copy(
    GPStringOut* dest,
    const void*restrict src,
    size_t n)
{
    gp_str_clear_contents(dest);
    gp_str_reserve(dest, n);
    memcpy(*dest, src, n);
    gp_str_set_new(dest)->length = n;
}

void gp_str_repeat(
    GPStringOut* dest,
    const size_t n,
    const void*restrict mem,
    const size_t mem_length)
{
    gp_str_clear_contents(dest);
    gp_str_reserve(dest, n * mem_length);
    if (mem_length == 1) {
        memset(*dest, *(uint8_t*)mem, n);
    } else for (size_t i = 0; i < n; i++) {
        memcpy(*dest + i * mem_length, mem, mem_length);
    }
    gp_str_set_new(dest)->length = n * mem_length;
}

void gp_str_slice(
    GPStringOut* str,
    size_t start,
    size_t end)
{
    GPArrayHeader* header = gp_str_header(*str);
    size_t aligment_offset = (uintptr_t)*str % sizeof(void*);
    if (start + aligment_offset > sizeof(void*))
    {
        GPChar* new_start = *str + start;
        memcpy(
            new_start - sizeof(void*) - (uintptr_t)new_start % sizeof(void*),
            &header, sizeof(void*));
    }
    *str += start;
    header->length    = end - start;
    header->capacity -= start;
}

void gp_str_substr(
    GPStringOut* dest,
    const void*restrict src,
    size_t start,
    size_t end)
{
    gp_str_clear_contents(dest);
    gp_str_reserve(dest, end - start);
    memcpy(*dest, src + start, end - start);
    gp_str_set_new(dest)->length = end - start;
}

void gp_str_append(
    GPStringOut* dest,
    const void* src,
    size_t src_length)
{
    gp_str_reserve(dest, gp_length(*dest) + src_length);
    memcpy(*dest + gp_length(*dest), src, src_length + sizeof(""));
    gp_str_set(dest)->length += src_length;
}

void gp_str_insert(
    GPStringOut* dest,
    size_t pos,
    const void*restrict src,
    size_t n)
{
    gp_str_reserve(dest, gp_length(*dest) + n);
    memmove(*dest + pos + n, *dest + pos, gp_length(*dest) - pos);
    memcpy(*dest + pos, src, n);
    gp_str_set(dest)->length += n;
}

static size_t gp_cstr_replace_range(
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

size_t gp_str_replace(
    GPStringOut* haystack,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length,
    size_t start)
{
    if ((start = gp_str_find(*haystack, needle, needle_length, start)) == GP_NOT_FOUND)
        return GP_NOT_FOUND;

    gp_str_reserve(haystack,
        gp_length(*haystack) + replacement_length - needle_length);

    const size_t end = start + needle_length;
    gp_str_set(haystack)->length = gp_cstr_replace_range(
        gp_length(*haystack),
        (char*)*haystack,
        start,
        end,
        replacement,
        replacement_length);

    return start;
}

size_t gp_str_replace_all(
    GPStringOut* haystack,
    const void*restrict needle,
    const size_t needle_length,
    const void*restrict replacement,
    const size_t replacement_length)
{
    size_t start = 0;
    size_t replacement_count = 0;
    while ((start = gp_str_find(*haystack, needle, needle_length, start)) != GP_NOT_FOUND)
    {
        gp_str_reserve(haystack,
            gp_length(*haystack) + replacement_length - needle_length);

        gp_str_set(haystack)->length = gp_cstr_replace_range(
            gp_length(*haystack),
            (char*)*haystack,
            start,
            start + needle_length,
            replacement,
            replacement_length);

        start += replacement_length;
        replacement_count++;
    }
    return replacement_count;
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

static inline size_t gp_print_cap_left(GPStringIn me, size_t n) {
    return gp_length(me) >= n ? 0 : n - gp_length(me);
}
static inline void gp_print_concat(
    GPStringOut* me, const size_t n, const void* src, const size_t length)
{
    memcpy(*me + gp_length(*me), src, gp_min(gp_print_cap_left(*me, n), length));
    gp_str_set(me)->length += length;
}
static inline bool gp_print_push_char(
    GPStringOut* me, const size_t n, const uint8_t c)
{
    if (gp_length(*me) < n)
        (*me)[gp_length(*me)].c = c;
    gp_str_set(me)->length++;
    return gp_print_cap_left(*me, n) != 0;
}

size_t gp_str_print_internal(
    bool is_println,
    bool is_n,
    GPStringOut* out,
    const size_t n,
    const size_t arg_count,
    const struct GPPrintable* objs,
    ...)
{
    va_list _args;
    va_start(_args, objs);
    pf_va_list args;
    va_copy(args.list, _args);

    bool capacity_sufficed_for_trailing_space = false;

    #define GP_PRINT_ADD_RESERVE(N) do \
    { \
        if ( ! is_n) \
            gp_str_reserve(out, gp_length(*out) + (N)); \
    } while (0)

    gp_str_clear_contents(out);
    // Avoid many small allocations by estimating a sufficient buffer size. This
    // estimation is currently completely arbitrary. // TODO better estimation.
    //GP_PRINT_ADD_RESERVE(arg_count * 10);

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
            GP_PRINT_ADD_RESERVE(pf_vsnprintf(NULL, 0, fmt, args.list));

            gp_str_set(out)->length += pf_vsnprintf_consuming(
                (char*)*out + gp_str_set(out)->length,
                gp_print_cap_left(*out, n),
                fmt,
                &args);

            if (is_println) {
                GP_PRINT_ADD_RESERVE(1);
                capacity_sufficed_for_trailing_space = gp_print_push_char(out, n, ' ');
            }
            continue;
        }

        switch (objs[i].type)
        {
            case GP_CHAR:
            case GP_SIGNED_CHAR:
            case GP_UNSIGNED_CHAR:
                GP_PRINT_ADD_RESERVE(1);
                gp_print_push_char(out, n, (char)va_arg(args.list, int));
                break;

            case GP_UNSIGNED_SHORT:
            case GP_UNSIGNED:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_utoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, unsigned));
                break;

            case GP_UNSIGNED_LONG:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_utoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, unsigned long));
                break;

            case GP_UNSIGNED_LONG_LONG:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_utoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, unsigned long long));
                break;

            case GP_BOOL:
                GP_PRINT_ADD_RESERVE(strlen("false"));
                if (va_arg(args.list, int))
                    gp_print_concat(out, n, "true", strlen("true"));
                else
                    gp_print_concat(out, n, "false", strlen("false"));
                break;

            case GP_SHORT:
            case GP_INT:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_itoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, int));
                break;

            case GP_LONG:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_itoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, long int));
                break;

            case GP_LONG_LONG:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_itoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, long long int));
                break;

            case GP_FLOAT:
            case GP_DOUBLE:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                gp_str_set(out)->length += pf_gtoa(
                    gp_print_cap_left(*out, n),
                    (char*)*out + gp_str_set(out)->length,
                    va_arg(args.list, double));
                break;

            char* p;
            size_t p_len;
            case GP_CHAR_PTR:
                p = va_arg(args.list, char*);
                p_len = strlen(p);
                GP_PRINT_ADD_RESERVE(p_len);
                gp_print_concat(out, n, p, p_len);
                break;

            GPString s;
            case GP_STRING:
                s = va_arg(args.list, GPString);
                GP_PRINT_ADD_RESERVE(gp_length(s));
                gp_print_concat(out, n, (char*)s, gp_length(s));
                break;

            case GP_PTR:
                GP_PRINT_ADD_RESERVE(gp_max_digits_in(objs[i].type));
                p = va_arg(args.list, void*);
                if (p != NULL) {
                    gp_print_concat(out, n, "0x", strlen("0x"));
                    gp_str_set(out)->length += pf_xtoa(
                        gp_print_cap_left(*out, n),
                        (char*)*out + gp_str_set(out)->length,
                        (uintptr_t)p);
                } else {
                    gp_print_concat(out, n, "(nil)", strlen("(nil)"));
                } break;
        }
        if (is_println) {
            GP_PRINT_ADD_RESERVE(1);
            capacity_sufficed_for_trailing_space = gp_print_push_char(out, n, ' ');
        }
    }
    va_end(_args);
    va_end(args.list);
    if (n > 0 && capacity_sufficed_for_trailing_space)
        (*out)[gp_str_set(out)->length - 1].c = '\n';

    if (is_n && gp_length(*out) > n) {
        size_t result = gp_length(*out);
        gp_str_set(out)->length = n;
        return result;
    }
    return gp_length(*out);
}

void gp_str_trim(
    GPStringOut* str,
    const char*restrict optional_char_set,
    int flags)
{
    size_t length = gp_length(*str);
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
            const size_t prefix_length = strspn(gp_cstr(*str), char_set);
            length -= prefix_length;
            // *str += prefix_length; // TODO this goes to bytes module
            memmove(*str, *str + prefix_length, length);

        }
        if (right && length > 0)
        {
            while (strchr(char_set, (*str)[length - 1].c) != NULL) {
                length--;
                if (length == 0)
                    break;
            }
        }
        gp_str_set(str)->length = length;
        return;
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
            size_t size = gp_str_codepoint_length(*str + prefix_length);
            memcpy(codepoint, *str + prefix_length, size);
            if (strstr(char_set, codepoint) == NULL)
                break;

            prefix_length += size;
        }
        length -= prefix_length;
        // *str += prefix_length; // TODO this goes to bytes module
        memmove(*str, *str + prefix_length, length);
    }
    if (right) while (length > 0)
    {
        char codepoint[8] = "";
        size_t i = length - 1;
        size_t size;
        while ((size = gp_str_codepoint_length(*str + i)) == 0 && --i != 0);
        memcpy(codepoint, *str + i, size);
        if (strstr(char_set, codepoint) == NULL)
            break;

        length -= size;
    }
    gp_str_set(str)->length = length;
}

static void gp_str_to_something(
    GPStringOut* str,
    wint_t(*towsomething)(wint_t))
{
    size_t length = gp_length(*str);
    size_t buf_cap  = 1 << 10;
    wchar_t stack_buf[1 << 10];
    wchar_t* buf = stack_buf;
    if (length + 1 >= buf_cap) {
        buf_cap = length + 1;
        buf = malloc(buf_cap * sizeof(wchar_t)); // TODO use allocator
    }
    const char* src = (char*)*str;
    size_t buf_length = mbsrtowcs(buf,
        &src, buf_cap, &(mbstate_t){0});
    for (size_t i = 0; i < buf_length; i++)
        buf[i] = towsomething(buf[i]);

    length = wcsrtombs((char*)*str,
        (const wchar_t**)&buf, sizeof(buf[0]) * buf_length, &(mbstate_t){0});

    if (buf != stack_buf)
        free(buf);
    gp_str_set(str)->length = length;
}

void gp_str_to_upper(
    GPStringOut* str)
{
    gp_str_to_something(str, towupper);
}

void gp_str_to_lower(
    GPStringOut* str)
{
    gp_str_to_something(str, towlower);
}

static size_t gp_bytes_find_invalid(
    const void* _haystack,
    const size_t start,
    const size_t length)
{
    const char* haystack = _haystack;
    for (size_t i = start; i < length;)
    {
        size_t cp_length = gp_mem_codepoint_length(haystack + i);
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
        size_t cp_length = gp_mem_codepoint_length(haystack + i);
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

void gp_str_to_valid(
    GPStringOut* str,
    const char* replacement)
{
          size_t length = gp_length(*str);
    const size_t replacement_length = strlen(replacement);

    size_t start = 0;
    while ((start = gp_bytes_find_invalid(*str, start, length)) != GP_NOT_FOUND)
    {
        length = gp_cstr_replace_range(
            length,
            (char*)*str,
            start,
            gp_bytes_find_valid(*str, start, length),
            replacement,
            replacement_length);

        start += replacement_length;
    }
    gp_str_set(str)->length = length;
}

int gp_str_case_compare( // TODO use allocator
    GPStringIn _s1,
    GPStringIn _s2)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;
    size_t s1_length = gp_length(s1);
    size_t s2_length = gp_length(s2);

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

