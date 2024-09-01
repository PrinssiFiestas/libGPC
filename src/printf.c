// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/printf.h>
#include <printf/format_scanning.h>
#include <printf/conversions.h>
#include "pfstring.h"

#include <gpc/string.h>
#include <gpc/unicode.h>

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>

struct pf_misc_data
{
    bool has_sign;
    bool has_0x;
    bool is_nan_or_inf;
};

static uintmax_t pf_get_uint(pf_va_list* args, const PFFormatSpecifier fmt)
{
    if (fmt.conversion_format == 'p')
        return va_arg(args->list, uintptr_t);

    switch (fmt.length_modifier)
    {
        case 'j':
            return va_arg(args->list, uintmax_t);

        case 'l' * 2:
            return va_arg(args->list, unsigned long long);

        case 'l':
            return va_arg(args->list, unsigned long);

        case 'h':
            return (unsigned short)va_arg(args->list, unsigned);

        case 'h' * 2:
            return (unsigned char)va_arg(args->list, unsigned);

        case 'z':
            return (size_t)va_arg(args->list, size_t);

        case 'B': // byte
            return (uint8_t)va_arg(args->list, unsigned);

        case 'W': // word
            return (uint16_t)va_arg(args->list, unsigned);

        case 'D': // double word
            return (uint32_t)va_arg(args->list, uint32_t);

        case 'Q': // quad word
            return (uint64_t)va_arg(args->list, uint64_t);

        default:
            return va_arg(args->list, unsigned);
    }
}

static unsigned pf_write_wc(
    struct pf_string* out,
    pf_va_list* args)
{
    size_t gp_utf8_decode(void*, uint32_t);
    char decoding[4];
    const size_t length = gp_utf8_decode(decoding, (wchar_t)va_arg(args->list, unsigned));
    pf_concat(out, decoding, length);
    return length;
}

static void pf_c_string_padding(
    struct pf_string* out,
    const PFFormatSpecifier fmt,
    const void* string,
    const size_t length)
{
    const unsigned field_width = fmt.field.width > length ?
        fmt.field.width : length;
    const unsigned diff = field_width - length;
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

static unsigned pf_write_s(
    struct pf_string* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const char* cstr = va_arg(args->list, const char*);

    size_t cstr_len = 0;
    if (fmt.precision.option == PF_NONE) // should be null-terminated
        cstr_len = strlen(cstr);
    else // who knows if null-terminated
        while (cstr_len < fmt.precision.width && cstr[cstr_len] != '\0')
            cstr_len++;

    pf_c_string_padding(out, fmt, cstr, cstr_len);
    return out->length - original_length;
}

static void pf_utf8_string_padding(
    struct pf_string* out,
    const PFFormatSpecifier fmt,
    const void* bytes,
    const size_t bytes_length,
    const size_t codepoint_count)
{
    const unsigned field_width = fmt.field.width > codepoint_count ?
        fmt.field.width : codepoint_count;
    const unsigned diff = field_width - codepoint_count;
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

static unsigned pf_write_S(
    struct pf_string* out,
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
            codepoint_count--;
            length = i - last_cp_length;
            break;
        } else if (i == length) {
            break;
        }
        codepoint_count++;
        i += last_cp_length = gp_utf8_codepoint_length(str, i);
    }
    pf_utf8_string_padding(out, fmt, str, length, codepoint_count);
    return out->length - original_length;
}

static void pf_write_leading_zeroes(
    struct pf_string* out,
    const unsigned written_by_utoa,
    const PFFormatSpecifier fmt)
{
    if (fmt.precision.option != PF_NONE)
    {
        const unsigned diff =
            fmt.precision.width <= written_by_utoa ? 0 :
            fmt.precision.width - written_by_utoa;
        memmove(
            out->data + out->length + diff,
            out->data + out->length,
            pf_limit(*out, written_by_utoa));
        memset(out->data + out->length, '0', pf_limit(*out, diff));
        out->length += written_by_utoa + diff;
    }
    else
    {
        out->length += written_by_utoa;
    }
}

