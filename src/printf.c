// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/printf.h>
#include <printf/format_scanning.h>
#include <printf/conversions.h>
#include "pfstring.h"

#include <gpc/string.h>
#include <gpc/unicode.h>
#include "common.h"

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>

typedef struct pf_misc_data
{
    bool has_sign;
    bool has_0x;
    bool is_nan_or_inf;
} PFMiscData;

typedef struct pf_uint
{
    union {
        unsigned long long ll;
        GPUInt128 _128;
    } u;
    bool is_128;
} PFUInt;

static PFUInt pf_get_uint(
    pf_va_list* args, const PFFormatSpecifier fmt)
{
    // IMPORTANT!
    // All potentially promoted args must be casted back to the original type to
    // truncate potential sign extension caused by default argument promotion.

    if (fmt.conversion_format == 'p')
        return (PFUInt){.u.ll = (uintptr_t)va_arg(args->list, gp_promoted_arg_uintptr_t)};

    switch (fmt.length_modifier)
    {
    case 0:
        return (PFUInt){.u.ll = va_arg(args->list, unsigned)};

    case 'j':
        return (PFUInt){.u.ll = va_arg(args->list, uintmax_t)};

    case 'l' * 2:
        return (PFUInt){.u.ll = va_arg(args->list, unsigned long long)};

    case 'l':
        return (PFUInt){.u.ll = va_arg(args->list, unsigned long)};

    case 'h':
        return (PFUInt){.u.ll = (unsigned short)va_arg(args->list, gp_promoted_arg_unsigned_short_t)};

    case 'h' * 2:
        return (PFUInt){.u.ll = (unsigned char)va_arg(args->list, gp_promoted_arg_unsigned_char_t)};

    case 'z':
        return (PFUInt){.u.ll = (size_t)va_arg(args->list, gp_promoted_arg_size_t)};

    case 'B': // byte
        return (PFUInt){.u.ll = (uint8_t)va_arg(args->list, gp_promoted_arg_uint8_t)};

    case 'W': // word
        return (PFUInt){.u.ll = (uint16_t)va_arg(args->list, gp_promoted_arg_uint16_t)};

    case 'D': // double word
        return (PFUInt){.u.ll = (uint32_t)va_arg(args->list, gp_promoted_arg_uint32_t)};

    case 'Q': // quad word
        return (PFUInt){.u.ll = (uint64_t)va_arg(args->list, gp_promoted_arg_uint64_t)};

    case 'O': // octa word
        return (PFUInt){.u._128 = va_arg(args->list, GPUInt128), .is_128 = true};

    case 'B'+'f': // fast byte
        return (PFUInt){.u.ll = (uint_fast8_t)va_arg(args->list, gp_promoted_arg_uint_fast8_t)};

    case 'W'+'f': // fast word
        return (PFUInt){.u.ll = (uint_fast16_t)va_arg(args->list, gp_promoted_arg_uint_fast16_t)};

    case 'D'+'f': // fast double word
        return (PFUInt){.u.ll = (uint_fast32_t)va_arg(args->list, gp_promoted_arg_uint_fast32_t)};

    case 'Q'+'f': // fast quad word
        return (PFUInt){.u.ll = (uint_fast64_t)va_arg(args->list, gp_promoted_arg_uint_fast64_t)};

    // fast octa word does not exist
    }
    GP_UNREACHABLE("");
    return (PFUInt){0};
}

static size_t pf_write_wc(
    PFString* out,
    pf_va_list* args)
{
    // At the time of wrtiting, pf_printf() doesn't check for errors. Also
    // invalid UTF16/UTF32 is relatively rare, so unsafe is somewhat ok.
    // Probably should fix some day...
    size_t gp_utf8_encode_unsafe(void*, uint32_t);
    char encoding[4];
    const size_t length = gp_utf8_encode_unsafe(
        encoding, (wint_t)va_arg(args->list, gp_promoted_arg_wint_t));
    pf_concat(out, encoding, length);
    return length;
}

static void pf_c_string_padding(
    PFString* out,
    const PFFormatSpecifier fmt,
    const void* string,
    const size_t length)
{
    const size_t field_width = fmt.field.width > length ?
        fmt.field.width : length;
    const size_t diff = field_width - length;
    if (fmt.flag.dash) // left justified
    { // first string, then pad
        pf_concat(out, string, length);
        pf_pad(out, ' ', diff);
    }
    else // first pad, then string
    {
        pf_pad(out, ' ', diff);
        pf_concat(out, string, length);
    }

}

