#define GPC_IMPLEMENTATION
#include "../build/gpc.h"
#include <x86intrin.h>

#define MEASURE(RESULT, FUNC) do { \
    T t_rdtsc = __rdtsc(); \
      t_rdtsc = __rdtsc() - t_rdtsc; \
    T t0 = __rdtsc(); \
    FUNC; \
    T t1 = __rdtsc(); \
    RESULT += t1 - t0 - t_rdtsc; \
} while (0)

typedef volatile uint64_t T;
#define ITERS (10 << 10)

int main(void)
{
    GPRandomState rs = gp_new_random_state((uint64_t)(uintptr_t)malloc(1));

    T t_std_f01_g_total = 0;
    T t_gp_f01_g_total  = 0;

    T t_std_f01_f_total = 0;
    T t_gp_f01_f_total  = 0;

    T t_std_fbits_g_total = 0;
    T t_gp_fbits_g_total  = 0;

    T t_std_fbits_f_total = 0;
    T t_gp_fbits_f_total  = 0;

    T t_std_u_total = 0;
    T t_gp_u_total  = 0;

    for (size_t i = 0; i < ITERS; i++)
    {
        char std_buf[1024] = "";
        char  gp_buf[1024] = "";
        double f01 = gp_frandom(&rs);
        uint32_t us[2] = { gp_random(&rs), gp_random(&rs) };
        double fbits;
        memcpy(&fbits, us, sizeof fbits);

        MEASURE(t_std_f01_g_total, snprintf(std_buf, sizeof std_buf, "%g",  f01));
        MEASURE(t_gp_f01_g_total, gp_bytes_n_print(gp_buf, sizeof gp_buf, f01));
        gp_assert(gp_bytes_equal(std_buf, strlen(std_buf), gp_buf, strlen(gp_buf)),
            std_buf, gp_buf, "[%x,%x]", us[0], us[1]);

        MEASURE(t_std_f01_f_total,        snprintf(std_buf, sizeof std_buf, "%f", f01));
        MEASURE(t_gp_f01_f_total, gp_bytes_n_print(gp_buf, sizeof gp_buf, "%f", f01));
        gp_assert(gp_bytes_equal(std_buf, strlen(std_buf), gp_buf, strlen(gp_buf)),
            std_buf, gp_buf, "[%x,%x]", us[0], us[1]);

        MEASURE(t_std_fbits_g_total, snprintf(std_buf, sizeof std_buf, "%g",  fbits));
        MEASURE(t_gp_fbits_g_total, gp_bytes_n_print(gp_buf, sizeof gp_buf, fbits));
        gp_assert(gp_bytes_equal(std_buf, strlen(std_buf), gp_buf, strlen(gp_buf)),
            std_buf, gp_buf, "[%x,%x]", us[0], us[1]);

        MEASURE(t_std_fbits_f_total,        snprintf(std_buf, sizeof std_buf, "%f", fbits));
        MEASURE(t_gp_fbits_f_total, gp_bytes_n_print(gp_buf, sizeof gp_buf, "%f", fbits));
        gp_assert(gp_bytes_equal(std_buf, strlen(std_buf), gp_buf, strlen(gp_buf)),
            std_buf, gp_buf, "[%x,%x]", us[0], us[1]);

        MEASURE(t_std_u_total, snprintf(std_buf, sizeof std_buf, "%u",  us[0]));
        MEASURE(t_gp_u_total, gp_bytes_n_print(gp_buf, sizeof gp_buf, us[0]));
        gp_assert(gp_bytes_equal(std_buf, strlen(std_buf), gp_buf, strlen(gp_buf)),
            std_buf, gp_buf, "[%x,%x]", us[0], us[1]);
    }
    gp_print("                                                  \r");

    gp_println("Random double between 0 and 1 using %%g");
    gp_println("std:", (double)t_std_f01_g_total / ITERS);
    gp_println("gp: ", (double)t_gp_f01_g_total  / ITERS);
    gp_println("gp / std:", (double)t_std_f01_g_total / (double)t_gp_f01_g_total, "\n");

    gp_println("Random double between 0 and 1 using %%f");
    gp_println("std:", (double)t_std_f01_f_total / ITERS);
    gp_println("gp: ", (double)t_gp_f01_f_total  / ITERS);
    gp_println("gp / std:", (double)t_std_f01_f_total / (double)t_gp_f01_f_total, "\n");

    gp_println("Double with random bits using %%g");
    gp_println("std:", (double)t_std_fbits_g_total / ITERS);
    gp_println("gp: ", (double)t_gp_fbits_g_total  / ITERS);
    gp_println("gp / std:", (double)t_std_fbits_g_total / (double)t_gp_fbits_g_total, "\n");

    gp_println("Double with random bits using %%f");
    gp_println("std:", (double)t_std_fbits_f_total / ITERS);
    gp_println("gp: ", (double)t_gp_fbits_f_total  / ITERS);
    gp_println("gp / std:", (double)t_std_fbits_f_total / (double)t_gp_fbits_f_total, "\n");

    gp_println("Unsigned integer");
    gp_println("std:", (double)t_std_u_total / ITERS);
    gp_println("gp: ", (double)t_gp_u_total  / ITERS);
    gp_println("gp / std:", (double)t_std_u_total / (double)t_gp_u_total, "\n");
}
