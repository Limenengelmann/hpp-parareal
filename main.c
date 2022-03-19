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

    double *y_res     = (double*) malloc((num_threads+1)*sizeof(double)); // current solution
    double *y_res_pt  = (double*) malloc((num_threads+1)*sizeof(double)); // current solution
    double *y_res_omp = (double*) malloc((num_threads+1)*sizeof(double)); // current solution

    /*
     * Benchmark start
     */
    tic();
    parareal(t_start, t_end, ncoarse, nfine, num_threads, y_0, y_res, fw_euler_step, rk4_step, f_id, piters);
    double time_para = toc();

    tic();
    parareal_pthread(t_start, t_end, ncoarse, nfine, num_threads, y_0, y_res_pt, fw_euler_step, rk4_step, f_id, piters);
    double time_para_pt = toc();

    tic();
    parareal_omp(t_start, t_end, ncoarse, nfine, num_threads, y_0, y_res_omp, fw_euler_step, rk4_step, f_id, piters);
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
    double l2err_pt = 0;
    double l2err_omp = 0;
    double tmp = -1;
    t = t_start;
    for (int i=0; i<num_threads+1; i++) {
        tmp = fabs(y_res[i] - sol_id(t));
        l2err += tmp*tmp;
        tmp = fabs(y_res_pt[i] - sol_id(t));
        l2err_pt += tmp*tmp;
        tmp = fabs(y_res_omp[i] - sol_id(t));
        l2err_omp += tmp*tmp;
        DPRINTF(1, "Error[%d] : %.2e\n", i, tmp);
        t += slice;
    }

    double speedup = time_serial/time_para;
    double speedup_pt = time_serial/time_para_pt;
    double speedup_omp = time_serial/time_para_omp;
    printf("%d, %d, %d, ", num_threads, ncoarse, piters);
    printf("%.2e, %.2e, %.2e, ", l2err, l2err_pt, l2err_omp);
    printf("%.2f, %.2f, %.2f, ",   time_para, speedup, speedup/num_threads);
    printf("%.2f, %.2f, %.2f, ",   time_para_pt, speedup_pt, speedup_pt/num_threads);
    printf("%.2f, %.2f, %.2f\n", time_para_omp, speedup_omp, speedup_omp/num_threads);

#if DBPARAPLOT
    write2file(t_start, slice, num_threads+1, y_res);
    gnuplot();
#endif

    free(y_res);
    free(y_res_pt);
    free(y_res_omp);

    return 0;
}
