#include "plot.h"

void write2file(double start, double h, int nsteps, double *xt) {
    FILE* fp = fopen("./outdata/data.out", "w");
    assert(fp && "Open failed\n");

    for(int i=0; i<nsteps+1; i++) {
        fprintf(fp, "%.4f %.4f\n", start+i*h, xt[i]);
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
