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
    double t, y_t, hf;
    int nfine, id;
    rhs_func f;
    singlestep_func fine;
    FILE* timings;
} task_data;

void* task(void* args) {
    struct timespec w_tic;
    double tic = gtoc();
    task_data* td = (task_data*) args;
    double y_t = td->y_t;
    for (int j=0; j<td->nfine; j++) {
        y_t = td->fine(td->t, y_t, td->hf, td->f);
        td->t += td->hf;
    }
    td->y_t = y_t;
    addTime2Plot(td->timings, td->id, tic, gtoc());
    return NULL;
}

// TODO proper malloc/free symmetry by passing buffers as arguments
// TODO add numthreads as arg
double* parareal(double start, double end, int ncoarse, int nfine, int num_threads,
        double y_0, singlestep_func coarse, singlestep_func fine, rhs_func f, int piters) {

    // init time measurements
    char fname[128];
    sprintf(fname, "outdata/timings.t%d.pw%d.sw%d.data", num_threads, ncoarse, nfine);
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
    addTime2Plot(timings, id, tic, gtoc());
    //gnuplot();

    // parareal iteration
    for (int K=0; K<piters; K++) {
        // parallel fine steps
        tic = gtoc();
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
            td[i].id = i+1;
            td[i].timings = timings;
            pthread_create(threads+i, NULL, task, (void*) &td[i]);
            t += slice;
        }
        addTime2Plot(timings, id, tic, gtoc());
        
        // wait for threads to finish
        for (int i=0; i<num_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        tic = gtoc();
        for (int i=0; i<num_threads; i++) {
            y_t_fine[i+1] = td[i].y_t;
        }

        //write2file(start, slice, num_threads+1, y_t_fine);
        //char b;
        //scanf("%c", &b);


        // corrections and sequential coarse steps
        // TODO currently limited to 1 step for coarse update
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
        addTime2Plot(timings, id, tic, gtoc());
    }
    free(y_t_old );
    free(y_t_fine);
    fclose(timings);

    return y_t_new;
}