static unsigned pf_write_i(
    struct pf_string* out,
    struct pf_misc_data* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    intmax_t i;
    switch (fmt.length_modifier)
    {
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
            i = (short)va_arg(args->list, int);
            break;

        case 'h' * 2: // signed char is NOT char!
            i = (signed char)va_arg(args->list, int);
            break;

        case 't':
            i = (ptrdiff_t)va_arg(args->list, ptrdiff_t);
            break;

        case 'B': // byte
            i = (int8_t)va_arg(args->list, int);
            break;

        case 'W': // word
            i = (int16_t)va_arg(args->list, int);
            break;

        case 'D': // double word
            i = (int32_t)va_arg(args->list, int32_t);
            break;

        case 'Q': // quad word
            i = (int64_t)va_arg(args->list, int64_t);
            break;

        default:
            i = va_arg(args->list, int);
    }

    const size_t original_length = out->length;

    const char sign = i < 0 ? '-' : fmt.flag.plus ? '+' : fmt.flag.space ? ' ' : 0;
    if (sign)
    {
        pf_push_char(out, sign);
        md->has_sign = true;
    }

    const unsigned max_written = pf_utoa(
        pf_capacity_left(*out), out->data + out->length, imaxabs(i));

    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned pf_write_o(
    struct pf_string* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = pf_get_uint(args, fmt);

    bool zero_written = false;
    if (fmt.flag.hash && u > 0)
    {
        pf_push_char(out, '0');
        zero_written = true;
    }

    const unsigned max_written = pf_otoa(
        pf_capacity_left(*out), out->data + out->length, u);

    // zero_written tells pad_zeroes() to add 1 less '0'
    pf_write_leading_zeroes(out, zero_written + max_written, fmt);
    // compensate for added zero_written to write_leading_zeroes()
    out->length -= zero_written;

    return out->length - original_length;
}

static unsigned pf_write_x(
    struct pf_string* out,
    struct pf_misc_data* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = pf_get_uint(args, fmt);

    if (fmt.flag.hash && u > 0)
    {
        pf_concat(out, "0x", strlen("0x"));
        md->has_0x = true;
    }

    const unsigned max_written = pf_xtoa(
        pf_capacity_left(*out), out->data + out->length, u);

    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned pf_write_X(
    struct pf_string* out,
    struct pf_misc_data* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = pf_get_uint(args, fmt);

    if (fmt.flag.hash && u > 0)
    {
        pf_concat(out, "0X", strlen("0X"));
        md->has_0x = true;
    }

    const unsigned max_written = pf_Xtoa(
        pf_capacity_left(*out), out->data + out->length, u);

    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned pf_write_u(
    struct pf_string* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = pf_get_uint(args, fmt);
    const unsigned max_written = pf_utoa(
        pf_capacity_left(*out), out->data + out->length, u);
    pf_write_leading_zeroes(out, max_written, fmt);
    return out->length - original_length;
}

static unsigned pf_write_p(
    struct pf_string* out,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const size_t original_length = out->length;
    const uintmax_t u = pf_get_uint(args, fmt);

    if (u > 0)
    {
        pf_concat(out, "0x", strlen("0x"));
        const unsigned max_written = pf_xtoa(
            pf_capacity_left(*out), out->data + out->length, u);
        pf_write_leading_zeroes(out, max_written, fmt);
    }
    else
    {
        pf_concat(out, "(nil)", strlen("(nil)"));
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

static unsigned pf_write_f(
    struct pf_string* out,
    struct pf_misc_data* md,
    pf_va_list* args,
    const PFFormatSpecifier fmt)
{
    const double f = va_arg(args->list, double);
    const unsigned written_by_conversion = pf_strfromd(
        out->data + out->length, out->capacity, fmt, f);
    out->length += written_by_conversion;


    md->has_sign = pf_signbit(f) || fmt.flag.plus || fmt.flag.space;
    md->is_nan_or_inf = pf_isnan(f) || pf_isinf(f);

    return written_by_conversion;
}

static unsigned pf_add_padding(
    struct pf_string* out,
    const unsigned written,
    const struct pf_misc_data md,
    const PFFormatSpecifier fmt)
{
    size_t start = out->length - written;
    const unsigned diff = fmt.field.width - written;

    const bool is_int_with_precision =
        strchr("diouxX", fmt.conversion_format) && fmt.precision.option != PF_NONE;
    const bool ignore_zero = is_int_with_precision || md.is_nan_or_inf;

    if (fmt.flag.dash) // left justified, append padding
    {
        pf_pad(out, ' ', diff);
    }
    else if (fmt.flag.zero && ! ignore_zero) // fill in zeroes
    { // 0-padding minding "0x" or sign prefix
        const unsigned offset = md.has_sign + 2 * md.has_0x;
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

int pf_vsnprintf_consuming(
    char*restrict out_buf,
    const size_t max_size,
    const char* format,
    pf_va_list* args)
{
    struct pf_string out = { out_buf ? out_buf : "", .capacity = max_size };

    while (1)
    {
        const PFFormatSpecifier fmt = pf_scan_format_string(format, args);
        if (fmt.string == NULL)
            break;

        pf_concat(&out, format, fmt.string - format);

        // Jump over format specifier for next iteration
        format = fmt.string + fmt.string_length;

        unsigned written_by_conversion = 0;
        struct pf_misc_data misc = {0};

        switch (fmt.conversion_format)
        {
            case 'c':
                if (fmt.length_modifier != 'l') {
                    pf_push_char(&out, (char)va_arg(args->list, int));
                    written_by_conversion = 1;
                } else {
                    written_by_conversion += pf_write_wc(&out, args);
                } break;

            case 's':
                written_by_conversion += pf_write_s(&out, args, fmt);
                break;

            case 'S':
                written_by_conversion += pf_write_S(&out, args, fmt);
                break;

            case 'd':
            case 'i':
                written_by_conversion += pf_write_i(&out, &misc, args, fmt);
                break;

            case 'o':
                written_by_conversion += pf_write_o(&out, args, fmt);
                break;

            case 'x':
                written_by_conversion += pf_write_x(&out, &misc, args, fmt);
                break;

            case 'X':
                written_by_conversion += pf_write_X(&out, &misc, args, fmt);
                break;

            case 'u':
                written_by_conversion += pf_write_u(&out, args, fmt);
                break;

            case 'p':
                written_by_conversion += pf_write_p(&out, args, fmt);
                break;

            case 'f': case 'F':
            case 'e': case 'E':
            case 'g': case 'G':
                written_by_conversion += pf_write_f(&out, &misc, args, fmt);
                break;

            case '%':
                pf_push_char(&out, '%');
                break;
        }

        if (written_by_conversion < fmt.field.width)
            pf_add_padding(
                &out,
                written_by_conversion,
                misc,
                fmt);
    }

    // Write what's left in format string
    pf_concat(&out, format, strlen(format));
    if (out.length < out.capacity)
        out.data[out.length] = '\0';
    return out.length;
}

int pf_vsnprintf(
    char* restrict out_buf,
    const size_t max_size,
    const char*restrict format,
    va_list _args)
{
    pf_va_list args;
    va_copy(args.list, _args);
    int result = pf_vsnprintf_consuming(out_buf, max_size, format, &args);
    va_end(args.list);
    return result;
}

int pf_vsprintf(
    char*restrict buf, const char*restrict fmt, va_list args)
{
    return pf_vsnprintf(buf, SIZE_MAX, fmt, args);
}

int pf_sprintf(char*restrict buf, const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = pf_vsnprintf(buf, INT_MAX, fmt, args);
    va_end(args);
    return written;
}

int pf_snprintf(
    char* restrict buf, const size_t n, const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = pf_vsnprintf(buf, n, fmt, args);
    va_end(args);
    return written;
}

// ------------------------------
// IO functtions

#define PAGE_SIZE 4096
#define BUF_SIZE (PAGE_SIZE + sizeof(""))

int pf_vfprintf(
    FILE*restrict stream, const char*restrict fmt, va_list args)
{
    char buf[BUF_SIZE];
    char* pbuf = buf;
    va_list args_copy;
    va_copy(args_copy, args);

    const int out_length = pf_vsnprintf(buf, BUF_SIZE, fmt, args);
    if (out_length >= (int)BUF_SIZE) // try again
    {
        pbuf = malloc(out_length + sizeof(""));
        pf_vsprintf(pbuf, fmt, args_copy);
    }
    fwrite(pbuf, sizeof(char), out_length, stream);

    if (pbuf != buf)
        free(pbuf);
    va_end(args_copy);
    return out_length;
}

int pf_vprintf(
    const char*restrict fmt, va_list args)
{
    return pf_vfprintf(stdout, fmt, args);
}

int pf_printf(
    const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = pf_vfprintf(stdout, fmt, args);
    va_end(args);
    return n;
}

int pf_fprintf(
    FILE*restrict stream, const char*restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = pf_vfprintf(stream, fmt, args);
    va_end(args);
    return n;
}


