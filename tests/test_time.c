// MIT License
// Copyright (c) 2023 Lauri Lorenzo Fiestas
// https://github.com/PrinssiFiestas/libGPC/blob/main/LICENSE.md

// This test just wastes time and is trivial, so just run them on changes.
//#ifndef GP_TIME_TESTS
#if 0

int main(void) {}

#else

#include "../src/time.c"
#include <gpc/assert.h>
#include <gpc/io.h>
#include <signal.h>

// Note: timing is inherently inaccurate, but it is not as inaccurate as the
// ranges used in the tests might imply. The chosen ranges are more or less
// arbitrary.

void handle_signal(int _) { (void)_; }

int interrupt_sleep(void*_)
{
    (void)_;
    gp_sleep_ns(1, 500llu*1000*1000);
    raise(SIGINT);
    return 0;
}

int main(void)
{
    gp_suite("Timing and Sleeping");
    {
        gp_test("Waste a millisecond");
        {
            GPUInt128 t0 = gp_time_begin_ns();
            int ret = gp_sleep_ns(0, 1000*1000);
            uint64_t t = gp_time_ns(&t0);
            gp_expect(1000*1000 < t && t < 1001*1000);
            gp_expect(ret == 0);
        }

        gp_test("Waste 10 milliseconds");
        {
            GPUInt128 t0 = gp_time_begin_ns();
            int ret = gp_sleep_ns(0, 5000*1000);
            ret +=    gp_sleep(.005);
            double   t_s  = gp_time(&t0);
            uint64_t t_ns = gp_time_ns(&t0);
            gp_expect(.010 < t_s && t_s < .01001);
            gp_expect(10llu*1000*1000 < t_ns && t_ns < 10llu*1000*1001);
            gp_expect(ret == 0);
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
            double t0 = gp_time(NULL);
            gp_expect(t0 < .0000001); // 0 on some systems, but not always
            gp_expect(gp_time(NULL) < .000001);

            // Note: you shouldn't use global time directly like we do here for
            // testing purposes. Global time is initialized once and cannot be
            // changed making it useless by itself. Always use deltas (t1 - t0)
            // or pass reference time as parameter to gp_time() to gp_time_ns()
            // when timing. The only reason why global time exists is that time
            // since epoch yields a completely inaccurate and unusable floating
            // point value.
            gp_sleep(.1);
            double t = gp_time(NULL);
            gp_expect(.1 < t && t < .1001, t);

        }

        gp_test("Countdown");
        {
            double t0 = gp_time(NULL);

            for (time_t t = 5; t != 0; --t) {
                gp_print(t, "\r");
                fflush(stdout);
                if (t & 1)
                    gp_sleep(1.);
                else
                    gp_sleep_ns(1, 0);
            }

            double t1 = gp_time(NULL);

            double t = t1 - t0;
            gp_expect(5. < t && t < 5.001, t);
        }
    } // gp_suite("Global Time");

    gp_suite("Signal Interrupt Sleep");
    {
        signal(SIGINT, handle_signal);
        GPThread interrupting_thread;
        gp_thread_create(&interrupting_thread, interrupt_sleep, NULL);

        time_t    s = 3;
        uint32_t ns = 0;
        int ret = gp_sleep_signal_ns(&s, &ns);
        gp_expect(ret == -1); // got signal
        gp_expect(s == 1);
        gp_expect(450llu*1000*1000 < ns && ns < 500llu*1000*1000, ns);

        gp_thread_join(interrupting_thread, NULL);
    }

    // TODO remove this after timing implemented to unit testing framework.
    gp_expect(0);
}

#endif
