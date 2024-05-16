// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/printf/blob/main/LICENSE.md

#include <printf/conversions.h>
#include <stdint.h>
#include "d2s_full_table.h"
#include "ryu.h"
#include "ryu_common.h"
#include "digit_table.h"
#include "d2fixed_full_table.h"
#include "d2s_intrinsics.h"
#include "pfstring.h"

#include <inttypes.h>
#include <math.h>
#include <limits.h>

#define DOUBLE_MANTISSA_BITS 52
#define DOUBLE_EXPONENT_BITS 11
#define DOUBLE_BIAS 1023

#define POW10_ADDITIONAL_BITS 120

// Max decimal digits in uintmax_t
#if CHAR_BIT == 8
#define MAX_DIGITS (sizeof(uintmax_t) * 3)
#else
#define MAX_DIGITS ((CHAR_BIT * sizeof(uintmax_t) * 3) / 8)
#endif

static void str_reverse_copy(
    char* restrict out,
    char* restrict buf,
    const size_t length,
    const size_t max)
{
    const size_t maxlen = max < length ? max : length;
    for (size_t i = 0; i < maxlen; i++)
        out[i] = buf[length - 1 - i];

    if (length < max)
        out[length] = '\0';
}

static inline void
append_n_digits(const uint32_t olength, uint32_t digits, char* const result);

size_t pf_utoa(const size_t n, char* out, unsigned long long x)
{
    if (n >= 10 && x < 1000000000) // use optimized
    {
        const uint32_t olength = decimalLength9(x);
        append_n_digits(olength, x, out);
        return olength;
    }

    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        buf[i++] = x % 10 + '0';
        x /= 10;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

size_t pf_itoa(size_t n, char* out, const long long ix)
{
    char buf[MAX_DIGITS];

    if (ix < 0)
    {
        if (n > 0)
        {
            out[0] = '-';
            n--;
        }
        out++;
    }

    unsigned long long x = imaxabs(ix);
    size_t i = 0;
    do // write all digits from low to high
    {
        buf[i++] = x % 10 + '0';
        x /= 10;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i + (ix < 0);
}

size_t pf_otoa(const size_t n, char* out, unsigned long long x)
{
    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        buf[i++] = x % 8 + '0';
        x /= 8;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

size_t pf_xtoa(const size_t n, char* out, unsigned long long x)
{
    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        size_t _x = x % 16;
        buf[i++] = _x < 10 ? _x + '0' : _x - 10 + 'a';
        x /= 16;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

size_t pf_Xtoa(const size_t n, char* out, unsigned long long x)
{
    char buf[MAX_DIGITS];
    size_t i = 0;
    do // write all digits from low to high
    {
        size_t _x = x % 16;
        buf[i++] = _x < 10 ? _x + '0' : _x - 10 + 'A';
        x /= 16;
    } while(x);

    str_reverse_copy(out, buf, i, n);
    return i;
}

// ---------------------------------------------------------------------------

static unsigned
pf_d2fixed_buffered_n(
    char* result,
    size_t n,
    PFFormatSpecifier fmt,
    double d);

static unsigned
pf_d2exp_buffered_n(
    char* result,
    const size_t n,
    PFFormatSpecifier fmt,
    double d);

size_t
pf_ftoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'f'};
    return pf_d2fixed_buffered_n(buf, n, fmt, f);
}

size_t
pf_Ftoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'F'};
    return pf_d2fixed_buffered_n(buf, n, fmt, f);
}

size_t
pf_etoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'e'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t
pf_Etoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'E'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t
pf_gtoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'g'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t
pf_Gtoa(const size_t n, char* const buf, const double f)
{
    const PFFormatSpecifier fmt = {.conversion_format = 'G'};
    return pf_d2exp_buffered_n(buf, n, fmt, f);
}

size_t pf_strfromd(
    char* const buf,
    const size_t n,
    const PFFormatSpecifier fmt,
    const double f)
{
    if (fmt.conversion_format == 'f' || fmt.conversion_format == 'F')
        return pf_d2fixed_buffered_n(buf, n, fmt, f);
    else
        return pf_d2exp_buffered_n(buf, n, fmt, f);
}

