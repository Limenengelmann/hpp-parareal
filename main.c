#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "parareal.h"
#include "tests.h"

#define tic() (tic = (clock_gettime(CLOCK_REALTIME, &w_tic) == 0 /*assert that call succeeded*/ ) * ((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f))
// return time since tic()
#define toc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - tic)

int main(int argc, char** argv) {
#if DBMAIN_TESTS
    if (run_tests()) {
        printf("Tests failed.\n");
        return -1;
    }
#endif
    struct timespec w_tic;
    double tic;

    double t;

    int num_threads = 4;
    if (argc >= 2)
        num_threads = atoi(argv[1]);

    double hc = 1./(1 <<  3);
    double hf = 1./(1 << 11);

    double y_0 = 1;
    double t_start = 0;
    double t_end   = 1;

    double slice = (t_end - t_start)/num_threads;
    int ncoarse = round(slice / hc);
    int nfine   = round(slice / hf);
    assert(fabs(ncoarse*hc - slice) < 1e-15 && "Stepsize not compatible with slize size!");
    assert(fabs(  nfine*hf - slice) < 1e-15 && "Stepsize not compatible with slize size!");

    tic();
    double* y_res = parareal(t_start, t_end, ncoarse, nfine, num_threads, y_0, fw_euler_step, rk4_step, f_id);
    double time_para = toc();

    t = t_start;
    double y_t = y_0;
    tic();
    for(int i=0; i<num_threads*nfine; i++) {
        y_t = rk4_step(t, y_t, hf, f_id);
        t += hf;
    }
    printf("Rk4 error: %.2e (res: %f, sol: %f\n", fabs(y_t-exp(t)), y_t, exp(t));
    double time_serial = toc();

    double l2err = 0;
    double tmp = -1;
    t = t_start;
    for (int i=0; i<num_threads+1; i++) {
        tmp = fabs(y_res[i] - exp(t));
        l2err += tmp*tmp;
        t += slice;
    }
    write2file(t_start, slice, num_threads+1, y_res);

    printf("Parareal l2error: %.2e, last step: %.2e (res: %f, sol: %f\n", l2err, tmp, y_res[num_threads], exp(t-slice));
    printf("Times: parar %.2fs, rk4 %.2fs\n", time_para, time_serial);
    printf("Speedup: %.2f, Efficiency: %.2f\n", time_serial/time_para, 
            (time_serial/time_para)/num_threads);

    //write2file(t_start, hc, num_threads+1, y_res);
    //gnuplot();

    free(y_res);

    return 0;
}
