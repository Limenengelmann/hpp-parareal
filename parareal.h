#ifndef _PARAREAL_H_
#define _PARAREAL_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <omp.h>
#include "funcs.h"
#include "aux.h"

// vanilla parareal
typedef struct task_data {
    double t, y_t, hf;
    int nfine, id;
    rhs_func f;
    singlestep_func fine;
} task_data;

typedef struct pipel_task_data {
    double t0, y0, hc, hf;
    double *y_next;
    int nfine, ncoarse, id, piters;
    volatile char *progress;    // pointer to volatile char
    rhs_func f;
    singlestep_func fine;
    singlestep_func coarse;
    FILE* timings;
} pipel_task_data;

double fw_euler_step(double t, double y_t, double h, rhs_func f);

double rk4_step(double t, double y_t, double h, rhs_func f);

double* parareal_pthread(double start, double end, int ncoarse, int nfine, int num_threads,
        double y_0,
        singlestep_func coarse,
        singlestep_func fine,
        rhs_func f, int piters);

double* parareal_omp(double start, double end, int ncoarse, int nfine, int num_threads,
        double y_0,
        singlestep_func coarse,
        singlestep_func fine,
        rhs_func f, int piters);

double* parareal(double start, double end, int ncoarse, int nfine, int num_threads,
        double y_0,
        singlestep_func coarse,
        singlestep_func fine,
        rhs_func f, int piters);

#endif // include guard
