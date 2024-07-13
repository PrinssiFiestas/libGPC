// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/unicode.h>
#include <gpc/utils.h>
#include "thread.h"
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

// ----------------------------------------------------------------------------
// Locale

GPLocale gp_locale_new(const char*const locale_code)
{
    char lc[8] = "";
    strncpy(lc, locale_code, strlen("xx_XX"));
    char full_locale_code[16] = "";
    strcpy(full_locale_code, lc);

    #ifndef _WIN32
    if (locale_code[0] == '\0')
        full_locale_code[0] = 'C';
    #endif
    #ifndef __MINGW32__
    strcat(full_locale_code, ".UTF-8");
    #endif

    GPLocale locale = {
        #if _WIN32
        _create_locale(LC_ALL, full_locale_code),
        #else
        newlocale(LC_ALL_MASK, full_locale_code, (locale_t)0),
        #endif
        { lc[0], lc[1], lc[2], lc[3], lc[4] }
    };
    return locale;
}

void gp_locale_delete(locale_t locale)
{
    GPLocale default_locale = gp_default_locale();
    if (locale == default_locale.locale || locale == (locale_t)0)
        return;
    #if _WIN32
    _free_locale(locale);
    #else
    freelocale(locale);
    #endif
}

static GPLocale gp_default_locale_internal = {0};

static void gp_delete_default_locale(void)
{
    if (gp_default_locale_internal.locale != (locale_t)0)
        gp_locale_delete(gp_default_locale_internal.locale);
}

static void gp_init_default_locale(void)
{
    gp_default_locale_internal = gp_locale_new("");
    atexit(gp_delete_default_locale); // shut up sanitizer
}

GPLocale gp_default_locale(void)
{
    static GPThreadOnce default_locale_once = GP_THREAD_ONCE_INIT;
    gp_thread_once(&default_locale_once, gp_init_default_locale);
    return gp_default_locale_internal;
}

// ----------------------------------------------------------------------------
// Unicode stuff

size_t gp_utf8_codepoint_length(
    const void*const _str, const size_t i)
{
    const uint8_t* str = (uint8_t*)_str;
    static const size_t sizes[] = {
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,0, 2,2,2,2,3,3,4,0 };
    return sizes[str[i] >> 3];
}

size_t gp_utf8_encode(uint32_t* encoding, const void*const _u8, const size_t i)
{
    const GPString u8 = (GPString)_u8;
    *encoding = 0;
    const size_t codepoint_length = gp_utf8_codepoint_length(u8, i);
    for (size_t j = 0; j < codepoint_length; j++)
        *encoding = (*encoding << 8) | u8[i + j].c;

    if (*encoding > 0x7F)
    {
        uint32_t mask = (*encoding <= 0x00EFBFBF) ? 0x000F0000 : 0x003F0000 ;
        *encoding = ((*encoding & 0x07000000) >> 6) |
            ((*encoding & mask )             >> 4) |
            ((*encoding & 0x00003F00)        >> 2) |
            (*encoding & 0x0000003F);
    }
    return codepoint_length;
}

size_t gp_utf8_decode(
    void*_decoding,
    uint32_t encoding)
{
    uint8_t* decoding = _decoding;
    if (encoding > 0x7F)
    {
        if (encoding < 0x800) {
            decoding[0] = ((encoding & 0x000FC0) >> 6) | 0xC0;
            decoding[1] = ((encoding & 0x00003F) >> 0) | 0x80;
            return 2;
        } else if (encoding < 0x10000) {
            decoding[0] = ((encoding & 0x03F000) >> 12) | 0xE0;
            decoding[1] = ((encoding & 0x000FC0) >>  6) | 0x80;
            decoding[2] = ((encoding & 0x00003F) >>  0) | 0x80;
            return 3;
        } else {
            decoding[0] = ((encoding & 0x1C0000) >> 18) | 0xF0;
            decoding[1] = ((encoding & 0x03F000) >> 12) | 0x80;
            decoding[2] = ((encoding & 0x000FC0) >>  6) | 0x80;
            decoding[3] = ((encoding & 0x00003F) >>  0) | 0x80;
            return 4;
        }
    }
    else
    { // This weird control flow is somewhat faster for some reason.
      // Maybe it's better for the branch predictor?
        decoding[0] = encoding;
        return 1;
    }
}

void gp_utf8_to_utf32(
    GPArray(uint32_t)* u32,
    const void*const   u8,
    const size_t       u8_length)
{
    ((GPArrayHeader*)*u32 - 1)->length = 0;
    size_t i = 0;
    size_t codepoint_length;
    uint32_t encoding;
    for (; gp_arr_length(*u32) < gp_arr_capacity(*u32); i += codepoint_length)
    {
        if (i >= u8_length)
            return;
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        (*u32)[((GPArrayHeader*)*u32 - 1)->length++] = encoding;
    }
    size_t gp_bytes_codepoint_count(const void*, size_t);
    *u32 = gp_arr_reserve(sizeof(*u32)[0], *u32,
        gp_arr_length(*u32) + gp_bytes_codepoint_count(u8 + i, u8_length));

    for (; i < u8_length; i += codepoint_length)
    {
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        (*u32)[((GPArrayHeader*)*u32 - 1)->length++] = encoding;
    }
}

static size_t gp_utf32_to_utf8_byte_length(uint32_t u32)
{
    if (u32 < 0x80)
        return 1;
    else if (u32 < 0x800)
        return 2;
    else if (u32 < 0x10000)
        return 3;
    else
        return 4;
}

void gp_utf32_to_utf8(
    GPString*        u8,
    const uint32_t*  u32,
    size_t           u32_length)
{
    ((GPStringHeader*)*u8 - 1)->length = 0;
    size_t i = 0;
    for (; gp_str_length(*u8) < gp_str_capacity(*u8); ++i)
    { // Manually inlined gp_utf8_decode() is faster for some reason.
        if (i >= u32_length)
            return;

        if (u32[i] > 0x7F)
        {
            if (u32[i] < 0x800) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x000FC0) >> 6) | 0xC0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x00003F) >> 0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 2;
            } else if (u32[i] < 0x10000) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x03F000) >> 12) | 0xE0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u32[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 3;
            } else {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x1C0000) >> 18) | 0xF0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x03F000) >> 12) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u32[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 3].c = ((u32[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 4;
            }
        }
        else
        { // Also this weird control flow is somewhat faster for some reason.
          // Maybe it's better for the branch predictor?
            (*u8)[((GPStringHeader*)*u8 - 1)->length++].c = u32[i];
        }
    }

    size_t required_capacity = gp_str_length(*u8);
    for (size_t j = i; j < gp_arr_length(u32); ++j)
    {
        if (u32[j] > 0x7F)
        {
            if (u32[j] < 0x800)
                required_capacity += 2;
            else if (u32[j] < 0x10000)
                required_capacity += 3;
            else
                required_capacity += 4;
        }
        else
            ++required_capacity;
    }
    gp_str_reserve(u8, required_capacity);

    for (; i < u32_length; ++i)
    {
        if (u32[i] > 0x7F)
        {
            if (u32[i] < 0x800) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x000FC0) >> 6) | 0xC0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x00003F) >> 0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 2;
            } else if (u32[i] < 0x10000) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x03F000) >> 12) | 0xE0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u32[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 3;
            } else {
                (*u8)[gp_str_length(*u8) + 0].c = ((u32[i] & 0x1C0000) >> 18) | 0xF0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u32[i] & 0x03F000) >> 12) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u32[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 3].c = ((u32[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 4;
            }
        }
        else
        {
            (*u8)[((GPStringHeader*)*u8 - 1)->length++].c = u32[i];
        }
    }
}