// ---------------------------------------------------------------------------
//
// Modified Ryū
//
// https://dl.acm.org/doi/pdf/10.1145/3192366.3192369
// https://dl.acm.org/doi/pdf/10.1145/3360595
// https://github.com/ulfjack/ryu
//
// ---------------------------------------------------------------------------

// Convert `digits` to a sequence of decimal digits. Append the digits to the
// result.
// The caller has to guarantee that:
//   10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void
append_n_digits(const uint32_t olength, uint32_t digits, char* const result)
{
    uint32_t i = 0;
    while (digits >= 10000)
    {
        #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
            const uint32_t c = digits - 10000 * (digits / 10000);
        #else
            const uint32_t c = digits % 10000;
        #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c0, 2);
        memcpy(result + olength - i - 4, DIGIT_TABLE + c1, 2);
        i += 4;
    }
    if (digits >= 100)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
        i += 2;
    }
    if (digits >= 10)
    {
        const uint32_t c = digits << 1;
        memcpy(result + olength - i - 2, DIGIT_TABLE + c, 2);
    }
    else
    {
        result[0] = (char) ('0' + digits);
    }
}

static inline uint32_t
mulShift_mod1e9(const uint64_t m, const uint64_t* const mul, const int32_t j)
{
    uint64_t high0;                                   // 64
    const uint64_t low0 = umul128(m, mul[0], &high0); // 0
    uint64_t high1;                                   // 128
    const uint64_t low1 = umul128(m, mul[1], &high1); // 64
    uint64_t high2;                                   // 192
    const uint64_t low2 = umul128(m, mul[2], &high2); // 128
    const uint64_t s0low = low0;              // 0
    (void) s0low; // unused
    const uint64_t s0high = low1 + high0;     // 64
    const uint32_t c1 = s0high < low1;
    const uint64_t s1low = low2 + high1 + c1; // 128
    // high1 + c1 can't overflow, so compare against low2
    const uint32_t c2 = s1low < low2;
    const uint64_t s1high = high2 + c2;       // 192
    assert(j >= 128);
    assert(j <= 180);
    #if defined(HAS_64_BIT_INTRINSICS)
        const uint32_t dist = (uint32_t) (j - 128); // dist: [0, 52]
        const uint64_t shiftedhigh = s1high >> dist;
        const uint64_t shiftedlow = shiftright128(s1low, s1high, dist);
        return uint128_mod1e9(shiftedhigh, shiftedlow);
    #else // HAS_64_BIT_INTRINSICS
        if (j < 160)
        { // j: [128, 160)
            const uint64_t r0 = mod1e9(s1high);
            const uint64_t r1 = mod1e9((r0 << 32) | (s1low >> 32));
            const uint64_t r2 = ((r1 << 32) | (s1low & 0xffffffff));
            return mod1e9(r2 >> (j - 128));
        }
        else
        { // j: [160, 192)
            const uint64_t r0 = mod1e9(s1high);
            const uint64_t r1 = ((r0 << 32) | (s1low >> 32));
            return mod1e9(r1 >> (j - 160));
        }
    #endif // HAS_64_BIT_INTRINSICS
}

// Convert `digits` to a sequence of decimal digits. Print the first digit,
// followed by a decimal dot '.' followed by the remaining digits. The caller
// has to guarantee that:
//     10^(olength-1) <= digits < 10^olength
// e.g., by passing `olength` as `decimalLength9(digits)`.
static inline void
append_d_digits(const uint32_t olength, uint32_t digits, char* const result)
{
    uint32_t i = 0;
    while (digits >= 10000)
    {
        #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
            const uint32_t c = digits - 10000 * (digits / 10000);
        #else
            const uint32_t c = digits % 10000;
        #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + olength + 1 - i - 2, DIGIT_TABLE + c0, 2);
        memcpy(result + olength + 1 - i - 4, DIGIT_TABLE + c1, 2);
        i += 4;
    }

    if (digits >= 100)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + olength + 1 - i - 2, DIGIT_TABLE + c, 2);
        i += 2;
    }

    if (digits >= 10)
    {
        const uint32_t c = digits << 1;
        result[2] = DIGIT_TABLE[c + 1];
        result[1] = '.';
        result[0] = DIGIT_TABLE[c];
    }
    else
    {
        result[1] = '.';
        result[0] = (char) ('0' + digits);
    }
}

