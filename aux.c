#include "aux.h"


void write2file(double start, double h, int size, double *yt) {
    FILE* fp = fopen("./outdata/data.out", "w");
    assert(fp && "Open failed\n");

    for(int i=0; i<size; i++) {
        fprintf(fp, "%.4f %.4f\n", start+i*h, yt[i]);
    }
    fclose(fp);
}

void gnuplot() {
    pid_t pid = fork();
    if (!pid) {
        execlp("gnuplot", "-p", "plot.gp", (char*) NULL);
        perror("exec failed");
        exit(-1);
    }
}
