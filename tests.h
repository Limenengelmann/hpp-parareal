#ifndef _TESTS_H_
#define _TESTS_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "parareal.h"
#include "plot.h"

#define tic() (tic = (clock_gettime(CLOCK_REALTIME, &w_tic) == 0 /*assert that call succeeded*/ ) * ((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f))
// return time since tic()
#define toc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - tic)
// w_time time passed since global reference point
#define gtoc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - g_tic)

#define addTime(fp, id, t1, t2) fprintf(fp, "%.9f %d\n%.9f %d\n\n5, t1, id, t2, id);

// can be switched by Makefile
#define DEBUGGING       1

#define DBTESTS         0
#define DBMAIN_TESTS    0
#define DBPARAPLOT      1

// debug printing
#if DEBUGGING
// print debug info
#define DPRINTF(DBFLAG, ...) \
    if (DBFLAG) { \
        printf("DBG(%s): ", __func__); printf(__VA_ARGS__); \
    }
// execute only when debugging (like assert statements)
#define DEBUG(DBFLAG, ...) \
    if (DBFLAG) { \
        __VA_ARGS__; \
    }
#else
#define DPRINTF(...)
#define DEBUG(...)
#endif

int run_tests();
double f_cos(double t, double x_t);
double f_id(double t, double x_t);

#endif  // include guard