static inline void
pf_append_d_digits(
    struct pf_string out[static 1],
    const uint32_t maximum, // first_available_digits
    const uint32_t digits)
{
    if (pf_capacity_left(*out) >= maximum) // write directly
    {
        append_d_digits(
            maximum, digits, out->data + out->length);
        out->length += maximum + strlen(".");
    }
    else // write only as much as fits
    {
        char buf[10];
        append_d_digits(maximum, digits, buf);
        pf_concat(out, buf, maximum + strlen("."));
    }
}

// Convert `digits` to decimal and write the last `count` decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void
append_c_digits(const uint32_t count, uint32_t digits, char* const result)
{
    // Copy pairs of digits from DIGIT_TABLE.
    uint32_t i = 0;
    for (; i < count - 1; i += 2)
    {
        const uint32_t c = (digits % 100) << 1;
        digits /= 100;
        memcpy(result + count - i - 2, DIGIT_TABLE + c, 2);
    }
    // Generate the last digit if count is odd.
    if (i < count)
    {
        const char c = (char) ('0' + (digits % 10));
        result[count - i - 1] = c;
    }
}

static inline void
pf_append_c_digits(
    struct pf_string out[static 1],
    const uint32_t count,
    const uint32_t digits)
{
    if (pf_capacity_left(*out) >= count) // write directly
    {
        append_c_digits(
            count, digits, out->data + out->length);
        out->length += count;
    }
    else // write only as much as fits
    {
        char buf[10];
        append_c_digits(
            count, digits, buf);
        pf_concat(out, buf, count);
    }
}

// Convert `digits` to decimal and write the last 9 decimal digits to result.
// If `digits` contains additional digits, then those are silently ignored.
static inline void
append_nine_digits(uint32_t digits, char* const result)
{
    if (digits == 0)
    {
        memset(result, '0', 9);
        return;
    }

    for (uint32_t i = 0; i < 5; i += 4)
    {
        #ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
            const uint32_t c = digits - 10000 * (digits / 10000);
        #else
            const uint32_t c = digits % 10000;
        #endif
        digits /= 10000;
        const uint32_t c0 = (c % 100) << 1;
        const uint32_t c1 = (c / 100) << 1;
        memcpy(result + 7 - i, DIGIT_TABLE + c0, 2);
        memcpy(result + 5 - i, DIGIT_TABLE + c1, 2);
    }
    result[0] = (char) ('0' + digits);
}

static inline void
pf_append_nine_digits(struct pf_string out[static 1], uint32_t digits)
{
    if (pf_capacity_left(*out) >= 9) // write directly
    {
        append_nine_digits(digits, out->data + out->length);
        out->length += 9;
    }
    else // write only as much as fits
    {
        char buf[10];
        append_nine_digits(digits, buf);
        pf_concat(out, buf, 9);
    }
}

static inline void
append_utoa(struct pf_string out[static 1], uint32_t digits)
{
    if (pf_capacity_left(*out) >= 9) // write directly
    {
        out->length += pf_utoa(
            pf_capacity_left(*out), out->data + out->length, digits);
    }
    else // write only as much as fits
    {
        char buf[10];
        unsigned buf_len = pf_utoa(sizeof(buf), buf, digits);
        pf_concat(out, buf, buf_len);
    }
}

static inline uint32_t indexForExponent(const uint32_t e)
{
    return (e + 15) / 16;
}

static inline uint32_t pow10BitsForIndex(const uint32_t idx)
{
    return 16 * idx + POW10_ADDITIONAL_BITS;
}

static inline uint32_t lengthForIndex(const uint32_t idx)
{
    // +1 for ceil, +16 for mantissa, +8 to round up when dividing by 9
    return (log10Pow2(16 * (int32_t) idx) + 1 + 16 + 8) / 9;
}

// ---------------------------------------------------------------------------
//
// START OF MODIFIED RYU

