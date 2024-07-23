// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/unicode.h>
#include <gpc/utils.h>
#include <gpc/hashmap.h>
#include "thread.h"
#include "common.h"
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

// ----------------------------------------------------------------------------
// Locale

static GPMap*  gp_locale_table;
static GPMutex gp_locale_table_mutex;

static void gp_locale_delete(void* locale)
{
    #if GP_LOCALE_AVAILABLE
    if ((GPLocale)locale != (GPLocale)0 && (GPLocale)locale != (GPLocale)-1)
        #if _WIN32
        _free_locale((GPLocale)locale);
        #else
        freelocale((GPLocale)locale);
        #endif
    #endif
    return;
}

static GPLocale gp_default_locale;

static void gp_delete_locale_table(void)
{
    gp_map_delete(gp_locale_table);
    gp_mutex_destroy(&gp_locale_table_mutex);
    gp_locale_delete(gp_default_locale);
}

static void gp_init_locale_table(void)
{
    const GPMapInitializer init = {
        .element_size =  0,
        .capacity     = 32,
        .destructor   = gp_locale_delete
    };
    gp_locale_table = gp_map_new(gp_heap, &init);
    gp_mutex_init(&gp_locale_table_mutex);

    #if GP_LOCALE_AVAILABLE
    #if !_WIN32
    gp_default_locale = newlocale(LC_ALL_MASK, "C.UTF-8", (locale_t)0);
    #elif __MINGW32__
    gp_default_locale = _create_locale(LC_ALL, "");
    #else
    gp_default_locale = _create_locale(LC_ALL, ".UTF-8");
    #endif
    #endif // GP_LOCALE_AVAILABLE

    atexit(gp_delete_locale_table); // shut up sanitizer
}

GPLocale gp_locale(const char* locale_code)
{
    #if ! GP_LOCALE_AVAILABLE
    return NULL;
    #endif
    if (locale_code == NULL)
        return (GPLocale)0;

    static GPThreadOnce locale_table_once = GP_THREAD_ONCE_INIT;
    gp_thread_once(&locale_table_once, gp_init_locale_table);

    if (locale_code[0] == '\0')
        return gp_default_locale;

    GPUint128 key = gp_u128(0, gp_bytes_hash64(locale_code, strlen(locale_code)));
    GPLocale locale = (GPLocale)gp_map_get(gp_locale_table, key);

    if (locale == (GPLocale)0) // Race condition might happen here.
    {
        gp_mutex_lock(&gp_locale_table_mutex);

        // Handle the race condition mentioned above.
        if ((locale = (GPLocale)gp_map_get(gp_locale_table, key)) != (GPLocale)0)
        {
            gp_mutex_unlock(&gp_locale_table_mutex);
            return locale != (GPLocale)-1 ? locale : (GPLocale)0;
        }
        char full_locale_code[16] = "";
        strncpy(full_locale_code, locale_code, strlen("xxx_XX"));
        #ifndef _WIN32
        if (locale_code[0] == '\0')
            full_locale_code[0] = 'C';
        #endif
        #ifndef __MINGW32__
        strcat(full_locale_code, ".UTF-8");
        #endif

        #if _WIN32
        locale = _create_locale(LC_ALL, full_locale_code);
        #else
        locale = newlocale(LC_ALL_MASK, full_locale_code, (GPLocale)0);
        #endif
        if (locale == (GPLocale)0) // mark the locale as unavailable
            locale = (GPLocale)-1;
        gp_map_put(gp_locale_table, key, locale);
        gp_mutex_unlock(&gp_locale_table_mutex);
    }
    if (locale == (GPLocale)-1)
        return (GPLocale)0;
    return locale;
}

const char* gp_set_utf8_global_locale(int category, const char* locale_code)
{
    char full_locale_code[16] = "";
    strncpy(full_locale_code, locale_code, strlen("xx_XX"));
    #ifndef _WIN32
    if (locale_code[0] == '\0')
        full_locale_code[0] = 'C';
    #endif
    #ifndef __MINGW32__
    strcat(full_locale_code, ".UTF-8");
    #endif

    return setlocale(category, full_locale_code);
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
        gp_arr_length(*u32) + gp_bytes_codepoint_count((uint8_t*)u8 + i, u8_length - i));

    for (; i < u8_length; i += codepoint_length)
    {
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        (*u32)[((GPArrayHeader*)*u32 - 1)->length++] = encoding;
    }
}

