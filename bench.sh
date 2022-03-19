#!/usr/local/bin/bash

# constant variables

function breakit() {
    # makes omp fail because of lapping
    t=16
    sw=1024
    K=12
    fname=timings.t$t.sw$sw.K$K.data
    if [ ! -e outdata/out.$fname ]; then
        main $t $sw $K | tee outdata/out.$fname
    fi

    grep '# [67](K [67]' outdata/out.timings.t16.sw1024.K12.data
    python timings.py outdata/$fname.pthread &
    python timings.py outdata/$fname.omp
}

### make executables
make debug0 main

if [ ! $? -eq 0 ]; then 
    echo "$0: Compilation failed. Aborting."; exit -1;
fi

### commands to run
# racing condition
for t in 1 2 4 8 16; do
    for sw in 1 2 4 8 16 32 64 128 256; do
        for k in seq 4; do
            for reruns in seq 5; do
                main $t $sw $k | tee -a outdata/bench.out
            done
        done
        echo ""
    done
    echo "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"
done
