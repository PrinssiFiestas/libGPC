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

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
//
//          API REFERENCE
//
// ----------------------------------------------------------------------------


/** Somewhat portably sets global locale to UTF-8.
 * locale_code should be in form "xx_YY", or "xxx_YY", or an empty string.
 * @return string that can be used to restore locale by passing it to
 * setlocale() with it's associated category, or NULL if arguments are invalid.
 */
GP_NONNULL_ARGS()
const char* gp_set_utf8_global_locale(int category, const char* locale_code);

/** Codepoint size in bytes.
 * Only reads one byte at the specified index. No bounds checks performed, which
 * is why this function is not suitable for iterating over codepoints in string.
 * No checks for validity of given codepoint are performed.
 * @return number of bytes a codepoint at index @p i occupies or 0 if @p i does
 * not point to the beginning of a codepoint.
 */
GP_NONNULL_ARGS() GP_NODISCARD
size_t gp_utf8_decode_codepoint_length(
    const void* str,
    size_t      i);

/** Decode UTF-8 codepoint to UTF-32 with error handling.
 * Decodes codepoint from @p utf8 at @p utf8_index and stores it to @p decoding.
 * If @p is_valid is NULL, then @p utf8 must point to valid UTF-8 string.
 * Otherwise if no decoding error it will be set true. If decoding error
 * occurres, false will be written to it and an invalid UTF-32 value will be
 * written to @p decoding that encodes back to the original invalid UTF-8 when
 * passed to @ref gp_utf8_encode().
 * @return amount of bytes read from @p utf8.
 */
GP_NONNULL_ARGS(1, 2)
size_t gp_utf8_decode(
    uint32_t*    decoding,
    const void*  utf8,
    size_t       utf8_length, // for bounds checking
    size_t       utf8_index,
    bool*        is_valid);

/** Encode UTF-32 codepoint to UTF-8 with error handling.
 * Writes encoded codepoint to @p encoding. The decoded codepoint will take
 * anywhere from 1 to 4 bytes, so @p encoding should be able to hold at least
 * that many bytes. The result will NOT be null-terminated. If @p is_valid is
 * NULL, then @p decoding must be a valid UTF-32 codepoint. Otherwise if no
 * encoding error it will be set true. If encoding error occurress, false will
 * be written to it and some invalid UTF-8 byte sequence is written to encoding.
 * @return encoded UTF-8 codepoint length in bytes.
 */
GP_NONNULL_ARGS(1)
size_t gp_utf8_encode(
    void*    encoding,
    uint32_t decoding,
    bool*    is_valid);

/** Fast decode UTF-8 codepoint to UTF-32.
 * Decodes codepoint from @p utf8 at @p utf8_index and stores it to @p decoding.
 * @p utf8 must point to valid UTF-8 string.
 * @return amount of bytes read from @p utf8.
 */
GP_NONNULL_ARGS()
size_t gp_utf8_decode_unsafe(
    uint32_t*    decoding,
    const void*  utf8,
    size_t       utf8_index);

/** Fast encode UTF-32 codepoint to UTF-8.
 * Writes encoded codepoint to @p encoding. The decoded codepoint will take
 * anywhere from 1 to 4 bytes, so @p encoding should be able to hold at least
 * that many bytes. The result will NOT be null-terminated and @p decoding must
 * be valid UTF-32.
 * @return encoded UTF-8 codepoint length in bytes.
 */
GP_NONNULL_ARGS()
size_t gp_utf8_encode_unsafe(
    void*    encoding,
    uint32_t decoding);

// ----------------------------------------------------------------------------
// Validation

/** Validate UTF-8 codepoint.
 * If @p optional_out_codepoint_length is not NULL, the number of bytes read
 * from string will be stored in it. If the codepoint is valid, then the stored
 * length will be the lenght of the codepoint. If the codepoint is invalid, then
 * the length will be some segment length that allows using this function for
 * well defined iteration.
 */
GP_NONNULL_ARGS(1) GP_NODISCARD
bool gp_utf8_is_valid_codepoint(
    const void* str,
    size_t      str_length,
    size_t      i,
    size_t*     optional_out_codepoint_length);

