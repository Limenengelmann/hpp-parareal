#ifndef _PARAREAL_H_
#define _PARAREAL_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "tests.h"
#include "plot.h"

double fw_euler_step(double t, double x_t, double h, double (*f) (double, double));

double rk4_step(double t, double x_t, double h, double (*f) (double, double));

// TODO data structure for fine and coarse solver?
double* parareal(double start, double end, int ncoarse, int nfine, 
        double x_0,
        double (*coarse) (double, double, double, double (*f) (double, double)),
        double (*fine) (double, double, double, double (*f) (double, double)),
        double (*f) (double, double));

#endif // include guard
