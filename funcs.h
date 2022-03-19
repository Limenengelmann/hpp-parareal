#ifndef _FUNCS_H_
#define _FUNCS_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

typedef double (*rhs_func) (double t, double y_t);
typedef double (*singlestep_func) (double t, double y_t, double h, rhs_func f);

double f_cos(double t, double x_t);
double f_id(double t, double x_t);
double sol_id(double t);
double fw_euler_step(double t, double y_t, double h, rhs_func f);
double rk4_step(double t, double y_t, double h, rhs_func f);

int run_tests();    // test integrators

// too lazy for forward declarations
#include "parareal.h"
#include "aux.h"
#endif  // include guard
