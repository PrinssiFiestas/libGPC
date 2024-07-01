// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/unicode.h>
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
    strcat(full_locale_code, ".UTF-8");
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
    if (locale == default_locale.locale)
        return;
    #if _WIN32
    _free_locale(locale);
    #else
    freelocale(locale);
    #endif
}

GPLocale gp_default_locale(void)
{
    // TODO
    return (GPLocale){0};
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

GP_NONNULL_ARGS()
void gp_utf8_to_utf32(
    GPArray(uint32_t)* u32,
    const void*const   u8,
    const size_t       u8_length)
{
    ((GPArrayHeader*)*u32 - 1)->length = 0;
    size_t gp_bytes_codepoint_count(const void*, size_t);
    *u32 = gp_arr_reserve(sizeof(*u32)[0], *u32, gp_bytes_codepoint_count(u8, u8_length));
    for (size_t i = 0, codepoint_length; i < u8_length; i += codepoint_length)
    {
        uint32_t encoding;
        codepoint_length = gp_utf8_encode(&encoding, u8, i);
        (*u32)[((GPArrayHeader*)*u32 - 1)->length++] = encoding;
    }
}

void gp_utf32_to_utf8(
    GPString*        u8,
    const uint32_t*  u32,
    size_t           u32_length)
{
    size_t required_capacity = 0;
    for (size_t i = 0; i < gp_arr_length(u32); ++i)
    {
        if (u32[i] > 0x7F)
        {
            if (u32[i] < 0x0000800)
                required_capacity += 2;
            else if (u32[i] < 0x0010000)
                required_capacity += 3;
            else
                required_capacity += 4;
        }
        else
            ++required_capacity;
    }
    gp_str_reserve(u8, required_capacity);

    ((GPStringHeader*)*u8 - 1)->length = 0;
    for (size_t i = 0; i < u32_length; ++i)
    { // Manually inlined gp_utf8_decode() is faster for some reason.
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
}

GP_NONNULL_ARGS()
void gp_utf8_to_utf16(
    GPArray(uint16_t)* out_utf16,
    const void*        utf8,
    size_t             utf8_length);

GP_NONNULL_ARGS()
void gp_utf16_to_utf8(
    GPString*        out_utf8,
    const uint32_t*  utf16,
    size_t           utf16_length);

GP_NONNULL_ARGS()
void gp_utf8_to_wcs(
    GPArray(wchar_t)* out_wcs,
    const void*       utf8,
    size_t            utf8_length);

GP_NONNULL_ARGS()
void gp_wcs_to_utf8(
    GPString*       out_utf8,
    const wchar_t*  wcs,
    size_t          wcs_length);


// ----------------------------------------------------------------------------
// String extensions

int gp_str_case_compare(
    const GPString _s1,
    const void*const _s2,
    const size_t s2_length,
    GPLocale locale)
{
    const char* s1 = (const char*)_s1;
    const char* s2 = (const char*)_s2;

    GPArena* scratch = gp_scratch_arena();
    const size_t buf1_cap = gp_str_length(_s1) + sizeof"";
    const size_t buf2_cap = s2_length          + sizeof"";
    wchar_t* buf1 = gp_mem_alloc((GPAllocator*)scratch, buf1_cap * sizeof*buf1);
    wchar_t* buf2 = gp_mem_alloc((GPAllocator*)scratch, buf2_cap * sizeof*buf2);

    mbsrtowcs(buf1, &(const char*){s1}, buf1_cap, &(mbstate_t){0});
    mbsrtowcs(buf2, &(const char*){s2}, buf2_cap, &(mbstate_t){0});

    #if _WIN32
    int result = _wcsicoll_l(buf1, buf2, locale.locale);
    #else
    int result = wcscasecmp_l(buf1, buf2, locale.locale);
    #endif

    gp_arena_rewind(scratch, buf1);
    return result;
}

