#!/usr/local/bin/bash

# constant variables

### make executables
make debug1 main

if [ ! $? -eq 0 ]; then 
    echo "$0: Compilation failed. Aborting."; exit -1;
fi

### commands to run
./main