static void gp_utf8_to_utf32_unsafe(
    GPArray(uint32_t)* u32,
    const void*const   u8,
    const size_t       u8_length)
{
    for (size_t i = 0, codepoint_length; i < u8_length; i += codepoint_length)
    {
        uint32_t encoding;
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
        codepoint_length = gp_utf8_codepoint_length(u8, j);
        capacity_needed += codepoint_length <= 3 ? 1 : 2;
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

static void gp_utf8_to_utf16_unsafe(
    GPArray(uint16_t)* u16,
    const void*        u8,
    size_t             u8_length)
{
    for (size_t i = 0, codepoint_length; i < u8_length; i += codepoint_length)
    {
        uint32_t encoding;
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

static void gp_utf8_to_wcs_unsafe(
    GPArray(wchar_t)* wcs,
    const void*       utf8,
    size_t            utf8_length)
{
    if (WCHAR_MAX > UINT16_MAX)
        gp_utf8_to_utf32_unsafe((GPArray(uint32_t)*)wcs, utf8, utf8_length);
    else
        gp_utf8_to_utf16_unsafe((GPArray(uint16_t)*)wcs, utf8, utf8_length);
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

uint32_t gp_u32_to_upper(uint32_t);
uint32_t gp_u32_to_lower(uint32_t);
uint32_t gp_u32_to_title(uint32_t);

// ----------------------------------------------------------------------------
// String extensions

static size_t gp_utf8_find_first_of(
    const void*const haystack,
    const size_t     haystack_length,
    const char*const char_set,
    const size_t     start)
{
    for (size_t cplen, i = start; i < haystack_length; i += cplen) {
        cplen = gp_utf8_codepoint_length(haystack, i);
        if (strstr(char_set, memcpy((char[8]){}, (uint8_t*)haystack + i, cplen)) != NULL)
            return i;
    }
    return GP_NOT_FOUND;
}

static size_t gp_utf8_find_first_not_of(
    const void*const haystack,
    const size_t     haystack_length,
    const char*const char_set,
    const size_t     start)
{
    for (size_t cplen, i = start; i < haystack_length; i += cplen) {
        cplen = gp_utf8_codepoint_length(haystack, i);
        if (strstr(char_set, memcpy((char[8]){}, (uint8_t*)haystack + i, cplen)) == NULL)
            return i;
    }
    return GP_NOT_FOUND;
}

GPArray(GPString) gp_str_split(
    const GPAllocator* allocator,
    const void*const str,
    const size_t str_length,
    const char*const separators)
{
    GPArray(GPString) substrs = NULL;
    size_t j, i = gp_utf8_find_first_not_of(str, str_length, separators, 0);
    if (i == GP_NOT_FOUND)
        return gp_arr_new(allocator, sizeof(GPString), 1);

    size_t indices_length = 0;
    struct start_end_pair {
        size_t start, end;
    } indices[256];

    while (true)
    {
        for (indices_length = 0;
            indices_length < sizeof indices / sizeof indices[0];
            ++indices_length)
        {
            indices[indices_length].start = i;
            i = gp_utf8_find_first_of(str, str_length, separators, i);
            if (i == GP_NOT_FOUND) {
                indices[indices_length++].end = str_length;
                break;
            }
            indices[indices_length].end = i;
            i = gp_utf8_find_first_not_of(str, str_length, separators, i);
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

        for (j = 0; j < indices_length; ++j)
        {
            substrs[gp_arr_length(substrs) + j] = gp_str_new(
                allocator,
                gp_next_power_of_2(indices[j].end - indices[j].start),
                "");
            ((GPStringHeader*)(substrs[gp_arr_length(substrs) + j]) - 1)->length =
                indices[j].end - indices[j].start;
            memcpy(
                substrs[gp_arr_length(substrs) + j],
                (uint8_t*)str + indices[j].start,
                gp_str_length(substrs[gp_arr_length(substrs) + j]));
        }
        ((GPArrayHeader*)substrs - 1)->length += indices_length;

        if (i == GP_NOT_FOUND)
            break;

    }
    return substrs;
}

void gp_str_join(GPString* out, const GPArray(GPString) strs, const char* separator)
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
    const char* locale_code)
{
    char code_buf[4] = "";
    if (locale_code == NULL)
        locale_code = strncpy(code_buf, setlocale(LC_ALL, NULL), 2);

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
            strncmp(locale_code, "lt", 2) == 0 &&
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
                if (strncmp(locale_code, "tr", 2) == 0 || strncmp(locale_code, "az", 2) == 0)
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

static bool gp_is_greek_letter(const uint32_t c)
{
    if ((0x0041 <= c && c <= 0x005a) ||
        (0x0061 <= c && c <= 0x007a) ||
        (0x0370 <= c && c <= 0x0377  && c != 0x0375)  ||
        (0x037a <= c && c <= 0x037F  && c != 0x037E)) return true;
    if ((0x0386 <= c && c <= 0x03FF)) {
        switch (c) {
            case 0x0387: case 0x038B: case 0x038D: case 0x03A2: case 0x03F6:
            return false;
        }   return true;
    }
    if ((0x1d00 <= c && c <= 0x1dbf) ||
        (0x1f00 <= c && c <= 0x1f15) ||
        (0x1f18 <= c && c <= 0x1f1d) ||
        (0x1f20 <= c && c <= 0x1f45) ||
        (0x1f48 <= c && c <= 0x1f4d) ||
        (0x1f50 <= c && c <= 0x1f7d) ||
        (0x1f80 <= c && c <= 0x1fbe) ||
        (0x1fc2 <= c && c <= 0x1fcc) ||
        (0x1fd0 <= c && c <= 0x1fd3) ||
        (0x1fd6 <= c && c <= 0x1fdb) ||
        (0x1fe0 <= c && c <= 0x1fec) ||
        (0x1ff2 <= c && c <= 0x1ffc) ||
        (0x2102 == c || c == 0x2107) ||
        (0x210a <= c && c <= 0x2113) ||
        (0x210a <= c && c <= 0x2115) ||
        (0x2119 <= c && c <= 0x211d) ||
        (0x2124 <= c && c <= 0x2139) ||
        (0x213c <= c && c <= 0x213f) ||
        (0x2145 <= c && c <= 0x2149) ||
        (0xab30 <= c && c <= 0xab69)) {
        switch (c) {
            case 0x214e: return true;
            case 0x1f58: case 0x1f5a: case 0x1f5c: case 0x1f5e: case 0x1fb5:
            case 0x1fbd: case 0x1fc5: case 0x1ff5: case 0x2114: case 0x2125:
            case 0x2127: case 0x2129: case 0x212e: case 0xab5b: return false;
        }
        return true;
    }
    return false;
}

static bool gp_is_greek_final(
    const uint32_t lookbehind, uint32_t lookahead, const char* str)
{
    if (!gp_is_greek_letter(lookbehind) && !gp_is_diatrical(lookbehind))
        return false;
    while (gp_is_diatrical(lookahead))
        str += gp_utf8_encode(&lookahead, str, 0);
    return !gp_is_greek_letter(lookahead);
}

uint32_t gp_u32_to_lower(uint32_t);
void gp_str_to_lower_full(
    GPString* str,
    const char* locale_code)
{
    char code_buf[4] = "";
    if (locale_code == NULL)
        locale_code = strncpy(code_buf, setlocale(LC_ALL, NULL), 2);

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
            if (gp_is_greek_final(lookbehind, lookahead, (char*)*str + i + codepoint_length))
                GP_u32_APPEND(0x03C2); // GREEK SMALL LETTER FINAL SIGMA
            else
                GP_u32_APPEND(0x03C3); // GREEK SMALL LETTER SIGMA
            continue;
        }

        if (strncmp(locale_code, "lt", 2) == 0) switch (encoding)
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
            if (strncmp(locale_code, "tr", 2) == 0 || strncmp(locale_code, "az", 2) == 0)
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
            if (strncmp(locale_code, "tr", 2) == 0 || strncmp(locale_code, "az", 2) == 0)
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
    const char* locale_code)
{
    char code_buf[4] = "";
    if (locale_code == NULL)
        locale_code = strncpy(code_buf, setlocale(LC_ALL, NULL), 2);

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
        strncmp(locale_code, "lt", 2) == 0 &&
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
            if (strncmp(locale_code, "tr", 2) == 0 || strncmp(locale_code, "az", 2) == 0)
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
    GPArray(wchar_t)* wcs, const void*_str, const size_t str_length, const char* locale_code)
{
    char code_buf[4] = "";
    if (locale_code == NULL)
        locale_code = strncpy(code_buf, setlocale(LC_ALL, NULL), 2);

    const uint8_t* str = _str;
    ((GPArrayHeader*)*wcs - 1)->length = 0;
    size_t wcs_capacity = str_length + sizeof"";
    *wcs = gp_arr_reserve(sizeof(*wcs)[0], *wcs, wcs_capacity);
    const bool turkish = strncmp(locale_code, "tr", 2) == 0 || strncmp(locale_code, "az", 2) == 0;

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
    const GPString s1 = *(GPString*)_s1;
    const GPString s2 = *(GPString*)_s2;
    const size_t min_length = gp_min(gp_str_length(s1), gp_str_length(s2));
    for (size_t i = 0, codepoint_length; i < min_length; i += codepoint_length)
    {
        uint32_t cp1, cp2;
        codepoint_length = gp_utf8_encode(&cp1, s1, i); gp_utf8_encode(&cp2, s2, i);
        if (cp1 != cp2)
            return cp1 - cp2;
    }
    return gp_str_length(s1) - gp_str_length(s2);
}

static int gp_utf8_codepoint_compare_reverse(const void*_s1, const void*_s2)
{
    return gp_utf8_codepoint_compare(_s2, _s1);
}

typedef struct gp_narrow_wide
{
    GPString         narrow;
    GPArray(wchar_t) wide;
    GPLocale         locale;
} GPNarrowWide;

static int gp_wcs_compare(const void*_s1, const void*_s2)
{
    const GPNarrowWide* s1 = _s1;
    const GPNarrowWide* s2 = _s2;
    return wcscmp(s1->wide, s2->wide);
}

static int gp_wcs_compare_reverse(const void*_s1, const void*_s2)
{
    const GPNarrowWide* s1 = _s1;
    const GPNarrowWide* s2 = _s2;
    return wcscmp(s2->wide, s1->wide);
}

static int gp_wcs_collate(const void*_s1, const void*_s2)
{
    const GPNarrowWide* s1 = _s1;
    const GPNarrowWide* s2 = _s2;
    if (s1->locale == (GPLocale)0)
        return wcscoll(s1->wide, s2->wide);
    #if _WIN32
    return _wcscoll_l(s1->wide, s2->wide, s1->locale);
    #elif GP_LOCALE_AVAILABLE
    return wcscoll_l(s1->wide, s2->wide, s1->locale);
    #else
    return wcscoll(s1->wide, s2->wide);
    #endif
}

static int gp_wcs_collate_reverse(const void* s1, const void* s2)
{
    return gp_wcs_collate(s2, s1);
}

void gp_str_sort(
    GPArray(GPString)* strs,
    const int flags,
    const char* locale_code)
{
    const bool fold    = flags & 0x4;
    const bool collate = flags & 0x1;
    const bool reverse = flags & 0x10;
    if ( ! (fold | collate)) {
        qsort(*strs, gp_arr_length(*strs), sizeof(GPString),
            !reverse ? gp_utf8_codepoint_compare : gp_utf8_codepoint_compare_reverse);
        return;
    }
    GPArena* scratch = gp_scratch_arena();
    GPNarrowWide* pairs = gp_mem_alloc((GPAllocator*)scratch, sizeof pairs[0] * gp_arr_length(*strs));

    for (size_t i = 0; i < gp_arr_length(*strs); ++i) {
        pairs[i].narrow = (*strs)[i];
        pairs[i].locale = gp_locale(locale_code);
        pairs[i].wide = gp_arr_new((GPAllocator*)scratch, sizeof pairs[i].wide[0], gp_str_length((*strs)[i]));
        if (fold)
            gp_wcs_fold_utf8(&pairs[i].wide, (*strs)[i], gp_str_length((*strs)[i]), locale_code);
        else
            gp_utf8_to_wcs_unsafe(&pairs[i].wide, (*strs)[i], gp_str_length((*strs)[i]));
    }

    if (collate)
        qsort(pairs, gp_arr_length(*strs), sizeof pairs[0], !reverse ? gp_wcs_collate : gp_wcs_collate_reverse);
    else
        qsort(pairs, gp_arr_length(*strs), sizeof pairs[0], !reverse ? gp_wcs_compare : gp_wcs_compare_reverse);

    for (size_t i = 0; i < gp_arr_length(*strs); ++i)
        (*strs)[i] = pairs[i].narrow;

    gp_arena_rewind(scratch, pairs);
}

int gp_str_compare(
    const GPString s1,
    const void*const s2,
    const size_t s2_length,
    const int flags,
    const char* locale_code)
{
    const bool fold    = flags & 0x4;
    const bool collate = flags & 0x1;
    const bool reverse = flags & 0x10;
    if ( ! (fold || collate))
    {
        const size_t min_length = gp_str_length(s1) < s2_length ? gp_str_length(s1) : s2_length;
        for (size_t i = 0, codepoint_length; i < min_length; i += codepoint_length)
        {
            uint32_t cp1, cp2;
            codepoint_length = gp_utf8_encode(&cp1, s1, i); gp_utf8_encode(&cp2, s2, i);
            if (cp1 != cp2)
                return !reverse ? cp1 - cp2 : cp2 - cp1;
        }
        return !reverse ? gp_str_length(s1) - s2_length : s2_length - gp_str_length(s1);
    }

    GPArena* scratch = gp_scratch_arena();
    GPArray(wchar_t) wcs1 = gp_arr_new(
        (GPAllocator*)scratch,
        sizeof wcs1[0],
        gp_bytes_codepoint_count(s1, gp_str_length(s1) + sizeof""));
    GPArray(wchar_t) wcs2 = gp_arr_new(
        (GPAllocator*)scratch,
        sizeof wcs2[0],
        gp_bytes_codepoint_count(s2, s2_length + sizeof""));

    if (fold) {
        gp_wcs_fold_utf8(&wcs1, s1, gp_str_length(s1), locale_code);
        gp_wcs_fold_utf8(&wcs2, s2, s2_length,         locale_code);
    } else {
        gp_utf8_to_wcs_unsafe(&wcs1, s1, gp_str_length(s1));
        gp_utf8_to_wcs_unsafe(&wcs2, s2, s2_length);
    }

    int result;
    if (collate) {
        if (locale_code == NULL)
            result = wcscoll(wcs1, wcs2);
        else
            #if _WIN32
            result = _wcscoll_l(wcs1, wcs2, gp_locale(locale_code));
            #elif GP_LOCALE_AVAILABLE
            result = wcscoll_l(wcs1, wcs2, gp_locale(locale_code));
            #else
            result = wcscoll(wcs1, wcs2);
            #endif
    } else {
        result = wcscmp(wcs1, wcs2);
    }

    gp_arena_rewind(scratch, gp_arr_allocation(wcs1));
    return !reverse ? result : -result;
}

// ----------------------------------------------------------------------------
// to_upper(), to_lower(), to_title()
//
// Turns out that Windows, despite Microshits claims, is not very Unicode
// capable. `towupper()` and relevants fail with a large set of codepoints even
// with /utf-8, /D_UNICODE, and any locale settings. So here we are, reinventing
// the wheel, once again, although I really don't feel like doing that, so I'll
// just rip off some Newlib source code. I extended it to handle Unicode 15.1.0.

// https://chromium.googlesource.com/native_client/nacl-newlib/+/refs/heads/main/newlib/libc/ctype

/* Copyright (c) 2002 Red Hat Incorporated.
   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
     Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
     Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
     The name of Red Hat Incorporated may not be used to endorse
     or promote products derived from this software without specific
     prior written permission.
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL RED HAT INCORPORATED BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

uint32_t gp_u32_to_upper(uint32_t c)
{
    /* Based on and tested against Unicode 5.2 */
    /* Expression used to filter out the characters for the below code:
       awk -F\; '{ if ( $13 != "" ) print $1; }' UnicodeData.txt
    */
    if (c < 0x100)
      {
        if (c == 0x00b5)
  	return 0x039c;

        if ((c >= 0x00e0 && c <= 0x00fe && c != 0x00f7) ||
  	  (c >= 0x0061 && c <= 0x007a))
  	return (c - 0x20);

        if (c == 0xff)
  	return 0x0178;

        return c;
      }
    else if (c < 0x300)
      {
        if ((c >= 0x0101 && c <= 0x012f) ||
  	  (c >= 0x0133 && c <= 0x0137) ||
  	  (c >= 0x014b && c <= 0x0177) ||
  	  (c >= 0x01df && c <= 0x01ef) ||
  	  (c >= 0x01f9 && c <= 0x021f) ||
  	  (c >= 0x0223 && c <= 0x0233) ||
  	  (c >= 0x0247 && c <= 0x024f))
  	{
  	  if (c & 0x01)
  	    return (c - 1);
  	  return c;
  	}
        if ((c >= 0x013a && c <= 0x0148) ||
  	  (c >= 0x01ce && c <= 0x01dc) ||
  	  c == 0x023c || c == 0x0242)
  	{
  	  if (!(c & 0x01))
  	    return (c - 1);
  	  return c;
  	}

        if (c == 0x0131)
  	return 0x0049;

        if (c == 0x017a || c == 0x017c || c == 0x017e)
  	return (c - 1);

        if (c >= 0x017f && c <= 0x0292)
  	{
  	  wint_t k;
  	  switch (c)
  	    {
  	    case 0x017f:
  	      k = 0x0053;
  	      break;
  	    case 0x0180:
  	      k = 0x0243;
  	      break;
  	    case 0x0183:
  	      k = 0x0182;
  	      break;
  	    case 0x0185:
  	      k = 0x0184;
  	      break;
  	    case 0x0188:
  	      k = 0x0187;
  	      break;
  	    case 0x018c:
  	      k = 0x018b;
  	      break;
  	    case 0x0192:
  	      k = 0x0191;
  	      break;
  	    case 0x0195:
  	      k = 0x01f6;
  	      break;
  	    case 0x0199:
  	      k = 0x0198;
  	      break;
  	    case 0x019a:
  	      k = 0x023d;
  	      break;
  	    case 0x019e:
  	      k = 0x0220;
  	      break;
  	    case 0x01a1:
  	    case 0x01a3:
  	    case 0x01a5:
  	    case 0x01a8:
  	    case 0x01ad:
  	    case 0x01b0:
  	    case 0x01b4:
  	    case 0x01b6:
  	    case 0x01b9:
  	    case 0x01bd:
  	    case 0x01c5:
  	    case 0x01c8:
  	    case 0x01cb:
  	    case 0x01f2:
  	    case 0x01f5:
  	      k = c - 1;
  	      break;
  	    case 0x01bf:
  	      k = 0x01f7;
  	      break;
  	    case 0x01c6:
  	    case 0x01c9:
  	    case 0x01cc:
  	      k = c - 2;
  	      break;
  	    case 0x01dd:
  	      k = 0x018e;
  	      break;
  	    case 0x01f3:
  	      k = 0x01f1;
  	      break;
  	    case 0x023f:
  	      k = 0x2c7e;
  	      break;
  	    case 0x0240:
  	      k = 0x2c7f;
  	      break;
  	    case 0x0250:
  	      k = 0x2c6f;
  	      break;
  	    case 0x0251:
  	      k = 0x2c6d;
  	      break;
  	    case 0x0252:
  	      k = 0x2c70;
  	      break;
  	    case 0x0253:
  	      k = 0x0181;
  	      break;
  	    case 0x0254:
  	      k = 0x0186;
  	      break;
  	    case 0x0256:
  	      k = 0x0189;
  	      break;
  	    case 0x0257:
  	      k = 0x018a;
  	      break;
  	    case 0x0259:
  	      k = 0x018f;
  	      break;
  	    case 0x025b:
  	      k = 0x0190;
  	      break;
  	    case 0x0260:
  	      k = 0x0193;
  	      break;
  	    case 0x0263:
  	      k = 0x0194;
  	      break;
  	    case 0x0268:
  	      k = 0x0197;
  	      break;
  	    case 0x0269:
  	      k = 0x0196;
  	      break;
  	    case 0x026b:
  	      k = 0x2c62;
  	      break;
  	    case 0x026f:
  	      k = 0x019c;
  	      break;
  	    case 0x0271:
  	      k = 0x2c6e;
  	      break;
  	    case 0x0272:
  	      k = 0x019d;
  	      break;
  	    case 0x0275:
  	      k = 0x019f;
  	      break;
  	    case 0x027d:
  	      k = 0x2c64;
  	      break;
  	    case 0x0280:
  	      k = 0x01a6;
  	      break;
  	    case 0x0283:
  	      k = 0x01a9;
  	      break;
  	    case 0x0288:
  	      k = 0x01ae;
  	      break;
  	    case 0x0289:
  	      k = 0x0244;
  	      break;
  	    case 0x028a:
  	      k = 0x01b1;
  	      break;
  	    case 0x028b:
  	      k = 0x01b2;
  	      break;
  	    case 0x028c:
  	      k = 0x0245;
  	      break;
  	    case 0x0292:
  	      k = 0x01b7;
  	      break;
  	    default:
  	      k = 0;
  	    }
  	  if (k != 0)
  	    return k;
  	}
      }
    else if (c < 0x0400)
      {
        wint_t k;
        if (c >= 0x03ad && c <= 0x03af)
        	return (c - 0x25);
        if (c >= 0x03b1 && c <= 0x03cb && c != 0x03c2)
  	return (c - 0x20);

        if (c >= 0x03d9 && c <= 0x03ef && (c & 1))
  	return (c - 1);
        switch (c)
  	{
  	case 0x0345:
  	  k = 0x0399;
  	  break;
  	case 0x0371:
  	case 0x0373:
  	case 0x0377:
  	case 0x03f8:
  	case 0x03fb:
  	  k = c - 1;
  	  break;
  	case 0x037b:
  	case 0x037c:
  	case 0x037d:
  	  k = c + 0x82;
  	  break;
  	case 0x03ac:
  	  k = 0x0386;
  	  break;
  	case 0x03c2:
  	  k = 0x03a3;
  	  break;
  	case 0x03cc:
  	  k = 0x038c;
  	  break;
  	case 0x03cd:
  	case 0x03ce:
  	  k = c - 0x3f;
  	  break;
  	case 0x03d0:
  	  k = 0x0392;
  	  break;
  	case 0x03d1:
  	  k = 0x0398;
  	  break;
  	case 0x03d5:
  	  k = 0x03a6;
  	  break;
  	case 0x03d6:
  	  k = 0x03a0;
  	  break;
  	case 0x03d7:
  	  k = 0x03cf;
  	  break;
  	case 0x03f0:
  	  k = 0x039a;
  	  break;
  	case 0x03f1:
  	  k = 0x03a1;
  	  break;
  	case 0x03f2:
  	  k = 0x03f9;
  	  break;
  	case 0x03f5:
  	  k = 0x0395;
  	  break;
  	default:
  	  k = 0;
  	}
        if (k != 0)
  	return k;
      }
    else if (c < 0x500)
      {
        if (c >= 0x0430 && c <= 0x044f)
  	return (c - 0x20);

        if (c >= 0x0450 && c <= 0x045f)
  	return (c - 0x50);

        if ((c >= 0x0461 && c <= 0x0481) ||
  	  (c >= 0x048b && c <= 0x04bf) ||
  	  (c >= 0x04d1 && c <= 0x04ff))
  	{
  	  if (c & 0x01)
  	    return (c - 1);
  	  return c;
  	}

        if (c >= 0x04c2 && c <= 0x04ce)
  	{
  	  if (!(c & 0x01))
  	    return (c - 1);
  	  return c;
  	}

        if (c == 0x04cf)
        	return 0x04c0;
        if (c >= 0x04f7 && c <= 0x04f9)
  	return (c - 1);
      }
    else if (c < 0x0600)
      {
        if (c >= 0x0501 && c <= 0x0525 && (c & 1))
        	return c - 1;
        if (c >= 0x0561 && c <= 0x0586)
  	return (c - 0x30);
      }
    else if (c < 0x1f00)
      {
        if (c == 0x1d79)
        	return 0xa77d;
        if (c == 0x1d7d)
        	return 0x2c63;
        if ((c >= 0x1e01 && c <= 0x1e95) ||
  	  (c >= 0x1ea1 && c <= 0x1eff))
  	{
  	  if (c & 0x01)
  	    return (c - 1);
  	  return c;
  	}

        if (c == 0x1e9b)
  	return 0x1e60;
      }
    else if (c < 0x2000)
      {

        if ((c >= 0x1f00 && c <= 0x1f07) ||
  	  (c >= 0x1f10 && c <= 0x1f15) ||
  	  (c >= 0x1f20 && c <= 0x1f27) ||
  	  (c >= 0x1f30 && c <= 0x1f37) ||
  	  (c >= 0x1f40 && c <= 0x1f45) ||
  	  (c >= 0x1f60 && c <= 0x1f67) ||
  	  (c >= 0x1f80 && c <= 0x1f87) ||
  	  (c >= 0x1f90 && c <= 0x1f97) ||
  	  (c >= 0x1fa0 && c <= 0x1fa7))
  	return (c + 0x08);
        if (c >= 0x1f51 && c <= 0x1f57 && (c & 0x01))
  	return (c + 0x08);

        if (c >= 0x1f70 && c <= 0x1ff3)
  	{
  	  wint_t k;
  	  switch (c)
  	    {
  	    case 0x1fb0:
  	      k = 0x1fb8;
  	      break;
  	    case 0x1fb1:
  	      k = 0x1fb9;
  	      break;
  	    case 0x1f70:
  	      k = 0x1fba;
  	      break;
  	    case 0x1f71:
  	      k = 0x1fbb;
  	      break;
  	    case 0x1fb3:
  	      k = 0x1fbc;
  	      break;
  	    case 0x1fbe:
  	      k = 0x0399;
  	      break;
  	    case 0x1f72:
  	      k = 0x1fc8;
  	      break;
  	    case 0x1f73:
  	      k = 0x1fc9;
  	      break;
  	    case 0x1f74:
  	      k = 0x1fca;
  	      break;
  	    case 0x1f75:
  	      k = 0x1fcb;
  	      break;
  	    case 0x1fc3:
  	      k = 0x1fcc;
  	      break;
  	    case 0x1fd0:
  	      k = 0x1fd8;
  	      break;
  	    case 0x1fd1:
  	      k = 0x1fd9;
  	      break;
  	    case 0x1f76:
  	      k = 0x1fda;
  	      break;
  	    case 0x1f77:
  	      k = 0x1fdb;
  	      break;
  	    case 0x1fe0:
  	      k = 0x1fe8;
  	      break;
  	    case 0x1fe1:
  	      k = 0x1fe9;
  	      break;
  	    case 0x1f7a:
  	      k = 0x1fea;
  	      break;
  	    case 0x1f7b:
  	      k = 0x1feb;
  	      break;
  	    case 0x1fe5:
  	      k = 0x1fec;
  	      break;
  	    case 0x1f78:
  	      k = 0x1ff8;
  	      break;
  	    case 0x1f79:
  	      k = 0x1ff9;
  	      break;
  	    case 0x1f7c:
  	      k = 0x1ffa;
  	      break;
  	    case 0x1f7d:
  	      k = 0x1ffb;
  	      break;
  	    case 0x1ff3:
  	      k = 0x1ffc;
  	      break;
  	    default:
  	      k = 0;
  	    }
  	  if (k != 0)
  	    return k;
  	}
      }
    else if (c < 0x3000)
      {
        if (c == 0x214e)
        	return 0x2132;
        if (c == 0x2184)
        	return 0x2183;
        if (c >= 0x2170 && c <= 0x217f)
  	return (c - 0x10);

        if (c >= 0x24d0 && c <= 0x24e9)
  	return (c - 0x1a);

        if (c >= 0x2c30 && c <= 0x2c5e)
  	return (c - 0x30);
        if ((c >= 0x2c68 && c <= 0x2c6c && !(c & 1)) ||
  	  (c >= 0x2c81 && c <= 0x2ce3 &&  (c & 1)) ||
  	  c == 0x2c73 || c == 0x2c76 ||
  	  c == 0x2cec || c == 0x2cee)
        	return (c - 1);
        if (c >= 0x2c81 && c <= 0x2ce3 && (c & 1))
  	return (c - 1);
        if (c >= 0x2d00 && c <= 0x2d25)
        	return (c - 0x1c60);
        switch (c)
        	{
  	case 0x2c61:
  	  return 0x2c60;
  	case 0x2c65:
  	  return 0x023a;
  	case 0x2c66:
  	  return 0x023e;
  	}
      }
    else if (c >= 0xa000 && c < 0xb000)
      {
        if (((c >= 0xa641 && c <= 0xa65f) ||
             (c >= 0xa663 && c <= 0xa66d) ||
             (c >= 0xa681 && c <= 0xa697) ||
             (c >= 0xa723 && c <= 0xa72f) ||
             (c >= 0xa733 && c <= 0xa76f) ||
             (c >= 0xa77f && c <= 0xa787)) &&
  	  (c & 1))
  	return (c - 1);

        if (c == 0xa77a || c == 0xa77c || c == 0xa78c)
  	return (c - 1);
      }
    else
      {
        if (c >= 0xff41 && c <= 0xff5a)
  	return (c - 0x20);

        if (c >= 0x10428 && c <= 0x1044f)
  	return (c - 0x28);

        // Run the awk expression again with updated UnicodeData.txt and compare
        // the diff with UnicodeData.txt version 5.2.0 to get the most recent
        // functionality. The code below is based on Unicode 15.1.0.
        //
        // awk -F\; '{ if ( $13 != "" ) print $1; }' UnicodeData.txt

        switch (c) {
            case 0x025C: return 0xA7AB;
            case 0x0261: return 0xA7AC;
            case 0x0265: return 0xA78D;
            case 0x0266: return 0xA7AA;
            case 0x026A: return 0xA7AE;
            case 0x026C: return 0xA7AD;
            case 0x0282: return 0xA7C5;
            case 0x0287: return 0xA7B1;
            case 0x029D: return 0xA7B2;
            case 0x029E: return 0xA7B0;
            case 0x03F3: return 0x037F;
            case 0x0527: return c - 1;
            case 0x0529: return c - 1;
            case 0x052B: return c - 1;
            case 0x052D: return c - 1;
            case 0x052F: return c - 1;
            case 0x1C80: return 0x0412;
            case 0x1C81: return 0x0414;
            case 0x1C82: return 0x041E;
            case 0x1C83: return 0x0421;
            case 0x1C84: return 0x0422;
            case 0x1C85: return 0x0422;
            case 0x1C86: return 0x042A;
            case 0x1C87: return 0x0462;
            case 0x1C88: return 0xA64A;
            case 0xA791: return 0xA790;
            case 0xA793: return 0xA792;
            case 0xA794: return 0xA7C4;
            case 0xA7C8: return c - 1;
            case 0xA7CA: return c - 1;
            case 0xA7D1: return c - 1;
            case 0xA7D7: return c - 1;
            case 0xA7D9: return c - 1;
            case 0xA7F6: return c - 1;
            case 0xAB53: return 0xA7B3;
            case 0x1D8E: return 0xA7C6;
            case 0x10FD: return 0x1CBD;
            case 0x10FE: return 0x1CBE;
            case 0x10FF: return 0x1CBF;
            case 0x2C5F: return 0x2C2F;
            case 0x2CF3: return 0x2cF2;
            case 0x2D27: return 0x10C7;
            case 0x2D2D: return 0x10CD;
            case 0xA661: return 0xA660;
            case 0xA699: return 0xA698;
            case 0xA69B: return 0xA69A;
        }

        if  (0x10D0  <= c && c <= 0x10FA)   return c + (0x1C90 - 0x10D0);
        if  (0x13F8  <= c && c <= 0x13FD)   return c - 8;
        if ((0xA797  <= c && c <= 0xA7A9)   ||
            (0xA7B5  <= c && c <= 0xA7C3))  return c - (c % 2);
        if  (0xAB70  <= c && c <= 0xABBF)   return c - (0xAB70 - 0x13A0);
        if ((0x10428 <= c && c <= 0x1044F)  ||
            (0x104D8 <= c && c <= 0x104FB)) return c - 0x28;
        if ((0x10597 <= c && c <= 0x105B9)  ||
            (0x105BB == c || c == 0x105BC)) return c - 0x27;
        if ((0x10CC0 <= c && c <= 0x10CF2)) return c - 0x40;
        if ((0x118C0 <= c && c <= 0x118DF)  ||
            (0x16E60 <= c && c <= 0x16E7F)) return c - 0x20;
        if ((0x1E922 <= c && c <= 0x1E943)) return c - 0x22;
      }
    return c;
}

uint32_t gp_u32_to_lower(uint32_t c)
{
    /* Based on and tested against Unicode 5.2 */
    /* Expression used to filter out the characters for the below code:
       awk -F\; '{ if ( $14 != "" ) print $1; }' UnicodeData.txt
    */
    if (c < 0x100)
      {
        if ((c >= 0x0041 && c <= 0x005a) ||
  	  (c >= 0x00c0 && c <= 0x00d6) ||
  	  (c >= 0x00d8 && c <= 0x00de))
  	return (c + 0x20);
        return c;
      }
    else if (c < 0x300)
      {
        if ((c >= 0x0100 && c <= 0x012e) ||
  	  (c >= 0x0132 && c <= 0x0136) ||
  	  (c >= 0x014a && c <= 0x0176) ||
  	  (c >= 0x01de && c <= 0x01ee) ||
  	  (c >= 0x01f8 && c <= 0x021e) ||
  	  (c >= 0x0222 && c <= 0x0232))
  	{
  	  if (!(c & 0x01))
  	    return (c + 1);
  	  return c;
  	}
        if (c == 0x0130)
  	return 0x0069;
        if ((c >= 0x0139 && c <= 0x0147) ||
  	  (c >= 0x01cd && c <= 0x01db))
  	{
  	  if (c & 0x01)
  	    return (c + 1);
  	  return c;
  	}

        if (c >= 0x178 && c <= 0x01f7)
  	{
  	  wint_t k;
  	  switch (c)
  	    {
  	    case 0x0178:
  	      k = 0x00ff;
  	      break;
  	    case 0x0179:
  	    case 0x017b:
  	    case 0x017d:
  	    case 0x0182:
  	    case 0x0184:
  	    case 0x0187:
  	    case 0x018b:
  	    case 0x0191:
  	    case 0x0198:
  	    case 0x01a0:
  	    case 0x01a2:
  	    case 0x01a4:
  	    case 0x01a7:
  	    case 0x01ac:
  	    case 0x01af:
  	    case 0x01b3:
  	    case 0x01b5:
  	    case 0x01b8:
  	    case 0x01bc:
  	    case 0x01c5:
  	    case 0x01c8:
  	    case 0x01cb:
  	    case 0x01cd:
  	    case 0x01cf:
  	    case 0x01d1:
  	    case 0x01d3:
  	    case 0x01d5:
  	    case 0x01d7:
  	    case 0x01d9:
  	    case 0x01db:
  	    case 0x01f2:
  	    case 0x01f4:
  	      k = c + 1;
  	      break;
  	    case 0x0181:
  	      k = 0x0253;
  	      break;
  	    case 0x0186:
  	      k = 0x0254;
  	      break;
  	    case 0x0189:
  	      k = 0x0256;
  	      break;
  	    case 0x018a:
  	      k = 0x0257;
  	      break;
  	    case 0x018e:
  	      k = 0x01dd;
  	      break;
  	    case 0x018f:
  	      k = 0x0259;
  	      break;
  	    case 0x0190:
  	      k = 0x025b;
  	      break;
  	    case 0x0193:
  	      k = 0x0260;
  	      break;
  	    case 0x0194:
  	      k = 0x0263;
  	      break;
  	    case 0x0196:
  	      k = 0x0269;
  	      break;
  	    case 0x0197:
  	      k = 0x0268;
  	      break;
  	    case 0x019c:
  	      k = 0x026f;
  	      break;
  	    case 0x019d:
  	      k = 0x0272;
  	      break;
  	    case 0x019f:
  	      k = 0x0275;
  	      break;
  	    case 0x01a6:
  	      k = 0x0280;
  	      break;
  	    case 0x01a9:
  	      k = 0x0283;
  	      break;
  	    case 0x01ae:
  	      k = 0x0288;
  	      break;
  	    case 0x01b1:
  	      k = 0x028a;
  	      break;
  	    case 0x01b2:
  	      k = 0x028b;
  	      break;
  	    case 0x01b7:
  	      k = 0x0292;
  	      break;
  	    case 0x01c4:
  	    case 0x01c7:
  	    case 0x01ca:
  	    case 0x01f1:
  	      k = c + 2;
  	      break;
  	    case 0x01f6:
  	      k = 0x0195;
  	      break;
  	    case 0x01f7:
  	      k = 0x01bf;
  	      break;
  	    default:
  	      k = 0;
  	    }
  	  if (k != 0)
  	    return k;
  	}
        else if (c == 0x0220)
        	return 0x019e;
        else if (c >= 0x023a && c <= 0x024e)
        	{
  	  wint_t k;
  	  switch (c)
  	    {
  	    case 0x023a:
  	      k = 0x2c65;
  	      break;
  	    case 0x023b:
  	    case 0x0241:
  	    case 0x0246:
  	    case 0x0248:
  	    case 0x024a:
  	    case 0x024c:
  	    case 0x024e:
  	      k = c + 1;
  	      break;
  	    case 0x023d:
  	      k = 0x019a;
  	      break;
  	    case 0x023e:
  	      k = 0x2c66;
  	      break;
  	    case 0x0243:
  	      k = 0x0180;
  	      break;
  	    case 0x0244:
  	      k = 0x0289;
  	      break;
  	    case 0x0245:
  	      k = 0x028c;
  	      break;
  	    default:
  	      k = 0;
  	    }
  	  if (k != 0)
  	    return k;
  	}
      }
    else if (c < 0x0400)
      {
        if (c == 0x0370 || c == 0x0372 || c == 0x0376)
        	return (c + 1);
        if (c >= 0x0391 && c <= 0x03ab && c != 0x03a2)
  	return (c + 0x20);
        if (c >= 0x03d8 && c <= 0x03ee && !(c & 0x01))
  	return (c + 1);
        if (c >= 0x0386 && c <= 0x03ff)
  	{
  	  wint_t k;
  	  switch (c)
  	    {
  	    case 0x0386:
  	      k = 0x03ac;
  	      break;
  	    case 0x0388:
  	      k = 0x03ad;
  	      break;
  	    case 0x0389:
  	      k = 0x03ae;
  	      break;
  	    case 0x038a:
  	      k = 0x03af;
  	      break;
  	    case 0x038c:
  	      k = 0x03cc;
  	      break;
  	    case 0x038e:
  	      k = 0x03cd;
  	      break;
  	    case 0x038f:
  	      k = 0x03ce;
  	      break;
  	    case 0x03cf:
  	      k = 0x03d7;
  	      break;
  	    case 0x03f4:
  	      k = 0x03b8;
  	      break;
  	    case 0x03f7:
  	      k = 0x03f8;
  	      break;
  	    case 0x03f9:
  	      k = 0x03f2;
  	      break;
  	    case 0x03fa:
  	      k = 0x03fb;
  	      break;
  	    case 0x03fd:
  	      k = 0x037b;
  	      break;
  	    case 0x03fe:
  	      k = 0x037c;
  	      break;
  	    case 0x03ff:
  	      k = 0x037d;
  	      break;
  	    default:
  	      k = 0;
  	    }
  	  if (k != 0)
  	    return k;
  	}
      }
    else if (c < 0x500)
      {
        if (c >= 0x0400 && c <= 0x040f)
  	return (c + 0x50);

        if (c >= 0x0410 && c <= 0x042f)
  	return (c + 0x20);

        if ((c >= 0x0460 && c <= 0x0480) ||
  	  (c >= 0x048a && c <= 0x04be) ||
  	  (c >= 0x04d0 && c <= 0x04fe))
  	{
  	  if (!(c & 0x01))
  	    return (c + 1);
  	  return c;
  	}

        if (c == 0x04c0)
  	return 0x04cf;
        if (c >= 0x04c1 && c <= 0x04cd)
  	{
  	  if (c & 0x01)
  	    return (c + 1);
  	  return c;
  	}
      }
    else if (c < 0x1f00)
      {
        if ((c >= 0x0500 && c <= 0x050e) ||
  	  (c >= 0x0510 && c <= 0x0524) ||
  	  (c >= 0x1e00 && c <= 0x1e94) ||
  	  (c >= 0x1ea0 && c <= 0x1ef8))
  	{
  	  if (!(c & 0x01))
  	    return (c + 1);
  	  return c;
  	}

        if (c >= 0x0531 && c <= 0x0556)
  	return (c + 0x30);
        if (c >= 0x10a0 && c <= 0x10c5)
  	return (c + 0x1c60);
        if (c == 0x1e9e)
  	return 0x00df;
        if (c >= 0x1efa && c <= 0x1efe && !(c & 0x01))
  	return (c + 1);
      }
    else if (c < 0x2000)
      {
        if ((c >= 0x1f08 && c <= 0x1f0f) ||
  	  (c >= 0x1f18 && c <= 0x1f1d) ||
  	  (c >= 0x1f28 && c <= 0x1f2f) ||
  	  (c >= 0x1f38 && c <= 0x1f3f) ||
  	  (c >= 0x1f48 && c <= 0x1f4d) ||
  	  (c >= 0x1f68 && c <= 0x1f6f) ||
  	  (c >= 0x1f88 && c <= 0x1f8f) ||
  	  (c >= 0x1f98 && c <= 0x1f9f) ||
  	  (c >= 0x1fa8 && c <= 0x1faf))
  	return (c - 0x08);
        if (c >= 0x1f59 && c <= 0x1f5f)
  	{
  	  if (c & 0x01)
  	    return (c - 0x08);
  	  return c;
  	}

        if (c >= 0x1fb8 && c <= 0x1ffc)
  	{
  	  wint_t k;
  	  switch (c)
  	    {
  	    case 0x1fb8:
  	    case 0x1fb9:
  	    case 0x1fd8:
  	    case 0x1fd9:
  	    case 0x1fe8:
  	    case 0x1fe9:
  	      k = c - 0x08;
  	      break;
  	    case 0x1fba:
  	    case 0x1fbb:
  	      k = c - 0x4a;
  	      break;
  	    case 0x1fbc:
  	      k = 0x1fb3;
  	      break;
  	    case 0x1fc8:
  	    case 0x1fc9:
  	    case 0x1fca:
  	    case 0x1fcb:
  	      k = c - 0x56;
  	      break;
  	    case 0x1fcc:
  	      k = 0x1fc3;
  	      break;
  	    case 0x1fda:
  	    case 0x1fdb:
  	      k = c - 0x64;
  	      break;
  	    case 0x1fea:
  	    case 0x1feb:
  	      k = c - 0x70;
  	      break;
  	    case 0x1fec:
  	      k = 0x1fe5;
  	      break;
  	    case 0x1ff8:
  	    case 0x1ff9:
  	      k = c - 0x80;
  	      break;
  	    case 0x1ffa:
  	    case 0x1ffb:
  	      k = c - 0x7e;
  	      break;
  	    case 0x1ffc:
  	      k = 0x1ff3;
  	      break;
  	    default:
  	      k = 0;
  	    }
  	  if (k != 0)
  	    return k;
  	}
      }
    else if (c < 0x2c00)
      {
        if (c >= 0x2160 && c <= 0x216f)
  	return (c + 0x10);
        if (c >= 0x24b6 && c <= 0x24cf)
  	return (c + 0x1a);

        switch (c)
        	{
  	case 0x2126:
  	  return 0x03c9;
  	case 0x212a:
  	  return 0x006b;
  	case 0x212b:
  	  return 0x00e5;
  	case 0x2132:
  	  return 0x214e;
  	case 0x2183:
  	  return 0x2184;
  	}
      }
    else if (c < 0x2d00)
      {
        if (c >= 0x2c00 && c <= 0x2c2e)
  	return (c + 0x30);
        if (c >= 0x2c80 && c <= 0x2ce2 && !(c & 0x01))
  	return (c + 1);
        switch (c)
        	{
  	case 0x2c60:
  	  return 0x2c61;
  	case 0x2c62:
  	  return 0x026b;
  	case 0x2c63:
  	  return 0x1d7d;
  	case 0x2c64:
  	  return 0x027d;
  	case 0x2c67:
  	case 0x2c69:
  	case 0x2c6b:
  	case 0x2c72:
  	case 0x2c75:
  	case 0x2ceb:
  	case 0x2ced:
  	  return c + 1;
  	case 0x2c6d:
  	  return 0x0251;
  	case 0x2c6e:
  	  return 0x0271;
  	case 0x2c6f:
  	  return 0x0250;
  	case 0x2c70:
  	  return 0x0252;
  	case 0x2c7e:
  	  return 0x023f;
  	case 0x2c7f:
  	  return 0x0240;
  	}
      }
    else if (c >= 0xa600 && c < 0xa800)
      {
        if ((c >= 0xa640 && c <= 0xa65e) ||
  	  (c >= 0xa662 && c <= 0xa66c) ||
  	  (c >= 0xa680 && c <= 0xa696) ||
  	  (c >= 0xa722 && c <= 0xa72e) ||
  	  (c >= 0xa732 && c <= 0xa76e) ||
  	  (c >= 0xa77f && c <= 0xa786))
  	{
  	  if (!(c & 1))
  	    return (c + 1);
  	  return c;
  	}
        switch (c)
        	{
  	case 0xa779:
  	case 0xa77b:
  	case 0xa77e:
  	case 0xa78b:
  	  return (c + 1);
  	case 0xa77d:
  	  return 0x1d79;
  	}
      }
    else
      {
        if (c >= 0xff21 && c <= 0xff3a)
  	return (c + 0x20);

        if (c >= 0x10400 && c <= 0x10427)
  	return (c + 0x28);

        // Run the awk expression again with updated UnicodeData.txt and compare
        // the diff with UnicodeData.txt version 5.2.0 to get the most recent
        // functionality. The code below is based on Unicode 15.1.0.
        //
        // awk -F\; '{ if ( $14 != "" ) print $1; }' UnicodeData.txt

        switch (c) {
            case 0x037F: return 0x03F3;
            case 0x10C7: return 0x2D27;
            case 0x10CD: return 0x2D2D;
            case 0x1CBD: return 0x10FD;
            case 0x1CBE: return 0x10FE;
            case 0x1CBF: return 0x10FF;
            case 0x2C2F: return 0x2C5F;
            case 0x2CF2: return 0x2CF3;
            case 0xA660: return 0xA661;
            case 0xA698: return 0xA699;
            case 0xA69A: return 0xA69B;
            case 0xA78D: return 0x0265;
            case 0xA790: return 0xA791;
            case 0xA792: return 0xA793;
            case 0xA7C5: return 0x0282;
            case 0xA7C6: return 0x1D8E;
            case 0xA7C7: return 0xA7C8;
            case 0xA7C9: return 0xA7CA;
            case 0xA7D0: return 0xA7D0;
            case 0xA7D6: return 0xA7D7;
            case 0xA7D8: return 0xA7D9;
            case 0xA7F5: return 0xA7F6;
            case 0xA7AA: return 0x0266;
            case 0xA7AB: return 0x025C;
            case 0xA7AC: return 0x0261;
            case 0xA7AD: return 0x026C;
            case 0xA7AE: return 0x026A;
            case 0xA7B0: return 0x029E;
            case 0xA7B1: return 0x0287;
            case 0xA7B2: return 0x029D;
            case 0xA7B3: return 0xAB53;
            case 0xA7B4: return 0xA7B5;
        }

        if ((0x0526  <= c && c <= 0x052E)   && !(c % 2)) return c + 1;
        if  (0x13A0  <= c && c <= 0x13EF)   return c + (0xAB70 - 0x13A0);
        if  (0x13F0  <= c && c <= 0x13F5)   return c + 8;
        if  (0x1C90  <= c && c <= 0x1CBA)   return c + (0x1C90 - 0x10D0);
        if ((0xA796  <= c && c <= 0xA7A8)   ||
            (0xA7B6  <= c && c <= 0xA7C4))  return c + !(c % 2);
        if ((0x10400 <= c && c <= 0x10427)  ||
            (0x104B0 <= c && c <= 0x104D3)) return c + 0x28;
        if ((0x10570 <= c && c <= 0x10592)  ||
            (0x10594 == c || c == 0x10595)) return c + 0x27;
        if ((0x10C80 <= c && c <= 0x10CB2)) return c + 0x40;
        if ((0x118A0 <= c && c <= 0x118BF)  ||
            (0x16E40 <= c && c <= 0x16E5F)) return c + 0x20;
        if ((0x1E900 <= c && c <= 0x1E921)) return c + 0x22;
      }
    return c;
}

uint32_t gp_u32_to_title(uint32_t c)
{
    // Code below is based on differences between `stc` (simple titlecase) and
    // `suc` (simple uppercase) fields in preparsed UnicodeData.txt.
    //
    // https://raw.githubusercontent.com/unicode-org/icu/main/icu4c/source/data/unidata/ppucd.txt

    if (c < 0x100)
    {
        if (c == 0x00b5)
	    return 0x039c;

        if ((c >= 0x00e0 && c <= 0x00fe && c != 0x00f7) ||
	    (c >= 0x0061 && c <= 0x007a))
	    return (c - 0x20);

        if (c == 0xff)
	    return 0x0178;

        return c;
    }
    if (0x01C4 <= c && c <= 0x01CC)
    {
        if (c < 0x01C7)
            return 0x01C5;
        else if (c < 0x01CA)
            return 0x01C8;
        else
            return 0x01CB;
    }
    if (0x01F1 <= c && c <= 0x01F3)
    {
        return 0x01F2;
    }
    if ((0x10D0 <= c && c <= 0x10FA) ||
        (0x10FD <= c && c <= 0x10FF))
    {
        return c;
    }
    return gp_u32_to_upper(c);
}