/** Validate UTF-8 string.*/
GP_NONNULL_ARGS(1) GP_NODISCARD
static inline bool gp_utf8_is_valid(
    const void* str,
    size_t      str_length,
    size_t*     optional_invalid_position)
{
    for (size_t cp_length, i = 0; i < str_length; i += cp_length) {
        if ( ! gp_utf8_is_valid_codepoint(str, str_length, i, &cp_length)) {
            if (optional_invalid_position != NULL)
                *optional_invalid_position = i;
            return false;
        }
    }
    return true;
}

/** Validate UTF-16 codepoint.
 * @return 0 if invalid, 2 if valid surrogate, 1 otherwise.
 */
GP_NONNULL_ARGS() GP_NODISCARD
static inline size_t gp_utf16_is_valid_codepoint(
    const uint16_t* str,
    size_t          str_length,
    size_t          i)
{
    gp_db_assert(i < str_length, "Index out of bounds.");
    if (0xDC00 <= str[i] && str[i] <= 0xDFFF) // trailing surrogate
        return false;
    if (0xD800 <= str[i] && str[i] <= 0xDBFF) { // leading surrogate
        if (i == str_length)
            return false;
        if (0xDC00 <= str[i + 1] && str[i + 1] <= 0xDFFF)
            return 2;
    }
    return true;
}

/** Validate UTF-16 string.*/
GP_NONNULL_ARGS(1) GP_NODISCARD
static inline bool gp_utf16_is_valid(
    const uint16_t* str,
    size_t          str_length,
    size_t*         optional_invalid_position)
{
    for (size_t cp_length, i = 0; i < str_length; i += cp_length) {
        cp_length = gp_utf16_is_valid_codepoint(str, str_length, i);
        if ( ! cp_length) {
            if (optional_invalid_position != NULL)
                *optional_invalid_position = i;
            return false;
        }
    }
    return true;
}

/** Validate UTF-32 codepoint.
 */
GP_NONNULL_ARGS() GP_NODISCARD
static inline bool gp_utf32_is_valid_codepoint(
    const uint32_t* str,
    size_t          str_length,
    size_t          i)
{
    gp_db_assert(i < str_length, "Index out of bounds.");
    if (0xD800 <= str[i] && str[i] <= 0xDFFF) // surrogates
        return false;
    return str[i] <= 0x10FFFF;
}

/** Validate UTF-32 string.*/
GP_NONNULL_ARGS(1) GP_NODISCARD
static inline bool gp_utf32_is_valid(
    const uint32_t* str,
    size_t          str_length,
    size_t*         optional_invalid_position)
{
    for (size_t i = 0; i < str_length; ++i) {
        if ( ! gp_utf32_is_valid_codepoint(str, str_length, i)) {
            if (optional_invalid_position != NULL)
                *optional_invalid_position = i;
            return false;
        }
    }
    return true;
}

// ----------------------------------------------------------------------------
// Full string encoding conversions

GP_NONNULL_ARGS()
size_t gp_utf8_to_utf32(
    GPArray(uint32_t)* out_utf32,
    const void*        utf8,
    size_t             utf8_length);

GP_NONNULL_ARGS()
size_t gp_utf32_to_utf8(
    GPString*        out_utf8,
    const uint32_t*  utf32,
    size_t           utf32_length);

GP_NONNULL_ARGS()
size_t gp_utf8_to_utf16(
    GPArray(uint16_t)* out_utf16,
    const void*        utf8,
    size_t             utf8_length);

GP_NONNULL_ARGS()
size_t gp_utf16_to_utf8(
    GPString*        out_utf8,
    const uint16_t*  utf16,
    size_t           utf16_length);

// Output will be null terminated in all cases except if output is a truncating
// array with a capacity of 0. Therefore, if user expects a valid wide string,
// the capacity of a truncating output must be larger than 0, although 0 can be
// used to probe how many wide characters would've been written if it wasn't 0.
//     A truncating array may truncate an extra wide character to ensure null
// termination.
GP_NONNULL_ARGS()
size_t gp_utf8_to_wcs(
    GPArray(wchar_t)* out_unicode_wide_string,
    const void*       utf8,
    size_t            utf8_length);

GP_NONNULL_ARGS()
size_t gp_wcs_to_utf8(
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