static size_t pf_write_s(
    PFString* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const char* cstr = va_arg(args->list, char*);

    size_t cstr_len = 0;
    if (fmt.precision.option == PF_NONE) // should be null-terminated
        cstr_len = strlen(cstr);
    else // who knows if null-terminated
        while (cstr_len < fmt.precision.width && cstr[cstr_len] != '\0')
            ++cstr_len;

    pf_c_string_padding(out, fmt, cstr, cstr_len);
    return out->length - original_length;
}

static void pf_utf8_string_padding(
    PFString* out,
    const PFFormatSpecifier fmt,
    const void* bytes,
    const size_t bytes_length,
    const size_t codepoint_count)
{
    const size_t field_width = fmt.field.width > codepoint_count ?
        fmt.field.width : codepoint_count;
    const size_t diff = field_width - codepoint_count;
    if (fmt.flag.dash) // left justified
    { // first string, then pad
        pf_concat(out, bytes, bytes_length);
        pf_pad(out, ' ', diff);
    }
    else // first pad, then string
    {
        pf_pad(out, ' ', diff);
        pf_concat(out, bytes, bytes_length);
    }

}

static size_t pf_write_S(
    PFString* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const GPString str = va_arg(args->list, GPString);

    size_t length = gp_str_length(str);
    if (fmt.precision.option != PF_NONE)
        length = pf_min(length, fmt.precision.width);

    size_t codepoint_count = 0;
    size_t last_cp_length  = 0;
    size_t i = 0;
    while (true)
    {
        if (i > length) {
            --codepoint_count;
            length = i - last_cp_length;
            break;
        } else if (i == length) {
            break;
        }
        ++codepoint_count;
        i += last_cp_length = gp_utf8_decode_codepoint_length(str, i);
    }
    pf_utf8_string_padding(out, fmt, str, length, codepoint_count);
    return out->length - original_length;
}

static void pf_write_leading_zeroes(
    PFString* out,
    const size_t written_by_utoa,
    const PFFormatSpecifier fmt)
{
    if (fmt.precision.option != PF_NONE)
    {
        const size_t diff =
            fmt.precision.width <= written_by_utoa ? 0 :
            fmt.precision.width - written_by_utoa;
        memmove(
            out->data + out->length + diff,
            out->data + out->length,
            pf_limit(*out, written_by_utoa));
        memset(out->data + out->length, '0', pf_limit(*out, diff));
        out->length += written_by_utoa + diff;
    } else
        out->length += written_by_utoa;
}

