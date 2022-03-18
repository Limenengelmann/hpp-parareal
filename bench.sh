#!/usr/local/bin/bash

# constant variables

### make executables
make debug1 main

if [ ! $? -eq 0 ]; then 
    echo "$0: Compilation failed. Aborting."; exit -1;
fi

### commands to run
# racing condition
#for t in 1 2 4 8 16; do
#    for sw in 1 2 4 8 16 32 64 128 256 512 1024; do
#        for k in 1 2 3 4; do
#            main $t $sw $k | tee -a outdata/bench.out
#        done
#        echo ""
#    done
#    echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"
#done
t=16
sw=1024
K=12
fname=timings.t$t.sw$sw.K$K.data
if [ ! -e asdlkgjasdlgkjasdglkjoutdata/out.$fname ]; then
    main $t $sw $K | tee outdata/out.$fname
fi

python timings.py outdata/$fname.pthread &
python timings.py outdata/$fname.omp
