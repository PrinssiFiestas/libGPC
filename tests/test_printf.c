// MIT Litense
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include <gpc/assert.h>
#include "../src/printf.c"
#include <time.h>

#define expect_str(str_a, str_b) \
do { \
    const char *_str_a = (str_a), *_str_b = (str_b); \
    bool is_true = strcmp(_str_a, _str_b) == 0; \
    gp_expect(!!#str_a " equals " #str_b == is_true, (_str_a), (_str_b)); \
} while (0)

#define assert_str(str_a, str_b) \
do { \
    const char *_str_a = (str_a), *_str_b = (str_b); \
    bool is_true = strcmp(_str_a, _str_b) == 0; \
    gp_assert(!!#str_a " equals " #str_b == is_true, (_str_a), (_str_b)); \
} while (0)

#ifndef FUZZ_COUNT
#define FUZZ_COUNT 65536
#endif

#ifndef FUZZ_SEED_OFFSET
#define FUZZ_SEED_OFFSET 0
#endif

GPRandomState g_rs;

int main(void)
{
    char buf[512] = "";
    char buf_std[512] = "";

    gp_suite("Basic type conversions");
    {
        gp_test("%c");
        {
            pf_sprintf(buf, "blah %c blah %lc", 'x', L'ö');
            expect_str(buf, "blah x blah ö");
        }

        gp_test("%s");
        {
            pf_sprintf(buf, "blah %s blah", "bloink");
            expect_str(buf, "blah bloink blah");
        }

        gp_test("%d and %i");
        {
            pf_sprintf(buf, "blah %d blah", 15);
            expect_str(buf, "blah 15 blah");

            pf_sprintf(buf, "blah %ti blah", (ptrdiff_t)-953);
            expect_str(buf, "blah -953 blah");

            pf_sprintf(buf,  "blah %lli blah", -LLONG_MAX + 5);
            sprintf(buf_std, "blah %lli blah", -LLONG_MAX + 5);
            expect_str(buf, buf_std);
        }

        gp_test("%o, %x, and %X");
        {
            pf_sprintf(buf,  "blah %o blah", 384);
            sprintf(buf_std, "blah %o blah", 384);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "blah %lx blah", 0xfeedl);
            sprintf(buf_std, "blah %lx blah", 0xfeedl);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "blah %zX blah", (size_t)0xBEEF);
            sprintf(buf_std, "blah %zX blah", (size_t)0xBEEF);
            expect_str(buf, buf_std);
        }

        gp_test("Floats");
        {
            pf_sprintf(buf,  "blah %f blah", 124.647);
            sprintf(buf_std, "blah %f blah", 124.647);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "blah %E blah", -42e6);
            sprintf(buf_std, "blah %E blah", -42e6);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "blah %g blah", -13.1);
            sprintf(buf_std, "blah %g blah", -13.1);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%f", 0.);
            sprintf(buf_std, "%f", 0.);
            expect_str(buf, buf_std);
            pf_sprintf(buf,  "%e", 0.);
            sprintf(buf_std, "%e", 0.);
            expect_str(buf, buf_std);
            pf_sprintf(buf,  "%g", 0.);
            sprintf(buf_std, "%g", 0.);
            expect_str(buf, buf_std);
            pf_sprintf(buf,  "%#g", 0.);
            sprintf(buf_std, "%#g", 0.);
            expect_str(buf, buf_std);
        }

        gp_test("%p");
        {
            void* p = (void*)-1;
            uintptr_t u = (uintptr_t)p;
            char _buf[sizeof(void*) * 3];
            const char* fmt =
                UINTPTR_MAX == ULLONG_MAX ? "%#llx" :
                UINTPTR_MAX == ULONG_MAX  ? "%#lx"  : "%#x";
            sprintf(_buf, fmt, u);
            pf_sprintf(buf, "%p", p);
            expect_str(buf, _buf);
        }
    } // gp_suite("Basic type conversions");

    gp_suite("Precision");
    {
        gp_test("Unsigned integers");
        {
            pf_sprintf(buf,  "%.4u", 3);
            sprintf(buf_std, "%.4u", 3);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%.24x", 0xe);
            sprintf(buf_std, "%.24x", 0xe);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%.*X", 3, 0XD);
            sprintf(buf_std, "%.*X", 3, 0XD);
            expect_str(buf, buf_std);
        }

        gp_test("Signed integers");
        {
            pf_sprintf(buf,  "%.3i", 2);
            sprintf(buf_std, "%.3i", 2);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%.3i", -2);
            sprintf(buf_std, "%.3i", -2);
            expect_str(buf, buf_std);
        }

        gp_test("Strings");
        {
            pf_sprintf(buf, "%.5s", "str");
            expect_str(buf, "str");

            pf_sprintf(buf, "%.5s", "String loger than 5 chars");
            expect_str(buf, "Strin");

            pf_sprintf(buf, "%.*s", 4, "String loger than 5 chars");
            expect_str(buf, "Stri");
        }
    } // gp_suite("Precision");

    gp_suite("Flags");
    {
        gp_test("-: Left justification");
        {
            pf_sprintf(buf, "|%-8i|", -2);
            expect_str(buf, "|-2      |");
        }

        gp_test("+: Add plus to signed positives");
        {
            pf_sprintf(buf, "%+i", 35);
            expect_str(buf, "+35");

            pf_sprintf(buf, "%+g", 1.41);
            expect_str(buf, "+1.41");
        }

        gp_test(" : Add space to signed positives");
        {
            pf_sprintf(buf, "% i", 35);
            expect_str(buf, " 35");

            pf_sprintf(buf, "% g", 1.41);
            expect_str(buf, " 1.41");
        }

        gp_test("0: Zero padding");
        {
            pf_sprintf(buf, "|%08i|", -1);
            expect_str(buf, "|-0000001|");
        }

        gp_test("#: Alternative form");
        {
            pf_sprintf(buf,  "%#x", 0);
            sprintf(buf_std, "%#x", 0);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%#.3X", 0xa);
            sprintf(buf_std, "%#.3X", 0xa);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%#.f", 1.0);
            sprintf(buf_std, "%#.f", 1.0);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%#g", 700.1);
            sprintf(buf_std, "%#g", 700.1);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%#g", 123456.0);
            sprintf(buf_std, "%#g", 123456.0);
            expect_str(buf, buf_std);
        }
    } // gp_suite("Flags");

    gp_suite("Fields");
    {
        gp_test("Basic field");
        {
            pf_sprintf(buf, "|%#8x|", 0x3);
            expect_str(buf, "|     0x3|");
        }
    }

    gp_suite("Misc");
    {
        gp_test("Return value");
        {
            unsigned chars_written = 0;
            unsigned chars_written_std = 0;

            chars_written     = pf_sprintf(buf,  "%s blah", "bloink");
            chars_written_std = sprintf(buf_std, "%s blah", "bloink");
            gp_expect(chars_written == chars_written_std,
                (chars_written), (chars_written_std));

            chars_written     = pf_sprintf(buf,  "blah %g", -2./9.);
            chars_written_std = sprintf(buf_std, "blah %g", -2./9.);
            gp_expect(chars_written == chars_written_std,
                (buf), (buf_std),
                (chars_written), (chars_written_std));
        }

        gp_test("Combinations");
        {
            pf_sprintf(buf,  "blah %f, %#0x", .5, 0x2);
            sprintf(buf_std, "blah %f, %#0x", .5, 0x2);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "%.3s, %+4i", "bloink", 63);
            sprintf(buf_std, "%.3s, %+4i", "bloink", 63);
            expect_str(buf, buf_std);

            pf_sprintf(buf,  "% 04i", 21);
            sprintf(buf_std, "% 04i", 21);
            expect_str(buf, buf_std);
        }

        gp_test("No format specifier");
        {
            pf_sprintf(buf, "Whatever");
            expect_str(buf, "Whatever");
        }

        gp_test("%%");
        {
            pf_sprintf(buf, "%% blah");
            expect_str(buf, "% blah");

            pf_sprintf(buf, "blah %%");
            expect_str(buf, "blah %");

            pf_sprintf(buf, "bl%%ah");
            expect_str(buf, "bl%ah");
        }
    } // gp_suite("Misc");

    gp_suite("Fuzz test");
    {
        // Seed RNG with date
        {
            time_t t = time(NULL);
            struct tm* gmt = gmtime(&t);
            gp_assert(gmt != NULL);
            g_rs = gp_random_state(
                gmt->tm_mday + 100*gmt->tm_mon + 10000*gmt->tm_year + FUZZ_SEED_OFFSET);
        }
        const unsigned loop_count = FUZZ_COUNT;
        const char* random_format(char conversion_type);

        gp_test("Random formats with random values");
        {
            for (unsigned iteration = 1; iteration <= loop_count; iteration++)
            {
                uintmax_t random_bytes;
                gp_random_bytes(&g_rs, &random_bytes, sizeof random_bytes);

                const char* all_specs = "diouxXeEfFgGcsp"; // exept unsupported
                                                           // 'n' and 'S' that
                                                           // differs from glibc
                                                           // snprintf().
                const char random_specifier =
                    all_specs[gp_random_range(&g_rs, 0, strlen(all_specs))];
                const char* fmt = random_format(random_specifier);
                uint32_t size = gp_random_range(&g_rs, 0, sizeof(buf));

                int _my_buf_return_value = 0;
                int buf_std_return_value = 0;

                // The important part is to pass a right sized argument, the
                // actual type isn't important.
                if (random_specifier == 's') // treat random_bytes as string
                {
                    ((char*)&random_bytes)[sizeof(uintmax_t) - 1] = '\0';
                    _my_buf_return_value = pf_snprintf(
                        buf, size, fmt, &random_bytes);
                    buf_std_return_value = snprintf(
                        buf_std, size, fmt, &random_bytes);
                }
                else if (random_specifier == 'c')
                {
                    _my_buf_return_value = pf_snprintf(
                        buf, size, fmt, (char)random_bytes);
                    buf_std_return_value = snprintf(
                        buf_std, size, fmt, (char)random_bytes);
                }
                else if (random_specifier == 'p') // pointer
                {
                    _my_buf_return_value = pf_snprintf(
                        buf, size, fmt, (intptr_t)random_bytes);
                    buf_std_return_value = snprintf(
                        buf_std, size, fmt, (intptr_t)random_bytes);
                }
                else if (strchr("eEfFgG", random_specifier) != NULL) // float
                {
                    union { uint64_t u; double f; } punner = {.u = random_bytes };
                    _my_buf_return_value = pf_snprintf(
                        buf, size, fmt, punner.f);
                    buf_std_return_value = snprintf(
                        buf_std, size, fmt, punner.f);
                }
                else // integer
                {
                    size_t len = strlen(fmt);
                    if (len >= 3 && fmt[len - 3] == 'h')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (char)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (char)random_bytes);
                    }
                    else if (fmt[len - 2] == 'h')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (short)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (short)random_bytes);
                    }
                    else if (len >= 3 && fmt[len - 3] == 'l')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (long long)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (long long)random_bytes);
                    }
                    else if (fmt[len - 2] == 'l')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (long)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (long)random_bytes);
                    }
                    else if (fmt[len - 2] == 'j')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (uintmax_t)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (uintmax_t)random_bytes);
                    }
                    else if (fmt[len - 2] == 'z')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (size_t)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (size_t)random_bytes);
                    }
                    else if (fmt[len - 2] == 't')
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (ptrdiff_t)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (ptrdiff_t)random_bytes);
                    }
                    else // no modifier
                    {
                        _my_buf_return_value = pf_snprintf(
                            buf, size, fmt, (int)random_bytes);
                        buf_std_return_value = snprintf(
                            buf_std, size, fmt, (int)random_bytes);
                    }
                }

                // Rename buf for aligned gp_assert() message
                const char* _my_buf = buf;
                gp_assert(strcmp(buf, buf_std) == 0,
                    fmt,
                    size,
                    "%#jx", random_bytes,
                    _my_buf,
                    buf_std,
                    iteration);

                gp_assert(_my_buf_return_value == buf_std_return_value,
                    fmt,
                    size,
                    _my_buf_return_value,
                    buf_std_return_value,
                    iteration);
            }
        }
    }

    // -------- INTERNAL ----------------- //

    gp_suite("PFString");
    {
        gp_test("insert_pad");
        {
            PFString str =
            {
                .data = (char[64]){"SomeData"},
                .length   = strlen("SomeData"),
                .capacity = strlen("SomeData"),
            };
            unsigned ret_val = pf_insert_pad(&str, 4, 'X', 3);
            expect_str(str.data, "SomeXXXD");
            gp_expect(ret_val == 0, (ret_val));
        }
    }
}