static size_t pf_write_i(
    PFString* out,
    PFMiscData* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    // IMPORTANT!
    // All potentially promoted args must be casted back to the original type to
    // truncate potential sign extension caused by default argument promotion.

    long long i;
    GPInt128 i128;
    bool is_128 = false;

    switch (fmt.length_modifier)
    {
    case 0:
        i = va_arg(args->list, int);
        break;

    case 'j':
        i = va_arg(args->list, intmax_t);
        break;

    case 'l' * 2:
        i = va_arg(args->list, long long);
        break;

    case 'l':
        i = va_arg(args->list, long);
        break;

    case 'h':
        i = (short)va_arg(args->list, gp_promoted_arg_short_t);
        break;

    case 'h' * 2: // signed char is NOT char!
        i = (signed char)va_arg(args->list, gp_promoted_arg_signed_char_t);
        break;

    // We currently support ssize_t only because GNUC type checker accept this,
    // which may affect user expectations. Do NOT document this until proven
    // useful.
    #ifdef SSIZE_MAX
    case 'z':
        i = (ssize_t)va_arg(args->list, gp_promoted_arg_ssize_t);
        break;
    #else // we DON'T want to try to guess, even the assumption that it's
          // the same size as size_t doesn't always hold. Maybe we could
          // provide gp_ssize_t instead? But only if it turns out to be useful.
    case 'z':
        gp_assert(false,
            "%zi for ssize_t not supported. Cast to intmax_t and use %ji instead.");
        break;
    #endif

    case 't':
        i = (ptrdiff_t)va_arg(args->list, gp_promoted_arg_ptrdiff_t);
        break;

    case 'B': // byte
        i = (int8_t)va_arg(args->list, gp_promoted_arg_int8_t);
        break;

    case 'W': // word
        i = (int16_t)va_arg(args->list, gp_promoted_arg_int16_t);
        break;

    case 'D': // double word
        i = (int32_t)va_arg(args->list, gp_promoted_arg_int32_t);
        break;

    case 'Q': // quad word
        i = (int64_t)va_arg(args->list, gp_promoted_arg_int64_t);
        break;

    case 'O': // octa word
        i128 = va_arg(args->list, GPInt128);
        is_128 = true;
        break;

    case 'B'+'f': // fast byte
        i = (int_fast8_t)va_arg(args->list, gp_promoted_arg_int_fast8_t);
        break;

    case 'W'+'f': // fast word
        i = (int_fast16_t)va_arg(args->list, gp_promoted_arg_int_fast16_t);
        break;

    case 'D'+'f': // fast double word
        i = (int_fast32_t)va_arg(args->list, gp_promoted_arg_int_fast32_t);
        break;

    case 'Q'+'f': // fast quad word
        i = (int_fast64_t)va_arg(args->list, gp_promoted_arg_int_fast64_t);
        break;

    // fast octa word does not exist

    default:
        GP_UNREACHABLE("");
    }

    const size_t original_length = out->length;

    bool is_negative = is_128 ? gp_int128_hi(i128) < 0 : i < 0;
    const char sign = is_negative ? '-' : fmt.flag.plus ? '+' : fmt.flag.space ? ' ' : 0;
    if (sign)
    {
        pf_push_char(out, sign);
        md->has_sign = true;
    }
    const size_t max_written = is_128 ?
        pf_u128toa(
            pf_capacity_left(*out),
            out->data + out->length,
            gp_uint128_negate(gp_uint128_i128(i128)))
      : pf_utoa(
            pf_capacity_left(*out), out->data + out->length, llabs(i));

    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static size_t pf_write_o(
    PFString* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const PFUInt u = pf_get_uint(args, fmt);

    bool zero_written = false;
    if (fmt.flag.hash && gp_u128_not_equal(u.u._128, gp_uint128(0, 0)))
    {
        pf_push_char(out, '0');
        zero_written = true;
    }

    const size_t max_written = u.is_128 ?
        pf_o128toa(
            pf_capacity_left(*out), out->data + out->length, u.u._128)
      : pf_otoa(
            pf_capacity_left(*out), out->data + out->length, u.u.ll);

    // zero_written tells pad_zeroes() to add 1 less '0'
    pf_write_leading_zeroes(out, zero_written + max_written, fmt);
    // compensate for added zero_written to write_leading_zeroes()
    out->length -= zero_written;

    return out->length - original_length;
}

static size_t pf_write_x(
    PFString* out,
    PFMiscData* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    PFUInt u = pf_get_uint(args, fmt);

    if (fmt.flag.hash && gp_uint128_not_equal(u.u._128, gp_uint128(0, 0)))
    {
        pf_concat(out, "0x", sizeof"0x"-sizeof"");
        md->has_0x = true;
    }

    const size_t max_written = u.is_128 ?
        pf_x128toa(
            pf_capacity_left(*out), out->data + out->length, u.u._128)
      : pf_xtoa(
            pf_capacity_left(*out), out->data + out->length, u.u.ll);

    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static size_t pf_write_X(
    PFString* out,
    PFMiscData* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const PFUInt u = pf_get_uint(args, fmt);

    if (fmt.flag.hash && gp_uint128_not_equal(u.u._128, gp_uint128(0, 0)))
    {
        pf_concat(out, "0X", sizeof"0X"-sizeof"");
        md->has_0x = true;
    }

    const size_t max_written = u.is_128 ?
        pf_X128toa(
            pf_capacity_left(*out), out->data + out->length, u.u._128)
      : pf_Xtoa(
            pf_capacity_left(*out), out->data + out->length, u.u.ll);

    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static size_t pf_write_u(
    PFString* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const PFUInt u = pf_get_uint(args, fmt);
    const size_t max_written = u.is_128 ?
        pf_u128toa(
            pf_capacity_left(*out), out->data + out->length, u.u._128)
      : pf_utoa(
            pf_capacity_left(*out), out->data + out->length, u.u.ll);
    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static size_t pf_write_p(
    PFString* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const unsigned long long u = pf_get_uint(args, fmt).u.ll;

    if (u > 0)
    {
        pf_concat(out, "0x", sizeof"0x"-sizeof"");
        const size_t max_written = pf_xtoa(
            pf_capacity_left(*out), out->data + out->length, u);
        pf_write_leading_zeroes(out, max_written, fmt);
    }
    else
    {
        pf_concat(out, "(nil)", sizeof"(nil)"-sizeof"");
    }
    return out->length - original_length;
}

#ifdef __COMPCERT__ // math functions missing, have to implement our own.
static inline int pf_fpclassify(double x) // only nan and inf handled here
{
	union { double f; uint64_t u; } punner = {.f = x };
	uint32_t exp = (uint32_t)((punner.u & 0x7fffffffffffffffULL) >> 52);
	if (exp == 0x7ff) {
		if (punner.u & 0x000fffffffffffffULL)
			return FP_NAN;
		return FP_INFINITE;
	}
	return FP_NORMAL;
}
static inline int pf_signbit(double x)
{
    union { double f; uint64_t u; } punner = {.f = x };
    return punner.u >> 63;
}
#define pf_isnan(x) (pf_fpclassify(x) == FP_NAN)
#define pf_isinf(x) (pf_fpclassify(x) == FP_INFINITE)
#else
#define pf_signbit(x) signbit(x)
#define pf_isnan(x)   isnan(x)
#define pf_isinf(x)   isinf(x)
#endif // __COMPCERT__

static size_t pf_write_f(
    PFString* out,
    PFMiscData* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    #if GP_HAS_LONG_DOUBLE
    // We don't have Ruy implementation for long double for now, we'll just
    // truncate for now, which is still better than reading from incorrect
    // registers. TODO if we decide to use libc, keep in mind that MINGW uses
    // UCRT which assumes long double to be the same as double, but GNUC uses
    // extended precision!
    const double f = fmt.length_modifier != 'L' ?
        va_arg(args->list, double)
      : va_arg(args->list, long double);
    #else
    const double f = va_arg(args->list, gp_promoted_arg_double_t);
    #endif
    const size_t written_by_conversion = pf_strfromd(
        out->data + out->length, out->capacity, fmt, f);
    out->length += written_by_conversion;

    md->has_sign = pf_signbit(f) || fmt.flag.plus || fmt.flag.space;
    md->is_nan_or_inf = pf_isnan(f) || pf_isinf(f);

    return written_by_conversion;
}

static size_t pf_add_padding(
    PFString* out,
    const size_t written,
    const PFMiscData md,
    const PFFormatSpecifier fmt)
{
    size_t start = out->length - written;
    const size_t diff = fmt.field.width - written;

    const bool is_int_with_precision =
        strchr("diouxX", fmt.conversion_format) && fmt.precision.option != PF_NONE;
    const bool ignore_zero = is_int_with_precision || md.is_nan_or_inf;

    if (fmt.flag.dash) // left justified, append padding
    {
        pf_pad(out, ' ', diff);
    }
    else if (fmt.flag.zero && ! ignore_zero) // fill in zeroes
    { // 0-padding minding "0x" or sign prefix
        const size_t offset = md.has_sign + 2 * md.has_0x;
        pf_insert_pad(out, start + offset, '0', diff);
    }
    else // fill in spaces
    {
        pf_insert_pad(out, start, ' ', diff);
    }

    return diff;
}



// ---------------------------------------------------------------------------
//
//
//
// IMPLEMENTATIONS OF PUBLIC FUNCTIONS
//
//
//
// ---------------------------------------------------------------------------



// ------------------------------
// String functtions

size_t pf_vsnprintf_consuming_no_null_termination(
    char*restrict out_buf,
    const size_t max_size,
    const char* format,
    pf_va_list* args)
{
    PFString out = { out_buf ? out_buf : "", .capacity = max_size };

    while (1)
    {
        const PFFormatSpecifier fmt = pf_scan_format_string(format, args);
        if (fmt.string == NULL)
            break;

        pf_concat(&out, format, fmt.string - format);

        // Jump over format specifier for next iteration
        format = fmt.string + fmt.string_length;

        size_t written_by_conversion = 0;
        PFMiscData misc = {0};

        switch (fmt.conversion_format)
        {
        case 'c':
            if (fmt.length_modifier != 'l') {
                pf_push_char(&out, (char)va_arg(args->list, gp_promoted_arg_char_t));
                written_by_conversion = 1;
            } else {
                written_by_conversion = pf_write_wc(&out, args);
            } break;

        case 's': // TODO wide strings!!!!!!!!!
            written_by_conversion = pf_write_s(&out, args, fmt);
            break;

        case 'S':
            written_by_conversion = pf_write_S(&out, args, fmt);
            break;

        case 'd':
        case 'i':
            written_by_conversion = pf_write_i(&out, &misc, args, fmt);
            break;

        case 'o':
            written_by_conversion = pf_write_o(&out, args, fmt);
            break;

        case 'x':
            written_by_conversion = pf_write_x(&out, &misc, args, fmt);
            break;

        case 'X':
            written_by_conversion = pf_write_X(&out, &misc, args, fmt);
            break;

        case 'u':
            written_by_conversion = pf_write_u(&out, args, fmt);
            break;

        case 'p':
            written_by_conversion = pf_write_p(&out, args, fmt);
            break;

        case 'f': case 'F':
        case 'e': case 'E':
        case 'g': case 'G':
            written_by_conversion = pf_write_f(&out, &misc, args, fmt);
            break;

        case '%':
            pf_push_char(&out, '%');
            break;
        }

        if (written_by_conversion < fmt.field.width)
            pf_add_padding(&out, written_by_conversion, misc, fmt);
    } // while (1)

    pf_concat(&out, format, strlen(format)); // write what's left in format
    return out.length;
}

size_t pf_vsnprintf_consuming(
    char*restrict out_buf,
    const size_t max_size,
    const char* format,
    pf_va_list* args)
{
    size_t length = pf_vsnprintf_consuming_no_null_termination(out_buf, max_size, format, args);
    if (max_size > 0)
        out_buf[length < max_size ? length : max_size - 1] = '\0';
    return length;
}

size_t pf_vsnprintf(
    char* restrict out_buf,
    const size_t max_size,
    const char*restrict format,
    va_list _args)
{
    pf_va_list args;
    va_copy(args.list, _args);
    size_t result = pf_vsnprintf_consuming(out_buf, max_size, format, &args);
    va_end(args.list);
    return result;
}

size_t pf_vsprintf(
    char*restrict buf, const char*restrict fmt, va_list args)
{
    return pf_vsnprintf(buf, SIZE_MAX, fmt, args);
}

size_t pf_sprintf(char*restrict buf, const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t written = pf_vsnprintf(buf, SIZE_MAX, fmt, args);
    va_end(args);
    return written;
}

size_t pf_snprintf(
    char* restrict buf, const size_t n, const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t written = pf_vsnprintf(buf, n, fmt, args);
    va_end(args);
    return written;
}

// ------------------------------
// IO functtions

#define PAGE_SIZE 4096
#define BUF_SIZE (PAGE_SIZE + sizeof(""))

size_t pf_vfprintf(
    FILE*restrict stream, const char*restrict fmt, va_list args)
{
    char buf[BUF_SIZE];
    char* pbuf = buf;
    va_list args_copy;
    va_copy(args_copy, args);

    const size_t out_length = pf_vsnprintf(buf, BUF_SIZE, fmt, args);
    if (out_length >= BUF_SIZE) // try again from the very beginning. Why not
    {                           // flush and continue where we left off? Because
                                // we don't where to continue from without state,
                                // which would make average case slower, this
                                // has worst case of 2x slowdown, usually less.
                                // I you have a better idea, feel free to implement.
        // TODO TODO WHY IS MALLOC HERE?? WE **DO NOT** USE MALLOC!!!!!
        // Also, why do we bother with null-termination when using fwrite() anyway??
        pbuf = malloc(out_length + sizeof(""));

        pf_vsprintf(pbuf, fmt, args_copy);
    }
    fwrite(pbuf, sizeof(char), out_length, stream);

    if (pbuf != buf)
        free(pbuf);
    va_end(args_copy);
    return out_length;
}

size_t pf_vprintf(
    const char*restrict fmt, va_list args)
{
    return pf_vfprintf(stdout, fmt, args);
}

size_t pf_printf(
    const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t n = pf_vfprintf(stdout, fmt, args);
    va_end(args);
    return n;
}

size_t pf_fprintf(
    FILE*restrict stream, const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t n = pf_vfprintf(stream, fmt, args);
    va_end(args);
    return n;
}