static inline unsigned
pf_copy_special_str_printf(
    struct pf_string out[const static 1],
    const uint64_t mantissa,
    const bool uppercase)
{
    if (mantissa != 0)
    {
        pf_concat(out, uppercase ? "NAN" : "nan", strlen("nan"));
        if (pf_capacity_left(*out))
            out->data[out->length] = '\0';
        return out->length;
    }
    else
    {
        pf_concat(out, uppercase ? "INF" : "inf", strlen("inf"));
        if (pf_capacity_left(*out))
            out->data[out->length] = '\0';
        return out->length;
    }
}

static unsigned
pf_d2fixed_buffered_n(
    char* const result,
    const size_t n,
    const PFFormatSpecifier fmt,
    const double d)
{
    struct pf_string out = { result, .capacity = n };
    const bool fmt_is_g =
        fmt.conversion_format == 'g' || fmt.conversion_format == 'G';
    unsigned precision;
    if (fmt.precision.option == PF_SOME)
        precision = fmt.precision.width;
    else
        precision = 6;

    const uint64_t bits = double_to_bits(d);

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign =
        ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent = (uint32_t)
        ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

    if (ieeeSign)
        pf_push_char(&out, '-');
    else if (fmt.flag.plus)
        pf_push_char(&out, '+');
    else if (fmt.flag.space)
        pf_push_char(&out, ' ');

    // Case distinction; exit early for the easy cases.
    if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u))
    {
        const bool uppercase =
            fmt.conversion_format == 'F' || fmt.conversion_format == 'G';
        return pf_copy_special_str_printf(&out, ieeeMantissa, uppercase);
    }

    if (ieeeExponent == 0 && ieeeMantissa == 0) // d == 0.0
    {
        pf_push_char(&out, '0');

        if (precision > 0 || fmt.flag.hash)
            pf_push_char(&out, '.');
        pf_pad(&out, '0', precision);

        if (pf_capacity_left(out))
            out.data[out.length] = '\0';
        return out.length;
    }

    int32_t e2;
    uint64_t m2;
    if (ieeeExponent == 0)
    {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = ieeeMantissa;
    }
    else
    {
        e2 = (int32_t) ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }

    bool is_zero = true; // for now

    uint32_t all_digits[256] = {}; // significant digits without trailing zeroes
    size_t digits_length = 0;
    size_t integer_part_end = 0; // place for decimal point

    if (e2 >= -52) // store integer part
    {
        const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t) e2);
        const uint32_t p10bits = pow10BitsForIndex(idx);
        const int32_t len = (int32_t)lengthForIndex(idx);

        for (int32_t i = len - 1; i >= 0; --i)
        {
            const uint32_t j = p10bits - e2;
            const uint32_t digits = mulShift_mod1e9(
                m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t) (j + 8));

            if ( ! is_zero)
            { // always subsequent iterations of loop
                all_digits[digits_length++] = digits;
            }
            else if (digits != 0)
            { // always 1st iteration of loop
                all_digits[digits_length++] = digits;
                is_zero = false;
            }
        }
        integer_part_end = digits_length;
    }

    if (is_zero)
    {
        all_digits[0]    = 0;
        digits_length    = 1;
        integer_part_end = 1;
    }
    else if (fmt_is_g)
    {
        const uint32_t significant_digits = decimalLength9(all_digits[0]) +
            9*(integer_part_end - 1);

        if (significant_digits >= precision)
            precision = 0;
        else
            precision -= significant_digits;
    }

    bool round_up = false;
    uint32_t lastDigit = 0; // to be cut off. Determines roundUp.
    uint32_t last_digit_magnitude = 1000*1000*1000;
    uint32_t maximum = 9;
    unsigned fract_leading_zeroes = 0;
    unsigned fract_trailing_zeroes = 0;

    // Might have to update precision with 'g' and recalculate, thus loop
    bool first_try = true;
    while (e2 < 0) // store fractional part
    {
        const int32_t idx = -e2 / 16;
        const uint32_t blocks = precision / 9 + 1;

        uint32_t i = 0;
        if (blocks <= MIN_BLOCK_2[idx])
        {
            i = blocks; // skip the for-loop below
            fract_leading_zeroes = precision;
        }
        else if (i < MIN_BLOCK_2[idx])
        {
            i = MIN_BLOCK_2[idx];
            fract_leading_zeroes = 9 * i;
        }

        uint32_t digits = 0;
        for (; i < blocks; ++i) // store significant fractional digits
        {
            const int32_t j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
            const uint32_t p = POW10_OFFSET_2[idx] + i - MIN_BLOCK_2[idx];

            if (p >= POW10_OFFSET_2[idx + 1])
            {
                fract_trailing_zeroes = precision - 9 * i;
                break;
            }

            digits = mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);
            all_digits[digits_length++] = digits;
        }

        if (fmt_is_g && is_zero && first_try)
        {
            uint32_t total_leading_zeroes = fract_leading_zeroes;

            size_t i;
            for (i = integer_part_end; i < digits_length - 1; i++)
            {
                if (all_digits[i] == 0)
                    total_leading_zeroes += 9;
                else break;
            }
            total_leading_zeroes += 9 - decimalLength9(all_digits[i]);

            if (total_leading_zeroes > 0)
            {
                precision += total_leading_zeroes;
                digits_length = integer_part_end; // reset all_digits[]
                first_try = false;
                continue; // try again
            }
        }

        if (i == blocks)
        {
            maximum = precision - 9 * (i - 1);

            uint32_t k;
            for (k = 0; k < 9 - maximum; ++k) // trim digits from right
            {
                lastDigit = digits % 10;
                digits /= 10;
            }
            const uint32_t magnitude_table[] = { // avoid work in loop
                1000000000,
                100000000,
                10000000,
                1000000,
                100000,
                10000,
                1000,
                100,
                10,
                1
            };
            last_digit_magnitude = magnitude_table[k];

            if (lastDigit != 5)
            {
                round_up = lastDigit > 5;
            }
            else
            {
                const bool any_left_in_digits = k < 9;
                const uint32_t next_digit = any_left_in_digits ?
                    digits : all_digits[digits_length - 2];

                const int32_t requiredTwos = -e2 - (int32_t) precision - 1;
                const bool trailingZeros = requiredTwos <= 0 || (
                    requiredTwos < 60 &&
                    multipleOfPowerOf2(m2, (uint32_t)requiredTwos)
                );

                round_up = next_digit % 2 || ! trailingZeros;
            }

            if (digits_length != integer_part_end) // update modified digits
                all_digits[digits_length - 1] = digits;
            else // digits never stored, nowhere to round
                round_up = false;
        }

        break;
    }

    if (round_up)
    {
        uint32_t last_real_mag = 0;
        if (fmt_is_g && is_zero)
            last_real_mag = decimalLength9(all_digits[1]);

        all_digits[digits_length - 1] += 1;

        if (all_digits[digits_length - 1] == last_digit_magnitude)
            all_digits[digits_length - 1] = 0; // carry 1
        else
            round_up = false;

        if (round_up)
        {
            for (size_t i = digits_length - 2; i > 0; i--) // keep rounding
            {
                all_digits[i] += 1;
                if (all_digits[i] == (uint32_t)1000*1000*1000) {
                    all_digits[i] = 0; // carry 1
                } else {
                    round_up = false;
                    break;
                }
            }
        }

        if (round_up)
            all_digits[0] += 1;

        if (fmt_is_g && is_zero)
        {
            if (round_up) { // 0.xxx turned to 1.xxx
                maximum--;
            } else if (decimalLength9(all_digits[1]) > last_real_mag) {
                maximum--;
                all_digits[1] /= 10;
            }
        }
    }

    // Start writing digits for integer part

    append_utoa(&out, all_digits[0]);

    for (size_t i = 1; i < integer_part_end; i++)
    {
        pf_append_nine_digits(&out, all_digits[i]);
    }

    // Start writing digits for fractional part

    if ( ! fmt_is_g || fmt.flag.hash)
    {
        if (precision > 0 || fmt.flag.hash)
            pf_push_char(&out, '.');

        if (digits_length != integer_part_end)
        {
            pf_pad(&out, '0', fract_leading_zeroes);

            for (size_t k = integer_part_end; k < digits_length - 1; k++)
                pf_append_nine_digits(&out, all_digits[k]);

            if (maximum > 0) // write the last digits left
                pf_append_c_digits(&out, maximum, all_digits[digits_length - 1]);

            pf_pad(&out, '0', fract_trailing_zeroes);
        }
        else
        {
            pf_pad(&out, '0', precision);
        }
    }
    else
    {
        // Trim trailing zeroes
        while (digits_length != integer_part_end)
        {
            if (all_digits[digits_length - 1] == 0)
            {
                digits_length--;
                maximum = 9;
                continue;
            }
            else
            {
                while (all_digits[digits_length - 1] != 0)
                {
                    if (all_digits[digits_length - 1] % 10 == 0) {
                        all_digits[digits_length - 1] /= 10;
                        maximum--;
                    } else
                        goto end_trim_zeroes;
                }
            }
        } end_trim_zeroes:

        if (digits_length > integer_part_end)
        {
            pf_push_char(&out, '.');
            pf_pad(&out, '0', fract_leading_zeroes);

            for (size_t k = integer_part_end; k < digits_length - 1; k++)
                pf_append_nine_digits(&out, all_digits[k]);

            pf_append_c_digits(&out, maximum, all_digits[digits_length - 1]);
        }
    }

    if (pf_capacity_left(out))
        out.data[out.length] = '\0';
    return out.length;
}