void gp_utf8_to_utf16(
    GPArray(uint16_t)* u16,
    const void*        u8,
    size_t             u8_length)
{
    ((GPArrayHeader*)*u16 - 1)->length = 0;
    size_t i = 0;
    size_t codepoint_length;
    uint32_t encoding;
    for (; gp_arr_length(*u16) < gp_arr_capacity(*u16); i += codepoint_length)
    {
        if (i >= u8_length)
            return;
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        if (encoding <= UINT16_MAX) {
            (*u16)[((GPArrayHeader*)*u16 - 1)->length++] = encoding;
        } else {
            if (GP_UNLIKELY(gp_arr_length(*u16) + 2 > gp_arr_capacity(*u16)))
                break;
            encoding &= ~0x10000;
            (*u16)[gp_arr_length(*u16) + 0] = (encoding >> 10)   | 0xD800;
            (*u16)[gp_arr_length(*u16) + 1] = (encoding & 0x3FF) | 0xDC00;
            ((GPArrayHeader*)*u16 - 1)->length += 2;
        }
    }
    size_t capacity_needed = gp_arr_length(*u16);
    for (size_t j = i; j < u8_length; j += codepoint_length) {
        codepoint_length = gp_utf8_encode(&encoding, u8, j);
        capacity_needed += encoding <= UINT16_MAX ? 1 : 2;
    }
    *u16 = gp_arr_reserve(sizeof(*u16)[0], *u16, capacity_needed);

    for (; i < u8_length; i += codepoint_length)
    {
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        if (encoding <= UINT16_MAX) {
            (*u16)[((GPArrayHeader*)*u16 - 1)->length++] = encoding;
        } else {
            encoding &= ~0x10000;
            (*u16)[gp_arr_length(*u16) + 0] = (encoding >> 10)   | 0xD800;
            (*u16)[gp_arr_length(*u16) + 1] = (encoding & 0x3FF) | 0xDC00;
            ((GPArrayHeader*)*u16 - 1)->length += 2;
        }
    }
}