bool coin_flip()
{
    static uint32_t bits;
    if (bits == 0)
        bits = gp_random(&g_rs);
    bool result = bits & 1;
    bits >>= 1;
    return result;
}

const char* random_format(char conversion_type)
{
    static size_t fmt_capacity = 128;
    static char* fmt;
    static bool initialized = false;

    if ( ! initialized)
    {
        fmt = malloc(fmt_capacity);
        initialized = true;
    }
    memset(fmt, 0, fmt_capacity);
    size_t fmt_i = 0;

    #define push_char(c) do \
    { \
        fmt[fmt_i] = (c); \
        ++fmt_i; \
        if (fmt_i >= fmt_capacity - sizeof("x")) \
        { \
            char* new_buf = calloc(2, fmt_capacity); \
            gp_assert(new_buf != NULL); \
            memcpy(new_buf, fmt, fmt_capacity); \
            free(fmt); \
            fmt = new_buf; \
            fmt_capacity *= 2; \
        } \
    } while (0);

    push_char('%');

    char flags[8] = "-"; // dash is common for all
    switch (conversion_type)
    {
        // signed conversions
        case 'd': case 'i':
        case 'f': case 'F':
        case 'e': case 'E':
        case 'g': case 'G':
            strcat(flags, "0 +");
            if ( ! (conversion_type == 'd' || conversion_type == 'i'))
                strcat(flags, "#");
            break;

        // unsinged conversions
        case 'o': case 'u': case 'x': case 'X':
            strcat(flags, "#0");
            break;

        case 'c': case 's': case 'p':
            break;

        default:
            strcpy(fmt, "Invalid conversion format character!");
            return fmt;
    }

    while (coin_flip()) // add random flags
    {
        push_char(flags[gp_random_range(&g_rs, 0, strlen(flags))]);
    }

    if (coin_flip()) // add random field width
    {
        push_char(gp_random_range(&g_rs, 0, 9) + '1');
        if (coin_flip())
            push_char(gp_random_range(&g_rs, 0, 10) + '0');
            // no need to go past 100
    }

    if (coin_flip() && conversion_type != 'c') // add random precision
    {
        push_char('.');
        push_char(gp_random_range(&g_rs, 0, 9) + '1');
        if (coin_flip())
            push_char(gp_random_range(&g_rs, 0, 10) + '0');
    }

    // Add random length modifier but only for integers. This is because most of
    // the other ones are undefined or not well supported.
    if (coin_flip() && strchr("diouxX", conversion_type) != NULL)
    {
        // The capital ones are for convinience and will be turned into "hh"
        // and "ll" respectively. 'z' will be turned into 't' if signed.
        const char modifiers[] = "hHlLhz"; // not including extensions, we
                                           // compare against glibc snprintf()
        const char modifier = modifiers[gp_random_range(&g_rs, 0, strlen(modifiers))];
        if (modifier == 'H')
        {
            push_char('h');
            push_char('h');
        }
        else if (modifier == 'L')
        {
            push_char('l');
            push_char('l');
        }
        else if (modifier == 'z' &&
            (conversion_type == 'd' || conversion_type == 'i'))
        {
            push_char('t');
        }
        else
        {
            push_char(modifier);
        }
    }

    fmt[fmt_i] = conversion_type;
    fmt[fmt_i + 1] = '\0';
    return fmt;
}
