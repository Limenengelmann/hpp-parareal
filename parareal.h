#ifndef _PARAREAL_H_
#define _PARAREAL_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include "tests.h"
#include "plot.h"

extern double g_tic;

typedef double (*rhs_func) (double t, double y_t);
typedef double (*singlestep_func) (double t, double y_t, double h, rhs_func f);

double fw_euler_step(double t, double y_t, double h, rhs_func f);

double rk4_step(double t, double y_t, double h, rhs_func f);

// TODO data structure for fine and coarse solver?
double* parareal(double start, double end, int ncoarse, int nfine, int num_threads,
        double y_0,
        singlestep_func coarse,
        singlestep_func fine,
        rhs_func f);

#endif // include guard
