#include "parareal.h"

extern double g_tic;    // global time reference point
extern FILE* gtimings;  // close open file on interrupt

static void* pipel_task(void* args) {
#if DBTIMINGS
    struct timespec w_tic;
    double tic = gtoc();
#endif
    pipel_task_data* td = (pipel_task_data*) args;

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
    DEBUG(DBRNDSLEEP_INI, SLEEPTIME(td->ncoarse));
    DEBUG(DBTIMINGS, addTime2Plot(td->timings, td->id, 0, tic, gtoc()));

    for (int K=0; K < td->piters; K++) {
        DEBUG(DBTIMINGS, tic = gtoc());
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
        DEBUG(DBRNDSLEEP_IND, SLEEPTIME(td->ncoarse));
        DEBUG(DBTIMINGS, addTime2Plot(td->timings, td->id, 1, tic, gtoc()));

        // wait for previous thread to finish iteration and update y_next
        DPRINTF(DBTHREADS, "#%d(K%d): Waiting for #%d to reach iter %d\n", td->id, K, td->id-1, K+1);
        while (td->progress[td->id-1] < K+1);
        DPRINTF(DBTHREADS, "#%d(K%d): Finished waiting, continuing.\n", td->id, K);
        
        DEBUG(DBTIMINGS, tic = gtoc());
        DEBUG(DBRNDSLEEP_DEP, SLEEPTIME(5*td->ncoarse));
        // serial coarse propagation step + parareal update
        y_next = td->y_next[td->id];
        y0 = y_next;                   // update next starting value
        t = td->t0;
        for (int i=0; i<td->ncoarse; i++) {
            y_next = td->coarse(t, y_next, hc, td->f);
            t += hc;
        }

        // parareal update
        y_next = y_next + y_fine - y_coarse;
        td->y_next[td->id+1] = y_next;
        td->progress[td->id]++;
        DPRINTF(DBTHREADS, "#%d(K%d): Iter done. Start iter %d.\n", td->id, K, td->progress[td->id]);

        DEBUG(DBTIMINGS, addTime2Plot(td->timings, td->id, 2, tic, gtoc()));
    }
    return NULL;
}

