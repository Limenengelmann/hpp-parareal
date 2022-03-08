#ifndef _TESTS_H_
#define _TESTS_H_

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include "parareal.h"
#include "plot.h"


// can be switched by Makefile
#define DEBUGGING       1

#define DBTESTS         1
#define DBMAIN_TESTS    1
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

#endif  // include guard
