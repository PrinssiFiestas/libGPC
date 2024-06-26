// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file unicode.h
 * Unicode handling and extended string functionality.
 */

#ifndef GP_UNICODE_INCLUDED
#define GP_UNICODE_INCLUDED

#include <gpc/string.h>
#include <gpc/array.h>
#include <locale.h>

// ----------------------------------------------------------------------------
// Locales

#if _WIN32
typedef _locale_t locale_t;
#endif
typedef struct gp_locale
{
    locale_t locale;
    char     code[8]; // in form "xx_YY"
} GPLocale;

// Some UTF-8 locale in category LC_ALL. No need to call gp_locale_delete().
GPLocale gp_default_locale(void);

// Creates an UTF-8 locale in category LC_ALL. The .locale field should be freed
// with gp_locale_delete().
// locale_code should be in form "xx_YY" or an empty string.
GPLocale gp_locale_new(const char* locale_code) GP_NONNULL_ARGS();
void     gp_locale_delete(locale_t);

// ----------------------------------------------------------------------------
// Unicode

// Only reads the first byte at str + i
GP_NONNULL_ARGS()
size_t gp_utf8_codepoint_length(
    const void* str,
    size_t      i);

// Never reads past buffer if u8 points to a valid UTF-8 string. Stores encoded
// codepoint to encoding and returns number of bytes read from utf8.
GP_NONNULL_ARGS()
size_t gp_utf8_encode(
    uint32_t*    encoding,
    const void*  utf8,
    size_t       utf8_index);

// Writes decoded codepoint to decoding and returns bytes written which is less
// than or equal to 4.
GP_NONNULL_ARGS()
size_t gp_utf8_decode(
    void*    decoding,
    uint32_t encoding);

GP_NONNULL_ARGS()
void gp_utf8_to_utf32(
    GPArray(uint32_t)* out_utf32,
    const void*        utf8,
    size_t             utf8_length);

GP_NONNULL_ARGS()
void gp_utf32_to_utf8(
    GPString*        out_utf8,
    const uint32_t*  utf32,
    size_t           utf32_length);

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
// Strings

// Full language sensitive Unicode case mapping.
GP_NONNULL_ARGS(1)
void gp_str_to_upper_full(
    GPString*,
    GPLocale);

// Full language sensitive Unicode case mapping.
GP_NONNULL_ARGS(1)
void gp_str_to_lower_full(
    GPString*,
    GPLocale);

// Full language sensitive Unicode case mapping.
GP_NONNULL_ARGS(1)
void gp_str_to_title_full(
    GPString*,
    GPLocale);

// Full language sensitive Unicode case folding and collation.
GP_NONNULL_ARGS(1)
int gp_str_case_compare(
    const GPString s1,
    const void*    s2,
    size_t         s2_length,
    GPLocale       locale);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#endif // GP_UNICODE_INCLUDED
