#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "parareal.h"
#include "tests.h"
#include "aux.h"

extern double g_tic;    // global time reference point
FILE* gtimings;
extern FILE* gtimings;  // global timings filedescriptor

void graceful_death(int s) {
    if (gtimings)
        fclose(gtimings);
    exit(-1);
}

int main(int argc, char** argv) {
#if DBMAIN_TESTS
    if (run_tests()) {
        printf("Tests failed.\n");
        return -1;
    }
#endif
    if (argc < 2) {
        printf("Usage: %s num_threads [coarse-steps-per-interval] [parareal-iters]\n", argv[0]);
        return -1;
    }

    int num_threads = 1;
    if (argc >= 2)
        num_threads = atoi(argv[1]);
    num_threads = num_threads > 0 ? num_threads : 1;    // cant run with 0 threads
                                                        // (main is also one)
    int ncoarse = 1<<9;
    if (argc >= 3)
        ncoarse = atoi(argv[2]);
    int piters = 2;
    if (argc >= 4)
        piters = atoi(argv[3]);
    if (piters > num_threads) {
        // TODO num_threads or num_threads-1?
        printf("Warning: Parareal converges after at most %d p-iterations with %d threads\n", num_threads, num_threads);
    }

    signal(SIGINT, graceful_death);  // exit gracefully on SIGINT

    struct timespec w_tic;
    double tic;
    g_tic = tic();  // start global timer

    const int pwork = 1<<10;  // Total work to be parallelized
    int nfine   = round(pwork/num_threads);
    assert(fabs(nfine*num_threads - pwork) < 1e-15 && "Parallel Work not dividable by num_threads");

    double y_0 = 1;
    double t_start = 0;
    double t_end   = 1;
    double slice = (t_end - t_start)/num_threads;
    double t;

    //double hc = slice/ncoarse;    // unused
    double hf = slice/nfine;

    tic();
    double* y_res = parareal_omp(t_start, t_end, ncoarse, nfine, num_threads, y_0, fw_euler_step, rk4_step, f_id, piters);
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
        printf("Error[%d] : %.2e\n", i, tmp*tmp);
        t += slice;
    }

    double speedup = time_serial/time_para;
    printf("Parareal l2error: %.2e, last step: %.2e (res: %f, sol: %f)\n", l2err, tmp, y_res[num_threads], exp(t-slice));
    printf("Threads: %d, total fine integrator steps: %d, coarse steps (per slice): %d\n", num_threads, pwork, ncoarse);
    printf("Times: parar %.2fs, rk4 %.2fs\n", time_para, time_serial);
    printf("Speedup: %.2f, Efficiency: %.2f\n", speedup, 
            speedup/num_threads);

    write2file(t_start, slice, num_threads+1, y_res);
    gnuplot();

    free(y_res);

    return 0;
}
