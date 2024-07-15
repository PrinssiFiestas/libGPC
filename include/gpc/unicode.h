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

// Portably sets global locale to UTF-8.
// locale_code should be in form "xx_YY" or an empty string.
GP_NONNULL_ARGS()
void gp_set_utf8_global_locale(int category, const char* locale_code);

#if !_WIN32 && _XOPEN_SOURCE < 700 && !_GNU_SOURCE && !_DEFAULT_SOURCE

// By default locale_t is available in GNU C Library if using GCC compatible
// compiler. However, with -std=c99 feature test macros must be used to enable
// local locales. Functions that take GPLocale* as argument will work, but they
// will use global locale instead. See
// https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html

typedef void GPLocale;
#define gp_default_locale() NULL
#define gp_locale_new()     NULL
#define gp_locale_delete() ((void)0)

#else

#define GP_LOCALE_T_AVAILABLE 1

typedef const struct gp_locale
{
    #if _WIN32
    _locale_t locale;
    #else
    locale_t locale;
    #endif
    char     code[];
} GPLocale;

// Some UTF-8 locale in category LC_ALL. Passing to gp_locale_delete() is safe
// but unnecessary.
GPLocale* gp_default_locale(void) GP_NONNULL_RETURN;

// Creates an UTF-8 locale in category LC_ALL.
// locale_code should be in form "xx_YY" or an empty string.
// Returns NULL if creating locale fails. Otherwise, the return value shuold be
// passed to gp_locale_delete() to free resources.
GPLocale* gp_locale_new(const char* locale_code) GP_NONNULL_ARGS();

void gp_locale_delete(GPLocale* optional);

#endif // !_WIN32 && _XOPEN_SOURCE < 700 && !_GNU_SOURCE && !_DEFAULT_SOURCE

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
    const uint16_t*  utf16,
    size_t           utf16_length);

// Output will be null-terminated.
GP_NONNULL_ARGS()
void gp_utf8_to_wcs(
    GPArray(wchar_t)* out_unicode_wide_string,
    const void*       utf8,
    size_t            utf8_length);

GP_NONNULL_ARGS()
void gp_wcs_to_utf8(
    GPString*       out_unicode_wide_string,
    const wchar_t*  wcs,
    size_t          wcs_length);

// ----------------------------------------------------------------------------
// Strings

// Full language sensitive Unicode case mapping. Uses global locale if locale is
// NULL.
GP_NONNULL_ARGS(1)
void gp_str_to_upper_full(
    GPString*,
    GPLocale* optional);

// Full language sensitive Unicode case mapping. Uses global locale if locale is
// NULL.
GP_NONNULL_ARGS(1)
void gp_str_to_lower_full(
    GPString*,
    GPLocale* optional);

// Capitalizes the first character according to full language sensitive Unicode
// titlecase mapping. Uses global locale if locale is NULL.
GP_NONNULL_ARGS(1)
void gp_str_capitalize(
    GPString*,
    GPLocale* optional);

#define GP_CASE_FOLD 'f'
#define GP_COLLATE   'c'

// Flags: 'f' or GP_CASE_FOLD for full language sensitive but case insensitive
// comparison. 'c' or GP_COLLATE for collation. Separate flags with |. 0 will
// compare codepoints lexicographically and is the fastest. Locale affects case
// insensitive comparison and collating. Uses global locale if locale is NULL.

GP_NONNULL_ARGS(1, 2)
int gp_str_compare(
    const GPString s1,
    const void*    s2,
    size_t         s2_length,
    int            flags,
    GPLocale*      optional);

GP_NONNULL_ARGS()
GPArray(GPString) gp_str_split(
    const GPAllocator*,
    GPString    str,
    const char* separator_char_set);

GP_NONNULL_ARGS()
void gp_str_join(
    GPString*         dest,
    GPArray(GPString) srcs,
    const char*       separator);

// Flags: 'f' or GP_CASE_FOLD for full language sensitive but case insensitive
// sorting. 'c' or GP_COLLATE for collation. Separate flags with |. 0 will sort
// codepoints lexicographically and is the fastest. locale affects case
// insensitive sorting and collating. Uses global locale if locale is NULL.
GP_NONNULL_ARGS()
void gp_str_sort(
    GPArray(GPString)* strs,
    int                flags,
    GPLocale*          optional);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#endif // GP_UNICODE_INCLUDED
