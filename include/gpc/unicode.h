// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

/**@file unicode.h
 * Unicode handling and extended string functionality
 */

#ifndef GP_UNICODE_INCLUDED
#define GP_UNICODE_INCLUDED 1

#include <gpc/string.h>
#include <gpc/array.h>
#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// Locales

/** Portably sets global locale to UTF-8.
 *locale_code should be in form "xx_YY", or "xxx_YY", or an empty string.
 * @return string that can be used to restore locale by passing it to
 * setlocale() with it's associated category, or NULL if arguments are invalid.
 */
GP_NONNULL_ARGS()
const char* gp_set_utf8_global_locale(int category, const char* locale_code);

// By default locale_t is available in GNU C Library if using GCC compatible
// compiler. However, with -std=c99 feature test macros must be used to enable
// local locales. Functions that take GPLocale* as argument will work, but they
// will use global locale instead. See
// https://www.gnu.org/software/libc/manual/html_node/Feature-Test-Macros.html
#if _WIN32 || _XOPEN_SOURCE >= 700 || defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE)

#define GP_HAS_LOCALE 1

#if _WIN32
typedef _locale_t GPLocale;
#else
typedef locale_t GPLocale;
#endif

/** Create or fetch locale.
 * Creates or fetches already created locale which can be used with _xxx_l()
 * family of functions in Microsoft UCRT library or with xxx_l() family of
 * functions in the GNU C Library. libGPC uses this internally when collating in
 * gp_str_compare() and gp_str_sort().
 *     locale_code should be in form "xx_YY", or "xxx_YY", or an empty string.
 * The created locale will be UTF-8 in category LC_ALL.
 *     Creating a locale is extremely expensive: glibc allocates over 200 times
 * internally. However, once created, they take very little space and there only
 * exists a limited set of locale codes. Due to these considerations, adding
 * thread safety and performance, libGPC does not provide a way of freeing the
 * created locales and you should NOT use native cleanup routines either. Any
 * subsequent calls with same locale_code will return a already created locale
 * without mutex locks.
 */
GP_NODISCARD
GPLocale gp_locale(const char* optional_locale_code);

#else // only global locale available

typedef void* GPLocale;
#define gp_locale(...) NULL

#endif // _WIN32 || _XOPEN_SOURCE >= 700 || defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE)

// ----------------------------------------------------------------------------
// Unicode

// TODO UNICODE VALIDATION FOR SINGLE CHARACTER

/** Codepoint size in bytes.
 * Only reads one byte at the specified index.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_utf8_codepoint_length(
    const void* str,
    size_t      i);

/** Encode UTF-8 codepoint to UTF-32.
 * Encodes codepoint from @p utf8 at @p utf8_index and stores it to @p encoding.
 * Never reads past buffer if @p utf8 points to a valid UTF-8 string.
 * @return amount of bytes read from @p utf8.
 */
GP_NONNULL_ARGS()
size_t gp_utf8_encode(
    uint32_t*    encoding,
    const void*  utf8,
    size_t       utf8_index);

/** Decode UTF-32 codepoint to UTF-8.
 * Writes decoded codepoint to @p decoding. The decoded codepoint will take
 * anywhere from 1 to 4 bytes, so @p decoding should be able to hold at least
 * that many bytes. The result will NOT be null-terminated.
 * @return decoded UTF-8 codepoint length in bytes.
 */
GP_NONNULL_ARGS()
size_t gp_utf8_decode(
    void*    decoding,
    uint32_t encoding);

// ----------------------------------------------------------------------------
// Full string encoding conversions

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

/** Full language sensitive Unicode case mapping.
 * Uses global locale if @p locale_code is NULL.
 */
GP_NONNULL_ARGS(1)
void gp_str_to_upper_full(
    GPString*,
    const char* optional_locale_code);

/** Full language sensitive Unicode case mapping.
 * Uses global locale if @p locale_code is NULL.
 */
GP_NONNULL_ARGS(1)
void gp_str_to_lower_full(
    GPString*,
    const char* optional_locale_code);

/** Capitalizes first character.
 * Capitalizes according to full language sensitive Unicode titlecase mapping.
 * Uses global locale if @p locale_code is NULL.
 */
GP_NONNULL_ARGS(1)
void gp_str_capitalize(
    GPString*,
    const char* optional_locale_code);

#define GP_CASE_FOLD 'f'
#define GP_COLLATE   'c'
#define GP_REVERSE   'r'

/** Advanced string comparison.
 * Flags: 'f' or GP_CASE_FOLD for full language sensitive but case insensitive
 * comparison. 'c' or GP_COLLATE for collation. 'r' or GP_REVERSE to invert
 * result. Combine flags with |. 0 will compare codepoints lexicographically
 * and is the fastest. Locale affects case insensitive comparison and collating.
 * Uses global locale if @p locale_code is NULL.
 */
GP_NONNULL_ARGS(1, 2) GP_NODISCARD
int gp_str_compare(
    const GPString s1,
    const void*    s2,
    size_t         s2_length,
    int            flags,
    const char*    optional_locale_code);

/** Create array of substrings.*/
GP_NONNULL_ARGS() GP_NODISCARD
GPArray(GPString) gp_str_split(
    GPAllocator*,
    const void* str,
    size_t      str_length,
    const char* utf8_separator_char_set);

/** Merge array of strings.*/
GP_NONNULL_ARGS()
void gp_str_join(
    GPString*         dest,
    GPArray(GPString) srcs,
    const char*       separator);

/** Advanced string sorting.
 * Flags: 'f' or GP_CASE_FOLD for full language sensitive but case insensitive
 * sorting. 'c' or GP_COLLATE for collation. 'r' or GP_REVERSE to reverse the
 * result order. Combine flags with |. 0 will sort codepoints lexicographically
 * and is the fastest. locale affects case insensitive sorting and collating.
 * Uses global locale if @p locale_code is NULL.
 */
GP_NONNULL_ARGS()
void gp_str_sort(
    GPArray(GPString)* strs,
    int                flags,
    const char*        optional_locale_code);


// ----------------------------------------------------------------------------
//
//          END OF API REFERENCE
//
//          Code below is for internal usage and may change without notice.
//
// ----------------------------------------------------------------------------


#ifdef __cplusplus
} // extern "C"
#endif

#endif // GP_UNICODE_INCLUDED
