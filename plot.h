#ifndef _PLOT_H_
#define _PLOT_H_

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

// TODO make varargs
void write2file(double start, double h, int nsteps, double *xt);

// TODO enable argument to gnuplot to determine file number
// this way different iterations can be plotted and compared
void gnuplot();

#endif
