#ifndef _AUX_H_
#define _AUX_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define tic() (tic = (clock_gettime(CLOCK_REALTIME, &w_tic) == 0 /*assert that call succeeded*/ ) * ((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f))
// return time since tic()
#define toc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - tic)

// w_time time passed since global reference point
//extern double g_tic;
#define gtoc() (clock_gettime(CLOCK_REALTIME, &w_tic) == 0) * (((double)w_tic.tv_sec + w_tic.tv_nsec/1e9f) - g_tic)


#define addTime2Plot(fp, id, t1, t2) fprintf(fp, "%.9f %d\n%.9f %d\n\n", t1, id, t2, id);


// TODO make varargs
void write2file(double start, double h, int nsteps, double *xt);

// TODO enable argument to gnuplot to determine file number
// this way different iterations can be plotted and compared
void gnuplot();

#endif