void gp_utf16_to_utf8(
    GPString*        u8,
    const uint16_t*  u16,
    size_t           u16_length)
{
    ((GPStringHeader*)*u8 - 1)->length = 0;
    size_t i = 0;
    for (; gp_str_length(*u8) < gp_str_capacity(*u8); ++i)
    {
        if (i >= u16_length)
            return;

        if (u16[i] > 0x7F)
        {
            if (u16[i] < 0x800) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u16[i] & 0x000FC0) >> 6) | 0xC0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u16[i] & 0x00003F) >> 0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 2;
            } else if (u16[i] <= 0xD7FF || 0xE000 <= u16[i]) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u16[i] & 0x03F000) >> 12) | 0xE0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u16[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u16[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 3;
            } else { // surrogate pair
                const uint32_t encoding = 0x10000
                    | ((uint32_t)(u16[i + 0] &~ 0xD800) << 10)
                    | ((uint32_t)(u16[i + 1] &~ 0xDC00));
                (*u8)[gp_str_length(*u8) + 0].c = ((encoding & 0x1C0000) >> 18) | 0xF0;
                (*u8)[gp_str_length(*u8) + 1].c = ((encoding & 0x03F000) >> 12) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((encoding & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 3].c = ((encoding & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 4;
                ++i;
            }
        }
        else
        {
            (*u8)[((GPStringHeader*)*u8 - 1)->length++].c = u16[i];
        }
    }

    size_t required_capacity = gp_str_length(*u8);
    for (size_t j = i; j < gp_arr_length(u16); ++j)
    {
        if (u16[j] > 0x7F)
        {
            if (u16[j] < 0x800)
                required_capacity += 2;
            else if (u16[j] <= 0xD7FF || 0xE000 <= u16[j])
                required_capacity += 3;
            else
                required_capacity += 4;
        }
        else
            ++required_capacity;
    }
    gp_str_reserve(u8, required_capacity);

    for (; i < u16_length; ++i)
    {
        if (u16[i] > 0x7F)
        {
            if (u16[i] < 0x800) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u16[i] & 0x000FC0) >> 6) | 0xC0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u16[i] & 0x00003F) >> 0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 2;
            } else if (u16[i] <= 0xD7FF || 0xE000 <= u16[i]) {
                (*u8)[gp_str_length(*u8) + 0].c = ((u16[i] & 0x03F000) >> 12) | 0xE0;
                (*u8)[gp_str_length(*u8) + 1].c = ((u16[i] & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((u16[i] & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 3;
            } else { // surrogate pair
                const uint32_t encoding = 0x10000
                    | ((uint32_t)(u16[i + 0] &~ 0xD800) << 10)
                    | ((uint32_t)(u16[i + 1] &~ 0xDC00));
                (*u8)[gp_str_length(*u8) + 0].c = ((encoding & 0x1C0000) >> 18) | 0xF0;
                (*u8)[gp_str_length(*u8) + 1].c = ((encoding & 0x03F000) >> 12) | 0x80;
                (*u8)[gp_str_length(*u8) + 2].c = ((encoding & 0x000FC0) >>  6) | 0x80;
                (*u8)[gp_str_length(*u8) + 3].c = ((encoding & 0x00003F) >>  0) | 0x80;
                ((GPStringHeader*)*u8 - 1)->length += 4;
                ++i;
            }
        }
        else
        {
            (*u8)[((GPStringHeader*)*u8 - 1)->length++].c = u16[i];
        }
    }
}

void gp_utf8_to_wcs(
    GPArray(wchar_t)* wcs,
    const void*       utf8,
    size_t            utf8_length)
{
    if (WCHAR_MAX > UINT16_MAX)
        gp_utf8_to_utf32((GPArray(uint32_t)*)wcs, utf8, utf8_length);
    else
        gp_utf8_to_utf16((GPArray(uint16_t)*)wcs, utf8, utf8_length);
    *wcs = gp_arr_reserve(sizeof(*wcs)[0], *wcs, gp_arr_length(*wcs) + sizeof"");
    (*wcs)[gp_arr_length(*wcs)] = L'\0';
}

void gp_wcs_to_utf8(
    GPString*       utf8,
    const wchar_t*  wcs,
    size_t          wcs_length)

{
    if (WCHAR_MAX > UINT16_MAX)
        gp_utf32_to_utf8(utf8, (uint32_t*)wcs, wcs_length);
    else
        gp_utf16_to_utf8(utf8, (uint16_t*)wcs, wcs_length);
}

uint32_t gp_u32_to_upper(uint32_t c)
{
    #if !_WIN32
    return towupper_l(c, gp_default_locale().locale);
    #else
    if (c <= WCHAR_MAX)
        return _towupper_l(c, gp_default_locale().locale);

    // Based on Unicode 15.1.0
    // Expression used to filter out the characters for the below code:
    // awk -F\; '{ if ( $13 != "" ) print $1; }' UnicodeData.txt

    if      ((0x10428 <= c && c <= 0x1044F)  ||
             (0x104D8 <= c && c <= 0x104FB)) return c - 0x28;
    else if ((0x10597 <= c && c <= 0x105B9)  ||
             (0x105BB == c || c == 0x105BC)) return c - 0x27;
    else if ((0x10CC0 <= c && c <= 0x10CF2)) return c - 0x40;
    else if ((0x118C0 <= c && c <= 0x118DF)  ||
             (0x16E60 <= c && c <= 0x16E7F)) return c - 0x20;
    else if ((0x1E922 <= c && c <= 0x1E943)) return c - 0x22;

    return c;
    #endif
}

uint32_t gp_u32_to_lower(uint32_t c)
{
    #if !_WIN32
    return towlower_l(c, gp_default_locale().locale);
    #else
    if (c <= WCHAR_MAX)
        return _towlower_l(c, gp_default_locale().locale);

    // Based on Unicode 15.1.0
    // Expression used to filter out the characters for the below code:
    // awk -F\; '{ if ( $14 != "" ) print $1; }' UnicodeData.txt

    if      ((0x10400 <= c && c <= 0x10427)  ||
             (0x104B0 <= c && c <= 0x104D3)) return c + 0x28;
    else if ((0x10570 <= c && c <= 0x10592)  ||
             (0x10594 == c || c == 0x10595)) return c + 0x27;
    else if ((0x10C80 <= c && c <= 0x10CB2)) return c + 0x40;
    else if ((0x118A0 <= c && c <= 0x118BF)  ||
             (0x16E40 <= c && c <= 0x16E5F)) return c + 0x20;
    else if ((0x1E900 <= c && c <= 0x1E921)) return c + 0x22;

    return c;
    #endif
}

// ----------------------------------------------------------------------------
// String extensions

GPArray(GPString) gp_str_split(
    const GPAllocator* allocator,
    const GPString str,
    const char*const separators)
{
    GPArray(GPString) substrs = NULL;
    size_t i = gp_str_find_first_not_of(str, separators, 0);
    if (i == GP_NOT_FOUND)
        return gp_arr_new(allocator, sizeof(GPString), 1);

    size_t indices_length = 0;
    struct start_end_pair {
        size_t start, end;
    } indices[128];

    while (true)
    {
        for (indices_length = 0;
            indices_length < sizeof indices / sizeof indices[0];
            ++indices_length)
        {
            indices[indices_length].start = i;
            i = gp_str_find_first_of(str, separators, i);
            if (i == GP_NOT_FOUND) {
                indices[indices_length++].end = gp_str_length(str);
                break;
            }
            indices[indices_length].end = i;
            i = gp_str_find_first_not_of(str, separators, i);
            if (i == GP_NOT_FOUND) {
                ++indices_length;
                break;
            }
        }

        if (substrs == NULL)
            substrs = gp_arr_new(
                allocator,
                sizeof(GPString),
                i == GP_NOT_FOUND ? indices_length : 2 * indices_length);
        else
            substrs = gp_arr_reserve(
                sizeof(GPString),
                substrs, i == GP_NOT_FOUND ?
                    gp_arr_length(substrs) + indices_length
                  : 3 * gp_arr_length(substrs));

        for (size_t j = 0; j < indices_length; ++j)
        {
            substrs[gp_arr_length(substrs) + j] = gp_str_new(
                allocator,
                gp_next_power_of_2(indices[j].end - indices[j].start),
                "");
            ((GPStringHeader*)(substrs[gp_arr_length(substrs) + j]) - 1)->length =
                indices[j].end - indices[j].start;
            memcpy(
                substrs[gp_arr_length(substrs) + j],
                str + indices[j].start,
                gp_str_length(substrs[gp_arr_length(substrs) + j]));
        }
        ((GPArrayHeader*)substrs - 1)->length += indices_length;

        if (i == GP_NOT_FOUND)
            break;

    }
    return substrs;
}

void gp_str_join(GPString* out, GPArray(GPString) strs, const char* separator)
{
    ((GPStringHeader*)*out - 1)->length = 0;
    if (gp_arr_length(strs) == 0)
        return;

    const size_t separator_length = strlen(separator);
    size_t required_capacity = -separator_length;
    for (size_t i = 0; i < gp_arr_length(strs); ++i)
        required_capacity += gp_str_length(strs[i]) + separator_length;

    gp_str_reserve(out, required_capacity);
    for (size_t i = 0; i < gp_arr_length(strs) - 1; ++i)
    {
        memcpy(*out + gp_str_length(*out), strs[i], gp_str_length(strs[i]));
        memcpy(*out + gp_str_length(*out) + gp_str_length(strs[i]), separator, separator_length);
        ((GPStringHeader*)*out - 1)->length += gp_str_length(strs[i]) + separator_length;
    }
    memcpy(
        *out + gp_str_length(*out),
        strs[gp_arr_length(strs) - 1],
        gp_str_length(strs[gp_arr_length(strs) - 1]));
    ((GPStringHeader*)*out - 1)->length += gp_str_length(strs[gp_arr_length(strs) - 1]);
}

static bool gp_is_soft_dotted(const uint32_t encoding)
{
    switch (encoding) {
        case 'i':     case 'j':     case 0x012F:  case 0x0249:  case 0x0268:
        case 0x029D:  case 0x02B2:  case 0x03F3:  case 0x0456:  case 0x0458:
        case 0x1D62:  case 0x1D96:  case 0x1DA4:  case 0x1DA8:  case 0x1E2D:
        case 0x1ECB:  case 0x2071:  case 0x2148:  case 0x2149:  case 0x2C7C:
        case 0x1D422: case 0x1D423: case 0x1D456: case 0x1D457: case 0x1D48A:
        case 0x1D48B: case 0x1D4BE: case 0x1D4BF: case 0x1D4F2: case 0x1D4F3:
        case 0x1D526: case 0x1D527: case 0x1D55A: case 0x1D55B: case 0x1D58E:
        case 0x1D58F: case 0x1D5C2: case 0x1D5C3: case 0x1D5F6: case 0x1D5F7:
        case 0x1D62A: case 0x1D62B: case 0x1D65E: case 0x1D65F: case 0x1D692:
        case 0x1D693: case 0x1DF1A: case 0x1E04C: case 0x1E04D: case 0x1E068:
        return true;
    }
    return false;
}

static bool gp_is_diatrical(const uint32_t encoding)
{
    return
        (0x0300 <= encoding && encoding <= 0x036F) ||
        (0x1AB0 <= encoding && encoding <= 0x1AFF) ||
        (0x1DC0 <= encoding && encoding <= 0x1DFF) ||
        (0x20D0 <= encoding && encoding <= 0x20FF) ||
        (0x2DE0 <= encoding && encoding <= 0x2DFF) ||
        (0xFE20 <= encoding && encoding <= 0xFE2F);
}

// Helpers for to_upper_full(), to_lower_full(). Don't use these for anything else!
#define GP_SEMICOLON(...) ;
#define GP_u32_APPEND1(CODEPOINT) \
    u32[((GPArrayHeader*)u32 - 1)->length++] = (CODEPOINT); \
    required_capacity += gp_utf32_to_utf8_byte_length(CODEPOINT)
#define GP_u32_APPEND(...) do \
{ \
    if (GP_COUNT_ARGS(__VA_ARGS__) > 1) \
        u32 = gp_arr_reserve(sizeof u32[0], u32, u32_capacity += GP_COUNT_ARGS(__VA_ARGS__) - 1); \
    GP_PROCESS_ALL_ARGS(GP_u32_APPEND1, GP_SEMICOLON, __VA_ARGS__); \
} while(0)

uint32_t gp_u32_to_upper(uint32_t);
void gp_str_to_upper_full(
    GPString* str,
    GPLocale locale)
{
    // TODO ASCII optimization would go here if not Turkish locale

    GPArena* scratch = gp_scratch_arena();
    size_t u32_capacity = gp_str_length(*str); // this gets incremented by GP_u32_APPEND()
    GPArray(uint32_t) u32 = gp_arr_new(
        (GPAllocator*)scratch, sizeof u32[0], u32_capacity);

    size_t required_capacity = 0; // this gets incremented by GP_u32_APPEND()
    (*str)[gp_str_length(*str)].c = '\0'; // last lookahead
    uint32_t lookahead;
    size_t codepoint_length = gp_utf8_encode(&lookahead, *str, 0);

    for (size_t i = 0; i < gp_str_length(*str);)
    {
        const uint32_t encoding = lookahead;
        i += codepoint_length;
        codepoint_length = gp_utf8_encode(&lookahead, *str, i);

        if (encoding == 0x0345 && gp_is_diatrical(lookahead)) // Combining Greek Ypogegrammeni
        { // Move iota-subscript to end of any sequence of combining marks.
            GP_u32_APPEND(lookahead);
            lookahead = encoding;
            continue;
        }

        if (lookahead == 0x0307                &&
            strncmp(locale.code, "lt", 2) == 0 &&
            gp_is_soft_dotted(encoding))
        { // remove DOT ABOVE after "i"
            i += codepoint_length;
            codepoint_length = gp_utf8_encode(&lookahead, *str, i);
        }

        switch (encoding) {
            case 0x00DF: GP_u32_APPEND('S',    'S'           ); break; // LATIN SMALL LETTER SHARP S

            case 0xFB00: GP_u32_APPEND(0x0046, 0x0046        ); break; // LATIN SMALL LIGATURE FF
            case 0xFB01: GP_u32_APPEND(0x0046, 0x0049        ); break; // LATIN SMALL LIGATURE FI
            case 0xFB02: GP_u32_APPEND(0x0046, 0x004C        ); break; // LATIN SMALL LIGATURE FL
            case 0xFB03: GP_u32_APPEND(0x0046, 0x0046, 0x0049); break; // LATIN SMALL LIGATURE FFI
            case 0xFB04: GP_u32_APPEND(0x0046, 0x0046, 0x004C); break; // LATIN SMALL LIGATURE FFL
            case 0xFB05: GP_u32_APPEND(0x0053, 0x0054        ); break; // LATIN SMALL LIGATURE LONG S T
            case 0xFB06: GP_u32_APPEND(0x0053, 0x0054        ); break; // LATIN SMALL LIGATURE ST

            case 0x0587: GP_u32_APPEND(0x0535, 0x0552        ); break; // ARMENIAN SMALL LIGATURE ECH YIWN
            case 0xFB13: GP_u32_APPEND(0x0544, 0x0546        ); break; // ARMENIAN SMALL LIGATURE MEN NOW
            case 0xFB14: GP_u32_APPEND(0x0544, 0x0535        ); break; // ARMENIAN SMALL LIGATURE MEN ECH
            case 0xFB15: GP_u32_APPEND(0x0544, 0x053B        ); break; // ARMENIAN SMALL LIGATURE MEN INI
            case 0xFB16: GP_u32_APPEND(0x054E, 0x0546        ); break; // ARMENIAN SMALL LIGATURE VEW NOW
            case 0xFB17: GP_u32_APPEND(0x0544, 0x053D        ); break; // ARMENIAN SMALL LIGATURE MEN XEH

            case 0x0149: GP_u32_APPEND(0x02BC, 0x004E        ); break; // LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
            case 0x0390: GP_u32_APPEND(0x0399, 0x0308, 0x0301); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
            case 0x03B0: GP_u32_APPEND(0x03A5, 0x0308, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
            case 0x01F0: GP_u32_APPEND(0x004A, 0x030C        ); break; // LATIN SMALL LETTER J WITH CARON

            case 0x1E96: GP_u32_APPEND(0x0048, 0x0331        ); break; // LATIN SMALL LETTER H WITH LINE BELOW
            case 0x1E97: GP_u32_APPEND(0x0054, 0x0308        ); break; // LATIN SMALL LETTER T WITH DIAERESIS
            case 0x1E98: GP_u32_APPEND(0x0057, 0x030A        ); break; // LATIN SMALL LETTER W WITH RING ABOVE
            case 0x1E99: GP_u32_APPEND(0x0059, 0x030A        ); break; // LATIN SMALL LETTER Y WITH RING ABOVE
            case 0x1E9A: GP_u32_APPEND(0x0041, 0x02BE        ); break; // LATIN SMALL LETTER A WITH RIGHT HALF RING
            case 0x1F50: GP_u32_APPEND(0x03A5, 0x0313        ); break; // GREEK SMALL LETTER UPSILON WITH PSILI
            case 0x1F52: GP_u32_APPEND(0x03A5, 0x0313, 0x0300); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA
            case 0x1F54: GP_u32_APPEND(0x03A5, 0x0313, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA
            case 0x1F56: GP_u32_APPEND(0x03A5, 0x0313, 0x0342); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI
            case 0x1FB6: GP_u32_APPEND(0x0391, 0x0342        ); break; // GREEK SMALL LETTER ALPHA WITH PERISPOMENI
            case 0x1FC6: GP_u32_APPEND(0x0397, 0x0342        ); break; // GREEK SMALL LETTER ETA WITH PERISPOMENI
            case 0x1FD2: GP_u32_APPEND(0x0399, 0x0308, 0x0300); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA
            case 0x1FD3: GP_u32_APPEND(0x0399, 0x0308, 0x0301); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA
            case 0x1FD6: GP_u32_APPEND(0x0399, 0x0342        ); break; // GREEK SMALL LETTER IOTA WITH PERISPOMENI
            case 0x1FD7: GP_u32_APPEND(0x0399, 0x0308, 0x0342); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI
            case 0x1FE2: GP_u32_APPEND(0x03A5, 0x0308, 0x0300); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA
            case 0x1FE3: GP_u32_APPEND(0x03A5, 0x0308, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA
            case 0x1FE4: GP_u32_APPEND(0x03A1, 0x0313        ); break; // GREEK SMALL LETTER RHO WITH PSILI
            case 0x1FE6: GP_u32_APPEND(0x03A5, 0x0342        ); break; // GREEK SMALL LETTER UPSILON WITH PERISPOMENI
            case 0x1FE7: GP_u32_APPEND(0x03A5, 0x0308, 0x0342); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI
            case 0x1FF6: GP_u32_APPEND(0x03A9, 0x0342        ); break; // GREEK SMALL LETTER OMEGA WITH PERISPOMENI

            case 0x1FB3: GP_u32_APPEND(0x0391, 0x0399        ); break; // GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI
            case 0x1FBC: GP_u32_APPEND(0x0391, 0x0399        ); break; // GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI
            case 0x1FC3: GP_u32_APPEND(0x0397, 0x0399        ); break; // GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI
            case 0x1FCC: GP_u32_APPEND(0x0397, 0x0399        ); break; // GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI
            case 0x1FF3: GP_u32_APPEND(0x03A9, 0x0399        ); break; // GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI
            case 0x1FFC: GP_u32_APPEND(0x03A9, 0x0399        ); break; // GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI

            case 0x1FB2: GP_u32_APPEND(0x1FBA, 0x0399        ); break; // GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI
            case 0x1FB4: GP_u32_APPEND(0x0386, 0x0399        ); break; // GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI
            case 0x1FC2: GP_u32_APPEND(0x1FCA, 0x0399        ); break; // GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI
            case 0x1FC4: GP_u32_APPEND(0x0389, 0x0399        ); break; // GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI
            case 0x1FF2: GP_u32_APPEND(0x1FFA, 0x0399        ); break; // GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI
            case 0x1FF4: GP_u32_APPEND(0x038F, 0x0399        ); break; // GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI

            case 0x1FB7: GP_u32_APPEND(0x0391, 0x0342, 0x0399); break; // GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND YPOGEGRAMMENI
            case 0x1FC7: GP_u32_APPEND(0x0397, 0x0342, 0x0399); break; // GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI
            case 0x1FF7: GP_u32_APPEND(0x03A9, 0x0342, 0x0399); break; // GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND YPOGEGRAMMENI

            default:
            if (0x1F80 <= encoding && encoding <= 0x1FAF) {
                if      (encoding < 0x1F88) GP_u32_APPEND(0x1F08 + encoding - 0x1F80, 0x0399);
                else if (encoding < 0x1F90) GP_u32_APPEND(0x1F08 + encoding - 0x1F88, 0x0399);
                else if (encoding < 0x1F98) GP_u32_APPEND(0x1F28 + encoding - 0x1F90, 0x0399);
                else if (encoding < 0x1FA0) GP_u32_APPEND(0x1F28 + encoding - 0x1F98, 0x0399);
                else if (encoding < 0x1FA8) GP_u32_APPEND(0x1F68 + encoding - 0x1FA0, 0x0399);
                else                        GP_u32_APPEND(0x1F68 + encoding - 0x1FA8, 0x0399);
            } else if (encoding == 'i') {
                if (strncmp(locale.code, "tr", 2) == 0 || strncmp(locale.code, "az", 2) == 0)
                    GP_u32_APPEND(0x0130);
                else
                    GP_u32_APPEND('I');
            } else {
                const uint32_t upper = gp_u32_to_upper(encoding);
                GP_u32_APPEND(upper);
            }
        }
    }
    gp_str_reserve(str, required_capacity);
    ((GPStringHeader*)*str - 1)->length = 0;
    for (size_t i = 0; i < gp_arr_length(u32); i++)
    {
        ((GPStringHeader*)*str - 1)->length += gp_utf8_decode(
            *str + gp_str_length(*str), u32[i]);
    }

    gp_arena_rewind(scratch, gp_arr_allocation(u32));
}

static bool gp_is_lithuanian_accent(uint32_t encoding)
{ // Only relevants for special cases in gp_to_lower_full() listed here.
    switch (encoding) {
        case 0x0300: // grave
        case 0x00B4: // acute
        case 0x0303: // tilde above
        case 0x02DB: // ogonek
        return true;
    }
    return false;
}

#if LITHUANIANS_COMPLAIN_ABOUT_NOT_HANDLING_ALL_ACCENTS
// use this instead of gp_is_lithuanian_accent().
static bool gp_is_above_combining_class(const uint32_t encoding)
{
    switch (encoding) {
        case 0x0346: case 0x034A: case 0x034B: case 0x034C: case 0x0350:
        case 0x0351: case 0x0352: case 0x0357: case 0x035B: case 0x05A8:
        case 0x05A9: case 0x05AB: case 0x05AC: case 0x05AF: case 0x05C4:
        case 0x0653: case 0x0654:
        return true;
    }
    if      (0x0300 <= encoding && encoding <= 0x0314) return true;
    else if (0x033D <= encoding && encoding <= 0x0344) return true;
    else if (0x0363 <= encoding && encoding <= 0x036F) return true;
    else if (0x0483 <= encoding && encoding <= 0x0487) return true;
    else if (0x0592 <= encoding && encoding <= 0x0599 && encoding != 0x0596) return true;
    else if (0x0483 <= encoding && encoding <= 0x0487) return true;
    else if (0x059C <= encoding && encoding <= 0x05A1) return true;
    else if (0x0610 <= encoding && encoding <= 0x0617) return true;
    else if (0x0657 <= encoding && encoding <= 0x065E && encoding != 0x065C) return true;
    else if (0x06D6 <= encoding && encoding <= 0x06D9) return true;
    return false;
}
#endif

static bool gp_is_word_final(const uint32_t lookbehind, const uint32_t lookahead, const GPLocale locale)
{
    #if !_WIN32
    return (lookbehind <= WCHAR_MAX && iswalpha_l(lookbehind, locale.locale)) &&
        !  (lookahead  <= WCHAR_MAX && iswalpha_l(lookahead,  locale.locale));
    #else
    return (lookbehind <= WCHAR_MAX && _iswalpha_l(lookbehind, locale.locale)) &&
        !  (lookahead  <= WCHAR_MAX && _iswalpha_l(lookahead,  locale.locale));
    #endif
}

uint32_t gp_u32_to_lower(uint32_t);
void gp_str_to_lower_full(
    GPString* str,
    GPLocale locale)
{
    // TODO ASCII optimization would go here if not Turkish locale.

    GPArena* scratch = gp_scratch_arena();
    size_t u32_capacity = gp_str_length(*str); // this gets incremented by GP_u32_APPEND()
    GPArray(uint32_t) u32 = gp_arr_new(
        (GPAllocator*)scratch, sizeof u32[0], u32_capacity);
    size_t required_capacity = 0;
    (*str)[gp_str_length(*str)].c = '\0'; // last lookahead
    uint32_t lookahead;
    size_t codepoint_length = gp_utf8_encode(&lookahead, *str, 0);
    uint32_t lookbehind = '\0';
    uint32_t encoding;

    for (size_t i = 0; i < gp_str_length(*str); lookbehind = encoding)
    {
        encoding = lookahead;
        i += codepoint_length;
        codepoint_length = gp_utf8_encode(&lookahead, *str, i);

        if (encoding == 0x03A3) // GREEK CAPITAL LETTER SIGMA
        {
            if (gp_is_word_final(lookbehind, lookahead, locale))
                GP_u32_APPEND(0x03C2); // GREEK SMALL LETTER FINAL SIGMA
            else
                GP_u32_APPEND(0x03C3); // GREEK SMALL LETTER SIGMA
            continue;
        }

        if (strncmp(locale.code, "lt", 2) == 0) switch (encoding)
        {
            case 'I':    GP_u32_APPEND('i');    if (gp_is_lithuanian_accent(lookahead)) GP_u32_APPEND(0x0307); continue; // LATIN CAPITAL LETTER I
            case 'J':    GP_u32_APPEND('j');    if (gp_is_lithuanian_accent(lookahead)) GP_u32_APPEND(0x0307); continue; // LATIN CAPITAL LETTER J
            case 0x012E: GP_u32_APPEND(0x012F); if (gp_is_lithuanian_accent(lookahead)) GP_u32_APPEND(0x0307); continue; // LATIN CAPITAL LETTER I WITH OGONEK
            case 0x00CC: GP_u32_APPEND('i', 0x0307, 0x0300); continue; // LATIN CAPITAL LETTER I WITH GRAVE
            case 0x00CD: GP_u32_APPEND('i', 0x0307, 0x0301); continue; // LATIN CAPITAL LETTER I WITH ACUTE
            case 0x0128: GP_u32_APPEND('i', 0x0307, 0x0303); continue; // LATIN CAPITAL LETTER I WITH TILDE
        }

        if (encoding == 'I')
        {
            if (strncmp(locale.code, "tr", 2) == 0 || strncmp(locale.code, "az", 2) == 0)
            {
                if (lookahead == 0x0307) { // COMBINING DOT ABOVE
                    GP_u32_APPEND('i');
                    i += codepoint_length;
                    codepoint_length = gp_utf8_encode(&lookahead, *str, i);
                } else {
                    GP_u32_APPEND(0x0131);
                }
            }
            else
            {
                GP_u32_APPEND('i');
            }
            continue;
        }

        if (encoding == 0x0130) // LATIN CAPITAL LETTER I WITH DOT ABOVE
        {
            if (strncmp(locale.code, "tr", 2) == 0 || strncmp(locale.code, "az", 2) == 0)
                GP_u32_APPEND('i');
            else // Preserve canonical equivalence for I with dot.
                GP_u32_APPEND('i', 0x0307);
            continue;
        }

        const uint32_t lower = gp_u32_to_lower(encoding);
        GP_u32_APPEND(lower);
    }

    gp_str_reserve(str, required_capacity);
    ((GPStringHeader*)*str - 1)->length = 0;
    for (size_t i = 0; i < gp_arr_length(u32); i++)
    {
        ((GPStringHeader*)*str - 1)->length += gp_utf8_decode(
            *str + gp_str_length(*str), u32[i]);
    }

    gp_arena_rewind(scratch, gp_arr_allocation(u32));
}

uint32_t gp_u32_to_title(uint32_t);
void gp_str_capitalize(
    GPString* str,
    GPLocale locale)
{
    // TODO ASCII optimization would go here if not Turkish locale

    if (gp_str_length(*str) == 0)
        return;

    uint32_t first, second = 0;
    size_t first_length = gp_utf8_encode(&first, *str, 0);
    size_t second_length = 0;
    if (first_length < gp_str_length(*str))
        second_length = gp_utf8_encode(&second, *str, first_length);

    if (first == 0x0345 && gp_is_diatrical(second))
    { // Move iota-subscript to end of any sequence of combining marks.
        size_t diatricals_length = second_length;
        while (true) {
            uint32_t cp;
            size_t cp_length = gp_utf8_encode(&cp, *str, first_length + diatricals_length);
            if ( ! gp_is_diatrical(cp))
                break;
            diatricals_length += cp_length;
        }
        memmove(*str, *str + first_length, diatricals_length);
        memcpy(*str + diatricals_length, "\u0399", strlen("\u0399"));
        return;
    }

    if (second == 0x0307                   &&
        strncmp(locale.code, "lt", 2) == 0 &&
        gp_is_soft_dotted(first))
    { // remove DOT ABOVE after "i"
        memmove(
            *str + first_length,
            *str + first_length + second_length,
            gp_str_length(*str) - (first_length + second_length));

        ((GPStringHeader*)*str - 1)->length -= second_length;
    }

    size_t u32_capacity = 0; // dummy for GP_u32_APPEND()
    GPArray(uint32_t) u32 = gp_arr_on_stack(NULL, 4, uint32_t);
    size_t required_capacity = 0; // this gets incremented by GP_u32_APPEND()

    switch (first) {
        case 0x00DF: GP_u32_APPEND(0x0053, 0x0073        ); break; // LATIN SMALL LETTER SHARP S

        case 0xFB00: GP_u32_APPEND(0x0046, 0x0066        ); break; // LATIN SMALL LIGATURE FF
        case 0xFB01: GP_u32_APPEND(0x0046, 0x0069        ); break; // LATIN SMALL LIGATURE FI
        case 0xFB02: GP_u32_APPEND(0x0046, 0x006C        ); break; // LATIN SMALL LIGATURE FL
        case 0xFB03: GP_u32_APPEND(0x0046, 0x0066, 0x0069); break; // LATIN SMALL LIGATURE FFI
        case 0xFB04: GP_u32_APPEND(0x0046, 0x0066, 0x006C); break; // LATIN SMALL LIGATURE FFL
        case 0xFB05: GP_u32_APPEND(0x0053, 0x0074        ); break; // LATIN SMALL LIGATURE LONG S T
        case 0xFB06: GP_u32_APPEND(0x0053, 0x0074        ); break; // LATIN SMALL LIGATURE ST

        case 0x0587: GP_u32_APPEND(0x0535, 0x0582        ); break; // ARMENIAN SMALL LIGATURE ECH YIWN
        case 0xFB13: GP_u32_APPEND(0x0544, 0x0576        ); break; // ARMENIAN SMALL LIGATURE MEN NOW
        case 0xFB14: GP_u32_APPEND(0x0544, 0x0565        ); break; // ARMENIAN SMALL LIGATURE MEN ECH
        case 0xFB15: GP_u32_APPEND(0x0544, 0x056B        ); break; // ARMENIAN SMALL LIGATURE MEN INI
        case 0xFB16: GP_u32_APPEND(0x054E, 0x0576        ); break; // ARMENIAN SMALL LIGATURE VEW NOW
        case 0xFB17: GP_u32_APPEND(0x0544, 0x056D        ); break; // ARMENIAN SMALL LIGATURE MEN XEH

        case 0x0149: GP_u32_APPEND(0x02BC, 0x004E        ); break; // LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
        case 0x0390: GP_u32_APPEND(0x0399, 0x0308, 0x0301); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
        case 0x03B0: GP_u32_APPEND(0x03A5, 0x0308, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
        case 0x01F0: GP_u32_APPEND(0x004A, 0x030C        ); break; // LATIN SMALL LETTER J WITH CARON

        case 0x1E96: GP_u32_APPEND(0x0048, 0x0331        ); break; // LATIN SMALL LETTER H WITH LINE BELOW
        case 0x1E97: GP_u32_APPEND(0x0054, 0x0308        ); break; // LATIN SMALL LETTER T WITH DIAERESIS
        case 0x1E98: GP_u32_APPEND(0x0057, 0x030A        ); break; // LATIN SMALL LETTER W WITH RING ABOVE
        case 0x1E99: GP_u32_APPEND(0x0059, 0x030A        ); break; // LATIN SMALL LETTER Y WITH RING ABOVE
        case 0x1E9A: GP_u32_APPEND(0x0041, 0x02BE        ); break; // LATIN SMALL LETTER A WITH RIGHT HALF RING
        case 0x1F50: GP_u32_APPEND(0x03A5, 0x0313        ); break; // GREEK SMALL LETTER UPSILON WITH PSILI
        case 0x1F52: GP_u32_APPEND(0x03A5, 0x0313, 0x0300); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA
        case 0x1F54: GP_u32_APPEND(0x03A5, 0x0313, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA
        case 0x1F56: GP_u32_APPEND(0x03A5, 0x0313, 0x0342); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI
        case 0x1FB6: GP_u32_APPEND(0x0391, 0x0342        ); break; // GREEK SMALL LETTER ALPHA WITH PERISPOMENI
        case 0x1FC6: GP_u32_APPEND(0x0397, 0x0342        ); break; // GREEK SMALL LETTER ETA WITH PERISPOMENI
        case 0x1FD2: GP_u32_APPEND(0x0399, 0x0308, 0x0300); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA
        case 0x1FD3: GP_u32_APPEND(0x0399, 0x0308, 0x0301); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA
        case 0x1FD6: GP_u32_APPEND(0x0399, 0x0342        ); break; // GREEK SMALL LETTER IOTA WITH PERISPOMENI
        case 0x1FD7: GP_u32_APPEND(0x0399, 0x0308, 0x0342); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI
        case 0x1FE2: GP_u32_APPEND(0x03A5, 0x0308, 0x0300); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA
        case 0x1FE3: GP_u32_APPEND(0x03A5, 0x0308, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA
        case 0x1FE4: GP_u32_APPEND(0x03A1, 0x0313        ); break; // GREEK SMALL LETTER RHO WITH PSILI
        case 0x1FE6: GP_u32_APPEND(0x03A5, 0x0342        ); break; // GREEK SMALL LETTER UPSILON WITH PERISPOMENI
        case 0x1FE7: GP_u32_APPEND(0x03A5, 0x0308, 0x0342); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI
        case 0x1FF6: GP_u32_APPEND(0x03A9, 0x0342        ); break; // GREEK SMALL LETTER OMEGA WITH PERISPOMENI

        case 0x1FB2: GP_u32_APPEND(0x1FBA, 0x0345        ); break; // GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI
        case 0x1FB4: GP_u32_APPEND(0x0386, 0x0345        ); break; // GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI
        case 0x1FC2: GP_u32_APPEND(0x1FCA, 0x0345        ); break; // GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI
        case 0x1FC4: GP_u32_APPEND(0x0389, 0x0345        ); break; // GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI
        case 0x1FF2: GP_u32_APPEND(0x1FFA, 0x0345        ); break; // GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI
        case 0x1FF4: GP_u32_APPEND(0x038F, 0x0345        ); break; // GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI

        case 0x1FB7: GP_u32_APPEND(0x0391, 0x0342, 0x0345); break; // GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND YPOGEGRAMMENI
        case 0x1FC7: GP_u32_APPEND(0x0397, 0x0342, 0x0345); break; // GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI
        case 0x1FF7: GP_u32_APPEND(0x03A9, 0x0342, 0x0345); break; // GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND YPOGEGRAMMENI

        default:
        if (first == 'i') {
            if (strncmp(locale.code, "tr", 2) == 0 || strncmp(locale.code, "az", 2) == 0)
                GP_u32_APPEND(0x0130);
            else
                GP_u32_APPEND('I');
        } else {
            const uint32_t title = gp_u32_to_title(first);
            GP_u32_APPEND(title);
        }
    }

    gp_str_reserve(str, gp_str_length(*str) + required_capacity - first_length);
    memmove(
        *str + required_capacity, *str + first_length, gp_str_length(*str) - first_length);
    for (size_t i = 0, j = 0; i < gp_arr_length(u32); i++)
        j += gp_utf8_decode(*str + j, u32[i]);

    ((GPStringHeader*)*str - 1)->length += required_capacity - first_length;
}

#define GP_wcs_APPEND1(CODEPOINT) \
    (*wcs)[((GPArrayHeader*)*wcs - 1)->length++] = (CODEPOINT)
#define GP_wcs_APPEND(...) do \
{ \
    if (GP_COUNT_ARGS(__VA_ARGS__) > 1) \
        *(wcs) = gp_arr_reserve(sizeof (*wcs)[0], *wcs, wcs_capacity += GP_COUNT_ARGS(__VA_ARGS__) - 1); \
    GP_PROCESS_ALL_ARGS(GP_wcs_APPEND1, GP_SEMICOLON, __VA_ARGS__); \
} while(0)

void gp_wcs_fold_utf8(
    GPArray(wchar_t)* wcs, const void*_str, const size_t str_length, const GPLocale locale)
{
    const uint8_t* str = _str;
    ((GPArrayHeader*)*wcs - 1)->length = 0;
    size_t wcs_capacity = str_length + sizeof"";
    *wcs = gp_arr_reserve(sizeof(*wcs)[0], *wcs, wcs_capacity);
    const bool turkish = strncmp(locale.code, "tr", 2) == 0 || strncmp(locale.code, "az", 2) == 0;

    size_t i = 0;
    if ( ! turkish) for (; i < str_length; ++i)
    {
        if (str[i] > 0x7F)
            break;
        (*wcs)[i] = 'A' <= str[i] && str[i] <= 'Z' ? str[i] + 'a' - 'A' : str[i];
    }
    ((GPArrayHeader*)*wcs - 1)->length = i;

    for (size_t codepoint_length; i < str_length; i += codepoint_length)
    {
        // TODO more ASCII optimization here

        uint32_t encoding;
        codepoint_length = gp_utf8_encode(&encoding, str, i);

        switch (encoding) {
            case 'I': if (turkish) GP_wcs_APPEND(0x0131); else GP_wcs_APPEND('i'); break;
            case 0x0130: GP_wcs_APPEND('i'); if ( ! turkish) GP_wcs_APPEND(0x0307); break; // LATIN CAPITAL LETTER I WITH DOT ABOVE

            case 0x00B5: GP_wcs_APPEND(0x03BC                ); break; // MICRO SIGN
            case 0x00DF: GP_wcs_APPEND(0x0073, 0x0073        ); break; // LATIN SMALL LETTER SHARP S
            case 0x0149: GP_wcs_APPEND(0x02BC, 0x006E        ); break; // LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
            case 0x017F: GP_wcs_APPEND(0x0073                ); break; // LATIN SMALL LETTER LONG S
            case 0x01F0: GP_wcs_APPEND(0x006A, 0x030C        ); break; // LATIN SMALL LETTER J WITH CARON
            case 0x0345: GP_wcs_APPEND(0x03B9                ); break; // COMBINING GREEK YPOGEGRAMMENI
            case 0x0390: GP_wcs_APPEND(0x03B9, 0x0308, 0x0301); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
            case 0x03B0: GP_wcs_APPEND(0x03C5, 0x0308, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
            case 0x03C2: GP_wcs_APPEND(0x03C3                ); break; // GREEK SMALL LETTER FINAL SIGMA
            case 0x03D0: GP_wcs_APPEND(0x03B2                ); break; // GREEK BETA SYMBOL
            case 0x03D1: GP_wcs_APPEND(0x03B8                ); break; // GREEK THETA SYMBOL
            case 0x03D5: GP_wcs_APPEND(0x03C6                ); break; // GREEK PHI SYMBOL
            case 0x03D6: GP_wcs_APPEND(0x03C0                ); break; // GREEK PI SYMBOL
            case 0x03F0: GP_wcs_APPEND(0x03BA                ); break; // GREEK KAPPA SYMBOL
            case 0x03F1: GP_wcs_APPEND(0x03C1                ); break; // GREEK RHO SYMBOL
            case 0x03F5: GP_wcs_APPEND(0x03B5                ); break; // GREEK LUNATE EPSILON SYMBOL
            case 0x0587: GP_wcs_APPEND(0x0565, 0x0582        ); break; // ARMENIAN SMALL LIGATURE ECH YIWN
            case 0x1E9E: GP_wcs_APPEND(0x0073, 0x0073        ); break; // LATIN CAPITAL LETTER SHARP S
            case 0x1F50: GP_wcs_APPEND(0x03C5, 0x0313        ); break; // GREEK SMALL LETTER UPSILON WITH PSILI
            case 0x1F52: GP_wcs_APPEND(0x03C5, 0x0313, 0x0300); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA
            case 0x1F54: GP_wcs_APPEND(0x03C5, 0x0313, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA
            case 0x1F56: GP_wcs_APPEND(0x03C5, 0x0313, 0x0342); break; // GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI
            case 0x1FB2: GP_wcs_APPEND(0x1F70, 0x03B9        ); break; // GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI
            case 0x1FB3: GP_wcs_APPEND(0x03B1, 0x03B9        ); break; // GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI
            case 0x1FB4: GP_wcs_APPEND(0x03AC, 0x03B9        ); break; // GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI
            case 0x1FB6: GP_wcs_APPEND(0x03B1, 0x0342        ); break; // GREEK SMALL LETTER ALPHA WITH PERISPOMENI
            case 0x1FB7: GP_wcs_APPEND(0x03B1, 0x0342, 0x03B9); break; // GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND YPOGEGRAMMENI
            case 0x1FBC: GP_wcs_APPEND(0x03B1, 0x03B9        ); break; // GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI
            case 0x1FBE: GP_wcs_APPEND(0x03B9                ); break; // GREEK PROSGEGRAMMENI
            case 0x1FC2: GP_wcs_APPEND(0x1F74, 0x03B9        ); break; // GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI
            case 0x1FC3: GP_wcs_APPEND(0x03B7, 0x03B9        ); break; // GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI
            case 0x1FC4: GP_wcs_APPEND(0x03AE, 0x03B9        ); break; // GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI
            case 0x1FC6: GP_wcs_APPEND(0x03B7, 0x0342        ); break; // GREEK SMALL LETTER ETA WITH PERISPOMENI
            case 0x1FC7: GP_wcs_APPEND(0x03B7, 0x0342, 0x03B9); break; // GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI
            case 0x1FCC: GP_wcs_APPEND(0x03B7, 0x03B9        ); break; // GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI
            case 0x1FD2: GP_wcs_APPEND(0x03B9, 0x0308, 0x0300); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA
            case 0x1FD3: GP_wcs_APPEND(0x03B9, 0x0308, 0x0301); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA
            case 0x1FD6: GP_wcs_APPEND(0x03B9, 0x0342        ); break; // GREEK SMALL LETTER IOTA WITH PERISPOMENI
            case 0x1FD7: GP_wcs_APPEND(0x03B9, 0x0308, 0x0342); break; // GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI
            case 0x1FE2: GP_wcs_APPEND(0x03C5, 0x0308, 0x0300); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA
            case 0x1FE3: GP_wcs_APPEND(0x03C5, 0x0308, 0x0301); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA
            case 0x1FE4: GP_wcs_APPEND(0x03C1, 0x0313        ); break; // GREEK SMALL LETTER RHO WITH PSILI
            case 0x1FE6: GP_wcs_APPEND(0x03C5, 0x0342        ); break; // GREEK SMALL LETTER UPSILON WITH PERISPOMENI
            case 0x1FE7: GP_wcs_APPEND(0x03C5, 0x0308, 0x0342); break; // GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI
            case 0x1FF2: GP_wcs_APPEND(0x1F7C, 0x03B9        ); break; // GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI
            case 0x1FF3: GP_wcs_APPEND(0x03C9, 0x03B9        ); break; // GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI
            case 0x1FF4: GP_wcs_APPEND(0x03CE, 0x03B9        ); break; // GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI
            case 0x1FF6: GP_wcs_APPEND(0x03C9, 0x0342        ); break; // GREEK SMALL LETTER OMEGA WITH PERISPOMENI
            case 0x1FF7: GP_wcs_APPEND(0x03C9, 0x0342, 0x03B9); break; // GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND YPOGEGRAMMENI
            case 0x1FFC: GP_wcs_APPEND(0x03C9, 0x03B9        ); break; // GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI
            case 0x1C80: GP_wcs_APPEND(0x0432                ); break; // CYRILLIC SMALL LETTER ROUNDED VE
            case 0x1C81: GP_wcs_APPEND(0x0434                ); break; // CYRILLIC SMALL LETTER LONG-LEGGED DE
            case 0x1C82: GP_wcs_APPEND(0x043E                ); break; // CYRILLIC SMALL LETTER NARROW O
            case 0x1C83: GP_wcs_APPEND(0x0441                ); break; // CYRILLIC SMALL LETTER WIDE ES
            case 0x1C84: GP_wcs_APPEND(0x0442                ); break; // CYRILLIC SMALL LETTER TALL TE
            case 0x1C85: GP_wcs_APPEND(0x0442                ); break; // CYRILLIC SMALL LETTER THREE-LEGGED TE
            case 0x1C86: GP_wcs_APPEND(0x044A                ); break; // CYRILLIC SMALL LETTER TALL HARD SIGN
            case 0x1C87: GP_wcs_APPEND(0x0463                ); break; // CYRILLIC SMALL LETTER TALL YAT
            case 0x1C88: GP_wcs_APPEND(0xA64B                ); break; // CYRILLIC SMALL LETTER UNBLENDED UK
            case 0x1E96: GP_wcs_APPEND(0x0068, 0x0331        ); break; // LATIN SMALL LETTER H WITH LINE BELOW
            case 0x1E97: GP_wcs_APPEND(0x0074, 0x0308        ); break; // LATIN SMALL LETTER T WITH DIAERESIS
            case 0x1E98: GP_wcs_APPEND(0x0077, 0x030A        ); break; // LATIN SMALL LETTER W WITH RING ABOVE
            case 0x1E99: GP_wcs_APPEND(0x0079, 0x030A        ); break; // LATIN SMALL LETTER Y WITH RING ABOVE
            case 0x1E9A: GP_wcs_APPEND(0x0061, 0x02BE        ); break; // LATIN SMALL LETTER A WITH RIGHT HALF RING
            case 0x1E9B: GP_wcs_APPEND(0x1E61                ); break; // LATIN SMALL LETTER LONG S WITH DOT ABOVE
            case 0xFB00: GP_wcs_APPEND(0x0066, 0x0066        ); break; // LATIN SMALL LIGATURE FF
            case 0xFB01: GP_wcs_APPEND(0x0066, 0x0069        ); break; // LATIN SMALL LIGATURE FI
            case 0xFB02: GP_wcs_APPEND(0x0066, 0x006C        ); break; // LATIN SMALL LIGATURE FL
            case 0xFB03: GP_wcs_APPEND(0x0066, 0x0066, 0x0069); break; // LATIN SMALL LIGATURE FFI
            case 0xFB04: GP_wcs_APPEND(0x0066, 0x0066, 0x006C); break; // LATIN SMALL LIGATURE FFL
            case 0xFB05: GP_wcs_APPEND(0x0073, 0x0074        ); break; // LATIN SMALL LIGATURE LONG S T
            case 0xFB06: GP_wcs_APPEND(0x0073, 0x0074        ); break; // LATIN SMALL LIGATURE ST
            case 0xFB13: GP_wcs_APPEND(0x0574, 0x0576        ); break; // ARMENIAN SMALL LIGATURE MEN NOW
            case 0xFB14: GP_wcs_APPEND(0x0574, 0x0565        ); break; // ARMENIAN SMALL LIGATURE MEN ECH
            case 0xFB15: GP_wcs_APPEND(0x0574, 0x056B        ); break; // ARMENIAN SMALL LIGATURE MEN INI
            case 0xFB16: GP_wcs_APPEND(0x057E, 0x0576        ); break; // ARMENIAN SMALL LIGATURE VEW NOW
            case 0xFB17: GP_wcs_APPEND(0x0574, 0x056D        ); break; // ARMENIAN SMALL LIGATURE MEN XEH

            default:
            if      (0x13F8 <= encoding && encoding <= 0x13FD) GP_wcs_APPEND(encoding - 0x8);
            else if (0x1F80 <= encoding && encoding <= 0x1F87) GP_wcs_APPEND(encoding - 0x80, 0x03B9);
            else if (0x1F88 <= encoding && encoding <= 0x1F8F) GP_wcs_APPEND(encoding - 0x88, 0x03B9);
            else if (0x1F90 <= encoding && encoding <= 0x1F97) GP_wcs_APPEND(encoding - 0x70, 0x03B9);
            else if (0x1F98 <= encoding && encoding <= 0x1F9F) GP_wcs_APPEND(encoding - 0x78, 0x03B9);
            else if (0x1FA0 <= encoding && encoding <= 0x1FA7) GP_wcs_APPEND(encoding - 0x40, 0x03B9);
            else if (0x1FA8 <= encoding && encoding <= 0x1FAF) GP_wcs_APPEND(encoding - 0x48, 0x03B9);
            else if (0xAB70 <= encoding && encoding <= 0xABBF) GP_wcs_APPEND(encoding - 0x97D0);
            else {
                uint32_t lower = gp_u32_to_lower(encoding);
                if (lower <= WCHAR_MAX) {
                    GP_wcs_APPEND(lower);
                } else { // surrogate pair in Windows
                    lower &= ~0x10000;
                    GP_wcs_APPEND((lower >> 10) | 0xD800, (lower & 0x3FF) | 0xDC00);
                }
            }
        }
    }
    (*wcs)[gp_arr_length(*wcs)] = L'\0';
}

static int gp_utf8_codepoint_compare(const void*_s1, const void*_s2)
{
    GPString s1 = *(GPString*)_s1;
    GPString s2 = *(GPString*)_s2;
    const size_t min_length = gp_str_length(s1) < gp_str_length(s2) ? gp_str_length(s1) : gp_str_length(s2);
    for (size_t i = 0, codepoint_length; i < min_length; i += codepoint_length)
    {
        uint32_t cp1, cp2;
        codepoint_length = gp_utf8_encode(&cp1, s1, i); gp_utf8_encode(&cp2, s1, i);
        if (cp1 != cp2)
            return cp1 - cp2;
    }
    return gp_str_length(s1) - gp_str_length(s2);
}

typedef struct gp_narrow_wide
{
    GPString         narrow;
    GPArray(wchar_t) wide;
    GPLocale*        locale;
} GPNarrowWide;

static int gp_wcs_compare(const void*_s1, const void*_s2)
{
    const GPNarrowWide* s1 = _s1;
    const GPNarrowWide* s2 = _s2;
    return wcscmp(s1->wide, s2->wide);
}

static int gp_wcs_collate(const void*_s1, const void*_s2)
{
    const GPNarrowWide* s1 = _s1;
    const GPNarrowWide* s2 = _s2;
    #if !_WIN32
    return wcscoll_l(s1->wide, s2->wide, s1->locale->locale);
    #else
    return _wcscoll_l(s1->wide, s2->wide, s1->locale->locale);
    #endif
}

void gp_str_sort(
    GPArray(GPString) strs,
    const int flags,
    GPLocale locale)
{
    const bool fold    = flags & 0x4;
    const bool collate = flags & 0x2;
    if ( ! (fold || collate)) {
        qsort(strs, gp_arr_length(strs), sizeof(GPString), gp_utf8_codepoint_compare);
        return;
    }
    GPArena* scratch = gp_scratch_arena();
    GPNarrowWide* pairs = gp_mem_alloc((GPAllocator*)scratch, sizeof pairs[0] * gp_arr_length(strs));

    for (size_t i = 0; i < gp_arr_length(strs); ++i) {
        pairs[i].narrow = strs[i];
        pairs[i].locale = &locale;
        pairs[i].wide = gp_arr_new((GPAllocator*)scratch, sizeof pairs[i].wide[0], 0);
        if (fold)
            gp_wcs_fold_utf8(&pairs[i].wide, strs[i], gp_str_length(strs[i]), locale);
        else
            gp_utf8_to_wcs(&pairs[i].wide, strs[i], gp_str_length(strs[i]));
    }

    if (collate)
        qsort(pairs, gp_arr_length(strs), sizeof pairs[0], gp_wcs_collate);
    else
        qsort(pairs, gp_arr_length(strs), sizeof pairs[0], gp_wcs_compare);

    for (size_t i = 0; i < gp_arr_length(strs); ++i)
        strs[i] = pairs[i].narrow;

    gp_arena_rewind(scratch, pairs);
}

int gp_str_compare(
    const GPString s1,
    const void*const s2,
    const size_t s2_length,
    const int flags,
    const GPLocale locale)
{
    const bool fold    = flags & 0x4;
    const bool collate = flags & 0x2;
    if ( ! (fold || collate))
    {
        const size_t min_length = gp_str_length(s1) < s2_length ? gp_str_length(s1) : s2_length;
        for (size_t i = 0, codepoint_length; i < min_length; i += codepoint_length)
        {
            uint32_t cp1, cp2;
            codepoint_length = gp_utf8_encode(&cp1, s1, i); gp_utf8_encode(&cp2, s1, i);
            if (cp1 != cp2)
                return cp1 - cp2;
        }
        return gp_str_length(s1) - s2_length;
    }
    GPArena* scratch = gp_scratch_arena();
    GPArray(wchar_t) wcs1 = gp_arr_new((GPAllocator*)scratch, sizeof wcs1[0], 0);
    GPArray(wchar_t) wcs2 = gp_arr_new((GPAllocator*)scratch, sizeof wcs2[0], 0);

    if (fold) {
        gp_wcs_fold_utf8(&wcs1, s1, gp_str_length(s1), locale);
        gp_wcs_fold_utf8(&wcs2, s2, s2_length,         locale);
    } else {
        gp_utf8_to_wcs(&wcs1, s1, gp_str_length(s1));
        gp_utf8_to_wcs(&wcs2, s2, s2_length);
    }

    int result;
    if (collate) {
        #if !_WIN32
        result = wcscoll_l(wcs1, wcs2, locale.locale);
        #else
        result = _wcscoll_l(wcs1, wcs2, locale.locale);
        #endif
    } else {
        result = wcscmp(wcs1, wcs2);
    }

    gp_arena_rewind(scratch, gp_arr_allocation(wcs1));
    return result;
}