void parareal_pthread(double start, double end, int ncoarse, int nfine, int num_threads,
        double y0, double* y_next, singlestep_func coarse, singlestep_func fine, rhs_func f, int piters) {

    FILE* timings = NULL;
#if DBTIMINGS
    // init time measurements
    char fname[128];
    sprintf(fname, "outdata/timings.t%d.sw%d.K%d.data.pthread", num_threads, ncoarse, piters);
    timings = fopen(fname, "w");
    if (! timings) {
        perror("Couldn't open timings.data");
        exit(-1);
    }
    gtimings = timings;

    struct timespec w_tic;
    double tic; 
    DEBUG(DBTIMINGS, tic = gtoc());
#endif

    int id = 0;  // main thread #0
    double slice = (end-start)/num_threads;
    double hc = slice/ncoarse;
    double hf = slice/nfine;
    double y_coarse, y_fine, y_new_coarse;

    // cheap "semaphores"
    volatile char progress[num_threads];     // signals which iteration a thread is at
    for (int i=0; i<num_threads; i++)
        progress[i] = 0;

    // init y_next
    for (int i=0; i<num_threads; i++) {
        y_next[i]  = y0;
    }

    pthread_t threads[num_threads];
    pipel_task_data td[num_threads];

    double t = start+slice;
    for(int i = 1; i < num_threads; i++) {
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
        pthread_create(threads+i, NULL, pipel_task, (void*) &td[i]);
        t += slice;
    }
    DEBUG(DBTIMINGS, addTime2Plot(timings, id, 0, tic, gtoc()));

    // parareal iteration
    for (int K=0; K<piters; K++) {
        DEBUG(DBTIMINGS, tic = gtoc());
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
        DEBUG(DBRNDSLEEP_IND, SLEEPTIME(ncoarse));
        DEBUG(DBTIMINGS, addTime2Plot(timings, id, 1, tic, gtoc()));
        // TODO find out which disturbance has the largest effect
        // Answer: DEPENDENT part (in hindsight obviously)
        /*
        DPRINTF(DBTHREADS, "#%d(K%d): Waiting for #%d to reach iter %d\n", id, K, num_threads-1, K);
        while (progress[num_threads-1] < K-1);
        DPRINTF(DBTHREADS, "#%d(K%d): Finished waiting, continuing.\n", id, K);
        */

        DEBUG(DBTIMINGS, tic = gtoc());
        DEBUG(DBRNDSLEEP_DEP, SLEEPTIME(ncoarse));
        t = start;
        y_new_coarse = y0;     // = y_next[0]
        for (int i=0; i<ncoarse; i++) {
            y_new_coarse = coarse(t, y_new_coarse, hc, f);
            t += hc;
        }
        // parareal update
        y_next[1] = y_new_coarse + y_fine - y_coarse;
        progress[0]++;
        DPRINTF(DBTHREADS, "#%d(K%d): Iter done. Start iter %d.\n", id, K, progress[id]);

        DEBUG(DBTIMINGS, addTime2Plot(timings, id, 2, tic, gtoc()));
    }

    for (int i=1; i<num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    if (timings) fclose(timings);
}

void parareal_omp(double start, double end, int ncoarse, int nfine, int num_threads,
        double y0, double *y, singlestep_func coarse, singlestep_func fine, rhs_func f, int piters) {

    FILE* timings = NULL;
    struct timespec w_tic;
    double tic;

#if DBTIMINGS
    char fname[128];
    sprintf(fname, "outdata/timings.t%d.sw%d.K%d.data.omp", num_threads, ncoarse, piters);
    timings = fopen(fname, "w");
    if (! timings) {
        perror("Couldn't open timings.data");
        exit(-1);
    }
    tic = gtoc();
#endif

    double *yc = (double*) malloc((num_threads+1)*sizeof(double)); // coarse solution
    double *dy = (double*) malloc((num_threads+1)*sizeof(double)); // difference of above

    double slice = (end-start)/num_threads;
    double hc = slice/ncoarse;
    double hf = slice/nfine;

    for (int i=0; i<num_threads+1; i++) {
        y[i] = y0;
        yc[i] = y0;
        dy[i] = 0;
    }

    omp_lock_t locks[num_threads+1];    // +1 removes edge case 
                                        // for last thread locking before updating
    for (int i=0; i<num_threads+1; i++) {
        omp_init_lock(locks+i);
    }
    // TODO wrong results for T8 or T16 and K 10 with random sleep

    omp_set_num_threads(num_threads);
    int P = num_threads;
    #pragma omp parallel firstprivate(tic, w_tic)
    {
        #pragma omp for nowait schedule(static)
        for (int p=0; p<P; p++) {
            y[p] = y[0];
            double t = start;
            // TODO if cond can be removed
            if (p>0 /*not first thread*/) {
                for (int i=0; i<p*ncoarse; i++) {
                    y[p] = coarse(t, y[p], hc, f);
                    t += hc;
                }
            }
            yc[p] = y[p];
            for (int i=0; i<ncoarse; i++) {
                yc[p] = coarse(t, yc[p], hc, f);
                t+=hc;
            }
            DEBUG(DBRNDSLEEP_INI, SLEEPTIME(ncoarse));
            DEBUG(DBTIMINGS, addTime2Plot(timings, p, 0, tic, gtoc()));
        }

        for (int k=0; k<piters; k++) {
            //#pragma omp for ordered schedule(static)      // gives correct result
            #pragma omp for ordered nowait schedule(static) // sometimes wrong result
            for (int p=0; p<P; p++) {
                omp_set_lock(locks+p);
                DEBUG(DBTIMINGS, tic = gtoc());
                DPRINTF(DBTHREADS, "#%2d: Starting iter %d.\n", p, k);
                double t = slice*p;
                for (int i=0; i<nfine; i++) {
                    y[p] = fine(t, y[p], hf, f);
                    t += hf;
                }
                dy[p] = y[p] - yc[p];
                omp_unset_lock(locks+p);
                DPRINTF(DBTHREADS, "#%2d(K%2d): Fine steps done. (dy[%2d] = y[%2d] - yc[%2d])\n", p, k, p, p, p);
                // TODO move sleep into lock for fairer comparison
                DEBUG(DBRNDSLEEP_IND, SLEEPTIME(ncoarse));
                DEBUG(DBTIMINGS, addTime2Plot(timings, p, 1, tic, gtoc()));
                #pragma omp ordered
                {
                    DEBUG(DBTIMINGS, tic = gtoc());
                    DEBUG(DBRNDSLEEP_DEP, SLEEPTIME(ncoarse));
                    if (p == 0) {
                        omp_set_lock(locks);
                        y[0] = y0;
                        omp_unset_lock(locks);
                    }
                    t = slice*p;
                    yc[p] = y[p];
                    for (int i=0; i<ncoarse; i++) {
                        yc[p] = coarse(t, yc[p], hc, f);
                        t += hc;
                    }
                    DPRINTF(DBTHREADS, "#%2d(K%2d): Coarse updt done.(yc[%2d] <-    coarse(y[%2d] up-to-date)\n", p, k, p, p);
                    omp_set_lock(locks+p+1);
                    y[p+1] = yc[p] + dy[p];
                    DPRINTF(DBTHREADS, "#%2d(K%2d): Updated            y[%2d] = yc[%2d] + dy[%2d]. (yc[%2d], dy[%2d] now stale).\n", p, k, p+1, p, p, p+1, p+1);
                    DEBUG(DBTIMINGS, addTime2Plot(timings, p, 2, tic, gtoc()));
                    omp_unset_lock(locks+p+1);
                    DPRINTF(DBTHREADS, "#%2d(K%2d): iter done.\n", p, k);
                }
            }
        }
    }

    if (timings) fclose(timings);
    free(yc);
    free(dy);
}

static void* task(void* args) {
#if DBTIMINGS
    struct timespec w_tic;
    double tic = gtoc();
#endif
    task_data* td = (task_data*) args;
    double y_t = td->y_t;
    for (int j=0; j<td->nfine; j++) {
        y_t = td->fine(td->t, y_t, td->hf, td->f);
        td->t += td->hf;
    }
    td->y_t = y_t;
    DEBUG(DBTIMINGS, addTime2Plot(td->timings, td->id, 1, tic, gtoc()));
    return NULL;
}

void parareal(double start, double end, int ncoarse, int nfine, 
        int num_threads, double y_0, double* y_t_new,
        singlestep_func coarse, singlestep_func fine, 
        rhs_func f, int piters) {
    FILE* timings = NULL;
#if DBTIMINGS
    char fname[128];
    sprintf(fname, "outdata/timings.t%d.sw%d.K%d.data.pthread", num_threads, ncoarse, piters);
    timings = fopen(fname, "w");
    if (! timings) {
        perror("Couldn't open timings.data");
        exit(-1);
    }
    gtimings = timings;

    struct timespec w_tic;
    double tic; 
    DEBUG(DBTIMINGS, tic = gtoc());
#endif
    int id = 0;

    double slice = (end-start)/num_threads;
    double hc = slice/ncoarse;
    double hf = slice/nfine;
    // +1 to account for y_0
    double *y_t_old  = (double*) malloc((num_threads+1)*sizeof(double));
    double *y_t_fine = (double*) malloc((num_threads+1)*sizeof(double));

    y_t_old[0]  = y_0;
    y_t_new[0]  = y_0;
    y_t_fine[0] = y_0;

    double y_new, y_old = y_0;
    double t = start;
    // Initial approximation
    for (int i=0; i<num_threads; i++) {
        for(int j=0; j<ncoarse; j++) {
            y_old = coarse(t, y_old, hc, f);
            t += hc;
        }
        y_t_old[i+1] = y_old;
    }
    DEBUG(DBTIMINGS, addTime2Plot(timings, id, 0, tic, gtoc()));

    // parareal iteration
    for (int K=0; K<piters; K++) {
        DEBUG(DBTIMINGS, tic = gtoc());
        // parallel fine steps
        t = start;

        pthread_t threads[num_threads];
        task_data td[num_threads];

        for(int i = 0; i < num_threads; i++) {
            td[i].t = t;
            td[i].y_t = y_t_old[i];
            td[i].hf = hf;
            td[i].nfine = nfine;
            td[i].f = f;
            td[i].fine = fine;
            td[i].id = i;
            td[i].timings = timings;
            pthread_create(threads+i, NULL, task, (void*) &td[i]);
            t += slice;
        }
        DEBUG(DBTIMINGS, addTime2Plot(timings, id, 0, tic, gtoc()));

        for (int i=0; i<num_threads; i++) {
            pthread_join(threads[i], NULL);
            y_t_fine[i+1] = td[i].y_t;
        }

        DEBUG(DBTIMINGS, tic = gtoc());

        // corrections and sequential coarse steps
        t = start;
        for (int i=0; i<num_threads; i++) {
            y_new = y_t_new[i];
            y_old = y_t_old[i];
            for (int j=0; j<ncoarse; j++) {
                y_new = coarse(t, y_new, hc, f);
                y_old = coarse(t, y_old, hc, f);
                t += hc;
            }
            y_t_new[i+1] = y_new + y_t_fine[i+1] - y_old;
        }
        // new -> old
        memcpy(y_t_old+1, y_t_new+1, num_threads*sizeof(double));
        DEBUG(DBTIMINGS, addTime2Plot(timings, id, 2, tic, gtoc()));
    }
    if (timings) fclose(timings);
    free(y_t_old );
    free(y_t_fine);
}
