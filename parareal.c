#include "parareal.h"

extern double g_tic;    // global time reference point

inline double fw_euler_step(double t, double y_t, double h, rhs_func f) {
    return y_t + h*f(t, y_t);
}

inline double rk4_step(double t, double y_t, double h, rhs_func f) {
    double k1 = f(t, y_t);
    double k2 = f(t+h/2, y_t + h*k1/2);
    double k3 = f(t+h/2, y_t + h*k2/2);
    double k4 = f(t+h, y_t+h*k3);
    return y_t + h/6*(k1 + 2*k2 + 2*k3 + k4);
}

typedef struct task_data {
    double t0, y0, hc, hf;
    double *y_next;
    int nfine, ncoarse, id, piters;
    volatile char *progress;    // pointer to volatile char
    rhs_func f;
    singlestep_func fine;
    singlestep_func coarse;
    FILE* timings;
} task_data;

void* task(void* args) {
    struct timespec w_tic;
    double tic = gtoc();
    task_data* td = (task_data*) args;

    double y_coarse, y_fine, y_next;
    double hc = td->hc;
    double hf = td->hf;

    // generate initial value
    double t = td->t0;
    double y0 = td->y0;
    for(int j=0; j < td->id*td->ncoarse; j++) {
        y0 = td->coarse(t, y0, hc, td->f);
        t += hc;
    }
    addTime2Plot(td->timings, td->id, tic, gtoc());

    for (int K=0; K < td->piters; K++) {
        tic = gtoc();
        // parallel coarse step
        y_coarse = y0;
        t = td->t0;
        for (int i=0; i < td->ncoarse; i++) {
            y_coarse = td->coarse(t, y_coarse, hc, td->f);
            t += hc;
        }

        // parallel fine step
        y_fine = y0;
        t = td->t0;
        for (int i=0; i < td->nfine; i++) {
            y_fine = td->fine(t, y_fine, hf, td->f);
            t += hf;
        }
        addTime2Plot(td->timings, td->id, tic, gtoc());

        // FIXME race condition for 8 threads 85 steps 3 iterations
        // provoke racing conditions or lapping

        // wait for previous thread to finish iteration and update y_next
        DPRINTF(DBTHREADS, "#%d(K%d): Waiting for #%d to reach iter %d\n", td->id, K, td->id-1, K+1);
        while (td->progress[td->id-1] < K+1);
        DPRINTF(DBTHREADS, "#%d(K%d): Finished waiting, continuing.\n", td->id, K);
        //if (td->id == 7) sleep(1);  // -> threads lap thread 7
        
        tic = gtoc();
        // serial coarse propagation step + parareal update
        y_next = td->y_next[td->id];    // TODO guarantee y_next is uptodate (sync)
        y0 = y_next;                   // update next starting value
        t = td->t0;
        for (int i=0; i<td->ncoarse; i++) {
            y_next = td->coarse(t, y_next, hc, td->f);
            t += hc;
        }

        // parareal update
        // TODO just accumulate result directly in y_next?
        y_next = y_next + y_fine - y_coarse;
        td->y_next[td->id+1] = y_next;
        td->progress[td->id]++;
        DPRINTF(DBTHREADS, "#%d(K%d): Iter done. Start iter %d.\n", td->id, K, td->progress[td->id]);

        addTime2Plot(td->timings, td->id, tic, gtoc());
    }
    return NULL;
}

// TODO plot idea: speedup vs relative serial work/total work
// TODO proper malloc/free symmetry by passing buffers as arguments
// TODO add numthreads as arg
extern FILE* gtimings;
double* parareal(double start, double end, int ncoarse, int nfine, int num_threads,
        double y0, singlestep_func coarse, singlestep_func fine, rhs_func f, int piters) {

    // init time measurements
    char fname[128];
    sprintf(fname, "outdata/timings.t%d.sw%d.K%d.data", num_threads, ncoarse, piters);
    FILE* timings = fopen(fname, "w");
    if (! timings) {
        perror("Couldn't open timings.data");
        return NULL;
    }
    gtimings = timings;

    int id = 0;  // main thread #0
    struct timespec w_tic;
    double tic = gtoc();

    double slice = (end-start)/num_threads;
    double hc = slice/ncoarse;
    double hf = slice/nfine;
    double y_coarse, y_fine, y_new_coarse;
    // +1 to account for y0
    double *y_next  = (double*) malloc((num_threads+1)*sizeof(double)); // current solution

    // cheap "semaphores"
    volatile char progress[num_threads];     // signals which iteration a thread is at
    for (int i=0; i<num_threads; i++)
        progress[i] = 0;

    y_next[0]  = y0;

    pthread_t threads[num_threads];
    task_data td[num_threads];

    // TODO measure total waiting time (downtime) by increasing a counter
    // TODO how to give different colors for different timings
    double t = start+slice;
    for(int i = 1; i < num_threads; i++) {
        // TODO pass y_fine pointer, then write result directly ?
        // how to slim this down ? only t and y change per iteration
        td[i].t0 = t;
        td[i].y0 = y0;      // obsolete, same as y_next[0]
        td[i].y_next = y_next;
        td[i].hf = hf;
        td[i].hc = hc;
        td[i].nfine   = nfine;
        td[i].ncoarse = ncoarse;
        td[i].f = f;
        td[i].fine   = fine;
        td[i].coarse = coarse;
        td[i].id = i;
        td[i].piters = piters;
        td[i].progress = progress;
        td[i].timings = timings;
        pthread_create(threads+i, NULL, task, (void*) &td[i]);
        t += slice;
    }
    addTime2Plot(timings, id, tic, gtoc());

    // parareal iteration
    for (int K=0; K<piters; K++) {
        tic = gtoc();
        t = start;
        y_coarse = y0;
        for(int i=0; i < ncoarse; i++) {
            y_coarse = coarse(t, y_coarse, hc, f);
            t += hc;
        }

        t = start;
        y_fine = y0;
        for (int i=0; i<nfine; i++) {
            y_fine = fine(t, y_fine, hf, f);
            t += hf;
        }
        addTime2Plot(timings, id, tic, gtoc());
        // wait for last thread to at least finish the previous iteration
        // to keep threads "together" and prevent them from lapping each other
        // TODO test via variable runtime lengths dep on id?
        DPRINTF(DBTHREADS, "#%d(K%d): Waiting for #%d to reach iter %d\n", id, K, num_threads-1, K);
        while (progress[num_threads-1] < K);
        DPRINTF(DBTHREADS, "#%d(K%d): Finished waiting, continuing.\n", id, K);

        tic = gtoc();
        t = start;
        y_new_coarse = y0;     // = y_next[0]
        for (int i=0; i<ncoarse; i++) {
            y_new_coarse = coarse(t, y_new_coarse, hc, f);
            t += hc;
        }
        // parareal update
        // TODO just accumulate result directly in y_next?
        y_next[1] = y_new_coarse + y_fine - y_coarse;
        progress[0]++;
        DPRINTF(DBTHREADS, "#%d(K%d): Iter done. Start iter %d.\n", id, K, progress[id]);

        addTime2Plot(timings, id, tic, gtoc());
    }

    for (int i=1; i<num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(timings);
    return y_next;
}
