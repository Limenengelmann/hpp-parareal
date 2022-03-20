#ifndef _AUX_H_
#define _AUX_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define F_WORK 300      // time to evaluate f in microseconds

/*
 * DEBUG Macros
 */
#define DEBUGGING       1

// debug sections
#define DBTESTS         0
#define DBMAIN_TESTS    0
#define DBPARAPLOT      0
#define DBTIMINGS       1
#define DBTHREADS       0

// randomize workload in different code sections
#define DBRNDSLEEP_INI  0
#define DBRNDSLEEP_IND  1
#define DBRNDSLEEP_DEP  1
#define DBRNDSLEEP_RK4  0
#define DBRNDSLEEP_FWE  0
#define SLEEPTIME(n) usleep(rand()%lround((n)*F_WORK/2))

// debug printing
#if DEBUGGING
// print debug info
// two print statements are not thread-'atomic' anymore
#define DPRINTF(DBFLAG, ...) \
    if (DBFLAG) { \
        /*printf("DBG(%s): ", __func__);*/ printf(__VA_ARGS__); \
    }
// execute only when debugging
#define DEBUG(DBFLAG, ...) \
    if (DBFLAG) { \
        __VA_ARGS__; \
    }
#else
#define DPRINTF(...)
#define DEBUG(...)
#endif  // #if DEBUGGING

/*
 * Timing Macros
 */
#define tic() (tic = (clock_gettime(CLOCK_REALTIME, &w_tic) == 0 /*assert that call succeeded*/ ) * ((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f))
// return time since tic()
#define toc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - tic)

// w_time time passed since global reference
// point extern double g_tic;
#define gtoc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - g_tic)

#define addTime2Plot(fp, id, s, t1, t2) fprintf(fp, "%.9f %d %d\n%.9f %d %d\n", t1, id, s, t2, id, s);

/*
 * Visualisation Helper
 */
void write2file(double start, double h, int nsteps, double *y, int id);

void gnuplot();

#include "parareal.h"
#endif
