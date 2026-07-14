// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

#include "../src/time.c"

// This test just wastes time and is trivial, so just run them manually on changes.
#ifndef 0 // GP_TIME_TESTS // TODO once tested in both Linux and Windows, uncomment

int main(void) {}

#else

#include <gpc/assert.h>
#include <gpc/io.h>
#include <pthread.h>

// Note: timing is inherently inaccurate, but it is not as inaccurate as the
// ranges used in the tests might imply. Biggest source of inaccuracy in these
// tests come from gp_sleep().

int main(void)
{
    gp_suite("Timing and Sleeping");
    {
        gp_test("Waste a millisecond");
        {
            GPInt128 t0 = gp_time_begin();
            gp_sleep(.001);
            uint64_t t = gp_time_ns(&t0);
            gp_expect(1000*1000 < t && t < 1200*1000, t);
        }

        gp_test("Waste 10 milliseconds");
        {
            GPInt128 t0 = gp_time_begin();
            gp_sleep(.010);
            double   t_s  = gp_time(&t0);
            uint64_t t_ns = gp_time_ns(&t0);
            gp_expect(.010 < t_s && t_s < .012, t_s);
            gp_expect(10llu*1000*1000 < t_ns && t_ns < 12llu*1000*1000, t_ns);
        }
    } // gp_suite("Timing and Sleeping");

    // For best accuracy, global time should not be used, but it is convenient
    // and a lot of times good enough.
    gp_suite("Global Time");
    {
        gp_test("Init global time");
        {
            // First call to gp_time() or gp_time_ns() with NULL parameter
            // initializes global time.
            double t, t0 = gp_time(NULL);
            gp_expect(t0 < .00001, t0); // 0 on some systems, but not always
            t = gp_time(NULL);
            gp_expect(t < .00001, t);

            // Note: you shouldn't use global time directly like we do here for
            // testing purposes. Global time is initialized once and cannot be
            // changed making it useless by itself. Always use deltas (t1 - t0)
            // or pass reference time as parameter to gp_time() to gp_time_ns()
            // when timing. The only reason why global time exists is that time
            // since epoch yields a completely inaccurate and unusable floating
            // point value.
            gp_sleep(.1);
            t = gp_time(NULL);
            gp_expect(.1 < t && t < .11, t, t);

        }

        gp_test("Countdown");
        {
            double t0 = gp_time(NULL);

            for (time_t t = 5; t != 0; --t) {
                gp_print(t, "\r");
                fflush(stdout);
                gp_sleep(1.);
            }
            gp_print(" ");

            double t1 = gp_time(NULL);

            double t = t1 - t0;
            gp_expect(5. < t && t < 5.3, t);
        }
    } // gp_suite("Global Time");

    gp_suite("Absolute Time Sleep");
    {
        GPInt128 start = gp_time_begin();
        GPInt128 end = gp_i128_add(start, 10*1000*1000);
        gp_sleep_absolute(end);
        uint64_t t = gp_time_ns(&start);
        gp_expect(1000*1000 < t && t < 1200*1000, t);
    } // gp_suite("Absolute Time Sleep");
}

#endif
