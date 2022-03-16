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
    double t, y, hc, hf;
    double *y_next;
    int nfine, ncoarse, id;
    volatile char *progress;    // pointer to volatile char
    rhs_func f;
    singlestep_func fine;
    singlestep_func coarse;
    FILE* timings;
} task_data;

// TODO update task to new pipeline version
void* task(void* args) {
    struct timespec w_tic;
    double tic = gtoc();
    task_data* td = (task_data*) args;

    double t, y_coarse, y_fine, y_next;
    double hc = td->hc;
    double hf = td->hf;


    // parallel coarse step
    t = td->t;
    y_coarse = td->y;
    for (int i=0; i<td->ncoarse; i++) {
        y_coarse = td->coarse(t, y_coarse, hc, td->f);
        t += hc;
    }

    // parallel fine step
    t = td->t;
    y_fine = td->y;
    for (int i=0; i<td->nfine; i++) {
        y_fine = td->fine(t, y_fine, hf, td->f);
        t += hf;
    }
    addTime2Plot(td->timings, td->id, tic, gtoc());

    // wait for previous thread to update y_next
    while (td->progress[td->id-1] != 1);    
    
    tic = gtoc();
    // serial coarse propagation step + parareal update
    t = td->t;
    y_next = td->y_next[td->id];  // TODO guarantee y_next is uptodate (sync)
    for (int i=0; i<td->ncoarse; i++) {
        y_next = td->coarse(t, y_next, hc, td->f);
        t += hc;
    }
    // parareal update
    // TODO just accumulate result directly in y_next?
    y_next = y_next + y_fine - y_coarse;
    td->y_next[td->id+1] = y_next;
    td->progress[td->id] = 1;

    addTime2Plot(td->timings, td->id, tic, gtoc());
    return NULL;
}

// TODO plot idea: speedup vs relative serial work/total work
// TODO proper malloc/free symmetry by passing buffers as arguments
// TODO add numthreads as arg
double* parareal(double start, double end, int ncoarse, int nfine, int num_threads,
        double y_0, singlestep_func coarse, singlestep_func fine, rhs_func f, int piters) {

    // init time measurements
    char fname[128];
    sprintf(fname, "outdata/timings.t%d.sw%d.K%d.data", num_threads, ncoarse, piters);
    FILE* timings = fopen(fname, "w");
    if (! timings) {
        perror("Couldn't open timings.data");
        return NULL;
    }
    int id = 0;  // main thread #0
    struct timespec w_tic;
    double tic = gtoc();

    double slice = (end-start)/num_threads;
    double hc = slice/ncoarse;
    double hf = slice/nfine;
    // +1 to account for y_0
    double *y_t_old  = (double*) malloc((num_threads+1)*sizeof(double));
    double *y_t_new  = (double*) malloc((num_threads+1)*sizeof(double));
    double *y_t_fine = (double*) malloc((num_threads+1)*sizeof(double));

    double *y       = (double*) malloc((num_threads+1)*sizeof(double)); // current solution
    double *y_next  = (double*) malloc((num_threads+1)*sizeof(double)); // current solution
    double *y_fine  = (double*) malloc((num_threads+1)*sizeof(double)); // fine sol
    double *y_c_old = (double*) malloc((num_threads+1)*sizeof(double)); // old coarse sol
    double *y_c_new = (double*) malloc((num_threads+1)*sizeof(double)); // new corase sol
    // TODO synchronisation: when thread i prepared y_next[i+1]
    // flag buffer? done[i]->1 thread i+1-> works and sets done[i]->0
    // possible race condition?
    char progress[num_threads];
    for (int i=0; i<num_threads; i++)
        progress[i] = 0;

    y[0]       = y_0;
    y_next[0]  = y_0;
    y_fine[0]  = y_0;
    y_c_old[0] = y_0;
    y_c_new[0] = y_0;

    y_t_old[0]  = y_0;
    y_t_new[0]  = y_0;
    y_t_fine[0] = y_0;

    double y_old = y_0;
    double t = start;
    // Initial approximation
    for (int i=0; i<num_threads; i++) {
        for(int j=0; j<ncoarse; j++) {
            y_old = coarse(t, y_old, hc, f);
            t += hc;
        }
        y_c_old[i+1] = y_old;
    }
    addTime2Plot(timings, id, tic, gtoc());
    //gnuplot();
    

    // parareal iteration
    for (int K=0; K<piters; K++) {
        // parallel fine steps

        pthread_t threads[num_threads];
        task_data td[num_threads];

        // TODO only create threads once
        t = start+slice;
        for(int i = 1; i < num_threads; i++) {
            // TODO pass y_fine pointer, then write result directly ?
            // how to slim this down ? only t and y change per iteration
            td[i].t = t;
            td[i].y = y[i];
            td[i].y_next = y_next;
            td[i].hf = hf;
            td[i].hc = hc;
            td[i].nfine   = nfine;
            td[i].ncoarse = ncoarse;
            td[i].f = f;
            td[i].fine   = fine;
            td[i].coarse = coarse;
            td[i].id = i;
            td[i].progress = progress;
            td[i].timings = timings;
            pthread_create(threads+i, NULL, task, (void*) &td[i]);
            t += slice;
        }
        // main thread works
        tic = gtoc();
        t = start;
        y_c_old[1] = y[0];
        for(int i=0; i < ncoarse; i++) {
            y_c_old[1] = coarse(t, y_c_old[1], hc, f);
            t += hc;
        }
        t = start;
        y_fine[1] = y[0];
        for (int i=0; i<nfine; i++) {
            y_fine[1] = fine(t, y_fine[1], hf, f);
            t += hf;
        }
        t = start;
        y_c_new[1] = y_next[0];     // = y[0]
        for (int i=0; i<ncoarse; i++) {
            y_c_new[1] = coarse(t, y_c_new[1], hc, f);
            t += hc;
        }
        // parareal update
        // TODO just accumulate result directly in y_next?
        y_next[1] = y_c_new[1] + y_fine[1] - y_c_old[1];
        progress[0] = 1;

        addTime2Plot(timings, id, tic, gtoc());
        
#if 1
        // wait for threads to finish
        for (int i=1; i<num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
#endif

        //write2file(start, slice, num_threads+1, y_t_fine);
        //char b;
        //scanf("%c", &b);


        // corrections and sequential coarse steps
        // TODO currently limited to 1 step for coarse update
        /*
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
        */
        // new -> old
        // TODO check for proper updatetobias.lima-engelmann@it.uu.se
        memcpy(y+1, y_next+1, num_threads*sizeof(double));
        memcpy(y_c_old+1, y_c_new+1, num_threads*sizeof(double));
        for (int i=0; i<num_threads; i++)
            progress[i] = 0;
    }

#if 0
    for (int i=0; i<num_threads+1; i++)
        printf("y[%d]: %f\n", i, y[i]);
    for (int i=0; i<num_threads+1; i++)
        printf("y_fine[%d]: %f\n", i, y_fine[i]);
    for (int i=0; i<num_threads+1; i++)
        printf("y_next[%d]: %f\n", i, y_next[i]);
    for (int i=0; i<num_threads+1; i++)
        printf("y_c_old[%d]: %f\n", i, y_c_old[i]);
    for (int i=0; i<num_threads+1; i++)
        printf("y_c_new[%d]: %f\n", i, y_c_new[i]);
#endif

    free(y_t_old );
    free(y_t_fine);

    free(y_next );
    free(y_fine );
    free(y_c_old);
    free(y_c_new);

    fclose(timings);

    return y;
}
