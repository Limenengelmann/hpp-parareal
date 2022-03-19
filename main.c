#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "parareal.h"
#include "funcs.h"
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
        //printf("Warning: Parareal converges after at most %d p-iterations with %d threads\n", num_threads, num_threads);
    }

    signal(SIGINT, graceful_death);  // exit gracefully on SIGINT

    // Timing variables
    struct timespec w_tic;
    double tic;
    g_tic = tic();  // start global timer

    // Algorithm variables
    const int pwork = 1<<10;        // total work to be parallelized
    int nfine       = round(pwork/num_threads);
    assert(fabs(nfine*num_threads - pwork) < 1e-15 && "Parallel Work not dividable by num_threads");
    double y_0      = 1;            // initial value
    double t_start  = 0;            // start time
    double t_end    = 1;            // end time
    double slice    = (t_end - t_start)/num_threads;
    double t;                       // time variable
    double hf       = slice/nfine;  // fine-integrator stepsize                  


    /*
     * Benchmark start
     */
    tic();
    double* y_res = parareal(t_start, t_end, ncoarse, nfine, num_threads, y_0, fw_euler_step, rk4_step, f_id, piters);
    double time_para = toc();

    tic();
    double* y_res_omp = parareal_omp(t_start, t_end, ncoarse, nfine, num_threads, y_0, fw_euler_step, rk4_step, f_id, piters);
    double time_para_omp = toc();

    // serial solve with fine integrator
    t = t_start;
    double y_t = y_0;
    tic();
    for(int i=0; i<num_threads*nfine; i++) {
        y_t = rk4_step(t, y_t, hf, f_id);
        t += hf;
    }
    double time_serial = toc();

    // calculate l2 errors
    double l2err = 0;
    double l2err_omp = 0;
    double tmp = -1;
    t = t_start;
    for (int i=0; i<num_threads+1; i++) {
        tmp = fabs(y_res[i] - exp(t));
        l2err += tmp*tmp;
        tmp = fabs(y_res_omp[i] - exp(t));
        l2err_omp += tmp*tmp;
        DPRINTF(1, "Error[%d] : %.2e\n", i, tmp);
        t += slice;
    }

    double speedup = time_serial/time_para;
    double speedup_omp = time_serial/time_para_omp;
    printf("Threads: %d, K: %d, total fine steps: %d, coarse steps (per slice): %d\n",
            num_threads, piters, pwork, ncoarse);
    printf("Errors: Rk4      last step: %.2e (res: %f, sol: %f\n", 
            fabs(y_t-exp(t_end)), y_t, exp(t_end));
    printf("        Pthreads last step: %.2e (res: %f, sol: %f), l2error: %.2e\n",
            fabs(y_res[num_threads] - exp(t_end)), y_res[num_threads], exp(t_end), l2err);
    printf("        OMP      last step: %.2e (res: %f, sol: %f), l2error: %.2e\n",
            fabs(y_res_omp[num_threads] - exp(t_end)), y_res_omp[num_threads], exp(t_end), l2err_omp);
    printf("Times: serial rk4 %.2fs\n", time_serial);
    printf("       pthread    %.2fs, speedup: %.2f, efficiency: %.2f\n", 
            time_para, speedup, speedup/num_threads);
    printf("       omp        %.2fs, speedup: %.2f, efficiency: %.2f\n", 
            time_para_omp, speedup_omp, speedup_omp/num_threads);

#if DBPARAPLOT
    write2file(t_start, slice, num_threads+1, y_res);
    gnuplot();
#endif

    free(y_res);
    free(y_res_omp);

    return 0;
}
