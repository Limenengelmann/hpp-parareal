#!/usr/local/bin/bash

# constant variables

### make executables
make debug1 main

if [ ! $? -eq 0 ]; then 
    echo "$0: Compilation failed. Aborting."; exit -1;
fi

### commands to run
# racing condition
main 8 85 3;
gnuplot -c timings.gp outdata/timings.t8.sw85.K3.data;
