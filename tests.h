#ifndef _TESTS_H_
#define _TESTS_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "parareal.h"
#include "aux.h"

// can be switched by Makefile
#define DEBUGGING       1

#define DBTESTS         0
#define DBMAIN_TESTS    0
#define DBPARAPLOT      0
#define DBTIMINGS       1
#define DBTHREADS       0

// debug printing
#if DEBUGGING
// print debug info
// two print statements are not thread-'atomic' anymore
#define DPRINTF(DBFLAG, ...) \
    if (DBFLAG) { \
        /*printf("DBG(%s): ", __func__);*/ printf(__VA_ARGS__); \
    }
// execute only when debugging (like assert statements)
#define DEBUG(DBFLAG, ...) \
    if (DBFLAG) { \
        __VA_ARGS__; \
    }
#else
#define DPRINTF(...)
#define DEBUG(...)
#endif  // #if DEBUGGING

int run_tests();
double f_cos(double t, double x_t);
double f_id(double t, double x_t);

#endif  // include guard
