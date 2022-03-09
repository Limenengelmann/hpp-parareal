#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "parareal.h"
#include "tests.h"

double g_tic = 0;   // global time reference point
extern double g_tic;

int main(int argc, char** argv) {
#if DBMAIN_TESTS
    if (run_tests()) {
        printf("Tests failed.\n");
        return -1;
    }
#endif

    int num_threads = 4;
    int ncoarse = 1<<9;
    if (argc >= 2)
        num_threads = atoi(argv[1]);
    if (argc >= 3)
        ncoarse = atoi(argv[2]);

    struct timespec w_tic;
    double tic;
    g_tic = tic();  // start global timer

    int work = 1<<10;
    int nfine   = round(work/num_threads);
    assert(fabs(nfine*num_threads - work) < 1e-15 && "Work not dividable by num_threads");

    double y_0 = 1;
    double t_start = 0;
    double t_end   = 1;
    double slice = (t_end - t_start)/num_threads;
    double t;

    double hc = slice/ncoarse;
    double hf = slice/nfine;

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

    double speedup = time_serial/time_para;
    printf("Parareal l2error: %.2e, last step: %.2e (res: %f, sol: %f)\n", l2err, tmp, y_res[num_threads], exp(t-slice));
    printf("Times: parar %.2fs, rk4 %.2fs\n", time_para, time_serial);
    printf("Speedup: %.2f, Efficiency: %.2f\n", time_serial/time_para, 
            speedup/num_threads);

    //write2file(t_start, hc, num_threads+1, y_res);
    //gnuplot();

    free(y_res);

    return 0;
}