static unsigned
pf_d2exp_buffered_n(
    char* const result,
    const size_t n,
    const PFFormatSpecifier fmt,
    const double d)
{
    struct pf_string out = { result, .capacity = n };
    const bool fmt_is_g =
        fmt.conversion_format == 'g' || fmt.conversion_format == 'G';

    unsigned precision;
    if ( ! fmt_is_g)
    {
        if (fmt.precision.option == PF_SOME)
            precision = fmt.precision.width;
        else
            precision = 6;
    }
    else // precision = significant digits so subtract 1, integer part
    {
        if (fmt.precision.option == PF_SOME)
            precision = fmt.precision.width - !!fmt.precision.width;
        else
            precision = 6 - 1;
    }

    const uint64_t bits = double_to_bits(d);

    // Decode bits into sign, mantissa, and exponent.
    const bool ieeeSign =
        ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
    const uint64_t ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
    const uint32_t ieeeExponent = (uint32_t)
        ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));

    if (ieeeSign)
        pf_push_char(&out, '-');
    else if (fmt.flag.plus)
        pf_push_char(&out, '+');
    else if (fmt.flag.space)
        pf_push_char(&out, ' ');

    // Case distinction; exit early for the easy cases.
    if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u))
    {
        const bool uppercase =
            fmt.conversion_format == 'E' || fmt.conversion_format == 'G';
        return pf_copy_special_str_printf(&out, ieeeMantissa, uppercase);
    }

    if (ieeeExponent == 0 && ieeeMantissa == 0) // d = 0.0
    {
        pf_push_char(&out, '0');
        if (fmt_is_g && ! fmt.flag.hash) {
            if (pf_capacity_left(out))
                out.data[out.length] = '\0';
            return out.length;
        }

        if (precision > 0 || fmt.flag.hash)
        {
            pf_push_char(&out, '.');
            pf_pad(&out, '0', precision);
        }

        if (fmt.conversion_format == 'e')
            pf_concat(&out, "e+00", strlen("e+00"));
        else if (fmt.conversion_format == 'E')
            pf_concat(&out, "E+00", strlen("E+00"));

        if (pf_capacity_left(out))
            out.data[out.length] = '\0';
        return out.length;
    }

    int32_t e2;
    uint64_t m2;
    if (ieeeExponent == 0) {
        e2 = 1 - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = ieeeMantissa;
    } else {
        e2 = (int32_t)ieeeExponent - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
        m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
    }

    const bool printDecimalPoint = precision > 0;
    ++precision;

    uint32_t digits = 0;
    uint32_t stored_digits = 0;
    uint32_t availableDigits = 0;
    int32_t exp = 0;

    uint32_t all_digits[256] = {}; // significant digits without trailing zeroes
    size_t digits_length = 0;
    uint32_t first_available_digits = 0;

    if (e2 >= -52)
    {
        const uint32_t idx = e2 < 0 ? 0 : indexForExponent((uint32_t)e2);
        const uint32_t p10bits = pow10BitsForIndex(idx);
        const int32_t len = (int32_t)lengthForIndex(idx);
        for (int32_t i = len - 1; i >= 0; --i)
        {
            const uint32_t j = p10bits - e2;
            // Temporary: j is usually around 128, and by shifting a bit, we
            // push it to 128 or above, which is a slightly faster code path in
            // mulShift_mod1e9. Instead, we can just increase the multipliers.
            digits = mulShift_mod1e9(
                m2 << 8, POW10_SPLIT[POW10_OFFSET[idx] + i], (int32_t)(j + 8));

            if (stored_digits != 0) // never first iteration
            { // store fractional part excluding last max 9 digits
                if (stored_digits + 9 > precision)
                {
                    availableDigits = 9;
                    break;
                }

                all_digits[digits_length++] = digits;
                stored_digits += 9;
            }
            else if (digits != 0) // only at first iteration
            { // store integer part, a single digit
                first_available_digits = decimalLength9(digits);
                exp = i * 9 + first_available_digits - 1;

                if (first_available_digits > precision)
                {
                    availableDigits = first_available_digits;
                    break;
                }

                all_digits[0] = digits;
                digits_length = 1;

                stored_digits = first_available_digits;
            }
        }
    }

    if (e2 < 0 && availableDigits == 0)
    {
        const int32_t idx = -e2 / 16;

        for (int32_t i = MIN_BLOCK_2[idx]; i < 200; ++i)
        {
            const int32_t j = ADDITIONAL_BITS_2 + (-e2 - 16 * idx);
            const uint32_t p = POW10_OFFSET_2[idx] + (uint32_t)i - MIN_BLOCK_2[idx];
            // Temporary: j is usually around 128, and by shifting a bit, we
            // push it to 128 or above, which is a slightly faster code path in
            // mulShift_mod1e9. Instead, we can just increase the multipliers.
            digits = (p >= POW10_OFFSET_2[idx + 1]) ?
                0 : mulShift_mod1e9(m2 << 8, POW10_SPLIT_2[p], j + 8);

            if (stored_digits != 0) // never first iteration
            { // store fractional part excluding last max 9 digits
                if (stored_digits + 9 > precision)
                {
                    availableDigits = 9;
                    break;
                }

                all_digits[digits_length++] = digits;
                stored_digits += 9;
            }
            else if (digits != 0) // only at first iteration
            { // store integer part, a single digit
                first_available_digits = decimalLength9(digits);
                exp = -(i + 1) * 9 + first_available_digits - 1;

                if (first_available_digits > precision)
                {
                    availableDigits = first_available_digits;
                    break;
                }

                all_digits[0] = digits;
                digits_length = 1;

                stored_digits = first_available_digits;
            }
        }
    }

    const uint32_t maximum = precision - stored_digits;

    if (availableDigits == 0)
        digits = 0;

    uint32_t lastDigit = 0;
    uint32_t k = 0;
    if (availableDigits > maximum) // find last digit
    {
        for (k = 0; k < availableDigits - maximum; ++k)
        {
            lastDigit = digits % 10;
            digits /= 10;
        }
    }
    const uint32_t magnitude_table[] = { // avoid work in loop
        1000000000,
        100000000,
        10000000,
        1000000,
        100000,
        10000,
        1000,
        100,
        10,
        1
    };
    const uint32_t last_digit_magnitude = magnitude_table[k];

    all_digits[digits_length++] = digits;

    bool round_up = false;
    if (lastDigit != 5)
    {
        round_up = lastDigit > 5;
    }
    else
    {
        const bool any_left_in_digits = k < 9;
        const uint32_t next_digit = any_left_in_digits ?
            digits : all_digits[digits_length - 2];

        const int32_t rexp = (int32_t)precision - exp;
        const int32_t requiredTwos = -e2 - rexp;
        bool trailingZeros = requiredTwos <= 0 ||
            (requiredTwos < 60 && multipleOfPowerOf2(m2, (uint32_t)requiredTwos));

        if (rexp < 0)
        {
            const int32_t requiredFives = -rexp;
            trailingZeros = trailingZeros &&
                multipleOfPowerOf5(m2, (uint32_t)requiredFives);
        }
        round_up = next_digit % 2 || ! trailingZeros;
    }

    if (round_up && digits_length >= 2)
    {
        all_digits[digits_length - 1] += 1;

        if (all_digits[digits_length - 1] == last_digit_magnitude)
            all_digits[digits_length - 1] = 0; // carry 1
        else
            round_up = false;

        if (round_up)
        {
            for (size_t i = digits_length - 2; i > 0; i--) // keep rounding
            {
                all_digits[i] += 1;
                if (all_digits[i] == (uint32_t)1000*1000*1000) {
                    all_digits[i] = 0; // carry 1
                } else {
                    round_up = false;
                    break;
                }
            }
        }

        if (round_up)
        {
            all_digits[0] += 1;
            if (all_digits[0] == magnitude_table[9 - first_available_digits])
            {
                all_digits[0] /= 10;
                ++exp;
            }
        }
    }
    else if (round_up)
    {
        all_digits[0] += 1;
        if (all_digits[0] ==
                last_digit_magnitude / magnitude_table[first_available_digits])
        {
            exp++;
        }
    }

    // Exponent is known now and we can determine the appropriate 'g' conversion
    if (fmt_is_g && ! (exp < -4 || exp >= (int32_t)precision))
        return pf_d2fixed_buffered_n(result, n, fmt, d);

    if ( ! printDecimalPoint)
    {
        if (all_digits[0] == 10) // rounded up from 9
            all_digits[0] = 1;
        pf_push_char(&out, '0' + all_digits[0]);
        if (fmt.flag.hash)
            pf_push_char(&out, '.');
    }
    else if ( ! fmt_is_g || fmt.flag.hash)
    {
        if (stored_digits != 0)
        {
            pf_append_d_digits(&out, first_available_digits, all_digits[0]);

            for (size_t i = 1; i < digits_length - 1; i++)
                pf_append_nine_digits(&out, all_digits[i]);

            if (all_digits[digits_length - 1] == 0)
                pf_pad(&out, '0', maximum);
            else
                pf_append_c_digits(&out, maximum, all_digits[digits_length - 1]);
        }
        else
        {
            pf_append_d_digits(&out, maximum, all_digits[0]);
        }
    }
    else // 'g'
    {
        uint32_t last_digits_length = maximum;
        // Trim trailing zeroes
        while (digits_length > 0)
        {
            if (all_digits[digits_length - 1] == 0)
            {
                digits_length--;
                last_digits_length = 9;
                continue;
            }
            else
            {
                while (all_digits[digits_length - 1] != 0)
                {
                    if (all_digits[digits_length - 1] % 10 == 0) {
                        all_digits[digits_length - 1] /= 10;
                        last_digits_length--;
                    } else
                        goto end_trim_zeroes;
                }
            }
        } end_trim_zeroes:

        if (digits_length > 1)
        {
            pf_append_d_digits(&out, first_available_digits, all_digits[0]);

            for (size_t i = 1; i < digits_length - 1; i++)
                pf_append_nine_digits(&out, all_digits[i]);

            if (all_digits[digits_length - 1] != 0)
                pf_append_c_digits(
                    &out, last_digits_length, all_digits[digits_length - 1]);
        }
        else
        {
            if (all_digits[0] >= 10)
                pf_append_d_digits(
                    &out, decimalLength9(all_digits[0]), all_digits[0]);
            else
                pf_push_char(&out, '0' + all_digits[0]);
        }
    }

    const bool uppercase =
        fmt.conversion_format == 'E' || fmt.conversion_format == 'G';
    pf_push_char(&out, uppercase ? 'E' : 'e');
    if (exp < 0) {
        pf_push_char(&out, '-');
        exp = -exp;
    } else {
        pf_push_char(&out, '+');
    }

    char buf[4] = "";
    if (exp >= 100) {
        const int32_t c = exp % 10;
        memcpy(buf, DIGIT_TABLE + 2 * (exp / 10), 2);
        buf[2] = '0' + c;
    } else {
        memcpy(buf, DIGIT_TABLE + 2 * exp, 2);
    }
    pf_concat(&out, buf, strlen(buf));

    if (pf_capacity_left(out))
        out.data[out.length] = '\0';
    return out.length;
}
